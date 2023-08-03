import UI from "./UI.mjs";
import clsPdfDocumentViewer from "./clsPdfDocumentViewer.mjs";
import clsTargomanPdfDocumentInterface from "./clsTargomanPdfDocumentInterface.mjs";

class clsTargomanPdfViewerPrivate {
    static availableSidebarTabsSpecs(parent = null) {
        return {
            settings: UI.makeButtonSpecs("settings", () => parent && parent.activateSidebarTab("settings")),
            thumbnail: UI.makeButtonSpecs("thumbnail", () => parent && parent.activateSidebarTab("thumbnail"), true),
            outline: UI.makeButtonSpecs("outline", () => parent && parent.activateSidebarTab("outline"), true),
            attachments: UI.makeButtonSpecs("attachments", () => parent && parent.activateSidebarTab("attachments"), true),
        };
    }

    static availableToolsSpecs(parent = null) {
        return {
            separator: { class: "tgpdf-spacer-v" },
            spacer: { classes: "tgpdf-tbar-spacer" },
            about: UI.makeButtonSpecs("about"),
            buy: UI.makeButtonSpecs("buy", null, true),
            credit: UI.makeButtonSpecs("credit", null, true),
            docProps: UI.makeButtonSpecs("docProps", parent && parent.showDocProps.bind(parent), true),
            download: UI.makeButtonSpecs("download", parent && parent.download.bind(parent), true),
            findNext: UI.makeButtonSpecs("findNext", parent && parent.findNext.bind(parent), true),
            findPrev: UI.makeButtonSpecs("findPrev", parent && parent.findPrev.bind(parent), true),
            firstPage: UI.makeButtonSpecs("firstPage", () => parent && parent.gotoPage.bind(parent)(1), true),
            lastPage: UI.makeButtonSpecs("lastPage", () => parent && parent.gotoPage.bind(parent)(-1), true),
            fullScreen: UI.makeButtonSpecs("fullScreen", parent && parent.fullScreen.bind(parent)),
            handTool: UI.makeButtonSpecs("handTool", () => parent && parent.setSelectionMode.bind(parent)("hand"), true),
            logout: UI.makeButtonSpecs("logout"),
            openFile: UI.makeButtonSpecs("openFile", parent && parent.openNew.bind(parent)),
            openMenu: UI.makeButtonSpecs("openMenu", parent && parent.openMenu.bind(parent)),
            nextPage: UI.makeButtonSpecs("nextPage", parent && parent.nextPage.bind(parent), true),
            prevPage: UI.makeButtonSpecs("prevPage", parent && parent.prevPage.bind(parent), true),
            print: UI.makeButtonSpecs("print", parent && parent.print.bind(parent), true),
            rotateCw: UI.makeButtonSpecs("rotateCw", parent && parent.rotatePage.bind(parent)(90), true),
            rotateCcw: UI.makeButtonSpecs("rotateCcw", parent && parent.rotatePage.bind(parent)(-90), true),
            scrollHorz: UI.makeButtonSpecs("scrollHorz", () => parent && parent.scroll.bind(parent)("h"), true),
            scrollVert: UI.makeButtonSpecs("scrollVert", () => parent && parent.scroll.bind(parent)("v"), true),
            scrollWrapped: UI.makeButtonSpecs("scrollWrapped", () => parent && parent.scroll.bind(parent)("w"), true),
            search: UI.makeButtonSpecs("search", parent && parent.showSearchDialog.bind(parent), true),
            toggleSidebar: UI.makeButtonSpecs("toggleSidebar", parent && parent.toggleSidebar.bind(parent), true),
            spreadEven: UI.makeButtonSpecs("spreadEven", () => parent && parent.setSpread.bind(parent)("e"), true),
            spreadNone: UI.makeButtonSpecs("spreadNone", () => parent && parent.setSpread.bind(parent)("n"), true),
            spreadOdd: UI.makeButtonSpecs("spreadOdd", () => parent && parent.setSpread.bind(parent)("o"), true),
            zoomIn: UI.makeButtonSpecs("zoomIn", () => parent && parent.zoomTo.bind(parent)("+1"), true),
            zoomOut: UI.makeButtonSpecs("zoomOut", () => parent && parent.zoomTo.bind(parent)("-1"), true),

            pageSelector: {
                tag: "span",
                classes: ["tgpdf-pageselector", "tgpdf-hide-xs"],
                attrs: { disabled: true },
                style: { display: "inline-block" },
                childs: [
                    { tag: "input", attrs: { type: "number", min: 0, value: 1 }, events: { change: parent && parent.gotoPage } },
                    { tag: "label", text: "(" },
                    { tag: "label", text: "-", attrs: { dataId: "page-no" } },
                    { tag: "label", text: " of " },
                    { tag: "label", text: "-", attrs: { dataId: "page-count" } },
                    { tag: "label", text: ")" },
                ],
            },
            zoomSelector: {
                tag: "span",
                classes: ["tgpdf-dropdown", "tgpdf-hide-sm"],
                attrs: { disabled: true },
                childs: [
                    {
                        tag: "select",
                        attrs: { title: "Zoom" },
                        childs: [
                            { tag: "option", attrs: { value: "auto", selected: "selected" }, text: "Automatic Zoom" },
                            { tag: "option", attrs: { value: "actual" }, text: "Actual Size" },
                            // { tag: "option", attrs: { value: "pageFit" }, text: "Page Fit" },
                            // { tag: "option", attrs: { value: "pageWidth" }, text: "Page Width" },
                            { class: "tgpdf-spacer-h" },
                            { tag: "option", attrs: { value: 0.5 }, text: "50%" },
                            { tag: "option", attrs: { value: 0.75 }, text: "75%" },
                            { tag: "option", attrs: { value: 1.0 }, text: "100%" },
                            { tag: "option", attrs: { value: 2.0 }, text: "200%" },
                            { tag: "option", attrs: { value: 3.0 }, text: "300%" },
                            { tag: "option", attrs: { value: 4.0 }, text: "400%" },
                        ],
                        events: {
                            change: (ev) => parent && parent.zoomTo(ev.target.value),
                        },
                    },
                ],
            },
            menuSeparator: { class: "tgpdf-mnu-separator" },
        };
    }

    constructor(root, parent) {
        this.activeSidebarTab = null;
        this.parent = parent;

        if (
            this.parent.activeOptions.splash &&
            this.parent.activeOptions.splash.childs &&
            this.parent.activeOptions.splash.childs[0].childs &&
            this.parent.activeOptions.splash.childs[0].childs.length == 4 &&
            this.parent.activeOptions.splash.childs[0].childs[3].tag == "button" &&
            !this.parent.activeOptions.splash.childs[0].childs[3].events
        ) {
            this.parent.activeOptions.splash.childs[0].childs[3].events = {
                click: this.openNew.bind(this),
            };
        }

        this.ui = UI.construct(this, root, this.parent.activeOptions, clsTargomanPdfViewerPrivate.availableToolsSpecs(this), clsTargomanPdfViewerPrivate.availableSidebarTabsSpecs(this));
        this.ui.fileInput.addEventListener("change", this.openPDFfile.bind(this));

        let progress = 1;
        const interval = setInterval(() => {
            this.ui.wasmLoader.progress.style.width = progress + "%";
            progress += 0.25;
            if (progress >= 100) clearInterval(interval);
        }, 50);

        //this.ui.

        clsTargomanPdfDocumentInterface.prepare().then(() => {
            this.ui.wasmLoader.style.display = "none";
            if (this.ui.splash) this.ui.splash.classList.add("tgpdf-active");
            this.PdfDocumentViewer = new clsPdfDocumentViewer(this.ui.viewer, this.parent.activeOptions.settings, 18, 18);

            this.PdfDocumentViewer.addEventListener("pdfOpened", (ev) => {
                UI.setDisabled(this.ui.controls.nextPage, false);
                UI.setDisabled(this.ui.controls.download, false);
                UI.setDisabled(this.ui.controls.zoomIn, false);
                UI.setDisabled(this.ui.controls.zoomOut, false);
                UI.setDisabled(this.ui.controls.zoomSelector, false);
                UI.setDisabled(this.ui.controls.pageSelector, false);
                UI.setDisabled(this.ui.controls.toggleSidebar, false);
                UI.setDisabled(this.ui.controls.firstPage, false);
                UI.setDisabled(this.ui.controls.lastPage, false);
                UI.setDisabled(this.ui.controls.docProps, false);
                if (this.ui.controls.pageSelector)
                    this.ui.controls.pageSelector.forEach((el) => {
                        this.updatePageInfo.bind(this)(el, 0);
                    });

                this.dispatchEvent("pdfOpened", { fileName: this.PdfDocumentViewer.fileName });
            });
            this.PdfDocumentViewer.addEventListener("activePageChanged", (_pageIndex) => {
                if (this.ui.controls.pageSelector) {
                    this.ui.controls.pageSelector.forEach((el) => this.updatePageInfo.bind(this)(el, _pageIndex));
                }
                this.dispatchEvent("activePageChanged", _pageIndex);
            });
            this.PdfDocumentViewer.addEventListener("activeSentenceChanged", (specs) => {
                this.PdfDocumentViewer.activeSentenceContent().then((text) => {
                    this.dispatchEvent("activeSentenceChanged", { specs, text });
                });
            });
        });
    }

    dispatchEvent(name, params) {
        this.parent.dispatchEvent(name, params);
    }

    openMenu() {
        if (this.ui.menu.classList.contains("tgpdf-hidden")) {
            this.ui.menu.classList.remove("tgpdf-hidden");
        } else {
            this.ui.menu.classList.add("tgpdf-hidden");
        }
    }

    showSearchDialog() {}

    findNext() {}

    findPrev() {}

    nextPage() {
        this.PdfDocumentViewer.setActivePageIndex(this.PdfDocumentViewer.activePageIndex + 1);
    }

    prevPage() {
        this.PdfDocumentViewer.setActivePageIndex(this.PdfDocumentViewer.activePageIndex - 1);
    }

    async gotoPage(ev) {
        if (ev == 1) return this.PdfDocumentViewer.setActivePageIndex(0);
        if (ev == -1) return this.PdfDocumentViewer.setActivePageIndex(this.PdfDocumentViewer.PageCount - 1);

        const label = ev.target.value;
        let PageNo = await this.PdfDocumentViewer.PdfDocument.pageNoByLabel(label);
        if (PageNo < 0 && parseInt(label) > 0) PageNo = parseInt(label);
        this.PdfDocumentViewer.setActivePageIndex(PageNo - 1);
    }

    rotatePage(clockwise) {}

    zoomTo(scale) {
        this.PdfDocumentViewer.setAbsoluteScale(scale);
    }

    fullScreen(ev) {
        if (this.FullScreenMode) {
            if (document.exitFullscreen) {
                document.exitFullscreen();
            } else if (document.mozCancelFullScreen) {
                /* Firefox */
                document.mozCancelFullScreen();
            } else if (document.webkitExitFullscreen) {
                /* Chrome, Safari and Opera */
                document.webkitExitFullscreen();
            } else if (document.msExitFullscreen) {
                /* IE/Edge */
                document.msExitFullscreen();
            }
            if (ev && ev.target) ev.target.classList.remove("tgpdf-toggled");
            this.FullScreenMode = false;
        } else {
            if (this.ui.container.requestFullscreen) {
                this.ui.container.requestFullscreen();
            } else if (this.ui.container.mozRequestFullScreen) {
                /* Firefox */
                this.ui.container.mozRequestFullScreen();
            } else if (this.ui.container.webkitRequestFullscreen) {
                /* Chrome, Safari and Opera */
                this.ui.container.webkitRequestFullscreen();
            } else if (this.ui.container.msRequestFullscreen) {
                /* IE/Edge */
                this.ui.container.msRequestFullscreen();
            } else return;
            if (ev && ev.target) ev.target.classList.add("tgpdf-toggled");
            this.FullScreenMode = true;
        }
    }

    async download() {
        const blob = new Blob([await this.PdfDocumentViewer.PdfDocument.getPDFBuffer()], { type: "application/pdf" });
        const dummyLink = document.createElement("a");
        if (!dummyLink.click) throw new Error('DownloadManager: "a.click()" is not supported.');
        dummyLink.href = window.URL.createObjectURL(blob);
        dummyLink.target = "_parent";
        "download" in dummyLink && (dummyLink.download = this.PdfDocumentViewer.fileName);
        (document.body || document.documentElement).appendChild(dummyLink);
        dummyLink.click();
        dummyLink.remove();
    }

    openNew() {
        this.ui.fileInput.click();
    }

    showDocProps() {
        this.PdfDocumentViewer.getPDFDocInfo(this.PdfDocumentViewer.activePageIndex).then((info) => console.log(info));
    }

    print() {}

    setSpread(spread) {}

    scroll(type) {}

    setSelectionMode() {}

    configChanged(section, key, value) {
        console.log(section, key, value);
        const settings = this.parent.activeOptions.settings;
        if (section === "layout") {
            if (settings.layout[key] != value) {
                settings.layout[key] = value;
                this.PdfDocumentViewer.PdfDocument.setConfigs(settings.layout).then(() => {
                    this.PdfDocumentViewer.renderVisiblePages();
                });
            }
        } else if (section == "markers") {
            if (settings.markers[key] != value) {
                settings.markers[key] = value;
                this.PdfDocumentViewer.renderVisiblePages();
            }
        }

        this.dispatchEvent("configChanged", section, key, value);
    }

    controlClicked(name) {
        this.dispatchEvent(name);
    }

    async updatePageInfo(el, pageIndex) {
        el.firstElementChild.value = await this.PdfDocumentViewer.PdfDocument.getPageLabel(pageIndex);
        el.children[2].innerText = pageIndex + 1;
        el.children[4].innerText = this.PdfDocumentViewer.PageCount;
        if (pageIndex < this.PdfDocumentViewer.PageCount) UI.setDisabled(this.ui.controls.nextPage, false);
        else UI.setDisabled(this.ui.controls.nextPage, true);
        if (pageIndex == 0) UI.setDisabled(this.ui.controls.prevPage, true);
        else UI.setDisabled(this.ui.controls.prevPage, false);
    }

    activateSidebarTab(tab) {
        if (!this.ui.sidebar.tools || !this.ui.sidebar.tools.btns || !this.ui.sidebar.tools.btns[tab]) return;
        for (const key in this.ui.sidebar.tools.btns) this.ui.sidebar.tools.btns[key].classList.remove("tgpdf-toggled");
        this.activeSidebarTab = this.ui.sidebar.tools.btns[tab];
        this.activeSidebarTab.classList.add("tgpdf-toggled");
        for (const key in this.ui.sidebar.tools.btns) this.ui.sidebar.content[key].classList.add("tgpdf-hidden");
        const activeContent = this.ui.sidebar.content[tab];
        activeContent.classList.remove("tgpdf-hidden");
    }

    toggleSidebar(ev) {
        if (!this.ui.sidebar.tools) return;
        if (ev.target.classList.contains("tgpdf-toggled") === false) {
            ev.target.classList.add("tgpdf-toggled");
            this.ui.container.classList.add("tgpdf-sidebarMoving", "tgpdf-sidebarOpen");
            setTimeout(() => this.ui.container.classList.remove("tgpdf-sidebarMoving"), 200);
            if (this.activeSidebarTab === null) {
                [].some.call(this.ui.sidebar.tools.children, (el) => {
                    if (el.getAttribute("disabled")) return false;
                    this.activateSidebarTab(el.getAttribute("name"));
                    return true;
                });
            }

            //hideui notif
            //
        } else {
            ev.target.classList.remove("tgpdf-toggled");
            this.ui.container.classList.add("tgpdf-sidebarMoving");
            this.ui.container.classList.remove("tgpdf-sidebarOpen");
        }
    }

    openPDFfile(e) {
        if (e.target.files.length > 0) {
            let file = e.target.files[0];
            if (!file.name.endsWith(".pdf")) {
                console.error("INPUT FILE IS NOT PDF!");
                return;
            }
            if (this.ui.splash) {
                this.ui.splash.classList.remove("tgpdf-active");
                setTimeout(() => {
                    this.ui.splash.classList.add("tgpdf-hidden");
                }, 1000);
            }

            this.PdfFileReader = new FileReader();
            this.PdfFileReader.onload = async () => {
                console.log("LOADING PDF FILE DONE.", elapsedTime());
                this.PdfDocumentViewer.open(this.PdfFileReader.result, file.name);
            };
            console.log("LOADING PDF FILE ...", markStartTime());
            this.PdfFileReader.readAsArrayBuffer(file);
        }
    }
}

export default clsTargomanPdfViewerPrivate;
