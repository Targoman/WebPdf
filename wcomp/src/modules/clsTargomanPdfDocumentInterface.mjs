const clsTargomanPdfDocument = (function () {
    const PdfWorker = (() => {
        var _scriptDir = typeof document !== "undefined" && document.currentScript ? document.currentScript.src : undefined;
        if (typeof __filename !== "undefined") _scriptDir = _scriptDir || __filename;
        if (_scriptDir) {
            _scriptDir = _scriptDir.split("/");
            _scriptDir = _scriptDir.slice(0, _scriptDir.length - 1).join("/");
        } else {
            _scriptDir = ".";
        }

        let resolve,
            reject,
            onReady = new Promise((r, j) => {
                resolve = r;
                reject = j;
            }),
            callPromiseIndex = 0,
            pdfWorker,
            callPromises = {};

        const prepare = () => {
            pdfWorker = new Worker(`${_scriptDir}/webpdf.js`);

            pdfWorker.onerror = (e) => console.error("PDFWorker", e);
            pdfWorker.onmessage = (e) => {
                switch (e.data.type) {
                    case "initialized":
                        for (let enumName in e.data.enums) window[enumName] = e.data.enums[enumName];
                        resolve();
                        break;
                    case "initializationError":
                        reject(e.data.error);
                        break;
                    case "callResult":
                        if (callPromises.hasOwnProperty(e.data.promiseIndex)) {
                            let resolve = callPromises[e.data.promiseIndex][0];
                            delete callPromises[e.data.promiseIndex];
                            resolve(e.data.result);
                        }
                        break;
                    case "callError":
                        if (callPromises.hasOwnProperty(e.data.promiseIndex)) {
                            let reject = callPromises[e.data.promiseIndex][1];
                            delete callPromises[e.data.promiseIndex];
                            reject(e.data.error);
                        }
                        break;
                }
            };
        };

        const call = (method, params, transferrables, isProperty) => {
            let currentIndex = callPromiseIndex++;
            let resolve,
                reject,
                result = new Promise((r, j) => {
                    resolve = r;
                    reject = j;
                });
            callPromises[currentIndex] = [resolve, reject];
            setTimeout(() => {
                if (callPromises.hasOwnProperty(currentIndex)) {
                    let reject = callPromises[currentIndex][1];
                    delete callPromises[currentIndex];
                    reject({ error: "Worker timed out." });
                }
            }, 60000);
            let message = {
                type: "call",
                method,
                params,
                isProperty: isProperty === true,
                promiseIndex: currentIndex,
            };
            pdfWorker.postMessage(message, transferrables);
            return result;
        };

        return {
            call,
            prepare,
            onReady: () => onReady,
        };
    })();

    class clsTargomanPdfDocument {
        static defaultConfigs() {
            return {
                RemoveWatermarks: true,
                DiscardTablesAndCaptions: true,
                DiscardImagesAndCaptions: true,
                DiscardHeaders: true,
                DiscardFooters: true,
                DiscardSidebars: true,
                DiscardFootnotes: true,
                AssumeLastAloneLineAsFooter: true,
                AssumeFirstAloneLineAsHeader: true,
                DiscardItemsIn2PercentOfPageMargin: true,
                MarkSubcripts: true,
                MarkSuperScripts: true,
                MarkBullets: true,
                MarkNumberings: true,
                ASCIIOffset: 0
            };
        }

        static prepare() {
            PdfWorker.prepare();
            return PdfWorker.onReady();
        }

        constructor() {
            this.ready = PdfWorker.onReady()
                .then(() => PdfWorker.call("new", [], []).then((handle) => (this.handle = handle)))
                .then(() => this);
        }

        loadPdf(buffer, configs) {
            return PdfWorker.call("loadPdf", [this.handle, buffer, configs], [buffer]);
        }

        get PageCount() {
            return PdfWorker.call("PageCount", [this.handle], [], true);
        }

        pageSize(_pageIndex) {
            return PdfWorker.call("pageSize", [this.handle, _pageIndex], []);
        }

        getPDFBuffer() {
            return PdfWorker.call("getPDFBuffer", [this.handle], []);
        }

        getAllPageSizes() {
            return PdfWorker.call("getAllPageSizes", [this.handle], []);
        }

        getPageLabel(_pageIndex) {
            return PdfWorker.call("pageLabel", [this.handle, _pageIndex], []);
        }

        pageNoByLabel(_label) {
            return PdfWorker.call("pageNoByLabel", [this.handle, _label], []);
        }

        getPDFDocInfo(_pageIndex) {
            return PdfWorker.call("getPDFDocInfo", [this.handle, _pageIndex], []);
        }

        getPageImage(_pageIndex, _backgroundColor, size) {
            return PdfWorker.call("getPageImage", [this.handle, _pageIndex, _backgroundColor, size], []);
        }

        getMarkables(_pageIndex) {
            return PdfWorker.call("getMarkables", [this.handle, _pageIndex], []);
        }

        setConfigs(configs) {
            return PdfWorker.call("setConfigs", [this.handle, configs], []);
        }

        setCurrentSentence(_pageIndex, _parIndex, _sntIndex, _loc) {
            return PdfWorker.call("setCurrentSentence", [this.handle, _pageIndex, _parIndex, _sntIndex, _loc], []);
        }

        gotoNextSentence() {
            return PdfWorker.call("gotoNextSentence", [this.handle], []);
        }

        gotoPrevSentence() {
            return PdfWorker.call("gotoPrevSentence", [this.handle], []);
        }

        getSentenceContent() {
            return PdfWorker.call("getSentenceContent", [this.handle], []);
        }

        get SentenceVirtualPageIdx() {
            return PdfWorker.call("SentenceVirtualPageIdx", [this.handle], [], true);
        }

        get SentenceRealPageIdx() {
            return PdfWorker.call("SentenceRealPageIdx", [this.handle], [], true);
        }

        get SentenceParIdx() {
            return PdfWorker.call("SentenceParIdx", [this.handle], [], true);
        }
        get SentenceIdx() {
            return PdfWorker.call("SentenceIdx", [this.handle], [], true);
        }

        delete() {
            return PdfWorker.call("delete", [this.handle], []);
        }
    }

    return clsTargomanPdfDocument;
})();

export default clsTargomanPdfDocument;
