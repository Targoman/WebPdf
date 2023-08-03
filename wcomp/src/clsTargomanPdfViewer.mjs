import clsEventTarget from "./modules/clsEventTarget.mjs";
import UI from "./modules/UI.mjs";
import l10n from "./modules/l10n.mjs";
import clsTargomanPdfViewerPrivate from "./modules/clsTargomanPdfViewerPrivate.mjs";
import clsTargomanPdfDocumentInterface from "./modules/clsTargomanPdfDocumentInterface.mjs";

class clsTargomanPdfViewer extends clsEventTarget {
    static availableTools() {
        return Object.keys(clsTargomanPdfViewerPrivate.availableToolsSpecs());
    }
    static availableSidebarTabs() {
        return Object.keys(clsTargomanPdfViewerPrivate.availableSidebarTabsSpecs());
    }

    static defaultOptions() {
        return {
            lang: "fa",
            ui: {
                navbar: {
                    enabled: true,
                    right: ["openFile", "separator", "openMenu"],
                    middle: ["zoomIn", "separator:show-sm:show-md", "zoomSelector:hide-md", "zoomOut"],
                    left: [
                        "toggleSidebar",
                        "spacer:hide-sm",
                        "fullScreen:hide-sm",
                        "separator:hide-sm",
                        "download:hide-sm",
                        "separator:hide-sm",
                        "pageSelector",
                        "prevPage:hide-sm",
                        "nextPage:hide-sm",
                    ],
                },
                sidebar: ["thumbnail", "outline", "attachments", "settings"],
                menu: [
                    "fullScreen:show-sm",
                    "download:show-sm",
                    "menuSeparator:show-sm",
                    "firstPage",
                    "nextPage:show-sm",
                    "prevPage:show-sm",
                    "lastPage",
                    "rotateCw",
                    "rotateCcw",
                    "menuSeparator",
                    "docProps",
                    "menuSeparator",
                    "about",
                ],
            },
            splash: {
                classes: ["tgpdf-splash-default"],
                childs: [
                    {
                        childs: [
                            { tag: "h2", text: "about_title" },
                            { tag: "img", attrs: { src: "./images/logo.gif" } },
                            { tag: "p", text: "about_text" },
                            { tag: "button", text: "about_call_action" },
                        ],
                    },
                ],
            },
            settings: {
                markers: {
                    markAllPragarpahs: true,
                    markAllNonTexts: true,
                    markSelectedSentence: true,
                    markSelectedParagraph: true,
                    markSenetenceOnHover: true,
                    nonMainTextsCanBeSelected: false,
                    activeParagraphHighlight: "#FFFFFFFF",
                    activeSentenceHighlight: "#FFFF00AA",
                    sentenceHoverColor: "#FF220022",
                    allParagraphsHighlight: "#FFFFFF55",
                    nonTextMarksHighlight: "#00007777",
                },
                layout: clsTargomanPdfDocumentInterface.defaultConfigs(),
            },
            controls: {
                handleKeyboardEvents: true,
            },
        };
    }

    static makeButton(name, onclick, defaultDisabled = false, extraClass = null) {
        return UI.makeButtonSpecs(name, onclick, defaultDisabled, extraClass);
    }

    static makeCheckBox(key, title, checked, parentName) {
        return UI.makeCheckboxSpecs(key, title, checked, parentName);
    }

    constructor(root, options) {
        super();
        this.activeOptions = options || clsTargomanPdfViewer.defaultOptions();
        if (this.activeOptions.lang != "en") l10n.loadTranslations(this.activeOptions.lang);
        if (this.activeOptions.customDic) l10n.setCustomTranslation(this.activeOptions.customDic);
        this.private = new clsTargomanPdfViewerPrivate(root, this);

        if (this.activeOptions.controls.handleKeyboardEvents) {
            document.addEventListener("keyup", (ev) => {
                if (ev.code == "ArrowUp") {
                    this.prevSentence();
                    ev.preventDefault();
                    ev.stopPropagation();
                } else if (ev.code == "ArrowDown") {
                    this.nextSentence();
                    ev.preventDefault();
                    ev.stopPropagation();
                }
            });
        }
    }

    setControlDisabled(name, state) {
        if (this.private.ui.controls[name]) UI.setDisabled(this.private.ui.controls[name], state);
    }

    setActiveSentence(pageIndex = -1, parIndex = -1, sntIndex = -1, loc = enuLocation.Main) {
        return this.private.PdfDocumentViewer.setActiveSentence(pageIndex, parIndex, sntIndex, loc);
    }

    nextSentence() {
        return this.private.PdfDocumentViewer.nextSentence();
    }

    prevSentence() {
        return this.private.PdfDocumentViewer.prevSentence();
    }

    async activeSentenceContent() {
        return await this.private.PdfDocumentViewer.activeSentenceContent();
    }
}

export default clsTargomanPdfViewer;
