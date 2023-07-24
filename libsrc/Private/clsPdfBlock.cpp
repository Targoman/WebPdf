#include "clsPdfBlock.h"


namespace Targoman {
namespace PDF {
namespace Private {

class clsPdfBlockData {
public:
    PdfLineSegmentPtrVector_t Lines;
    PdfBlockPtrVector_t         InnerBlocks;
    enuLocation                 Location;
    enuJustification            Justification;
    enuContentType              ContentType;
    uint8_t                     InnerColsCountByLayout;
    bool                        CertainlyFinished;
    uint8_t                     __PADDING__[2];
    float                       FirstLineIndent;
    float                       TextIndent;
    float                       Space2Left;
    float                       Space2Right;

    clsPdfBlockData(enuLocation _location = enuLocation::None,
                    enuJustification _justification = enuJustification::None,
                    enuContentType _contentType = enuContentType::None) :
        Location(_location),
        Justification(_justification),
        ContentType(_contentType),
        InnerColsCountByLayout(0),
        CertainlyFinished(false),
        FirstLineIndent(0.f),
        TextIndent(0.f),
        Space2Left(0.f),
        Space2Right(0.f)
    {}

};

clsPdfBlock::clsPdfBlock(enuLocation _location,
                         enuContentType _contentType,
                         enuJustification _justification) :
    Data(new clsPdfBlockData(_location, _justification, _contentType))
{

}

clsPdfBlock::~clsPdfBlock()
{ }

PdfLineSegmentPtrVector_t clsPdfBlock::lines() const
{
    return this->Data->Lines;
}

void clsPdfBlock::clearLines()
{
    this->Data->Lines.clear();
}

void clsPdfBlock::replaceLines(const PdfLineSegmentPtrVector_t& _newLines)
{
    this->Data->Lines = _newLines;
}

PdfBlockPtrVector_t& clsPdfBlock::innerBlocks()
{
    return this->Data->InnerBlocks;
}

void clsPdfBlock::appendInnerBlock(PdfBlockPtr_t _block)
{
    this->Data->InnerBlocks.push_back(_block);
    this->unionWith_(_block);
}

enuLocation clsPdfBlock::location() const
{
    return this->Data->Location;
}

void clsPdfBlock::setLocation(enuLocation _loc)
{
    this->Data->Location = _loc;
}

enuContentType clsPdfBlock::contentType() const
{
    return this->Data->ContentType;
}

void clsPdfBlock::setContentType(enuContentType _type)
{
    this->Data->ContentType = _type;
}

enuJustification clsPdfBlock::justification() const
{
    return this->Data->Justification;
}

PdfLineSegmentPtrVector_t clsPdfBlock::horzRulers(float _minWidth)
{
    auto CandidateHorzRulers = filter<PdfLineSegmentPtr_t>(this->Data->Lines, [&](const PdfLineSegmentPtr_t& _line) {
        return  _line->isHorizontalRuler();
    });

    PdfLineSegmentPtrVector_t MergedRulers;
    for(const auto& Line : CandidateHorzRulers) {
        bool Merged = false;
        for(const auto& MR : MergedRulers){
            if(std::abs(MR->centerY() - Line->centerY()) < 2.f) {
                MR->unionWith_(Line);
                Merged = true;
                break;
            }
        }
        if(Merged == false)
            MergedRulers.push_back(Line);
    }

    return filter<PdfLineSegmentPtr_t>(MergedRulers, [&](const PdfLineSegmentPtr_t& _line){return _line->width() > _minWidth; });
}

PdfLineSegmentPtrVector_t clsPdfBlock::vertRulers(float _minHeight)
{
    auto CandidateVertRulers = filter<PdfLineSegmentPtr_t>(this->Data->Lines, [&](const PdfLineSegmentPtr_t& _line) {
        return  _line->isVerticalRuler();
    });

    PdfLineSegmentPtrVector_t MergedRulers;
    for(const auto& Line : CandidateVertRulers) {
        bool Merged = false;
        for(const auto& MR : MergedRulers){
            if(std::abs(MR->centerX() - Line->centerX()) < 2.f) {
                MR->unionWith_(Line);
                Merged = true;
                break;
            }
        }
        if(Merged == false)
            MergedRulers.push_back(Line);
    }

    return filter<PdfLineSegmentPtr_t>(MergedRulers, [&](const PdfLineSegmentPtr_t& _line){return _line->width() > _minHeight; });
}

bool clsPdfBlock::mostlyImage() const
{
    clsPdfItem ImageItem;
    for (const auto& Line : this->Data->Lines) {
        if(Line->isPureImage()
           && Line->isHorizontalRuler() == false)
            ImageItem.unionWith_(Line);
    }
    return ImageItem.width() > 0
            && (ImageItem.width() * ImageItem.height()) > (.5f * this->height() * this->tightWidth());
}

bool clsPdfBlock::isPureImage() const
{
    clsPdfItem ImageItem;
    for (const auto& Line : this->Data->Lines)
        if(Line->isPureImage() == false)
            return false;
    return true;
}

float clsPdfBlock::approxLineHeight() const
{
    std::map<int16_t, size_t> Heights;

    std::function<void(const clsPdfBlock&)> extractBlockLineHeights = [&](const clsPdfBlock& _block){
        if(_block.Data->InnerBlocks.size())
            for(const auto& IB : _block.Data->InnerBlocks)
                extractBlockLineHeights(*IB.get());
        else
            for(const auto& Line : _block.Data->Lines) {
                if(Line->isPureImage()) continue;
                int16_t Rounded = static_cast<int16_t>(round(Line->height() * 10.f) / 10);
                if(Heights.find(Rounded) == Heights.end())
                    Heights[Rounded] = Line->items().size();
                else
                    Heights[Rounded] += Line->items().size();
        }
    };

    extractBlockLineHeights(*this);

    float Median = -1;
    size_t BestCount = 0;
    for(auto HIter = Heights.begin(); HIter != Heights.end(); ++HIter)
        if(HIter->second > BestCount) {
            Median = HIter->first;
            BestCount = HIter->second;
        }
    return Median;
}

uint8_t clsPdfBlock::innerColsCountByLayout()
{
    return this->Data->InnerColsCountByLayout;
}

void clsPdfBlock::setInnerColsCountByLayout(uint8_t _count)
{
    this->Data->InnerColsCountByLayout = _count;
}

float clsPdfBlock::space2Left() const
{
    return this->Data->Space2Left;
}

float clsPdfBlock::space2Right() const
{
    return this->Data->Space2Right;
}

void clsPdfBlock::setSpace2Left(float _value)
{
    this->Data->Space2Left = _value;
}

void clsPdfBlock::setSpace2Right(float _value)
{
    this->Data->Space2Right = _value;
}

float clsPdfBlock::firstLineIndent() const
{
    return this->Data->FirstLineIndent;
}

float clsPdfBlock::textIndent() const
{
    return this->Data->TextIndent;
}

float clsPdfBlock::textIndentPos() const
{
    return this->x0() + this->Data->TextIndent;
}

void clsPdfBlock::setFirstLineIndent(float _value) const
{
    this->Data->FirstLineIndent = _value;
}

void clsPdfBlock::setTextIndent(float _value) const
{
    this->Data->TextIndent = _value;
}

float clsPdfBlock::tightLeft() const
{
    return this->x0() + this->Data->Space2Left;
}

float clsPdfBlock::tightRight() const
{
    return this->x1() - this->Data->Space2Right;
}

float clsPdfBlock::tightWidth() const
{
    return this->tightRight() - this->tightLeft();
}

bool clsPdfBlock::certainlyFinished() const
{
    return this->Data->CertainlyFinished;
}

void clsPdfBlock::setCertainlyFinished(bool _value)
{
    this->Data->CertainlyFinished = _value;
}

bool clsPdfBlock::doesShareFontsWith(const clsPdfBlock &_other) const
{
    if(this->Data->InnerBlocks.size() > 0 || _other.Data->InnerBlocks.size() > 0) {
        for(const auto& InnerBlock : this->Data->InnerBlocks)
            if(InnerBlock->doesShareFontsWith(_other))
                return true;
        return false;
    } else {
        for(const auto& Line : this->Data->Lines)
            for(const auto& OtherLine : _other.Data->Lines)
                if(Line->doesShareFontsWith(OtherLine))
                    return true;
        return false;
    }
}

bool clsPdfBlock::doesShareFontsWith(const PdfBlockPtr_t &_other) const
{
    return this->doesShareFontsWith(*_other);
}

bool clsPdfBlock::doesShareFontsWith(const PdfLineSegmentPtr_t &_otherLine) const
{
    if(this->Data->InnerBlocks.size() > 0) {
        for(const auto& InnerBlock : this->Data->InnerBlocks)
            if(InnerBlock->doesShareFontsWith(_otherLine))
                return true;
        return false;
    } else {
        for(const auto& Line : this->Data->Lines)
            if(Line->doesShareFontsWith(_otherLine))
                return true;
        return false;
    }
}

void clsPdfBlock::appendLineSegment(const PdfLineSegmentPtr_t& _line, bool _mergeOverlappingLines)
{
    if(this->Data->InnerBlocks.size() > 0) {
        assert(false);
    } else {
        if(_mergeOverlappingLines
           && _line->type() != enuPdfItemType::VirtualLine
           && _line->type() != enuPdfItemType::Background) {
            auto AllLines = this->Data->Lines;
            AllLines.push_back(_line);

            sort(AllLines, [](const PdfLineSegmentPtr_t& _a, const PdfLineSegmentPtr_t& _b){return  _a->x0() < _b->x0();});

            while(true) {
                bool MergedOccurredAtLeastOnce = false;
                PdfLineSegmentPtrVector_t AllLinesImproved;
                for(auto& Line : AllLines) {
                    bool Merged = false;
                    for(auto& OtherLine : AllLinesImproved) {
                        if(OtherLine->isPureImage() && OtherLine->doesContain(Line)) {
                            if(Line->isPureImage() && OtherLine->isBackground() == false) {
                                if(OtherLine->mergeWith_(Line)) {
                                    Merged = true;
                                    MergedOccurredAtLeastOnce = true;
                                }
                            }
                            if(Merged == false)
                                Line->setRelativeAndAssociation(OtherLine, enuLineSegmentAssociation::InsideTextOf);
                            break;
                        } else if (Line->isPureImage() && Line->doesContain(OtherLine)) {
                            if(OtherLine->isPureImage() && Line->isBackground() == false) {
                                if(OtherLine->mergeWith_(Line)) {
                                    Merged = true;
                                    MergedOccurredAtLeastOnce = true;
                                }
                            }
                            if(Merged == false)
                                OtherLine->setRelativeAndAssociation(Line, enuLineSegmentAssociation::InsideTextOf);
                            else
                                break;
                        } else if (OtherLine->tightVertOverlap(Line) > 0.5f * std::min(OtherLine->tightHeight(), Line->tightHeight())) {
                            float Threshold = std::max(Line->mostUsedFontSize(), OtherLine->mostUsedFontSize());

                            if(Threshold > 0 && OtherLine->horzSpacer(Line).width() > 10 * Threshold)
                                break;

                            if(OtherLine->mergeWith_(Line)) {
                                Merged = true;
                                MergedOccurredAtLeastOnce = true;
                            }
                            break;
                        }
                    }
                    if(Merged == false)
                        AllLinesImproved.push_back(Line);
                }

                if(MergedOccurredAtLeastOnce == false)
                    break;

                AllLines = AllLinesImproved;
            }

            this->Data->Lines = AllLines;

        } else
            this->Data->Lines.push_back(_line);

        this->unionWith_(_line);
    }
}

float clsPdfBlock::approxLineSpacing() const
{
    std::map<int16_t, size_t> Spacings;

    std::function<void(const clsPdfBlock&)> extractLineSpacings = [&](const clsPdfBlock& _block){
        if(_block.Data->InnerBlocks.size())
            for(const auto& IB : _block.Data->InnerBlocks)
                extractLineSpacings(*IB.get());
        else
            for(size_t i = 1; i < _block.lines().size(); ++i) {
                auto PrevLine = _block.lines().at(i - 1);
                auto CurrLine = _block.lines().at(i);
                if(
                   PrevLine->isPureImage()
                   || CurrLine->isPureImage()
                   || PrevLine->association() == enuLineSegmentAssociation::InsideTextOf
                   || CurrLine->association() == enuLineSegmentAssociation::InsideTextOf
                ) continue;
                int16_t Rounded = static_cast<int16_t>(round(CurrLine->baseline() - PrevLine->baseline()));
                if(Spacings.find(Rounded) == Spacings.end())
                    Spacings[Rounded] = CurrLine->items().size();
                else
                    Spacings[Rounded] += CurrLine->items().size();
        }
    };

    extractLineSpacings(*this);

    float Median = -1;
    size_t BestCount = 0;
    for(auto HIter = Spacings.begin(); HIter != Spacings.end(); ++HIter)
        if(HIter->second > BestCount) {
            Median = HIter->first;
            BestCount = HIter->second;
        }
    return Median;
}

float clsPdfBlock::averageCharHeight() const
{
    if(this->Data->InnerBlocks.size() > 0) {
        float MinAverage = std::numeric_limits<float>::max();
        for(auto Block : this->Data->InnerBlocks)
            MinAverage = std::min(MinAverage, Block->averageCharHeight());
        return  MinAverage;
    } else {
        float Result = 0;
        size_t N = 0;
        for(const auto& Item : this->Data->Lines){
            if(Item->isPureImage()) continue;
            for(const auto& Char : *Item) {
                if(Char->type() != enuPdfItemType::Char)
                    continue;
                Result += Char->height();
                ++N;
            }
        }
        if(std::abs(Result) > MIN_ITEM_SIZE)
            return Result / N;
        return 0;
    }
}

void clsPdfBlock::mergeWith_(const clsPdfBlock& _other, bool _forceOneColumnMerge)
{
    if(this == &_other)
        return;

    auto canUnite = [&](const clsPdfBlock& _this, const clsPdfBlock& _other){
        if(_forceOneColumnMerge)
            return true;
        if(_this.isHorizontalRuler())
            return false;
        if(_other.isHorizontalRuler())
            return false;
        return -_this.horzOverlap(_other) < 3.f;
    };

    auto recalcGeometry = [&](){
        this->setX0(std::numeric_limits<float>::max());
        this->setY0(std::numeric_limits<float>::max());
        this->setX1(-std::numeric_limits<float>::max());
        this->setY1(-std::numeric_limits<float>::max());
        if(this->Data->InnerBlocks.size() > 0)
            for (const auto &Block : this->Data->InnerBlocks)
                this->unionWith_(Block);
        else
            for (const auto &Line : this->Data->Lines)
                this->unionWith_(Line);
    };

    if(this->Data->InnerBlocks.size() > 0 || _other.Data->InnerBlocks.size() > 0) {
        PdfBlockPtrVector_t AllInnerBlocks;
        this->appendToFlatBlockVector(AllInnerBlocks);
        _other.appendToFlatBlockVector(AllInnerBlocks);
        this->Data->Lines.clear();

        bool ChangeOccurred;
        do {
            ChangeOccurred = false;
            PdfBlockPtrVector_t Copy = AllInnerBlocks;
            AllInnerBlocks.clear();
            for(auto& Item : Copy) {
                bool Merged = false;
                for(const auto& OtherItem : AllInnerBlocks) {
                    if(canUnite(*OtherItem, *Item)) {
                        OtherItem->mergeWith_(Item, _forceOneColumnMerge);
                        Merged = true;
                    }
                }
                if(Merged == false)
                    AllInnerBlocks.push_back(Item);
                else
                    ChangeOccurred = true;
            }
        } while(ChangeOccurred);

        this->Data->InnerBlocks = AllInnerBlocks;
    } else {

        if(canUnite(*this, _other)){

            if(this->isNonText()) {
                for(const auto& OtherItem : _other.Data->Lines) {
                    PdfLineSegmentPtr_t CandidateCover;
                    for(const auto& Item : this->Data->Lines)
                        if(Item->isPureImage() && Item->doesTouch(OtherItem)) {
                            CandidateCover = Item;
                            break;
                        }
                    this->Data->Lines.push_back(OtherItem);
                    if(CandidateCover.get() != nullptr) {
                        OtherItem->setRelativeAndAssociation(CandidateCover, enuLineSegmentAssociation::InsideTextOf);
                        CandidateCover->setRelativeAndAssociation(OtherItem, enuLineSegmentAssociation::HasAsInsideText);
                    }
                }
            } else {
                for (const auto &Line : _other.Data->Lines) {
                    this->appendLineSegment(Line, true);
                }
            }
        } else {
            auto Block = *this;
            Block.detach();
            this->Data->InnerBlocks.push_back(std::make_shared<clsPdfBlock>(Block));
            this->Data->InnerBlocks.push_back(std::make_shared<clsPdfBlock>(_other));
            this->Data->Lines.clear();
        }
    }
    recalcGeometry();
}

void clsPdfBlock::mergeWith_(const PdfBlockPtr_t &_other, bool _forceOneColumnMerge)
{
    return this->mergeWith_(*_other, _forceOneColumnMerge);
}

void clsPdfBlock::appendToFlatBlockVector(PdfBlockPtrVector_t& _flatBlockVector) const
{
    if(this->Data->InnerBlocks.size() > 0) {
        for(auto& InnerBlock : this->Data->InnerBlocks) {
            InnerBlock->setLocation(this->location());
            _flatBlockVector.push_back(InnerBlock);
        }
    } else {
        auto Copy = *this;
        Copy.detach();
        _flatBlockVector.push_back(std::make_shared<clsPdfBlock>(Copy));
    }
}

bool clsPdfBlock::isNonText() const
{
    if(this->Data->InnerBlocks.size() > 0) {
        for(auto Block : this->Data->InnerBlocks)
            if(Block->isNonText() == false)
                return false;
    } else {
        for(const auto& Line : this->Data->Lines) {
            for(const auto& Item : *Line)
                if(Item->type() == enuPdfItemType::Char)
                    return false;
        }
    }
    return true;
}

bool clsPdfBlock::isMainTextBlock() const
{
    return this->location() == enuLocation::Main && (this->contentType() == enuContentType::Text || this->contentType() == enuContentType::List);
}

void clsPdfBlock::naiveSortByReadingOrder()
{
    if(this->Data->InnerBlocks.size()){
        for(auto& Block : this->Data->InnerBlocks)
            Block->naiveSortByReadingOrder();
        this->Data->InnerBlocks = naiveSortItemsByReadingOrder(this->Data->InnerBlocks);
    }

    this->Data->Lines = naiveSortItemsByReadingOrder(this->Data->Lines);
}

void clsPdfBlock::prepareLineSegments(const FontInfo_t& _fontInfo, const stuConfigsPtr_t& _configs)
{
    if(this->Data->InnerBlocks.size() > 0) {
        sort(this->Data->InnerBlocks, [] (const PdfBlockPtr_t& _a, const PdfBlockPtr_t& _b) {
            return _a->x0() < _b->x0();
        });
        for(auto& InnerBlock : this->Data->InnerBlocks) {
            InnerBlock->setLocation(this->location());
            InnerBlock->prepareLineSegments(_fontInfo, _configs);
        }
    } else {
        this->sortLineSegmentsTopToBottom();
        for(auto& Line : this->Data->Lines)
            Line->consolidateSpaces(_fontInfo, _configs);
    }
}

void clsPdfBlock::sortLineSegmentsTopToBottom()
{
    if(this->Data->InnerBlocks.size() > 0) {
        sort(this->Data->InnerBlocks, [] (const PdfBlockPtr_t& _a, const PdfBlockPtr_t& _b) {
            if(_a->isHorizontalRuler() || _b->isHorizontalRuler())
                return _a->centerY() < _b->centerY();
            return _a->x0() < _b->x0();
        });
        for(auto& InnerBlock : this->Data->InnerBlocks) {
            InnerBlock->setLocation(this->location());
            InnerBlock->sortLineSegmentsTopToBottom();
        }
    } else {
        sort(this->Data->Lines, [] (const PdfLineSegmentPtr_t& a, const PdfLineSegmentPtr_t& b) {
            if(a->tightVertOverlap(b) >= std::min(a->tightHeight(), b->tightHeight()))
                return a->x0() < b->x0();
            return a->y0() < b->y0();
        });
    }
}

void clsPdfBlock::estimateParagraphParams() {

    this->sortLineSegmentsTopToBottom();

    if(this->lines().size() == 0) {
        // TODO: Issue warning
        this->Data->Justification = enuJustification::Justified;
        this->Data->FirstLineIndent = 0.f;
        this->Data->TextIndent = 0.f;
    }

    clsFloatHistogram LeftHistogram(1.f), RightHistogram(1.f);

    bool IsCenterAligned = true;

    for(size_t i = 0; i < this->lines().size(); ++i) {
        bool Ignore = false;
        for(size_t j = 0; j < i; ++j) {
            if(this->lines().at(j)->vertOverlap(this->lines().at(i)) > MIN_ITEM_SIZE) {
                Ignore = true;
                break;
            }
        }
        if(Ignore)
            continue;
        float Left = this->lines().at(i)->x0();
        float Right = this->lines().at(i)->x1();
        for(size_t j = i + 1; j < this->lines().size(); ++j) {
            if(this->lines().at(j)->vertOverlap(this->lines().at(i)) > MIN_ITEM_SIZE) {
                Left = std::min(this->lines().at(j)->x0(), Left);
                Right = std::max(this->lines().at(j)->x1(), Right);
            }
        }
        LeftHistogram.insert(Left);
        RightHistogram.insert(Right);
        IsCenterAligned &= std::abs(Left - this->x0() - this->x1() + Right) < 2.f;
    }

    clsFloatHistogram::HistogramItem_t WinnerLeftItem = { this->x0(), 0 };
    for(const auto& Item : LeftHistogram.items())
        if(Item.second > WinnerLeftItem.second || (Item.second == WinnerLeftItem.second && Item.first < WinnerLeftItem.first))
            WinnerLeftItem = Item;

    clsFloatHistogram::HistogramItem_t WinnerRightItem = { this->x1(), 0 };
    for(const auto& Item : RightHistogram.items())
        if(Item.second > WinnerRightItem.second || (Item.second == WinnerRightItem.second && Item.first > WinnerRightItem.first))
            WinnerRightItem = Item;

    if(WinnerLeftItem.second > 3 * static_cast<int>(LeftHistogram.size()) / 4) {
        this->Data->Justification = this->Data->Justification | enuJustification::Left;
        this->Data->TextIndent = WinnerLeftItem.first - this->x0();
    } else
        this->Data->TextIndent = 0.f;

    if(WinnerRightItem.first >= this->tightRight() - 2.f && WinnerRightItem.second > 3 * static_cast<int>(RightHistogram.size()) / 4)
        this->Data->Justification = this->Data->Justification | enuJustification::Right;

    if(this->Data->Justification == enuJustification::None && IsCenterAligned) {
        this->Data->Justification = enuJustification::Center;
        this->Data->FirstLineIndent = 0.f;
        this->Data->TextIndent = 0.f;
    }

    if(this->Data->Justification != enuJustification::Center) {
        this->Data->FirstLineIndent = this->Data->TextIndent;
        for(auto Item : LeftHistogram.items())
            if(std::abs(Item.first - this->Data->TextIndent) > std::abs(this->Data->FirstLineIndent - this->Data->TextIndent))
                this->Data->FirstLineIndent = Item.first;
    }

    if(this->lines().size() == 2 && this->Data->Justification != enuJustification::Center) {
        if(WinnerLeftItem.first <= this->tightLeft() + 2.f && WinnerRightItem.first >= this->tightRight() - 2.f)
            this->Data->Justification = enuJustification::Justified;
    }

    if(this->Data->Justification == enuJustification::None)
        this->Data->Justification = enuJustification::Justified;
}

}
}
}
