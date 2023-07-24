#include "Stamping.h"

namespace Targoman {
namespace PDF {
namespace Private {
namespace LA {

void markLocation(const PdfItemPtr_t& _item, const stuSize _pageSize){
    if(_item->y1() < HEADER_FOOTER_PAGE_MARGIN_THRESHOLD * _pageSize.Height)
        _item->markAsHeader(true);
    else if (_item->y1() > (1 - HEADER_FOOTER_PAGE_MARGIN_THRESHOLD) * _pageSize.Height)
        _item->markAsFooter(true);
};

void sigStampDetectionDone(int _pageIndex, const PdfItemPtrVector_t& _pageItems, bool _onlyNewOnes) {

    PdfBlockPtrVector_t WMarks;
    for(const auto& Item : _pageItems) {
        if(Item->isWatermark() && (_onlyNewOnes == false || Item->isWatermark())){
            auto Block = std::make_shared<clsPdfBlock>();
            Block->setX0(Item->x0());
            Block->setX1(Item->x1());
            Block->setY0(Item->y0());
            Block->setY1(Item->y1());
            WMarks.push_back(Block);
        }
    }
    auto WMarkCandids = filter<PdfItemPtr_t>(_pageItems, [](const PdfItemPtr_t& _item){ return _item->isRepeated() && _item->isWatermark() == false; });
    sigStepDone(
                _onlyNewOnes ? "onStampsExpanded" : "onStampsMarked",
                _pageIndex,
                WMarks,
                WMarkCandids
                );
}

}

void clsLayoutAnalyzer::markStamps(int _pageIndex, PdfItemPtrVector_t &_pageItems, const stuPageMargins& _pageMargins, const stuSize &_pageSize) const
{
    auto canBeWatermark = [] (const PdfItemPtr_t& _item) {
        return _item->isHorizontalRuler() == false &&
                _item->isVerticalRuler() == false && (
                    (_item->type() != enuPdfItemType::Char) ||
                    (_item->height() > 8.f && _item->width() + _item->height() > 40.f)
                    );
    };

    stuSize PageSize = this->PdfiumWrapper.pageSize(_pageIndex);

    auto PageCount = this->PdfiumWrapper.pageCount();

    std::vector<int> ComparingPageIndexes;
    uint8_t PrevPageCount = 0, NextPageCount = 0;
    auto appendComparePageIndex = [&] (int _index) {
        auto ComparingPageSize = this->PdfiumWrapper.pageSize(_index);
        if(std::abs(ComparingPageSize.Width - PageSize.Width) > MIN_ITEM_SIZE)
            return;
        if(std::abs(ComparingPageSize.Height - PageSize.Height) > MIN_ITEM_SIZE)
            return;
        ComparingPageIndexes.push_back(_index);
    };

    for(int i = 1; i < std::min(3, PageCount); ++i){
        if(_pageIndex >= i && PrevPageCount < 2){
            appendComparePageIndex(_pageIndex - i);
            ++PrevPageCount;
        }
        if(_pageIndex + i < PageCount && NextPageCount < 2) {
            appendComparePageIndex(_pageIndex + i);
            ++NextPageCount;
        }
        if(ComparingPageIndexes.size() >= 4 || (NextPageCount == 2 && i > _pageIndex))
            break;
    }

    PdfItemPtrVector_t CandidateWatermarks;

    for(size_t j = 0; j < ComparingPageIndexes.size(); ++j) {
        int ComparingPageIndex = ComparingPageIndexes.at(j);
        PdfItemPtrVector_t ComparingPageItems;
        stuPageMargins ComparingPageMargins;
        std::tie(ComparingPageItems, ComparingPageMargins) = this->PdfiumWrapper.getPageItems(ComparingPageIndex);

        std::multimap<int, PdfItemPtr_t> ComparingPageItemMapWithBottomOffset;
        std::multimap<int, PdfItemPtr_t> ComparingPageItemMapWithTopOffset;
        std::multimap<int, PdfItemPtr_t> ComparingPageItemMapOriginal;

        auto computeKey = [&](const PdfItemPtr_t& _item, float _pageOffset, int _margin = 0) {
            return static_cast<int>(10 * (_item->y0() + _pageOffset)) + _margin;
        };

        for(const auto& Item : ComparingPageItems) {
            ComparingPageItemMapWithBottomOffset.emplace(computeKey(Item, ComparingPageMargins.Bottom), Item);
            ComparingPageItemMapWithTopOffset.emplace(computeKey(Item, ComparingPageMargins.Top * -1), Item);
            ComparingPageItemMapOriginal.emplace(computeKey(Item, 0), Item);
        }

        for(const auto& Item : _pageItems) {
            if(Item->isRepeated())
                continue;

            for(int Offset = -1; Offset < 2; ++Offset) {
                bool Found = false;
                std::multimap<int, PdfItemPtr_t>* ComparingList = &ComparingPageItemMapWithBottomOffset;
                int Key = computeKey(Item, _pageMargins.Bottom, Offset);
                auto ComparingPageItemIter = ComparingList->find(Key);
                if(ComparingPageItemIter == ComparingList->end()) {
                    ComparingList = &ComparingPageItemMapWithTopOffset;
                    Key = computeKey(Item, _pageMargins.Top * -1, Offset);
                    ComparingPageItemIter = ComparingList->find(Key);
                }
                if(ComparingPageItemIter == ComparingList->end()) {
                    ComparingList = &ComparingPageItemMapOriginal;
                    Key = computeKey(Item, 0, Offset);
                    ComparingPageItemIter = ComparingList->find(Key);
                }

                while(ComparingPageItemIter != ComparingList->end()) {
                    if(Item->isSame(_pageMargins, ComparingPageItemIter->second, ComparingPageMargins)) {
                        CandidateWatermarks.push_back(Item);
                        Item->markAsRepeated(static_cast<int32_t>(ComparingPageIndex - _pageIndex));
                        LA::markLocation(Item, _pageSize);
                        Found = true;
                        break;
                    }
                    ++ComparingPageItemIter;
                    if(ComparingPageItemIter->first != Key)
                        break;
                }
                if(Found)
                    break;
            }
        }

    }

    PdfItemPtrVector_t CertainWatermarks;
    for(const auto& Item : CandidateWatermarks)
        if(canBeWatermark(Item)) {
            Item->markAsWatermark(true);
            Item->resetWatermarkRepetition();
            CertainWatermarks.push_back(Item);
        }



    for(const auto& Candidate : CandidateWatermarks){
        if(Candidate->isWatermark()) continue;
        for(const auto& WMark : CertainWatermarks){
            if(WMark->horzOverlap(Candidate) > MIN_ITEM_SIZE && WMark->vertOverlap(Candidate) > MIN_ITEM_SIZE) {
                if(
                   std::abs(WMark->height() - Candidate->height()) < 2 ||
                   std::abs(WMark->width() - Candidate->width()) < 2
                   ){
                    Candidate->markAsWatermark(true);
                    Candidate->resetWatermarkRepetition();
                    break;
                }
            }
        }
    }
}

}
}
}
