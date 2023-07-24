#include <map>
#include <set>
#include "common.h"
#include "LayoutAnalysis/Stamping.h"
#include "LayoutAnalysis/HeaderFooterAndSideBar.h"
#include "LayoutAnalysis/BlockCreation.h"
#include "LayoutAnalysis/SubBlockMarking.h"

namespace Targoman {
namespace PDF {
namespace Private {

clsLayoutAnalyzer::clsLayoutAnalyzer(const clsPdfiumWrapper &_pdfiumWrapper, const stuConfigsPtr_t &_configs) :
    PdfiumWrapper(_pdfiumWrapper),
    Configs(_configs)
{}

stuParState getPrevPageState(clsLayoutAnalyzer* _this, int _pageIndex, const FontInfo_t& _fontInfo) {
    std::shared_ptr<stuProcessedPage> PrevProcessedPage = nullptr;
    PdfBlockPtr_t PrevPageLastMainBlock;
    int PrevPageIndex = _pageIndex - 1;
    while(PrevPageIndex >= 0) {
        auto CachedContent = _this->CachedProcessedPage.find(PrevPageIndex);
        if(CachedContent !=  _this->CachedProcessedPage.end()) {
            PrevProcessedPage = CachedContent->second;
        } else {
            _this->CachedProcessedPage[PrevPageIndex] =
                    std::make_shared<stuProcessedPage>(_this->getProcessedPage(static_cast<int16_t>(PrevPageIndex), _fontInfo, stuParState()));
            PrevProcessedPage = _this->CachedProcessedPage[PrevPageIndex];
            PrevProcessedPage->IsDirty = true;
        }
        if(PrevPageIndex == _pageIndex - 1)
            PrevPageLastMainBlock = PrevProcessedPage->LastMainBlockState.LastPageMainBlock;
        if(PrevProcessedPage->LastMainBlockState.TextBlock.get() != nullptr)
            break;
        _this->CachedProcessedPage[PrevPageIndex]->IsDirty = false;
        --PrevPageIndex;
    }

    int TextParagraphCount = 0;
    if(PrevPageIndex > 0) {
        for(const auto& Paragraph : PrevProcessedPage->Content)
            if(Paragraph->isMainTextBlock()) {
                ++TextParagraphCount;
                if(TextParagraphCount > 1)
                    break;
            }
    }

    PrevProcessedPage->LastMainBlockState.LastPageMainBlock = PrevPageLastMainBlock;

    if(PrevPageIndex == 0 || TextParagraphCount > 1 || PrevProcessedPage->LastMainBlockState.ContinuationState == enuContinuationState::Cannot)
        return PrevProcessedPage->LastMainBlockState;
    else {
        auto ParState = getPrevPageState(_this, _pageIndex - 1, _fontInfo);
        ParState.LastPageMainBlock = PrevPageLastMainBlock;
        return ParState;
    }
};


const PdfBlockPtrVector_t& clsLayoutAnalyzer::getPageContent(int16_t _pageIndex, const FontInfo_t& _fontInfo) const
{
    auto _this = const_cast<clsLayoutAnalyzer*>(this);

    std::shared_ptr<stuProcessedPage> ProcessedPage = nullptr;

    auto CachedContent = _this->CachedProcessedPage.find(_pageIndex);
    if(CachedContent !=  _this->CachedProcessedPage.end()) {
        if(CachedContent->second->IsDirty == false) {
            return  CachedContent->second->Content;
        }
        ProcessedPage = CachedContent->second;
    }
    debugShow("Start Of Layout Analysis on Page: "<<_pageIndex + 1);


    stuParState PrevPageParState;
    if(_pageIndex > 0)
        PrevPageParState = getPrevPageState(_this, _pageIndex, _fontInfo);

    if(ProcessedPage == nullptr || ProcessedPage->IsDirty == false) {
        _this->CachedProcessedPage[_pageIndex] =
                std::make_shared<stuProcessedPage>(this->getProcessedPage(_pageIndex, _fontInfo, PrevPageParState));
        ProcessedPage = _this->CachedProcessedPage[_pageIndex];
    } else {
        updateFirstParagraph(this->Configs, ProcessedPage->Content, PrevPageParState);
        ProcessedPage->IsDirty = false;
    }

    debugShow("Layout Analysis on Page: "<<_pageIndex + 1<< " Finished.");

    return _this->CachedProcessedPage[_pageIndex]->Content;
}

PdfItemPtrVector_t mergeVertRulerParts(const PdfItemPtrVector_t& _pageItems) {
    PdfItemPtrVector_t Result;
    std::set<clsPdfItem*> Used;

    for(auto& Item : _pageItems) {
        if(Used.find(Item.get()) != Used.end())
            continue;
        if(Item->type() == enuPdfItemType::Char || Item->isVerticalRuler() == false) {
            Result.push_back(Item);
            Used.insert(Item.get());
            continue;
        }
        PdfItemPtrVector_t Candidates;
        for(auto& OtherItem : _pageItems) {
            if(OtherItem.get() == Item.get() || Used.find(OtherItem.get()) != Used.end())
                continue;
            if(OtherItem->type() == enuPdfItemType::Char || OtherItem->isVerticalRuler() == false)
                continue;
            if(OtherItem->horzOverlap(Item) < MIN_ITEM_SIZE)
                continue;
            Candidates.push_back(OtherItem);
            Used.insert(OtherItem.get());
        }
        Candidates.push_back(Item);
        Used.insert(Item.get());
        sort(Candidates, [] (const PdfItemPtr_t& _a, const PdfItemPtr_t& _b) { return _a->y0() < _b->y0(); });
        PdfItemPtr_t VertRuler = Candidates.at(0);
        for(size_t NextIndex = 1; NextIndex < Candidates.size(); ++NextIndex) {
            if(Candidates.at(NextIndex)->y0() > VertRuler->y1() + 1.5f) {
                Result.push_back(VertRuler);
                VertRuler = Candidates.at(NextIndex);
            } else
                VertRuler->unionWith_(Candidates.at(NextIndex));
        }
        Result.push_back(VertRuler);
    }
    return Result;
}

stuPreprocessedPage clsLayoutAnalyzer::getPreprocessedPage(int _pageIndex) const
{
    auto _this = const_cast<clsLayoutAnalyzer*>(this);
    auto CachedLines = _this->CachedPreprocessedPage.find(_pageIndex);
    if(CachedLines !=  _this->CachedPreprocessedPage.end())
        return  *CachedLines->second;

    PdfItemPtrVector_t PageItems; stuPageMargins PageMargins;
    stuSize PageSize = this->PdfiumWrapper.pageSize(_pageIndex);

    // ///////////////////////////////////////////////////
    // 0: get page items and mark them
    // ///////////////////////////////////////////////////
    std::tie(PageItems, PageMargins) = this->PdfiumWrapper.getPageItems(_pageIndex);
    SIG_STEP_DONE("onItems", PageItems);

    if(this->Configs->RemoveWatermarks) {
        // ///////////////////////////////////////////////////
        // 1: mark watermarks
        // ///////////////////////////////////////////////////
        this->markStamps(_pageIndex, PageItems, PageMargins, PageSize);
        LA::sigStampDetectionDone(_pageIndex, PageItems, false);

        // ///////////////////////////////////////////////////
        //2: remove all watermark texts
        // ///////////////////////////////////////////////////
        PageItems = filter<PdfItemPtr_t>(PageItems, [&] (const PdfItemPtr_t& _item) {
            return _item->isWatermark() == false;
        });
    }

    PageItems = mergeVertRulerParts(PageItems);

    // ///////////////////////////////////////////////////
    // 4: merge vertically overlapping items to form stripes of text
    // ///////////////////////////////////////////////////
    auto Stripes = LA::findVertStripes(PageItems, PageSize);
    SIG_STEP_DONE("onStripsIdentified", Stripes);

    return *(_this->CachedPreprocessedPage[_pageIndex] =
            std::make_shared<stuPreprocessedPage>(stuPreprocessedPage {Stripes, PageMargins}));
}

stuProcessedPage clsLayoutAnalyzer::getProcessedPage(int16_t _pageIndex, const FontInfo_t& _fontInfo, const stuParState& _prevPageState) const
{
    stuSize PageSize = this->PdfiumWrapper.pageSize(_pageIndex);
#ifdef TARGOMAN_SHOW_DEBUG
    gPageIndex = _pageIndex;
#endif

    auto PreprocessedPage = this->getPreprocessedPage(_pageIndex);

    // ///////////////////////////////////////////////////
    //1: mark repeated header and footers
    // ///////////////////////////////////////////////////
    float PageHeadeBottom = 0, PageFooterTop = PageSize.Height;

    if(this->Configs->DiscardHeaders || this->Configs->DiscardFooters) {
        std::tie(PageHeadeBottom, PageFooterTop) = this->markHeadersAndFooters(_pageIndex, PreprocessedPage, PageSize);
        LA::sigHeaderAndFooter(_pageIndex, PreprocessedPage.Stripes, PageSize);
    }

    // ///////////////////////////////////////////////////
    //2: extract pdf items
    // ///////////////////////////////////////////////////
    PdfItemPtrVector_t PageItems;
    PreprocessedPage.Margins.Left = PageSize.Width;
    PreprocessedPage.Margins.Right = 0;
    for(const auto& Stripe : PreprocessedPage.Stripes){
        PreprocessedPage.Margins.Left = std::min(PreprocessedPage.Margins.Left, Stripe->x0());
        PreprocessedPage.Margins.Right = std::max(PreprocessedPage.Margins.Right, Stripe->x1());
        for(const auto& Item : Stripe->items()) {
            bool WasRepeated = Item->isRepeated() || Stripe->isRepeated();
            int32_t RepetitionOffset = Item->repetitionPageOffset();
            Item->resetFlags();
            if(WasRepeated)
                Item->markAsRepeated(RepetitionOffset);
            if(this->Configs->DiscardHeaders && Stripe->isHeader()) {
                Item->markAsMainContent(false);
                Item->markAsHeader(true);
            } else if(this->Configs->DiscardFooters && Stripe->isFooter()) {
                Item->markAsMainContent(false);
                Item->markAsFooter(true);
            }  else
                Item->markAsMainContent(true);

            PageItems.push_back(Item);
        }
    }

    PreprocessedPage.Margins.Right = PageSize.Width -  PreprocessedPage.Margins.Right;

    PreprocessedPage.Stripes.clear();

    // ///////////////////////////////////////////////////
    //3: split horizontal lines to segments by horizontal gap
    // ///////////////////////////////////////////////////
    auto PageLines  = LA::findLines(PageItems);
    auto MainPageLines = filter<PdfLineSegmentPtr_t>(PageLines, [&](const PdfLineSegmentPtr_t& _line) {
        return (_line->isHeader() == false || this->Configs->DiscardHeaders == false) &&
                (_line->isFooter() == false || this->Configs->DiscardFooters == false) &&
                (_line->isSidebar() == false || this->Configs->DiscardSidebars == false)
                ;
    });
    SIG_STEP_DONE("onMainContentReady", MainPageLines);

    // ///////////////////////////////////////////////////
    //4: find sidebars and mark them
    // ///////////////////////////////////////////////////
    if(this->Configs->DiscardSidebars) {
        this->markSidebars(_pageIndex, PageLines, PreprocessedPage.Margins, PageSize);
        LA::sigHeaderAndFooter(_pageIndex, PageLines, PageSize);
    }

    // ///////////////////////////////////////////////////
    //4: identify main page layout
    // ///////////////////////////////////////////////////
    clsLayoutStorage LayoutStorage(this->Configs, MainPageLines, true);
    LayoutStorage.identifyMainColumns(_pageIndex, PreprocessedPage.Margins, PageSize);
#ifdef TARGOMAN_SHOW_DEBUG
    if(gMaxStep < 8)  return stuProcessedPage();
#endif

    // ///////////////////////////////////////////////////
    //5: split to blocks
    // ///////////////////////////////////////////////////
    auto PageBlocks = LA::makeBlocks(this->Configs, PageSize, LayoutStorage, PageLines, _fontInfo);
    PageLines.clear();

    sigStepDone("onRawBlocksFound", _pageIndex, PageBlocks, PageBlocks);

    // ///////////////////////////////////////////////////
    //6: split and mark blocks
    // ///////////////////////////////////////////////////
    LA::splitAndMarkBlocks(this->Configs, _pageIndex, PreprocessedPage.Margins, PageSize, _fontInfo, PageBlocks, _prevPageState);
    sigStepDone("onMarkedBlocks", _pageIndex, PageBlocks, PageBlocks);


    // ///////////////////////////////////////////////////
    //7: extract final sentences
    // ///////////////////////////////////////////////////
    auto ProcessedPage = extractAndEnumerateSentences(this->Configs, _pageIndex, PageBlocks, _fontInfo, _prevPageState);
    sigStepDone("onFinalSentences", _pageIndex, ProcessedPage.Content, ProcessedPage.Content);
    return ProcessedPage;
}

}
}
}
