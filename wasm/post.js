function webWorkerImpl() {
    let TGPDF = null;

    var _scriptName = typeof self !== "undefined" && self.location && self.location.href ? self.location && self.location.href : undefined;
    if (typeof __filename !== "undefined") _scriptName = _scriptName || __filename;
    if (_scriptName) _scriptName = _scriptName.split(".js")[0];

    let resolve,
        reject,
        ready = new Promise((r, j) => {
            resolve = r;
            reject = j;
        });

    try {
        if (calledRun) {
            TGPDF = Module;
            resolve();
        } else {
            // we are not ready to call then() yet. we must call it
            // at the same time we would call onRuntimeInitialized.
            var old = Module["onRuntimeInitialized"];
            Module["onRuntimeInitialized"] = function () {
                if (old) old();
                TGPDF = Module;
                resolve();
            };
        }
    } catch (e) {
        reject(e);
    }

    const postResults = (result, promiseIndex, transferrables) => {
        self.postMessage(
            {
                type: "callResult",
                result,
                promiseIndex,
            },
            transferrables
        );
    };

    const postError = (e, promiseIndex) => {
        self.postMessage(
            {
                type: "callError",
                error: typeof e == "string" ? new Error(e) : e,
                promiseIndex,
            },
            []
        );
    };

    let instances = {};
    let instanceIndex = 0;

    self.takeHeapSlice = function (offset, size) {
        return TGPDF.HEAPU8.buffer.slice(offset, offset + size);
    };

    const isArray = (v) => v instanceof Array || (v && v.buffer && v.buffer instanceof ArrayBuffer && ArrayBuffer.isView(v, v.buffer));
    const isStdVector = (v) => typeof v.size === "function" && typeof v.get === "function";

    let enums = [];

    const sanitizeResult = (v, doDelete = true) => {
        if (!v || typeof v !== "object" || v instanceof ArrayBuffer || isArray(v)) return v;
        for (let enum_ of enums) if (v instanceof enum_) return v.value;
        let sanitized;
        if (isStdVector(v)) {
            sanitized = new Array(v.size()).fill().map((_, i) => v.get(i));
            for (let i = 0; i < sanitized.length; ++i) sanitized[i] = sanitizeResult(sanitized[i], false);
            if (doDelete) v.delete();
        } else {
            for (let k in v) v[k] = sanitizeResult(v[k], doDelete);
            sanitized = v;
        }
        return sanitized;
    };

    self.onmessage = (e) => {
        switch (e.data.type) {
            case "call":
                if (e.data.method === "new") {
                    instances[instanceIndex] = TGPDF.clsPdfDocument.new();
                    postResults(instanceIndex, e.data.promiseIndex, []);
                    ++instanceIndex;
                } else if (e.data.method === "delete") {
                    const [instanceIndex] = e.data.params;
                    const promiseIndex = e.data.promiseIndex;
                    if (!(instanceIndex in instances)) postError("Class instance does not exist.", promiseIndex);
                    else {
                        let instance = instances[instanceIndex];
                        TGPDF._free(instance.__bufferOffset);
                        instance.delete();
                        postResults(true, e.data.promiseIndex, []);
                    }
                } else {
                    const [instanceIndex, ...params] = e.data.params;
                    let newParams = params;
                    const promiseIndex = e.data.promiseIndex;
                    if (!(instanceIndex in instances)) postError("Class instance does not exist.", promiseIndex);
                    else {
                        let instance = instances[instanceIndex];
                        if (e.data.method === "getPDFBuffer") {
                            const offset = instances[instanceIndex].__bufferOffset;
                            const length = instances[instanceIndex].__bufferLength;
                            let result = TGPDF.HEAPU8.buffer.slice(offset, offset + length);
                            let transferrables = [result];
                            postResults(result, promiseIndex, transferrables);
                            return;
                        }
                        if (!(e.data.method in instance)) postError(`Method does not exist: ${e.data.method}`, promiseIndex);
                        else {
                            if (e.data.method === "loadPdf") {
                                const [instanceIndex, buffer, configs] = e.data.params;
                                let offset = TGPDF._malloc(buffer.byteLength);
                                instances[instanceIndex].__bufferOffset = offset;
                                instances[instanceIndex].__bufferLength = buffer.byteLength;
                                TGPDF.HEAPU8.set(new Uint8Array(buffer), offset);
                                newParams = [offset, buffer.byteLength, configs];
                            }
                            ready.then(
                                () => {
                                    try {
                                        let result;
                                        if (e.data.isProperty) {
                                            if (newParams.length != 0) throw new Error("Properties can't have params.");
                                            result = instance[e.data.method];
                                        } else result = instance[e.data.method].apply(instance, newParams, promiseIndex);
                                        result = sanitizeResult(result);
                                        let transferrables = result instanceof ArrayBuffer ? [result] : [];
                                        postResults(result, promiseIndex, transferrables);
                                    } catch (e) {
                                        postError(e, promiseIndex);
                                    }
                                },
                                (e) => {
                                    postError(e, promiseIndex);
                                }
                            );
                        }
                    }
                }
                break;
        }
    };

    function convertWasmEnum(enum_) {
        let result = {};
        let values = enum_.values;
        for (let k in enum_)
            if (typeof enum_[k].value !== "undefined" && values[enum_[k].value] == enum_[k]) {
                result[k] = enum_[k].value;
            }
        return result;
    }

    function isEnum(v) {
        if (!v || typeof v !== "function" || v instanceof ArrayBuffer || isArray(v)) return false;
        if (!("values" in v) || typeof v.values !== "object") return false;
        for (let k in v.values) if (!(v.values[k] instanceof v)) return false;
        return true;
    }

    ready.then(
        () => {
            let enumsMap = {};

            for (let k in TGPDF)
                if (isEnum(TGPDF[k])) {
                    TGPDF[k].enumName = k;
                    enumsMap[k] = convertWasmEnum(TGPDF[k]);
                    enums.push(TGPDF[k]);
                }

            self.postMessage({
                type: "initialized",
                enums: enumsMap,
            });
        },
        (e) => {
            self.postMessage({
                type: "initializationError",
                error: e,
            });
        }
    );
}

webWorkerImpl();
