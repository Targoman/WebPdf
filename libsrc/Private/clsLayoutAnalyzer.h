#ifndef TARGOMAN_PDF_PRIVATE_CLSLAYOUTANALYSIS_H
#define TARGOMAN_PDF_PRIVATE_CLSLAYOUTANALYSIS_H

#include "clsPdfiumWrapper.h"
#include "clsPdfItem.h"
#include "clsPdfBlock.h"
#include "ParagraphAndSentenceManager.h"

namespace Targoman {
namespace PDF {
namespace Private {

class clsLayoutAnalyzer {
public:
    clsLayoutAnalyzer(const clsPdfiumWrapper& _pdfiumWrapper, const stuConfigsPtr_t &_configs);

    const PdfBlockPtrVector_t& getPageContent(int16_t _pageIndex, const FontInfo_t& _fontInfo) const;
    const PdfBlockPtrVector_t& getPageSubContent(int16_t _pageIndex, const FontInfo_t& _fontInfo) const;
    void clearCache();

private:
    std::shared_ptr<stuProcessedPage> cacheAndGetProcessedPage(int16_t _pageIndex, const FontInfo_t& _fontInfo);

    stuPreprocessedPage getPreprocessedPage(int _pageIndex) const;
    stuProcessedPage getProcessedPage(int16_t _pageIndex, const FontInfo_t& _fontInfo, const stuParState& _prevPageState) const;
    void markStamps(int _pageIndex, PdfItemPtrVector_t& _pageItems, const stuPageMargins& _pageMargins, const stuSize& _pageSize) const;
    void identifyColumns(const PdfItemPtrVector_t &_pageItems) const;
    std::tuple<float, float> markHeadersAndFooters(int _pageIndex, const stuPreprocessedPage &_page, const stuSize& _pageSize) const;
    void markSidebars(int _pageIndex, PdfLineSegmentPtrVector_t &_lines, const stuPageMargins& _pageMargins, const stuSize& _pageSize) const;

private:
    const clsPdfiumWrapper& PdfiumWrapper;
    tmplLimitedCache<int, stuPreprocessedPage, MAX_CACHED_PAGES> CachedPreprocessedPage;
    tmplLimitedCache<int, stuProcessedPage, MAX_CACHED_PAGES> CachedProcessedPage;
    stuConfigsPtr_t Configs;
    friend stuParState getPrevPageState(clsLayoutAnalyzer*, int, const FontInfo_t&);
};

}
}
}

#endif // TARGOMAN_PDF_PRIVATE_CLSLAYOUTANALYSIS_H
