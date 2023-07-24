#include "PdfLineSegmentManager.h"

namespace Targoman {
namespace PDF {
namespace Private {
namespace LA {

PdfLineSegmentPtrVector_t findVertStripes(const PdfItemPtrVector_t &_items, const stuSize& _pageSize)
{
    PdfItemPtrVector_t Texts, Images;
    std::tie(Texts, Images) = splitAndMap<PdfItemPtr_t, PdfItemPtr_t, PdfItemPtr_t>(
                _items,
                [] (const PdfItemPtr_t& _item) { return _item->type() == enuPdfItemType::Char; },
                [] (const PdfItemPtr_t& _item) { return _item; },
                [] (const PdfItemPtr_t& _item) { return _item; }
    );

    sort(Texts, [&] (const PdfItemPtr_t& _a, const PdfItemPtr_t& _b) {
        return _a->x1() < _b->x1();
    });

    PdfLineSegmentPtrVector_t Result;
    for(const auto& Item : Texts) {
        PdfLineSegmentPtr_t CandidateOwnerLine = nullptr;
        for(const auto& Line : Result) {
            if(Item->vertOverlap(Line) > 0.5f * Item->height()) {
                CandidateOwnerLine = Line;
                break;
            }
        }
        if (CandidateOwnerLine.get() == nullptr) {
            CandidateOwnerLine = std::make_shared<clsPdfLineSegment>();
            Result.push_back(CandidateOwnerLine);
        }
        CandidateOwnerLine->appendPdfItem(Item);
    }

    PdfLineSegmentPtrVector_t ImageResults;

    for(const auto& Image : Images) {
        PdfLineSegmentPtr_t CandidateOwnerLine = nullptr;
        for(const auto& Line : ImageResults) {
            if(Image->type() != enuPdfItemType::Background
               && Line->type() != enuPdfItemType::Background
               && Image->vertOverlap(Line) > (
                   (Image->isHorizontalRuler() && Image->width() > .75f * _pageSize.Width) ||
                   (Line->isHorizontalRuler() && Line->width() > .75f * _pageSize.Width) ?
                   -2 :
                   -15)) {
                CandidateOwnerLine = Line;
                break;
            }
        }
        if (CandidateOwnerLine.get() == nullptr) {
            CandidateOwnerLine = std::make_shared<clsPdfLineSegment>();
            ImageResults.push_back(CandidateOwnerLine);
        }
        CandidateOwnerLine->appendPdfItem(Image);
    }

    for(const auto& Line : ImageResults)
        Result.push_back(Line);

    sort(Result, [](const PdfLineSegmentPtr_t& _a, const PdfLineSegmentPtr_t& _b){return _a->y0() < _b->y0();});
    return Result;
}

PdfLineSegmentPtrVector_t findLines(const PdfItemPtrVector_t &_items)
{
    PdfItemPtrVector_t Texts, Images;
    std::tie(Texts, Images) = splitAndMap<PdfItemPtr_t, PdfItemPtr_t, PdfItemPtr_t>(
                _items,
                [] (const PdfItemPtr_t& _item) { return _item->type() == enuPdfItemType::Char; },
                [] (const PdfItemPtr_t& _item) { return _item; },
                [] (const PdfItemPtr_t& _item) { return _item; }
    );

    sort(Texts, [&] (const PdfItemPtr_t& _a, const PdfItemPtr_t& _b) {
        return _a->x1() < _b->x1();
    });

    PdfLineSegmentPtrVector_t Result;
    for(const auto& Item : Texts) {
        PdfLineSegmentPtr_t CandidateOwnerLine;
        for(const auto& Line : Result) {
            if(Item->conformsToLine(Line->y0(), Line->y1(), Line->baseline())) {
                float Threshold = 0.5f * std::min(
                    Line->items().back()->as<clsPdfChar>()->fontSize(),
                    Item->as<clsPdfChar>()->fontSize()
                );
                if(Line->items().back()->isMainContent() == false && Item->isMainContent() == false)
                    Threshold = std::numeric_limits<float>::max();
                auto Spacer = Item->horzSpacer(Line);
                if(
                   Spacer.width() < Threshold &&
                   !anyOf<PdfItemPtr_t>(Images, [&] (const PdfItemPtr_t& _item) {
                        return !_item->isVeryThin() && !_item->isVeryFat() && _item->doesTouch(Spacer);
                   })
                ) {
                    CandidateOwnerLine = Line;
                    break;
                }
            }
        }

        if (CandidateOwnerLine.get() == nullptr) {
            CandidateOwnerLine = std::make_shared<clsPdfLineSegment>();
            Result.push_back(CandidateOwnerLine);
        }
        if(Item->isRepeated()) {
            if(Item->isHeader()) {
                CandidateOwnerLine->markAsRepeated(0);
                CandidateOwnerLine->markAsHeader(true);
            } else if(Item->isFooter()) {
                CandidateOwnerLine->markAsRepeated(0);
                CandidateOwnerLine->markAsFooter(true);
            } else if (Item->isSidebar()) {
                CandidateOwnerLine->markAsRepeated(0);
                CandidateOwnerLine->markAsSidebar(true);
            }
        }
        CandidateOwnerLine->appendPdfItem(Item);
    }

    for(const auto& Item : Images) {
        PdfLineSegmentPtr_t CandidateOwnerLine;
        if(Item->type() != enuPdfItemType::Background) {
            for(const auto& Line : Result) {
                if(Line->items().size() == 1
                   && Line->items().front()->type() == enuPdfItemType::Background)
                    continue;
                if(Item->conformsToLine(Line->y0(), Line->y1(), Line->baseline())) {
                    auto Spacer = Item->horzSpacer(Line);
                    if(Spacer.width() < 5.f) {
                        CandidateOwnerLine = Line;
                        break;
                    }
                }
            }
        }
        if (CandidateOwnerLine.get() == nullptr){
            CandidateOwnerLine = std::make_shared<clsPdfLineSegment>();
            Result.push_back(CandidateOwnerLine);
        }
        if(Item->isRepeated()) {
            if(Item->isHeader()) {
                CandidateOwnerLine->markAsRepeated(0);
                CandidateOwnerLine->markAsHeader(true);
            } else if(Item->isFooter()) {
                CandidateOwnerLine->markAsRepeated(0);
                CandidateOwnerLine->markAsFooter(true);
            } else if (Item->isSidebar()) {
                CandidateOwnerLine->markAsRepeated(0);
                CandidateOwnerLine->markAsSidebar(true);
            }
        }
        CandidateOwnerLine->appendPdfItem(Item);
    }

    return Result;
}

PdfLineSegmentPtrVector_t mergeOverlappingLines(const stuConfigsPtr_t& _configs, const PdfLineSegmentPtrVector_t& _blockLines, const FontInfo_t& _fontInfo)
{
    PdfLineSegmentPtrVector_t CorrectedLines;

    auto areSameLineParts = [] (const PdfLineSegmentPtr_t& _line0, const PdfLineSegmentPtr_t& _line1) {
        float Threshold = std::max(_line0->mostUsedFontSize(), _line1->mostUsedFontSize());

        if(Threshold > 0 && _line1->horzSpacer(_line0).width() > 10 * Threshold)
            return false;

        float VertOverlap = _line0->vertOverlap(_line1);

        auto needsPureImageMergePrevention = [&VertOverlap](const PdfLineSegmentPtr_t& _pureImageCandidate, const PdfLineSegmentPtr_t& _textCandidate) {
            if(_pureImageCandidate->isPureImage() == false)
                return false;
            if(_textCandidate->isPureImage())
                return false;
            if(_pureImageCandidate->height() > 1.5f * _textCandidate->height() || _pureImageCandidate->width() > 5.f * _textCandidate->height())
                return true;
            if(VertOverlap < 0.75f * _textCandidate->height())
                return true;
            return false;
        };

        if(needsPureImageMergePrevention(_line0, _line1))
            return false;

        if(needsPureImageMergePrevention(_line1, _line0))
            return false;

        if(VertOverlap > 0.5f * std::min(_line0->height(), _line1->height()))
            return true;

        if(VertOverlap > 1.f) {
            if(_line0->height() < 0.75f * _line1->height())
                return true;
            if(_line1->height() < 0.75f * _line0->height())
                return true;
        }
        return false;
    };

    for(const auto& Line : _blockLines) {
        bool Merged = false;
        if(Line->items().size() != 1 || Line->items().front()->type() != enuPdfItemType::Background)
            for(auto& CorrectedLine : CorrectedLines) {
                if(areSameLineParts(CorrectedLine, Line)) {
                    CorrectedLine->mergeWith_(Line);
                    Merged = true;
                    break;
                }
            }
        if(Merged == false)
            CorrectedLines.push_back(Line);
    }

    for(auto& Line : CorrectedLines)
        Line->consolidateSpaces(_fontInfo, _configs);

    return CorrectedLines;
}

}
}
}
}
