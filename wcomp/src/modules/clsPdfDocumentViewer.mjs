import UI from "./UI.mjs";
import clsTargomanPdfDocument from "./clsTargomanPdfDocumentInterface.mjs";
import clsEventTarget from "./clsEventTarget.mjs";
import { getKeyName } from "./common.mjs";

(() => {
    let Start;
    window.markStartTime = () => (Start = new Date());
    window.elapsedTime = () => new Date() - Start;
})();

class clsPdfDocumentViewer extends clsEventTarget {
    static isVisible(node) {
        return this.visibilityPortion(node) > 0;
    }

    static docViewPort() {
        return {
            vw: Math.max(document.documentElement.clientWidth || 0, window.innerWidth || 0),
            vh: Math.max(document.documentElement.clientHeight || 0, window.innerHeight || 0),
        };
    }

    static visibilityPortion(node, dir) {
        if (!node || !node.offsetParent) return 0;
        let b = node.getBoundingClientRect();
        const { vw, vh } = clsPdfDocumentViewer.docViewPort();
        if (b.bottom < 0 || b.right < 0 || b.top > vh || b.left > vw) return 0;
        if (dir == "v") {
            return b.top <= 0 && b.bottom > vh ? 1 : b.bottom > vh ? 1 - b.top / vh : b.bottom / vh;
        } else {
            return b.left <= 0 && b.right > vh ? 1 : b.right > vh ? 1 - b.left / vh : b.right / vh;
        }
    }

    constructor(_root, _configs, _vertPageMargin, _horzPageMargin) {
        super();

        this.VertPageMargin = _vertPageMargin;
        this.HorzPageMargin = _horzPageMargin;

        _root.innerHTML = "";
        this.ProgressBar = UI.createElement(_root, {
            classes: ["tgpdf-loadingBar", "tgpdf-hidden"],
            childs: [{ classes: ["tgpdf-progress"] }],
        });
        this.ProgressBar.progress = this.ProgressBar.firstElementChild;

        this.ZoomHelper = UI.createElement(_root, { class: "tgpdf-zoom-helper" });
        this.DocumentContainer = UI.createElement(this.ZoomHelper, { class: "tgpdf-doc" });

        this.Configs = _configs;
        this.clear();

        _root.classList.add("tgpdf-doc-wrapper");

        _root.addEventListener("scroll", (e) => this.scheduleRenderVisiblePages(e));
        _root.addEventListener("wheel", (e) => this.handleMouseWheel(e));

        let startTouches = [],
            startClientPos,
            startScale,
            pinchEventCount = 0;
        const computeDistance = (touches) => {
            let DeltaX = touches[0].clientX - touches[1].clientX;
            let DeltaY = touches[0].clientY - touches[1].clientY;
            return Math.sqrt(DeltaX * DeltaX + DeltaY * DeltaY);
        };
        const computeCenter = (touches) => {
            let N = 0;
            let X = 0;
            let Y = 0;
            Array.from(touches).forEach((touch) => {
                ++N;
                X += touch.clientX;
                Y += touch.clientY;
            });
            return [X / N, Y / N];
        };
        _root.addEventListener("touchstart", (e) => {
            if (e.touches.length > 1) {
                e.preventDefault();
                startTouches = e.touches;
                startClientPos = computeCenter(startTouches);
                startScale = this.Scale / computeDistance(startTouches);
                pinchEventCount = 1;
            }
        });
        _root.addEventListener("touchmove", (e) => {
            if (e.touches.length > 1) {
                e.preventDefault();
                if (startTouches.length < 2) {
                    startTouches = e.touches;
                    startClientPos = computeCenter(startTouches);
                    startScale = this.Scale;
                    pinchEventCount = 1;
                    return;
                }
                ++pinchEventCount;
                if (pinchEventCount < 4) return;
                const NewScale = startScale * computeDistance(e.touches);
                if (this.isValidScale(NewScale)) this.setScale(NewScale, startClientPos[0], startClientPos[1]);
            }
        });
        _root.addEventListener("touchend", (e) => {
            startTouches = [];
            startClientPos = undefined;
            startScale = undefined;
            pinchEventCount = 0;
        });

        new ResizeObserver(() => this.scaleType === "auto" && this.setAbsoluteScale("auto")).observe(_root);
    }

    clear() {
        this.Scale = 1.0;
        this.PdfDocument = null;
        this.RenderCancelled = false;
        this.RenderPromise = null;
        this.PageContainers = [];
        this.PageLoadingSpinners = [];
        this.PageMarkableOverlays = [];
        this.PageCanvases = [];
        this.PageMarkablePars = [];
        this.PageMarkableLines = [];
        this.DocumentContainer.innerHTML = "";
        this.activePageIndex = 0;
        this.selected = {
            realPage: -1,
            page: -1,
            par: -1,
            snt: -1,
            loc: enuLocation.Main,
        };
    }

    updateZoomHelperSize() {
        this.ZoomHelper.style.height = this.Scale * this.TotalHeight + "px";
        this.ZoomHelper.style.width = this.Scale * (this.MaxWidth + this.HorzPageMargin) + "px";
    }

    isValidScale(_scale) {
        return _scale <= 5 && _scale >= 0.25;
    }

    computeNewScale(_dir) {
        return this.Scale * (1 - (_dir > 0 ? 5 : -5) / 100);
    }

    computeBestFitScale() {
        let NewScale = 1;
        const ScreenWidth = this.ZoomHelper.parentElement.getBoundingClientRect().width;
        if (this.MaxWidth > ScreenWidth - this.HorzPageMargin) {
            NewScale = (ScreenWidth - this.HorzPageMargin) / this.MaxWidth;
        } else if (ScreenWidth > 1024 && this.MaxWidth < 1024) {
            NewScale = 1024 / this.MaxWidth;
        }
        return NewScale;
    }

    setAbsoluteScale(_scale) {
        this.scaleType = _scale;
        let NewScale = null;
        switch (_scale) {
            case "+1":
                NewScale = this.computeNewScale(-1);
                break;
            case "-1":
                NewScale = this.computeNewScale(1);
                break;
            case "auto":
                NewScale = this.computeBestFitScale();
                break;
            case "actual":
                NewScale = 1;
                break;
            default:
                NewScale = _scale;
        }
        if (this.isValidScale(NewScale)) {
            this.setScale(NewScale, this.ZoomHelper.parentElement.getBoundingClientRect().width / 2, 0);
            this.scheduleRenderVisiblePages();
        }
    }

    setScale(_scale, _offsetX, _offsetY) {
        _offsetX = (_offsetX - this.ZoomHelper.parentElement.getBoundingClientRect().x + this.ZoomHelper.parentElement.scrollLeft) / this.Scale;
        _offsetY = (_offsetY - this.ZoomHelper.getBoundingClientRect().y + this.ZoomHelper.offsetTop) / this.Scale;

        let DeltaScrollTop = 0;
        if (_offsetY) DeltaScrollTop = _offsetY * (_scale - this.Scale);
        let DeltaScrollLeft = 0;
        if (_offsetX) DeltaScrollLeft = _offsetX * (_scale - this.Scale);

        this.Scale = _scale;
        this.updateZoomHelperSize();
        this.DocumentContainer.style.transform = `scale(${_scale})`;

        if (DeltaScrollTop != 0) this.ZoomHelper.parentElement.scrollTop += DeltaScrollTop;
        if (DeltaScrollLeft != 0) this.ZoomHelper.parentElement.scrollLeft += DeltaScrollLeft;
    }

    async buildUiElements() {
        if (!this.PdfDocument) return;

        console.log("BUILDING VIEW ...", elapsedTime());
        let TotalHeight = 0;
        let MaxWidth = 0;
        let PageCount = this.PageCount;
        for (let i = 0; i < PageCount; ++i) {
            let PageSize = this.PageSizes[i];

            const PageContainer = UI.createElement(this.DocumentContainer, {
                class: "tgpdf-page",
                style: { width: PageSize.Width + this.HorzPageMargin + "px", height: PageSize.Height + this.VertPageMargin + "px" },
                childs: [
                    { tag: "canvas", style: { width: PageSize.Width, height: PageSize.Height } },
                    { class: "tgpdf-page-overlay" },
                    {
                        class: "tgpdf-page-spinner",
                        childs: [
                            { class: "tgpdf-sp-large", childs: [{ class: "tgpdf-sp-progress" }] },
                            { class: "tgpdf-sp-small", childs: [{ class: "tgpdf-lds-ellipsis", childs: [{}, {}, {}, {}] }] },
                        ],
                    },
                ],
            });

            this.PageContainers.push(PageContainer);
            this.PageCanvases.push(PageContainer.children[0]);
            this.PageMarkableOverlays.push(PageContainer.children[1]);
            this.PageLoadingSpinners.push(PageContainer.children[2]);

            TotalHeight += PageSize.Height + this.VertPageMargin;
            MaxWidth = Math.max(MaxWidth, PageSize.Width);
        }

        this.TotalHeight = TotalHeight;
        this.MaxWidth = MaxWidth;

        this.DocumentContainer.style.width = this.MaxWidth + this.HorzPageMargin + "px";

        this.updateZoomHelperSize();
        this.setAbsoluteScale("auto");

        console.log("BUILDING VIEW DONE.", elapsedTime());
    }

    async getPDFDocInfo(pageIndex) {
        const Result = await this.PdfDocument.getPDFDocInfo(pageIndex);
        Result.fileName = this.fileName;
        return Result;
    }

    async open(_buffer, fileName) {
        if (this.PdfDocument) {
            console.log("DELETING PREVIOUS INSTANCE...", elapsedTime());
            this.PdfDocument.delete();
            this.clear();
            console.log("DELETING PREVIOUS INSTANCE DONE.", elapsedTime());
        }
        this.fileName = fileName;

        this.buffer = _buffer;

        console.log("INTERPRETING PDF ...", elapsedTime());
        this.PdfDocument = new clsTargomanPdfDocument();
        await this.PdfDocument.ready;
        await this.PdfDocument.loadPdf(_buffer, this.Configs.layout);
        console.log("INTERPRETING PDF DONE.", elapsedTime());

        this.PageCount = await this.PdfDocument.PageCount;

        this.ProgressBar.classList.remove("tgpdf-hidden");
        const TotalProgressTime = (2500 * this.PageCount) / 655;
        const ProgressUpdateStep = 30;
        let Progress = 0;
        let ProgressUpdateInterval = setInterval(() => {
            Progress += (100 * ProgressUpdateStep) / TotalProgressTime;
            this.ProgressBar.progress.style.width = Progress + "%";
            if (Progress > 99.5) clearInterval(ProgressUpdateInterval);
        }, ProgressUpdateStep);

        this.PageSizes = await this.PdfDocument.getAllPageSizes();

        await this.buildUiElements();

        clearInterval(ProgressUpdateInterval);
        this.ProgressBar.classList.add("tgpdf-hidden");

        setTimeout(() => this.setActiveSentence(0, -1, -1, enuLocation.Main), 100);

        this.dispatchEvent("pdfOpened");
    }

    async renderPageImage(_pageIndex) {
        if (!this.PdfDocument) return;

        let Canvas = this.PageCanvases[_pageIndex];
        if (Canvas.RenderScale == this.Scale) return;

        const MaxSize = 2048;
        let Scale = this.Scale;
        let PageSize = { ...this.PageSizes[_pageIndex] };
        if (Scale > MaxSize / PageSize.Width) Scale = MaxSize / PageSize.Width;
        if (Scale > MaxSize / PageSize.Height) Scale = MaxSize / PageSize.Height;

        PageSize.Width = Math.round(PageSize.Width * Scale);
        PageSize.Height = Math.round(PageSize.Height * Scale);

        let Context = Canvas.getContext("2d");

        console.log(`RENDERING PAGE ${_pageIndex} ...`, markStartTime());
        let BitmapBuffer = await this.PdfDocument.getPageImage(_pageIndex, -1, PageSize);
        let Data = new ImageData(new Uint8ClampedArray(BitmapBuffer), PageSize.Width, PageSize.Height);

        Context.canvas.width = PageSize.Width;
        Context.canvas.height = PageSize.Height;
        Context.putImageData(Data, 0, 0);

        console.log(`RENDERING PAGE ${_pageIndex} DONE.`, elapsedTime());
        Canvas.RenderScale = this.Scale;
    }

    async updateSelectedSentence(sentenceSpecs) {
        this.selected.loc = sentenceSpecs.Location;
        this.selected.realPage = sentenceSpecs.RealPageIndex;
        this.selected.page = sentenceSpecs.PageIndex;
        this.selected.par = sentenceSpecs.ParIndex;
        this.selected.snt = sentenceSpecs.SntIndex;

        [].forEach.call(this.ZoomHelper.querySelectorAll(".tgpdf-sentence", ".tgpdf-paragraph.tgpdf-active"), (el) => (el.style.backgroundColor = ""));

        const { vw, vh } = clsPdfDocumentViewer.docViewPort();

        const markables = await this.PdfDocument.getMarkables(this.selected.realPage);
        let bestSenetenceSegment = null;
        markables.some(
            (par) =>
                par.PageIndex === this.selected.page &&
                par.ParIndex === this.selected.par &&
                par.Location === this.selected.loc &&
                par.InnerSegments.some((snt) => {
                    if (snt.SntIndex === this.selected.snt) {
                        bestSenetenceSegment = snt;
                        return true;
                    }
                })
        );

        const pageTop = this.computePageScrollTop(this.selected.realPage);
        const nodeTop = pageTop + this.Scale * bestSenetenceSegment.BoundingBox.Y0;
        const scrollTop = this.ZoomHelper.parentElement.scrollTop;

        if (nodeTop > scrollTop + vh * 0.66) this.ZoomHelper.parentElement.scrollTop = nodeTop - vh * 0.66;
        else if (nodeTop < scrollTop + 42) this.ZoomHelper.parentElement.scrollTop = nodeTop - 42;

        this.renderVisiblePages();
        this.dispatchEvent("activeSentenceChanged", sentenceSpecs);
    }

    async setActiveSentence(pageIndex, parIndex, sntIndex, loc) {
        const newSentence = await this.PdfDocument.setCurrentSentence(pageIndex, parIndex, sntIndex, loc);
        if (newSentence.loc != enuLocation.None) this.updateSelectedSentence(newSentence);
    }

    async nextSentence() {
        const newSentence = await this.PdfDocument.gotoNextSentence();
        if (newSentence.loc != enuLocation.None)
            this.updateSelectedSentence(newSentence).then(() => {
                this.PdfDocument.getMarkables(newSentence.RealPageIndex + 1);
            });
    }

    async prevSentence() {
        const newSentence = await this.PdfDocument.gotoPrevSentence();
        if (newSentence.loc != enuLocation.None)
            this.updateSelectedSentence(newSentence).then(() => {
                this.PdfDocument.getMarkables(newSentence.RealPageIndex - 1);
            });
    }

    async activeSentenceContent() {
        return await this.PdfDocument.getSentenceContent();
    }

    async renderPageMarkables(_pageIndex) {
        if (!this.PdfDocument) return;
        this.PdfDocument.getMarkables(_pageIndex).then((markables) => {
            this.PageMarkableOverlays[_pageIndex].innerHTML = "";
            markables.forEach((par) => {
                let bgColor = "transparent";
                let childs = [];
                let typeClass = "tgpdf-other";
                if (this.Configs.markers.markSelectedParagraph && par.PageIndex === this.selected.page && par.ParIndex === this.selected.par) {
                    bgColor = this.Configs.markers.activeParagraphHighlight;
                    typeClass = "tgpdf-active";
                } else if (par.Location != enuLocation.Main) {
                    if (this.Configs.markers.markAllNonTexts) {
                        bgColor = this.Configs.markers.nonTextMarksHighlight;
                        childs = [{ text: getKeyName(enuLocation, par.Location) }];
                        typeClass = "tgpdf-no-text";
                    }
                } else {
                    if (par.Type != enuContentType.Text && par.Type != enuContentType.List) {
                        if (this.Configs.markers.markAllNonTexts) {
                            bgColor = this.Configs.markers.nonTextMarksHighlight;
                            childs = [{ text: getKeyName(enuContentType, par.Type) }];
                            typeClass = "tgpdf-no-text";
                        }
                    } else if (this.Configs.markers.markAllPragarpahs) {
                        bgColor = this.Configs.markers.allParagraphsHighlight;
                    }
                }

                const ParagraphEl = UI.createElement(this.PageMarkableOverlays[_pageIndex], {
                    classes: ["tgpdf-paragraph", typeClass],
                    style: {
                        left: Math.round(par.BoundingBox.X0 - 2) + "px",
                        top: Math.round(par.BoundingBox.Y0 - 2) + "px",
                        width: Math.round(par.BoundingBox.X1 - par.BoundingBox.X0 + 4) + "px",
                        height: Math.round(par.BoundingBox.Y1 - par.BoundingBox.Y0 + 4) + "px",
                        backgroundColor: bgColor,
                    },
                    childs,
                });

                par.InnerSegments.forEach((line, index) => {
                    const Sentence = {
                        classes: ["tgpdf-sentence"],
                        style: {
                            left: Math.round(line.BoundingBox.X0) + "px",
                            top: Math.round(line.BoundingBox.Y0) + "px",
                            width: Math.round(line.BoundingBox.X1 - line.BoundingBox.X0) + "px",
                            height: Math.round(line.BoundingBox.Y1 - line.BoundingBox.Y0) + "px",
                        },
                        attrs: {
                            dataLoc: line.Location,
                            dataType: line.Type,
                            dataRealPageIdx: index > 0 ? _pageIndex : line.PageIndex,
                            dataPageIdx: line.PageIndex,
                            dataParIdx: line.ParIndex,
                            dataSntIdx: line.SntIndex,
                        },
                        events: {
                            click: (ev) => {
                                this.setActiveSentence(
                                    parseInt(ev.target.getAttribute("data-page-idx")),
                                    parseInt(ev.target.getAttribute("data-par-idx")),
                                    parseInt(ev.target.getAttribute("data-snt-idx")),
                                    parseInt(ev.target.getAttribute("data-loc"))
                                );
                            },
                        },
                    };
                    if (
                        this.Configs.markers.markSenetenceOnHover &&
                        (line.Type == enuContentType.Text || line.Type == enuContentType.List) &&
                        (this.Configs.markers.nonMainTextsCanBeSelected || line.Location == enuLocation.Main)
                    ) {
                        Sentence.classes.push("tgpdf-can-hover");
                        Sentence.events.mouseover = (ev) => {
                            const hovered = {
                                loc: ev.target.getAttribute("data-loc"),
                                page: ev.target.getAttribute("data-page-idx"),
                                par: ev.target.getAttribute("data-par-idx"),
                                snt: ev.target.getAttribute("data-snt-idx"),
                            };
                            [].forEach.call(this.ZoomHelper.querySelectorAll(".tgpdf-sentence"), (el) => {
                                if (el.classList.contains("tgpdf-active")) return;
                                if (
                                    el.getAttribute("data-loc") == hovered.loc &&
                                    el.getAttribute("data-page-idx") == hovered.page &&
                                    el.getAttribute("data-par-idx") == hovered.par &&
                                    el.getAttribute("data-snt-idx") == hovered.snt
                                ) {
                                    el.style.backgroundColor = this.Configs.markers.sentenceHoverColor;
                                } else {
                                    el.style.backgroundColor = "";
                                }
                            });
                        };
                        Sentence.events.mouseout = () => {
                            [].forEach.call(this.ZoomHelper.querySelectorAll(".tgpdf-sentence"), (el) => {
                                if (el.classList.contains("tgpdf-active")) return;
                                el.style.backgroundColor = "";
                            });
                        };
                    }
                    if (
                        this.Configs.markers.markSelectedSentence &&
                        line.Location === this.selected.loc &&
                        line.PageIndex === this.selected.page &&
                        line.ParIndex === this.selected.par &&
                        line.SntIndex === this.selected.snt
                    ) {
                        Sentence.style.backgroundColor = this.Configs.markers.activeSentenceHighlight;
                        Sentence.classes.push("tgpdf-active");
                    }

                    UI.createElement(this.PageMarkableOverlays[_pageIndex], Sentence).parentPar = ParagraphEl;
                });
                this.PageLoadingSpinners[_pageIndex].style.display = "none";
            });
            this.PageLoadingSpinners[_pageIndex].style.display = "none";
        });
    }

    async renderPage(_pageIndex) {
        if (!this.PdfDocument) return;
        await this.renderPageImage(_pageIndex);
        await this.renderPageMarkables(_pageIndex);
    }

    computePageScrollTop(_pageIndex) {
        let ScrollTop = 0;
        for (let i = 0; i < _pageIndex; ++i) ScrollTop += this.Scale * (this.PageSizes[i].Height + this.VertPageMargin);
        return ScrollTop;
    }

    computePageScrollLeft(_pageIndex) {
        return 0;
    }

    setActivePageIndex(_pageIndex) {
        if (_pageIndex >= this.PageCount || _pageIndex < 0 || _pageIndex == this.activePageIndex) return;

        this.ZoomHelper.parentElement.scrollTop = this.computePageScrollTop(_pageIndex);
        this.scheduleRenderVisiblePages();
    }

    async renderVisiblePages() {
        if (!this.PdfDocument) return;
        if (this.RenderCancelled) return;
        if (this.RenderPromise) {
            this.RenderCancelled = true;
            this.RenderPromise.then(() => this.renderVisiblePages());
        }
        this.RenderPromise = new Promise((resolve, _) => {
            let PageIndex = 0;
            while (!this.RenderCancelled && PageIndex < this.PageCount && !clsPdfDocumentViewer.isVisible(this.PageContainers[PageIndex])) ++PageIndex;
            let NewActivePageIndex = PageIndex;
            while (!this.RenderCancelled && PageIndex < this.PageCount && clsPdfDocumentViewer.isVisible(this.PageContainers[PageIndex])) {
                this.renderPage(PageIndex++);
                if (clsPdfDocumentViewer.visibilityPortion(this.PageContainers[PageIndex], "v") > 0.5) {
                    NewActivePageIndex = PageIndex;
                }
            }

            if (this.activePageIndex != NewActivePageIndex) {
                this.activePageIndex = NewActivePageIndex;
                this.dispatchEvent("activePageChanged", this.activePageIndex);
            }

            resolve();
        }).finally(() => {
            this.RenderCancelled = false;
            this.RenderPromise = null;
        });
    }

    async scheduleRenderVisiblePages(e) {
        if (!this.PdfDocument) return;
        if (this.RenderScheduleTimeout) clearTimeout(this.RenderScheduleTimeout);
        this.RenderScheduleTimeout = setTimeout(() => this.renderVisiblePages(), 100);
    }

    async handleMouseWheel(e) {
        if (!e.ctrlKey) return;

        e.stopImmediatePropagation();
        e.stopPropagation();
        e.preventDefault();

        if (!this.PdfDocument) return;

        if (e.target != this.ZoomHelper.parentElement && this.ZoomHelper.parentElement.contains(e.target)) {
            const NewScale = this.computeNewScale(e.deltaY);
            if (this.isValidScale(NewScale)) {
                this.setScale(NewScale, e.clientX, e.clientY);
                this.scheduleRenderVisiblePages();
            }
        }
    }
}

export default clsPdfDocumentViewer;
