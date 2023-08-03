const l10n = (() => {
    const activeLang = {
        dir: "ltr",
        dic: {
            about: "About",
            buy: "Increase Credit",
            credit: "Show remaining credit",
            docProps: "Show document properties",
            download: "Download PDF",
            findNext: "Find Next",
            findPrev: "Find Prev",
            firstPage: "Go to first page",
            lastPage: "Go to last page",
            fullScreen: "Presentation Mode",
            handTool: "Change mode to grab",
            logout: "Logout",
            openFile: "Open New File",
            openMenu: "Other tools",
            pageDown: "Next Page",
            pageUp: "Previous Page",
            print: "Print Page",
            rotateCw: "Rotate Clockwise",
            rotateCcw: "Rotate Counterclockwise",
            scrollHorz: "Scroll Horizontal",
            scrollVert: "Scroll Vertical",
            scrollWrapped: "Scroll Wrapped",
            search: "Find in document",
            toggleSidebar: "toggle sidebar",
            spreadEven: "Spread Even",
            spreadNone: "Spread None",
            spreadOdd: "Spread Odd",
            zoomIn: "Zooom In",
            zoomOut: "Zoom Out",
            settings: "Manage Settings",
            thumbnail: "View Thumbnails",
            outline: "View Outlines",
            attachments: "View Attachments",
            RemoveWatermarks: "Remove all watermarks",
            DiscardTablesAndCaptions: "Discard Tables and Their Captions",
            DiscardImagesAndCaptions: "Discard Images and Their Captions",
            DiscardHeaders: "Discard Headers",
            DiscardFooters: "Discard Footers",
            DiscardSidebars: "Discard Sidebars",
            DiscardFootnotes: "Discard Footnotes",
            AssumeFirstAloneLineAsHeader: "Assume First Alone Line As Header",
            AssumeLastAloneLineAsFooter: "Assume Last Alone Line As Footer",
            DiscardItemsIn2PercentOfPageMargin: "Discard Items In 2% Of Page Margin",
            MarkSubcripts: "Mark Subcripts (x²)",
            MarkSuperScripts: "Mark Superscripts (x₂)",
            MarkBullets: "Mark Bullets",
            MarkNumberings: "Mark Numberings",
            ASCIIOffset: "ASCII codes Offset",
            markAllPragarpahs: "Mark All Pragarpahs",
            markAllNonTexts: "Mark All Non Texts",
            markSelectedSentence: "Mark Selected Sentence",
            markSelectedParagraph: "Mark Selected Paragraph",
            nonMainTextsCanBeSelected: "Non text lines can be selected (Experimental)",
            allParagraphsHighlight: "Non-Active Paragraph Highlight",
            activeParagraphHighlight: "Focused Paragraph Highlight",
            activeSentenceHighlight: "Active Sentence Highlight",
            markSenetenceOnHover: "Mark Sentence on Hover",
            sentenceHoverColor: "Sentence background on Hover",
            nonTextMarksHighlight: "Non-Text Highlight",
            about_title: "Targoman PDF Viewer",
            about_text: "Targoman PDF Viewer van show and highlight text and also export sentences...",
            about_call_action: "Open New PDF File",
            setting_layout: "Layout Analysis",
            setting_markers:"Markers"
        },
    };

    function loadTranslations(lang) {
        const langSpecs = {};
        if (lang === "fa") {
            activeLang.dir = "rtl";
            activeLang.dic["Automatic Zoom"] = "بزرگنمایی خودکار";
            activeLang.dic["Actual Size"] = "اندازه واقعی";
            activeLang.dic["Page Fit"] = "صفحه کامل";
            activeLang.dic["Page Width"] = "عرض صفحه";
            activeLang.dic["Find in Document..."] = "جستجو در سند…";
            activeLang.dic["Find input"] = "عبارت مورد جستجو";
            activeLang.dic[" of "] = " از ";
            activeLang.dic["?"] = "؟";
            activeLang.dic["Find"] = "جستجو";
            activeLang.dic["about"] = "درباره";
            activeLang.dic["buy"] = "افزایش اعتبار";
            activeLang.dic["credit"] = "مشاهده اعتبار باقیمانده";
            activeLang.dic["docProps"] = "نمایش مشخصات سند";
            activeLang.dic["download"] = "دریافت PDF";
            activeLang.dic["findNext"] = "یافتن بعدی";
            activeLang.dic["findPrev"] = "یافتن قبلی";
            activeLang.dic["firstPage"] = "برو به صفحه اول";
            activeLang.dic["lastPage"] = "برو به صفحه آخر";
            activeLang.dic["fullScreen"] = "تغییر به حالت ارائه";
            activeLang.dic["handTool"] = "Change mode to grab";
            activeLang.dic["logout"] = "خروج";
            activeLang.dic["openFile"] = "گشودن فایل جدید";
            activeLang.dic["openMenu"] = "سایر گزینه‌ها";
            activeLang.dic["nextPage"] = "صفحه بعدی";
            activeLang.dic["prevPage"] = "صفحه قبلی";
            activeLang.dic["print"] = "چاپ";
            activeLang.dic["rotateCw"] = "چرخش ساعتگرد";
            activeLang.dic["rotateCcw"] = "چرخش پادساعتگرد";
            activeLang.dic["scrollHorz"] = "Scroll Horizontal";
            activeLang.dic["scrollVert"] = "Scroll Vertical";
            activeLang.dic["scrollWrapped"] = "Scroll Wrapped";
            activeLang.dic["search"] = "جستجو در سند";
            activeLang.dic["toggleSidebar"] = "گشودن منوی جانبی";
            activeLang.dic["spreadEven"] = "Spread Even";
            activeLang.dic["spreadNone"] = "Spread None";
            activeLang.dic["spreadOdd"] = "Spread Odd";
            activeLang.dic["zoomIn"] = "بزرگ‌نمایی";
            activeLang.dic["zoomOut"] = "کوچک‌نمایی";
            activeLang.dic["settings"] = "تنظیمات";
            activeLang.dic["thumbnail"] = "تصاویر بندانگشتی";
            activeLang.dic["outline"] = "طرح نوشتار";
            activeLang.dic["attachment"] = "پیوست‌ها";
            activeLang.dic["RemoveWatermarks"] = "حذف واترمارک‌ها";
            activeLang.dic["DiscardTablesAndCaptions"] = "حذف جداول و توضیحات آن‌ها";
            activeLang.dic["DiscardImagesAndCaptions"] = "حذف تصاویر و توضیحات آن‌ها";
            activeLang.dic["DiscardHeaders"] = "حذف سرآیند (Header) صفحه";
            activeLang.dic["DiscardFooters"] = "حذف ته‌آیند (Footer) صفحه";
            activeLang.dic["DiscardSidebars"] = "حذف حاشیه‌نویس صفحه ";
            activeLang.dic["DiscardFootnotes"] = "(Footnote) حذف پاورقی";
            activeLang.dic["AssumeFirstAloneLineAsHeader"] = "اولین خط تنها سرآیند است";
            activeLang.dic["AssumeLastAloneLineAsFooter"] = "آخرین خط تنها ته‌آیند است";
            activeLang.dic["DiscardItemsIn2PercentOfPageMargin"] = "حذف محتوای کناره‌های صفحه";
            activeLang.dic["MarkSubcripts"] = "نشان‌گذاری بالانگاشت‌ها (x²)";
            activeLang.dic["MarkSuperScripts"] = "نشان‌گذاری پایین‌نگاشت‌ها (x₂)";
            activeLang.dic["MarkBullets"] = "نشان‌گذاری گوشواره‌ها";
            activeLang.dic["MarkNumberings"] = "نشان‌گذاری شماره‌‌ها";
            activeLang.dic["ASCIIOffset"] = "انحراف کدهای اسکی";
            activeLang.dic["markAllPragarpahs"] = "هایلایت همه پاراگراف‌ها";
            activeLang.dic["markAllNonTexts"] = "هایلایت بخش‌های غیر متنی";
            activeLang.dic["markSelectedSentence"] = "هایلایت جمله انتخابی";
            activeLang.dic["markSelectedParagraph"] = "هایلایت پاراگراف انتخابی";
            activeLang.dic["markSenetenceOnHover"] = "هایلایت جمله با عبور موس";
            activeLang.dic["nonMainTextsCanBeSelected"] = "امکان انتخاب خطوط غیر متنی (آزمایشی)";
            activeLang.dic["allParagraphsHighlight"] = "رنگ سایر پاراگراف‌ها";
            activeLang.dic["activeParagraphHighlight"] = "رنگ پاراگراف انتخابی";
            activeLang.dic["activeSentenceHighlight"] = "رنگ جمله انتخابی";
            activeLang.dic["sentenceHoverColor"] = "رنگ جمله قابل کلیک";
            activeLang.dic["nonTextMarksHighlight"] = "رنگ بخش‌های غیر متنی";
            activeLang.dic["setting_layout"] = "پردازشگر الگو";
            activeLang.dic["setting_markers"] = "نشان‌گرها";
            activeLang.dic["Loading TargomanPDF"] = "در حال آماده‌سازی";
            activeLang.dic["about_title"] = "نمایشگر PDF ترگمان";
            activeLang.dic["about_text"] = "این یک نمایشگر معمولی نیست!";
            activeLang.dic["about_call_action"] = "گشودن فایل PDF جدید";
        }
    }

    return {
        dir: () => activeLang.dir,
        tr: (text, conf, attr) => {
            if (!attr || attr === "placeholder" || attr === "text" || attr === "value" || attr === "title" || attr === "aria") {
                if (conf && conf.l10n) {
                    const key = conf.l10n + (attr ? `.${attr}` : "");
                    if (key in activeLang.dic) return activeLang.dic[key];
                    if (conf.l10n in activeLang.dic) return activeLang.dic[conf.l10n];
                }
                if (text in activeLang.dic) return activeLang.dic[text];
            }
            return text;
        },
        loadTranslations,
        setCustomTranslation: (_dic) => {
            for (const entry in _dic) {
                console.log(entry, _dic[entry]);
                activeLang.dic[entry] = _dic[entry];
            }
        },
        translations: (_lang) => (_lang == "en" || loadTranslations(_lang)) && activeLang.dic,
    };
})();

export default l10n;
