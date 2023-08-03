/**************************************************************/
export function safeGet(obj, key) {
    if (!obj || typeof obj != "object") return null;
    const keyParts = key.split(".");
    let o = obj;
    let k = keyParts[0];
    for (let i = 0; i < keyParts.length - 1; ++i) {
        if (!(k in o)) return null;
        o = o[k];
        k = keyParts[i + 1];
    }
    if (k) {
        if (Array.isArray(o[k]))
            if (o[k].length == 0) return null;
            else if (isObject(o[k])) if (o[k].length == 0) return null;

        return o[k];
    }

    return null;
}

export function getKeyName(obj, key) {
    for (const k in obj) {
        if (obj[k] === key)
            return k;
    }
}

/**************************************************************/
export function isObject(o) {
    return typeof o === "object" && o != null;
}

/**************************************************************/
export function camel2Dash(key) {
    return key.replace(/([A-Z])/g, "-$1").toLowerCase();
}

/**************************************************************/
export function setStyle(el, styles, merge = false) {
    const oldStyleString = el.getAttribute("style");

    if (merge) {
        const oldStyles = {};
        oldStyleString.split(";").forEach((item) => {
            const itemParts = item.split(":");
            oldStyles[itemParts[0]] = itemParts.splice(1).join(":");
        });
        for (key in styles) {
            oldStyles[camel2Dash(key)] = styles[key];
        }
        styles = oldStyles;
    }
    const style = [];
    Object.keys(styles).forEach((key) => {
        style.push(`${camel2Dash(key)}:${styles[key]}`);
    });
    el.setAttribute("style", style.join(";"));
}
