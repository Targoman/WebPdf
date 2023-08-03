import { safeGet, isObject, camel2Dash, setStyle } from "./common.mjs";
import l10n from "./l10n.mjs";

/**************************************************************/
function makeButtonSpecs(name, onclick = null, defaultDisabled = false, extraClass = null) {
    const classes = ["tgpdf-tbtn", `tgpdf-i-${name}`];
    if (extraClass) classes.push(extraClass);
    const attrs = { title: name, name };
    if (defaultDisabled) attrs.disabled = true;

    return {
        tag: "button",
        l10n: name,
        attrs,
        classes,
        events: { click: onclick },
        childs: [{ tag: "span", l10n: `${name}.label`, text: name }],
    };
}

/**************************************************************/
function makeCheckboxSpecs(key, title, checked, parent) {
    const inputAttrs = {
        type: "checkbox",
        name: key,
        value: key,
    };
    if (checked) inputAttrs.checked = true;
    if (parent) inputAttrs.parent = parent;

    return {
        tag: "li",
        childs: [
            { tag: "input", attrs: inputAttrs },
            { tag: "label", l10n: key, attrs: { for: key }, text: title },
        ],
    };
}

/**************************************************************/
function setDisabled(el, state) {
    if (!el) return;
    if (Array.isArray(el)) el.forEach((itm) => setDisabled(itm, state));
    else {
        if (state) el.setAttribute("disabled", true);
        else el.removeAttribute("disabled");
    }
}

/**************************************************************/
function createElement(parent, config) {
    try {
        const applyByType = (opt, callbacks) => {
            if (!opt) return;
            if (Array.isArray(opt) && callbacks.onArray) for (const a of opt) callbacks.onArray(a);
            else if (isObject(opt) && callbacks.onObject) for (const key in opt) callbacks.onObject(camel2Dash(key), opt[key]);
            else if (typeof opt === "string" && callbacks.onString) callbacks.onString(opt);
            else {
                console.error("No handler found for", opt);
                throw "No handler found";
            }
        };

        config.tag = config.tag || "div";
        const el = document.createElement(config.tag);

        applyByType(config.class, { onString: (v) => (el.className = v) });
        applyByType(config.classes, {
            onString: (v) => (el.className = v),
            onArray: (v) => el.classList.add(v),
        });

        if (config.style) setStyle(el, config.style);

        applyByType(config.attrs, {
            onObject: (k, v) => el.setAttribute(k, l10n.tr(v, config, k)),
        });

        if (config.l10n) el.setAttribute("data-l10n-id", config.l10n);

        applyByType(config.childs, {
            onArray: (c) => el.appendChild(createElement(el, c)),
        });
        applyByType(config.events, {
            onObject: (k, v) => el.addEventListener(k, v),
        });

        if (config.id) el.id = config.id;
        if (config.html) el.innerHTML = config.html;
        if (config.text) el.innerText = l10n.tr(config.text, config);

        if (parent) parent.appendChild(el);
        return el;
    } catch (e) {
        console.error(e, parent, config);
        throw e.message;
    }
}

function construct(parent, root, options, availableTools, availableSidebarTabs) {
    const ui = {};
    ui.controls = {};

    /**************************************************************/
    function applyControls(container, configs, tools) {
        if (Array.isArray(configs)) {
            configs.forEach((conf) => {
                let extraClasses = null;
                if (typeof conf === "string" && conf.indexOf(":") > 0) {
                    extraClasses = conf.split(":").slice(1);
                    conf = conf.split(":")[0];
                }

                const key = conf;
                if (typeof conf === "string") conf = (tools || availableTools)[conf];
                if (Array.isArray(conf)) {
                    console.error(conf);
                    throw "config must be string or object";
                } else if (isObject(conf)) {
                    if (conf.class) {
                        conf.classes = [conf.class];
                        delete conf.class;
                    }
                    if (typeof conf.classes === "string") conf.classes = [conf.classes];
                    let allClasses = JSON.parse(JSON.stringify(conf.classes));
                    if (extraClasses) {
                        extraClasses.forEach((ec) => {
                            if (ec.indexOf("show") == 0 || ec.indexOf("hide") == 0) ec = `tgpdf-${ec}`;
                            allClasses.push(ec);
                        });
                    }

                    if (conf.events && conf.events.click === null)
                        conf.events.click = (ev) => {
                            if (ev.target.getAttribute("disabled")) return;
                            parent.controlClicked(key);
                        };

                    const el = createElement(container, { ...conf, classes: allClasses });
                    if (typeof key === "string") {
                        if (key in ui.controls) ui.controls[key].push(el);
                        else ui.controls[key] = [el];
                    }
                }
            });
        } else {
            console.error("controls must be array", configs);
            throw "controls must be array";
        }
    }

    /**************************************************************/
    ui.container = createElement(root, { classes: ["tgpdf", l10n.dir()] });
    ui.container.addEventListener("click", (ev) => {
        if (ev.target.getAttribute("name") != "openMenu" && ui.menu && ev.target.parentElement.parentElement != ui.menu && ev.target != ui.menu && ev.target.parentElement != ui.menu)
            ui.menu.classList.add("tgpdf-hidden");
    });

    //--------------------------------------------------------------
    ui.about = createElement(ui.container, { classes: ["tgpdf-about", "tgpdf-hidden"] });
    ui.sidebar = createElement(ui.container, { classes: ["tgpdf-sidebar"] });
    ui.main = createElement(ui.container, { class: "tgpdf-main" });

    //--------------------------------------------------------------
    if (safeGet(options, "splash")) ui.splash = createElement(ui.container, options.splash);

    //--------------------------------------------------------------
    ui.wasmLoader = createElement(ui.container, {
        class: "tgpdf-wasmLoader",
        childs: [
            {
                childs: [
                    { tag: "img", attrs: { src: "./images/logo.gif" } },
                    { classes: ["tgpdf-loadingBar"], childs: [{ classes: ["tgpdf-progress"] }] },
                ],
            },
        ],
    });
    ui.wasmLoader.progress = ui.wasmLoader.firstElementChild.lastElementChild.firstElementChild;

    //--------------------------------------------------------------
    if (safeGet(options, "ui.sidebar")) {
        const tools = safeGet(options, "ui.sidebar");
        if (Array.isArray(tools) == false) throw "sidebar tools must be array of string";

        ui.sidebar.tools = createElement(ui.sidebar, {
            classes: ["tgpdf-toolbar"],
            childs: [{ classes: ["tgpdf-bar-buttons", "tgpdf-toggled"] }],
        });
        ui.sidebar.tools = ui.sidebar.tools.firstElementChild;
        ui.sidebar.content = createElement(ui.sidebar, { classes: ["tgpdf-sb-content"] });
        ui.sidebar.resizer = createElement(ui.sidebar, { classes: ["tgpdf-sb-resizer"] });

        ui.sidebar.tools.btns = {};
        tools.forEach((tool) => {
            if (typeof tool === "string" && availableSidebarTabs[tool]) {
                ui.sidebar.tools.btns[tool] = createElement(ui.sidebar.tools, availableSidebarTabs[tool]);
                ui.sidebar.content[tool] = createElement(ui.sidebar.content, { classes: [`tgpdf-sb-${tool}`, "tgpdf-hidden"] });
            } else if (isObject(tool)) {
                if (tool.button && tool.content) {
                    tool.content.classes.push(`tgpdf-sb-${tool}`);
                    tool.content.classes.push("tgpdf-hidden");
                    ui.sidebar.tools.btns[tool] = createElement(ui.sidebar.tools, tool.button);
                    ui.sidebar.tools.btns[tool] = createElement(ui.sidebar.content, tool.content);
                }
            }
        });

        if (ui.sidebar.content.settings) {
            const settings = ui.sidebar.content.settings;
            const createSettingSection = (sectionName) => {
                const opts = options.settings[sectionName];
                const section = createElement(settings, { class: "tgpdf-setting-sec" });
                createElement(section, {
                    class: "tgpdf-setting-title",
                    l10n: sectionName,
                    text: l10n.tr(`setting_${sectionName}`),
                    events: {
                        click: () => {
                            if (section.classList.contains("tgpdf-toggled")) section.classList.remove("tgpdf-toggled");
                            else section.classList.add("tgpdf-toggled");
                        },
                    },
                });

                const ul = createElement(section, { tag: "ul" });
                for (const key in opts) {
                    if (typeof opts[key] === "boolean") {
                        createElement(ul, makeCheckboxSpecs(key, l10n.tr(key), opts[key], sectionName)).addEventListener("change", (ev) => parent.configChanged(sectionName, key, ev.target.checked));
                    } else if (typeof opts[key] === "string") {
                        if (opts[key].indexOf("#") === 0) {
                            if (!window.jscolor) continue;
                            if (!window.applyColor) {
                                window.tgpdfApplyColor = (key, picker) => {
                                    parent.configChanged(sectionName, key, picker.toRGBAString());
                                };
                            }
                            let alpha = 1;
                            if (opts[key].length == 9) alpha = parseInt(opts[key].substr(7), 16) / 255;

                            createElement(ul, {
                                tag: "li",
                                childs: [
                                    {
                                        tag: "button",
                                        attrs: {
                                            dataJscolor: JSON.stringify({
                                                onChange: `tgpdfApplyColor('${key}', this)`,
                                                alpha,
                                                value: opts[key].substr(1, 6),
                                            }),
                                        },
                                    },
                                    { tag: "label", text: l10n.tr(key) },
                                ],
                            });
                        } else {
                            createElement(ul, {
                                tag: "li",
                                childs: [
                                    {
                                        tag: "input",
                                        attrs: { type: "text", value: opts[key] },
                                        events: {
                                            change: (ev) => parent.configChanged(sectionName, key, ev.target.value),
                                        },
                                    },
                                    { tag: "label", text: l10n.tr(key) },
                                ],
                            });
                        }
                    } else if (typeof opts[key] === "number") {
                        createElement(ul, {
                            tag: "li",
                            childs: [
                                {
                                    tag: "input",
                                    attrs: { type: "number", value: opts[key] },
                                    events: {
                                        change: (ev) => parent.configChanged(sectionName, key, parseInt(ev.target.value)),
                                    },
                                },
                                { tag: "label", text: l10n.tr(key) },
                            ],
                        });
                    }
                }
            };

            for (const section of Object.keys(options.settings)) {
                createSettingSection(section);
            }
        }
    }

    //--------------------------------------------------------------
    if (safeGet(options, "ui.menu")) {
        ui.menu = createElement(ui.main, {
            classes: ["tgpdf-menu", "tgpdf-hidden", "tgpdf-doorhanger-right"],
            childs: [{ class: "tgpdf-mnu-cnt" }],
        });
        applyControls(ui.menu.firstElementChild, options.ui.menu);
    }

    //--------------------------------------------------------------
    const toolbarContainer = createElement(ui.main, {
        class: "tgpdf-toolbar",
    });

    if (safeGet(options, "ui.navbar")) {
        ui.navbar = createElement(toolbarContainer, { class: "tgpdf-nav" });
        ui.navbar.left = createElement(ui.navbar, { class: "tgpdf-nav-left" });
        ui.navbar.right = createElement(ui.navbar, { class: "tgpdf-nav-right" });
        ui.navbar.middle = createElement(ui.navbar, { class: "tgpdf-nav-middle" });

        applyControls(ui.navbar.left, options.ui.navbar.left);
        applyControls(ui.navbar.right, options.ui.navbar.right);
        applyControls(ui.navbar.middle, options.ui.navbar.middle);
    }

    //--------------------------------------------------------------
    if (true) {
        ui.findbar = createElement(ui.main, { classes: ["tgpdf-toolbar", "tgpdf-findbar", "tgpdf-hidden", "tgpdf-doorhanger"] });
        ui.findbar.input = createElement(ui.findbar, { class: "tgpdf-fb-input" });
        ui.findbar.opts = createElement(ui.findbar, { class: "tgpdf-fb-opts" });
        ui.findbar.msg = createElement(ui.findbar, { class: "tgpdf-fb-msg", childs: [{ tag: "label" }] });
        ui.findbar.msg = ui.findbar.msg.firstElementChild;

        ui.findbar.input.el = createElement(ui.findbar.input, {
            tag: "input",
            attrs: { placeholder: l10n.tr("Find in Document..."), ariaLabel: l10n.tr("Find input"), title: l10n.tr("Find") },
        });

        ui.findbar.btns = createElement(ui.findbar.input, { classes: ["tgpdf-bar-buttons"] });
        ui.findbar.btns.prev = createElement(ui.findbar.btns, availableTools.findPrev);
        createElement(ui.findbar.btns, availableTools.separator);
        ui.findbar.btns.next = createElement(ui.findbar.btns, availableTools.findNext);

        ui.findbar.opts.findAll = createElement(ui.findbar.opts, makeCheckboxSpecs("findAll", "Highligh all found phrases", false));
        ui.findbar.opts.matchCase = createElement(ui.findbar.opts, makeCheckboxSpecs("caseSensitive", "Match Case", false));
        ui.findbar.opts.matchWhole = createElement(ui.findbar.opts, makeCheckboxSpecs("wholeWord", "Whole Word", false));
        ui.findbar.opts.count = createElement(ui.findbar.opts, { tag: "span" });
    }

    //--------------------------------------------------------------
    ui.viewer = createElement(ui.main, { class: "tgpdf-viewer" });
    ui.fileInput = createElement(ui.main, { tag: "input", attrs: { type: "file" }, style: { display: "none" } });

    //--------------------------------------------------------------
    if (safeGet(options, "ui.footer")) {
        ui.footer = createElement(ui.container, {
            class: "tgpdf-footer",
        });
        applyControls(ui.footer, options.ui.footer);
    }

    if (window.jscolor) window.jscolor.install();

    return ui;
}

/**************************************************************/

export default {
    construct,
    makeButtonSpecs,
    makeCheckboxSpecs,
    createElement,
    setDisabled,
};
