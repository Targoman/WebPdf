#include <set>
#include "clsLayoutStorage.h"
#include "clsPdfBlock.h"

namespace Targoman {
namespace PDF {
namespace Private {

inline float minAcceptableColWidth(float _contentWidth){
    return  .15f * _contentWidth;
}

float getRealX0(const PdfLineSegmentPtr_t& _line) {
    return _line->x0();
}

float getRealY0(const PdfLineSegmentPtr_t& _line) {
    return _line->y0();
}

float getRealX1(const PdfLineSegmentPtr_t& _line) {
    return _line->x1();
}

float getRealY1(const PdfLineSegmentPtr_t& _line) {
    return _line->y1();
}

float getYOverlap(const PdfLineSegmentPtr_t& _line0, const PdfLineSegmentPtr_t& _line1) {
    return _line0->vertOverlap(_line1);
}

float getXOverlap(const PdfLineSegmentPtr_t& _line0, const PdfLineSegmentPtr_t& _line1) {
    return _line0->horzOverlap(_line1);
}

void sortByTopLeft(PdfLineSegmentPtrVector_t& _lines){
    sort(_lines, [] (const PdfLineSegmentPtr_t& a, const PdfLineSegmentPtr_t& b) {
        if(a->y0() < b->centerY())
            return true;
        if(a->y1() > b->centerY())
            return false;
        return a->x0() < b->x0();
    });
}

bool splitLayoutStorage(clsLayoutStorage& _parent, enuDirection _direction, const stuConfigsPtr_t& _configs, bool _allowBackImages) {
    auto getThreshold = [&](const PdfLineSegmentPtrVector_t& _lines, enuDirection _direction) {
        auto SortedLines = _lines;
        if(_direction == enuDirection::Vertical) {
            if(_lines.size() == 1)
                return 2.f;

            sortByTopLeft(SortedLines);
            float Threshold = std::numeric_limits<float>::max();
            size_t j = 0;
            for(size_t i = 1; i < SortedLines.size() - 1; ++i) {
                if(SortedLines[i]->y0() <= SortedLines[j]->centerY())
                    continue;
                Threshold = std::min(std::max(SortedLines[i]->y0() - SortedLines[j]->y1(), 0.f), Threshold);
                j = i;
            }
            if(Threshold < 3.f)
                return 3.f;
            return 1.1f * Threshold + 4.f;
        } else if(_direction == enuDirection::Horizontal) {
            return 3.f;
        }
        return 10.f;
    };

    float (*_p0)(const PdfLineSegmentPtr_t& _line) = _direction == enuDirection::Horizontal ? getRealX0 : getRealY0;
    float (*_p1)(const PdfLineSegmentPtr_t& _line) = _direction == enuDirection::Horizontal ? getRealX1 : getRealY1;
    float (*_op)(const PdfLineSegmentPtr_t& _line0, const PdfLineSegmentPtr_t& _line1) = _direction == enuDirection::Horizontal ? getYOverlap : getXOverlap;

    const float Threshold = getThreshold(_parent.Lines, _direction);

    auto FilteredLines = _parent.Lines;

    std::vector<std::pair<float, float>> Filled;
    for(const auto& Item : FilteredLines) {
        if(Filled.size() == 0) {
            Filled.push_back(std::make_pair(_p0(Item), _p1(Item)));
            continue;
        }
        size_t i = 0;
        size_t j = Filled.size();
        for(size_t k = Filled.size(); k > 0; --k)
            if(_p0(Item) >= Filled.at(k - 1).first) {
                i = k - 1;
                break;
            }
        for(size_t k = 0; k < Filled.size(); ++k)
            if(_p1(Item) <= Filled.at(k).second) {
                j = k + 1;
                break;
            }
        if(_p0(Item) >= Filled.at(i).second)
            ++i;
        if(_p1(Item) <= Filled.at(j - 1).first)
            --j;
        std::vector<std::pair<float, float>> UpdatedFill;
        for(size_t k = 0; k < i; ++k)
            UpdatedFill.push_back(Filled.at(k));
        if(i < j)
            UpdatedFill.push_back(std::make_pair(
                                      std::min(_p0(Item), Filled.at(i).first),
                                      std::max(_p1(Item), Filled.at(j - 1).second)
                                      ));
        else if (j < Filled.size()) {
            UpdatedFill.push_back(std::make_pair(_p0(Item), _p1(Item)));
        }
        for(size_t k = j; k < Filled.size(); ++k)
            UpdatedFill.push_back(Filled.at(k));
        if(i == Filled.size() && j == Filled.size())
            UpdatedFill.push_back(std::make_pair(_p0(Item), _p1(Item)));
        Filled = UpdatedFill;
    }

    std::vector<std::pair<float, float>> Gaps;
    std::vector<float> BreakPoints;

    for(size_t i = 1; i < Filled.size(); ++i) {
        float GapCenter = 0.5f * (Filled.at(i).first + Filled.at(i - 1).second);
        float RealGap = Filled.at(i).first - Filled.at(i - 1).second;
        if(RealGap > MIN_ITEM_SIZE) {
            PdfLineSegmentPtrVector_t Side0, Side1;
            for(const auto& Line : FilteredLines)
                if(_p1(Line) < GapCenter)
                    Side0.push_back(Line);
                else
                    Side1.push_back(Line);
            sort(Side0, [](const PdfLineSegmentPtr_t& _a, const PdfLineSegmentPtr_t& _b){return _a->y0() < _b->y0();});
            sort(Side1, [](const PdfLineSegmentPtr_t& _a, const PdfLineSegmentPtr_t& _b){return _a->y0() < _b->y0();});
            RealGap = std::numeric_limits<float>::max();
            for(const auto& Side0Line : Side0)
                for(const auto& Side1Line : Side1)
                    if(_op(Side0Line, Side1Line) > MIN_ITEM_SIZE) {
                        RealGap = std::min(RealGap, _p0(Side1Line) - _p1(Side0Line));
                    }
            if(_direction == enuDirection::Vertical &&
               RealGap > MIN_ITEM_SIZE &&
               RealGap < Threshold &&
               Side0.size() && Side1.size() && (
                   (
                       Side0.back()->y1() < Side1.back()->y0() &&
                       Side0.back()->isPureImage() &&
                       Side1.front()->isPureImage() == false
                       ) || (
                       Side1.back()->y1() > Side0.back()->y0() &&
                       Side1.back()->isPureImage() &&
                       Side0.front()->isPureImage() == false
                       ) || (
                       Side0.front()->y0() > Side1.back()->y1() &&
                       Side0.front()->isPureImage() &&
                       Side1.back()->isPureImage() == false
                       ) || (
                       Side1.front()->y0() > Side0.back()->y1() &&
                       Side1.front()->isPureImage() &&
                       Side0.back()->isPureImage() == false
                       )
                   ))
                RealGap += Threshold;
        }
        if(RealGap > Threshold) {
            BreakPoints.push_back(GapCenter);
            Gaps.push_back(std::make_pair(Filled.at(i - 1).second, Filled.at(i).first));
        }
    }

    auto splitAtBreakpoints = [&] (clsLayoutStorage& _parent, std::vector<float> BreakPoints) {
        _parent.Children.resize(BreakPoints.size() + 1);
        for(size_t i = 0; i < _parent.Children.size(); ++i)
            _parent.Children.at(i) = std::make_shared<clsLayoutStorage>(_configs);

        for(const auto& Line : FilteredLines) {
            size_t Index = BreakPoints.size();
            for(size_t i = 0; i < BreakPoints.size(); ++i)
                if(0.5f * (_p0(Line) + _p0(Line)) < BreakPoints.at(i)) {
                    Index = i;
                    break;
                }
            _parent.Children.at(Index)->Lines.push_back(Line);
        }
        _parent.Lines.clear();

        for(const auto& Child : _parent.Children)
            for(const auto& Line : Child->Lines)
                Child->unionWith_(Line);

        _parent.Lines.clear();
    };

    if(BreakPoints.size() == 0) {
        if(_direction == enuDirection::Horizontal)
            return false;
        float ConditionalBreakPoint = -1, BiggestGap = 0.f;
        for(size_t i = 1; i < Filled.size(); ++i) {
            float GapCenter = 0.5f * (Filled.at(i).first + Filled.at(i - 1).second);
            float Gap = Filled.at(i).first - Filled.at(i - 1).second;
            if(Gap > BiggestGap) {
                BiggestGap = Gap;
                ConditionalBreakPoint = GapCenter;
            }
        }

        if(ConditionalBreakPoint < _parent.tightTop() + 10.f || ConditionalBreakPoint > _parent.tightBottom() - 10.f)
            return false;

        clsLayoutStorage DummyLayoutStorage(_configs, _parent.Lines, _allowBackImages);
        splitAtBreakpoints(DummyLayoutStorage, { ConditionalBreakPoint });

        bool ConditionalBreakOccurred = false;
        for(const auto& Child : DummyLayoutStorage.Children)
            if(splitLayoutStorage(*Child, enuDirection::Horizontal, _configs, _allowBackImages)) {
                ConditionalBreakOccurred = true;
                break;
            }
        if(ConditionalBreakOccurred == false)
            return false;
        BreakPoints.push_back(ConditionalBreakPoint);
    }

    splitAtBreakpoints(_parent, BreakPoints);

    return true;
}

enuDirection nextDirection(enuDirection _direction) {
    return _direction == enuDirection::Vertical ?
                enuDirection::Horizontal : enuDirection::Vertical;
}

void appendToItemList(clsLayoutStorage& _parent, ColumnPtrVector_t& _items, float _contentWidth) {
    if(_parent.Children.size()) {
        for(const auto& Child : _parent.Children)
            appendToItemList(*Child, _items, _contentWidth);
    }else {
        sort(_parent.Lines, [](const PdfLineSegmentPtr_t& _a, const PdfLineSegmentPtr_t& _b) {return _a->y0() < _b->y0();});
        float MostTopImageY0 = -1, MostBottomImageY1 = -1;
        if(_parent.Lines.size() && _parent.Lines.front()->isPureImage() && _parent.Lines.front()->isHorizontalRuler() == false)
            MostTopImageY0 = _parent.Lines.front()->y0();
        if(_parent.Lines.size() && _parent.Lines.back()->isPureImage() && _parent.Lines.back()->isHorizontalRuler() == false)
            MostBottomImageY1 = _parent.Lines.back()->y1();

        float MostLeftImageX0 = -1, MostRightImageX1 = -1;
        sort(_parent.Lines, [](const PdfLineSegmentPtr_t& _a, const PdfLineSegmentPtr_t& _b) {return _a->x0() < _b->x1();});
        if(_parent.Lines.size() && _parent.Lines.front()->isPureImage() && _parent.Lines.front()->isVerticalRuler() == false)
            MostLeftImageX0 = _parent.Lines.front()->x0();
        if(_parent.Lines.size() && _parent.Lines.back()->isPureImage() && _parent.Lines.back()->isVerticalRuler() == false)
            MostRightImageX1 = _parent.Lines.back()->x1();


        auto NewBlock = std::make_shared<clsColumn>(_parent.width(), MostTopImageY0, MostBottomImageY1, MostLeftImageX0, MostRightImageX1);
        NewBlock->unionWith_(_parent);
        _items.push_back(NewBlock);
    }
}

PdfItemPtrVector_t toItemVector(const PdfLineSegmentPtrVector_t& _lines) {
    PdfItemPtrVector_t Result;
    for(const auto& Line : _lines)
        Result.push_back(Line);
    return Result;
}

PdfItemPtrVector_t toItemVector(const ColumnPtrVector_t& _cols) {
    PdfItemPtrVector_t Result;
    for(const auto& Line : _cols)
        Result.push_back(Line);
    return Result;
}

PdfBlockPtrVector_t toBlockVector(const ColumnPtrVector_t& _cols){
    PdfBlockPtrVector_t Blocks;
    for(const auto& Col : _cols) {
        Blocks.push_back(std::make_shared<clsPdfBlock>());
        Blocks.back()->setX0(Col->x0());
        Blocks.back()->setX1(Col->x1());
        Blocks.back()->setY0(Col->y0());
        Blocks.back()->setY1(Col->y1());
    }
    return Blocks;
}

bool applyXyCut(int _pageIndex, const stuConfigsPtr_t& _configs, clsLayoutStorage& _parent, enuDirection _direction, bool _allowBackImages, int _level = 0) {

    auto sendIntermediateStepSignal = [&](){
#ifdef TARGOMAN_SHOW_DEBUG
        if(_configs->MidStepDebugLevel > 1) {
            ColumnPtrVector_t Dummy;
            appendToItemList(_parent, Dummy, _parent.width());

            sigStepDone("onColumnsIdentified", _pageIndex, toBlockVector(Dummy), toItemVector(_parent.Lines));
        }
#endif
    };

    if(_parent.Children.size() > 0) {
        for(auto& Child : _parent.Children)
            applyXyCut(_pageIndex, _configs, *Child, _direction, _allowBackImages, _level + 1);
        return false;
    }

    sendIntermediateStepSignal();

    bool SplitHappend = splitLayoutStorage(_parent, _direction, _configs, _allowBackImages);

#ifdef TARGOMAN_SHOW_DEBUG
    if(_configs->MidStepDebugLevel > 1) {
        ColumnPtrVector_t Dummy;
        appendToItemList(_parent, Dummy, _parent.width());
        sigStepDone("onColumnsIdentified", _pageIndex, toBlockVector(Dummy), toItemVector(_parent.Lines));
    }
#endif

    if(SplitHappend == false) {
        _direction = nextDirection(_direction);
        SplitHappend = splitLayoutStorage(_parent, _direction, _configs, _allowBackImages);

        sendIntermediateStepSignal();
    }

    if(SplitHappend) {
        _parent.Direction = _direction;
        applyXyCut(_pageIndex, _configs, _parent, nextDirection(_direction), _allowBackImages, _level + 1);

        sendIntermediateStepSignal();
    }

    return SplitHappend;
}

void resetExpansions(ColumnPtrVector_t& _cols){
    for(const ColumnPtr_t& Col : _cols){
        Col->setX0(Col->x0() + Col->SpaceToLeft);
        Col->setX1(Col->x1() - Col->SpaceToRight);
        Col->SpaceToLeft = 0;
        Col->SpaceToRight = 0;
    }
};

bool hasChanged(const ColumnPtrVector_t& _old, const ColumnPtrVector_t& _new){
    if(_old.size() != _new.size()) return true;
    for(size_t i = 0; i< _old.size(); ++i)
        if(std::abs(_old.at(i)->x0() - _new.at(i)->x0()) > MIN_ITEM_SIZE ||
           std::abs(_old.at(i)->x1() - _new.at(i)->x1()) > MIN_ITEM_SIZE ||
           std::abs(_old.at(i)->y0() - _new.at(i)->y0()) > MIN_ITEM_SIZE ||
           std::abs(_old.at(i)->y1() - _new.at(i)->y1()) > MIN_ITEM_SIZE
           )
            return true;
    return false;
};

float colsMarginThreshold(enuDirection _direction, ColumnPtr_t _firstCol, ColumnPtr_t _secondCol){
    float MarginThreshold = DEFAULT_HORIZONTAL_MARGIN;
    if(_direction == enuDirection::Vertical){
        if(
           (_firstCol->y1() < _secondCol->y0() &&
            _firstCol->bottomIsImage() != _secondCol->topIsImage()
            ) ||
           (_firstCol->y0() > _secondCol->y1() &&
            _firstCol->topIsImage() != _secondCol->bottomIsImage()
            )
           ){
            MarginThreshold = 0;
        }else if(
                 (_firstCol->y1() < _secondCol->y0() &&
                  _firstCol->bottomIsImage() &&
                  _firstCol->bottomIsImage() == _secondCol->topIsImage()
                  ) ||
                 (_firstCol->y0() > _secondCol->y1() &&
                  _firstCol->topIsImage() &&
                  _firstCol->topIsImage() == _secondCol->bottomIsImage()
                  )
                 ){
            MarginThreshold = -15.f;
        }else
            MarginThreshold = DEFAULT_VERTICAL_MARGIN;
    } else
        MarginThreshold = DEFAULT_HORIZONTAL_MARGIN;
    return MarginThreshold;
}

std::tuple<bool, ColumnPtrVector_t> mergeOverlappedItems(int _pageIndex,
                                                         const stuPageMargins& _pageMargins,
                                                         const stuSize _pageSize,
                                                         ColumnPtrVector_t& _cols,
                                                         enuDirection _direction,
                                                         const char* _midStepSignal) {
    ColumnPtrVector_t MergeResult;
    bool ChangeOccurred = false;

    auto Cols = _cols;
    if(_direction == enuDirection::Vertical)
        resetExpansions(Cols);

    bool InnerChangeOccurred = false;
    for(size_t OverlapMergeTries = 0; OverlapMergeTries < MAX_MERGE_TRIES; ++OverlapMergeTries) {
        if(OverlapMergeTries > 1) debugShow("OverlapMergeTries: "<<OverlapMergeTries);
        InnerChangeOccurred = false;

        sort(Cols, [](const ColumnPtr_t& _a, const ColumnPtr_t& _b){ return  _a->y0() < _b->y0();});

        std::set<clsColumn*> Used;
        MergeResult.clear();

        for(const auto& Col : Cols) {
            if(Used.find(Col.get()) != Used.end()) continue;
            for(const auto& OtherCol : Cols){
                if(OtherCol.get() == Col.get())
                    continue;

                auto MarginThreshold = colsMarginThreshold(_direction, Col, OtherCol);

                if(
                   (
                       _direction == enuDirection::Vertical ?
                       OtherCol->horzOverlap(Col) > std::min(.01f * Col->width(), .01f * OtherCol->width()) :
                       OtherCol->vertOverlap(Col) > std::min(.01f * Col->height(), .01f * OtherCol->height())
                       ) && (
                       _direction == enuDirection::Vertical ?
                       OtherCol->vertOverlap(Col) > MarginThreshold :
                       OtherCol->horzOverlap(Col) > MarginThreshold
                       )
                   ){
                    auto MergeBound = OtherCol->unionWith(Col);
                    int a = 0;
                    if(_direction == enuDirection::Vertical
                       && anyOf<ColumnPtr_t>(Cols, [&](const ColumnPtr_t& _col){
                                             return (
                                                 (_col->IsMerged == false
                                                  && _col->width() > .5f * (_pageSize.Width - _pageMargins.Left - _pageMargins.Right)
                                                  ) || (
                                                     OtherCol->IsMerged == false
                                                     && OtherCol->width() > .5f * (_pageSize.Width - _pageMargins.Left - _pageMargins.Right)
                                                     ) || (
                                                     Col->IsMerged == false
                                                     && Col->width() > .5f * (_pageSize.Width - _pageMargins.Left - _pageMargins.Right)
                                                     )
                                                 )
                                             && MergeBound.doesTouch(_col)
                                             && MergeBound.doesContain(_col, DEFAULT_VERTICAL_MARGIN) == false
                                             ; }) == false)
                    {
                        if(Used.find(OtherCol.get()) != Used.end())
                            OtherCol->mergeWith_(Col, false);
                        else {
                            Col->mergeWith_(OtherCol, false);
                            if(Used.find(Col.get()) == Used.end())
                                MergeResult.push_back(Col);
                        }

                        Used.insert(Col.get());
                        Used.insert(OtherCol.get());
                        Col->IsMerged = true;
                        OtherCol->IsMerged = true;
                        InnerChangeOccurred = true;
                        if(_midStepSignal) sigStepDone(_midStepSignal,_pageIndex, {} ,toItemVector(MergeResult));
                        ChangeOccurred = true;
                    }
                }
            }
            if(Used.find(Col.get()) == Used.end())
                MergeResult.push_back(Col);
        }
        if(_midStepSignal) sigStepDone(_midStepSignal,_pageIndex, {} ,toItemVector(MergeResult));

        if(InnerChangeOccurred == false)
            break;
        Cols = MergeResult;
    }
    return std::make_tuple(ChangeOccurred, MergeResult);
};

void mergeCols(int _pageIndex, const stuPageMargins& _pageMargins, const stuSize &_pageSize, ColumnPtrVector_t& _cols, float _contentWidth, bool _ignoreVerticalSpace, const char* _midLevelStep) {
    ColumnPtrVector_t ApprovedColumns = _cols;
    ColumnPtrVector_t MergeResult;
    ColumnPtrVector_t RemainingColumns;
    PdfItemPtrVector_t SmallColumns;
    bool ChangeOccurred = false;

    /************************************************************************/
    /************************************************************************/
    /************************************************************************/
    /************************************************************************/
    auto expandMargins = [&](ColumnPtrVector_t& _cols, bool _excludeLines) {
        for(size_t ExpansionTries = 0; ExpansionTries < MAX_MERGE_TRIES; ++ExpansionTries) {
            if(ExpansionTries > 1) debugShow("ExpansionTry: "<<ExpansionTries);
            ChangeOccurred = false;
            for(auto& Col : _cols) {
                if(_excludeLines && Col->isHorizontalRuler())
                    continue;
                //float X0LimitAbove = 0, X0LimitBelow = 0;
                //float X1LimitAbove = std::numeric_limits<float>::max(), X1LimitBelow = std::numeric_limits<float>::max();
                float LeftNeighbourLimit =0;
                float RightNeighbourLimit = std::numeric_limits<float>::max();
                ColumnPtr_t NearestAboveLeft, NearestAboveRight, NearestBelowLeft, NearestBelowRight;
                ColumnPtr_t NearestLineAbove, NearestLineBelow;

                for(auto& OtherCol : _cols) {
                    if(OtherCol.get() == Col.get())
                        continue;
                    if(_excludeLines && OtherCol->isHorizontalRuler())
                        continue;
                    if(OtherCol->vertOverlap(Col) >= MIN_ITEM_SIZE) {
                        if(OtherCol->x1() < Col->x0())
                            LeftNeighbourLimit = std::max(OtherCol->x1(), LeftNeighbourLimit);
                        if(OtherCol->x0() > Col->x1())
                            RightNeighbourLimit = std::min(OtherCol->x0(), RightNeighbourLimit);
                    } else if(OtherCol->horzOverlap(Col) > 2.f) {
                        if(OtherCol->x0() - 2.f < Col->x0()) {
                            if(OtherCol->y0() < Col->centerY()) {
                                if(NearestAboveLeft.get() == nullptr || NearestAboveLeft->y1() < OtherCol->y1()){
                                    NearestAboveLeft = OtherCol;
                                }
                            } else if(OtherCol->y1() > Col->centerY()) {
                                if(NearestBelowLeft.get() == nullptr || NearestBelowLeft->y0() > OtherCol->y0()) {
                                    NearestBelowLeft = OtherCol;
                                }
                            }
                        }
                        if(OtherCol->x1() + 2.f > Col->x1()){
                            if(OtherCol->y0() < Col->centerY()) {
                                if(NearestAboveRight.get() == nullptr || NearestAboveRight->y1() < OtherCol->y1()) {
                                    NearestAboveRight = OtherCol;
                                }
                            } else if(OtherCol->y1() > Col->centerY()) {
                                if(NearestBelowRight.get() == nullptr || NearestBelowRight->y0() > OtherCol->y0()) {
                                    NearestBelowRight = OtherCol;
                                }
                            }
                        }
                    }
                }

                auto D2LAbove = NearestAboveLeft.get() ? Col->vertOverlap(NearestAboveLeft) : std::numeric_limits<float>::max();
                auto D2LBelow = NearestBelowLeft.get() ? Col->vertOverlap(NearestBelowLeft) : std::numeric_limits<float>::max();
                float BestX0 = Col->x0();

                if(NearestBelowLeft.get() == nullptr) {
                    if(NearestAboveLeft.get() != nullptr)
                        if(NearestAboveLeft->x0() > LeftNeighbourLimit - .5f)
                            BestX0 = NearestAboveLeft->x0();
                }else if(NearestAboveLeft.get() == nullptr){
                    if(NearestBelowLeft->x0() > LeftNeighbourLimit - .5f)
                        BestX0 = NearestBelowLeft->x0();
                }else{
                    if(NearestAboveLeft->x0() > LeftNeighbourLimit - .5f
                       && NearestBelowLeft->x0() > LeftNeighbourLimit - .5f
                       ){
                        if ((D2LAbove >= DEFAULT_VERTICAL_MARGIN && D2LBelow >= DEFAULT_VERTICAL_MARGIN)
                            ||(D2LAbove < DEFAULT_VERTICAL_MARGIN && D2LBelow < DEFAULT_VERTICAL_MARGIN)
                            ){
                            BestX0 = std::min(NearestBelowLeft->x0(), NearestAboveLeft->x0());
                        } else if (D2LAbove >= DEFAULT_VERTICAL_MARGIN)
                            BestX0 = NearestAboveLeft->x0();
                        else
                            BestX0 = NearestBelowLeft->x0();
                    }else if (NearestAboveLeft->x0() > LeftNeighbourLimit - .5f)
                        BestX0 = NearestAboveLeft->x0();
                    else if (NearestBelowLeft->x0() > LeftNeighbourLimit - .5f) {
                        BestX0 = NearestBelowLeft->x0();
                    }
                }

                if(BestX0 < Col->x0()){
                    Col->expandToLeft(BestX0);
                    ChangeOccurred = true;
                }

                auto D2RAbove = NearestAboveRight.get() ? Col->vertOverlap(NearestAboveRight) : std::numeric_limits<float>::max();
                auto D2RBelow = NearestBelowRight.get() ? Col->vertOverlap(NearestBelowRight) : std::numeric_limits<float>::max();
                float BestX1 = Col->x1();

                if(NearestBelowRight.get() == nullptr){
                    if(NearestAboveRight.get() != nullptr)
                        if(NearestAboveRight->x1() < RightNeighbourLimit + .5f)
                            BestX1 = NearestAboveRight->x1();
                }else if(NearestAboveRight.get() == nullptr){
                    if(NearestBelowRight->x1() < RightNeighbourLimit + .5f)
                        BestX1 = NearestBelowRight->x1();
                }else {
                    if(NearestAboveRight->x1() < RightNeighbourLimit + .5f
                       && NearestBelowRight->x1() < RightNeighbourLimit + .5f
                       ){
                        if ((D2RAbove >= DEFAULT_VERTICAL_MARGIN && D2RBelow >= DEFAULT_VERTICAL_MARGIN)
                            ||(D2RAbove < DEFAULT_VERTICAL_MARGIN && D2RBelow < DEFAULT_VERTICAL_MARGIN)
                            ){
                            BestX1 = std::max(NearestBelowRight->x1(), NearestAboveRight->x1());
                        } else if (D2RAbove >= DEFAULT_VERTICAL_MARGIN)
                            BestX1 = NearestAboveRight->x1();
                        else
                            BestX1 = NearestBelowRight->x1();
                    }else if (NearestAboveRight->x1() < RightNeighbourLimit + .5f)
                        BestX1 = NearestAboveRight->x1();
                    else if (NearestBelowRight->x1() < RightNeighbourLimit + .5f) {
                        BestX1 = NearestBelowRight->x1();
                    }
                }

                if(BestX1 > Col->x1()){
                    Col->expandToRight(BestX1);
                    ChangeOccurred = true;
                }
            }

            if(_midLevelStep) sigStepDone(_midLevelStep,_pageIndex, _cols);

            if(ChangeOccurred == false)
                break;
        }
    };

    /************************************************************************/
    /************************************************************************/
    /************************************************************************/
    /************************************************************************/
    auto mergeColsInsideOther = [](ColumnPtrVector_t& _sourceCols, const ColumnPtrVector_t& _comparingCols){
        ColumnPtrVector_t Result;
        std::set<clsColumn*> Used;
        for(const auto& Col : _sourceCols){
            bool IsInsideAnother = false;
            for (const auto& OtherCol : _comparingCols){
                if(Col.get() == OtherCol.get())
                    continue;
                if(OtherCol->doesContain(Col)) {
                    IsInsideAnother = true;
                    break;
                }
            }
            if(IsInsideAnother == false)
                Result.push_back(Col);
        }
        _sourceCols = Result;
    };

    /************************************************************************/
    /************************************************************************/
    /************************************************************************/
    /************************************************************************/
    auto mergeSameExpandedWidthColumns = [&](ColumnPtrVector_t& _cols){
        std::set<clsColumn*> Used;
        MergeResult.clear();
        RemainingColumns.clear();
        ChangeOccurred = false;

        if(_midLevelStep){
            SmallColumns.clear();
            for(const auto& Col : _cols)
                SmallColumns.push_back(Col);
            if(_midLevelStep) sigStepDone(_midLevelStep,_pageIndex, toBlockVector(MergeResult), SmallColumns);
        }

        sort(_cols, [](const ColumnPtr_t& _a, const ColumnPtr_t& _b){return _a->width() < _b->width();});

        for(const auto& Col : _cols) {
            if(Used.find(Col.get()) != Used.end() || Col->isHorizontalRuler())
                continue;

            if(Col->width() > minAcceptableColWidth(_contentWidth)) {
                auto SameColItems = filter<ColumnPtr_t>(_cols, [&] (const ColumnPtr_t& _col) {
                    return  (
                                Used.find(_col.get()) == Used.end()
                                && (std::abs(_col->x0() - Col->x0()) < 2.f ||
                                    std::abs(_col->x0() - Col->x0() + Col->SpaceToLeft) < 2.f ||
                                    std::abs(_col->x0() + _col->SpaceToLeft - Col->x0()) < 2.f ||
                                    std::abs(_col->x0() + _col->SpaceToLeft - Col->x0() + Col->SpaceToLeft) < 2.f
                                    ) &&
                                (std::abs(_col->x1() - Col->x1()) < 2.f ||
                                 std::abs(_col->x1() - Col->x1() - Col->SpaceToRight) < 2.f ||
                                 std::abs(_col->x1() - _col->SpaceToRight - Col->x1()) < 2.f ||
                                 std::abs(_col->x1() - _col->SpaceToRight - Col->x1() - Col->SpaceToRight) < 2.f
                                 )
                                );
                });
                sort(SameColItems, [] (const ColumnPtr_t& _a, const ColumnPtr_t& _b) { return _a->y0() < _b->y0(); });
                ColumnPtr_t NewSegment = SameColItems.at(0)->makeCopy();

                std::vector<clsColumn*> ToBeUsedCols;
                ToBeUsedCols.push_back(SameColItems.at(0).get());

                auto insertNewSegmentIntoResults = [&](){
                    if(NewSegment->contentWidth() > minAcceptableColWidth(_contentWidth)){
                        if(NewSegment->isHorizontalRuler())
                            return;
                        MergeResult.push_back(NewSegment);
                        for(const auto Col : ToBeUsedCols){
                            if(Col->isHorizontalRuler())
                                continue;
                            Used.insert(Col);
                        }
                    }
                };

                std::set<clsColumn*> SameColItemsDic;
                for(const auto& Col : SameColItems)
                    SameColItemsDic.insert(Col.get());

                for(size_t i = 1; i < SameColItems.size(); ++i) {
                    bool Break = false;

                    if(_ignoreVerticalSpace == false)  {
                        auto MarginThreshold = colsMarginThreshold(enuDirection::Vertical, NewSegment, SameColItems.at(i));
                        if(NewSegment->vertOverlap(SameColItems.at(i)) < MarginThreshold)
                            Break = true;
                    }
                    ColumnPtrVector_t Interferences;
                    if(Break == false) {
                        auto Spacer = NewSegment->vertSpacer(SameColItems.at(i));
                        Spacer.setX0(NewSegment->x0());
                        Spacer.setX1(NewSegment->x1());
                        Interferences = filter<ColumnPtr_t>(_cols, [&] (const ColumnPtr_t& _col) {
                            return SameColItemsDic.find(_col.get()) == SameColItemsDic.end() &&
                                    _col->doesTouch(Spacer);
                        });

                        for(const auto& Interference : Interferences) {
                            if(NewSegment->width() < 0.76f * _contentWidth) {
                                if(
                                   (Interference->x0() + Interference->SpaceToLeft  < Spacer.x0() - 2.f &&
                                    Interference->x1() - Interference->SpaceToRight > Spacer.x0() - 2.f
                                    ) ||
                                   (Interference->x0() + Interference->SpaceToLeft  < Spacer.x1() - 2.f &&
                                    Interference->x1() - Interference->SpaceToRight > Spacer.x1() - 2.f
                                    )) {
                                    Break = true;
                                    break;
                                }
                            } else {
                                if(Interference->contentWidth() > minAcceptableColWidth(_contentWidth)) {
                                    Break = true;
                                    break;
                                }
                            }
                        }
                    }


                    if(Break) {
                        insertNewSegmentIntoResults();
                        NewSegment = SameColItems.at(i);
                        ToBeUsedCols.clear();
                        ToBeUsedCols.push_back(SameColItems.at(i).get());
                    } else {
                        SameColItems.at(i)->setX0(NewSegment->x0());
                        SameColItems.at(i)->setX1(NewSegment->x1());
                        NewSegment->mergeWith_(SameColItems.at(i), false);
                        ToBeUsedCols.push_back(SameColItems.at(i).get());
                        for(const auto& Item : Interferences)
                            ToBeUsedCols.push_back(Item.get());
                    }
                }
                insertNewSegmentIntoResults();
                if(_midLevelStep) sigStepDone(_midLevelStep,_pageIndex, toBlockVector(MergeResult), SmallColumns);
                continue;
            }
        }

        for(const auto& Col : _cols)
            if(Used.find(Col.get()) == Used.end())
                RemainingColumns.push_back(Col);


        mergeColsInsideOther(MergeResult, MergeResult);
        if(_midLevelStep) sigStepDone(_midLevelStep,_pageIndex, toBlockVector(MergeResult), SmallColumns);
        mergeColsInsideOther(RemainingColumns, MergeResult);
        if(_midLevelStep) sigStepDone(_midLevelStep,_pageIndex, toBlockVector(MergeResult), SmallColumns);

        SmallColumns.clear();
        for(const auto& Col : RemainingColumns)
            SmallColumns.push_back(Col);

        if(_midLevelStep) sigStepDone(_midLevelStep,_pageIndex, toBlockVector(MergeResult), SmallColumns);
        _cols = MergeResult;
        return;
    };

    /************************************************************************/
    /************************************************************************/
    /************************************************************************/
    /************************************************************************/
    auto hasCollisionWithOtherCols = [&](const ColumnPtr_t& _toBeMergedCol, const ColumnPtr_t& _mergeCandidateCol, bool _tight){
        clsPdfItem Merged(
                    std::min(_mergeCandidateCol->x0(), _toBeMergedCol->x0() + (_tight ? _toBeMergedCol->SpaceToLeft : 0.f)),
                    std::min(_mergeCandidateCol->y0(), _toBeMergedCol->y0()),
                    std::max(_mergeCandidateCol->x1(), _toBeMergedCol->x1() - (_tight ? _toBeMergedCol->SpaceToRight : 0.f)),
                    std::max(_mergeCandidateCol->y1(), _toBeMergedCol->y1()),
                    enuPdfItemType::None
                    );

        for (const auto& CheckingCol : ApprovedColumns) {
            if (CheckingCol.get() == _mergeCandidateCol.get() || CheckingCol.get() == _toBeMergedCol.get())
                continue;
            if(Merged.doesTouch(CheckingCol))
                return true;
        }

        for (const auto& CheckingCol : RemainingColumns) {
            if (CheckingCol.get() == _mergeCandidateCol.get() || CheckingCol.get() == _toBeMergedCol.get())
                continue;
            if(Merged.doesTouch(CheckingCol))
                return true;
        }

        return false;
    };

    bool MainSmallColsChangeOccurred = false;

    /************************************************************************/
    /************************************************************************/
    /************************************************************************/
    /************************************************************************/
    auto mergeAdjacentColumns = [&](ColumnPtrVector_t& _toBeMergedCols,
            const ColumnPtrVector_t& _mergeableCols,
            enuDirection _direction,
            std::function<bool(const ColumnPtr_t&)> _isAcceptableCol = [](const ColumnPtr_t&){ return  true;},
            bool _updateSmallCols = true)
    {
        for(size_t AdjacentMergeTries = 0; AdjacentMergeTries < MAX_MERGE_TRIES; ++AdjacentMergeTries) {
            bool AdjacentColsMergeChangeOccurred = false;
            if(AdjacentMergeTries > 1) debugShow("AdjacentMergeTries: "<<AdjacentMergeTries);

            std::set<clsColumn*> Used;
            if(_updateSmallCols) SmallColumns.clear();
            for (const auto& ToBeMergedCol : _toBeMergedCols) {
                if(_isAcceptableCol(ToBeMergedCol) == false)
                    continue;
                ColumnPtr_t NearestMergeableNeighbourBefore, NearestMergeableNeighbourAfter;

                bool MergeTight = false;
                for (const auto& MergeCandidateCol : _mergeableCols) {
                    if(MergeCandidateCol.get() == ToBeMergedCol.get() || Used.find(MergeCandidateCol.get()) != Used.end())
                        continue;
                    if((_direction == enuDirection::Horizontal ? ToBeMergedCol->vertOverlap(MergeCandidateCol) : ToBeMergedCol->horzOverlap(MergeCandidateCol)) > MIN_ITEM_SIZE) {
                        if(hasCollisionWithOtherCols(ToBeMergedCol, MergeCandidateCol, false)) {
                            if(_direction == enuDirection::Horizontal || hasCollisionWithOtherCols(ToBeMergedCol, MergeCandidateCol, true)){
                                if(_updateSmallCols) SmallColumns.push_back(ToBeMergedCol);
                                continue;
                            }
                            MergeTight = true;
                        }

                        if(_direction == enuDirection::Horizontal ? (MergeCandidateCol->x1() < ToBeMergedCol->x0()) : (MergeCandidateCol->y1() < ToBeMergedCol->y0()) && (
                               NearestMergeableNeighbourBefore.get() == nullptr || (
                                   _direction == enuDirection::Horizontal ?
                                   ToBeMergedCol->horzOverlap(MergeCandidateCol) > ToBeMergedCol->horzOverlap(NearestMergeableNeighbourBefore) :
                                   ToBeMergedCol->vertOverlap(MergeCandidateCol) > ToBeMergedCol->vertOverlap(NearestMergeableNeighbourBefore)
                                   )
                               )
                           )
                            NearestMergeableNeighbourBefore = MergeCandidateCol;

                        if(_direction == enuDirection::Horizontal ? (MergeCandidateCol->x0() > ToBeMergedCol->x1()) : (MergeCandidateCol->y0() > ToBeMergedCol->y1()) && (
                               NearestMergeableNeighbourAfter.get() == nullptr || (
                                   _direction == enuDirection::Horizontal ?
                                   ToBeMergedCol->horzOverlap(MergeCandidateCol) > ToBeMergedCol->horzOverlap(NearestMergeableNeighbourAfter) :
                                   ToBeMergedCol->vertOverlap(MergeCandidateCol) > ToBeMergedCol->vertOverlap(NearestMergeableNeighbourAfter)
                                   )
                               )
                           )
                            NearestMergeableNeighbourAfter = MergeCandidateCol;
                    }
                }

                if(NearestMergeableNeighbourBefore.get() || NearestMergeableNeighbourAfter.get()) {
                    auto findNearestSupports = [&](const ColumnPtr_t& _col, ColumnPtr_t& _before, ColumnPtr_t& _after){
                        for (const auto& SupportCandidate : ApprovedColumns) {
                            if(SupportCandidate.get() == _col.get())
                                continue;
                            if(_direction == enuDirection::Horizontal ?
                               _col->x0() + (-1 * DEFAULT_HORIZONTAL_MARGIN) - 1.f > SupportCandidate->x0() && _col->x1() - (-1 * DEFAULT_HORIZONTAL_MARGIN) + 1.f < SupportCandidate->x1() :
                               _col->y0() + (-1 * DEFAULT_VERTICAL_MARGIN)   - 1.f > SupportCandidate->y0() && _col->y1() - (-1 * DEFAULT_VERTICAL_MARGIN)   + 1.f < SupportCandidate->y1()
                               ){
                                if(_direction == enuDirection::Horizontal ?
                                   _col->y0() > SupportCandidate->y1() :
                                   _col->x0() > SupportCandidate->x1()){
                                    if(_before.get() == nullptr ||
                                       (_direction == enuDirection::Horizontal ?
                                        _col->vertOverlap(SupportCandidate) > _col->vertOverlap(_before) :
                                        _col->horzOverlap(SupportCandidate) > _col->horzOverlap(_before)
                                        )){
                                        _before = SupportCandidate;
                                    }
                                } else {
                                    if(_after.get() == nullptr ||
                                       (_direction == enuDirection::Horizontal ?
                                        _col->vertOverlap(SupportCandidate) > _col->vertOverlap(_after) :
                                        _col->horzOverlap(SupportCandidate) > _col->horzOverlap(_after)
                                        )){
                                        _after = SupportCandidate;
                                    }
                                }
                            }
                        }
                    };

                    ColumnPtr_t TobeMergedSupportBefore, TobeMergedSupportAfter;
                    ColumnPtr_t AcceptedMergeCol;
                    findNearestSupports(ToBeMergedCol, TobeMergedSupportBefore, TobeMergedSupportAfter);
                    if(NearestMergeableNeighbourBefore.get()) {
                        ColumnPtr_t NeighbourSupportBefore, NeighbourSupportAfter;
                        findNearestSupports(NearestMergeableNeighbourBefore, NeighbourSupportBefore, NeighbourSupportAfter);

                        if((TobeMergedSupportBefore.get() && NeighbourSupportBefore.get() == TobeMergedSupportBefore.get()) ||
                           (TobeMergedSupportAfter.get()  && NeighbourSupportAfter.get()  == TobeMergedSupportAfter.get()) ||
                           (NearestMergeableNeighbourBefore->width() + ToBeMergedCol->width() > _contentWidth - minAcceptableColWidth(_contentWidth)) ||
                           (TobeMergedSupportBefore.get() == TobeMergedSupportAfter.get() &&
                            NeighbourSupportBefore.get() == NeighbourSupportAfter.get() &&
                            TobeMergedSupportBefore == nullptr && NeighbourSupportBefore == nullptr
                            )
                           ){
                            if(NearestMergeableNeighbourAfter.get() == nullptr || (
                                   _direction == enuDirection::Horizontal ?
                                   NearestMergeableNeighbourAfter->horzOverlap(ToBeMergedCol) < NearestMergeableNeighbourBefore->horzOverlap(ToBeMergedCol) :
                                   NearestMergeableNeighbourAfter->vertOverlap(ToBeMergedCol) < NearestMergeableNeighbourBefore->vertOverlap(ToBeMergedCol))
                               ){
                                AcceptedMergeCol = NearestMergeableNeighbourBefore;
                            }
                        }
                    }

                    if(AcceptedMergeCol.get() == nullptr && NearestMergeableNeighbourAfter.get()) {
                        ColumnPtr_t NeighbourSupportBefore, NeighbourSupportAfter;
                        findNearestSupports(NearestMergeableNeighbourAfter, NeighbourSupportBefore, NeighbourSupportAfter);

                        if((TobeMergedSupportBefore.get() && NeighbourSupportBefore.get() == TobeMergedSupportBefore.get()) ||
                           (TobeMergedSupportAfter.get()  && NeighbourSupportAfter.get() == TobeMergedSupportAfter.get()) ||
                           (NearestMergeableNeighbourAfter->width() + ToBeMergedCol->width() > _contentWidth - minAcceptableColWidth(_contentWidth)) ||
                           (TobeMergedSupportBefore.get() == TobeMergedSupportAfter.get() &&
                            NeighbourSupportBefore.get() == NeighbourSupportAfter.get() &&
                            TobeMergedSupportBefore == nullptr && NeighbourSupportBefore == nullptr
                            )
                           )
                            AcceptedMergeCol = NearestMergeableNeighbourAfter;
                    }


                    if(AcceptedMergeCol.get()){
                        AcceptedMergeCol->mergeWith_(ToBeMergedCol, MergeTight);
                        Used.insert(ToBeMergedCol.get());
                        AdjacentColsMergeChangeOccurred = true;
                        MainSmallColsChangeOccurred = true;
                        ChangeOccurred = true;
                    }else if(_updateSmallCols)
                        SmallColumns.push_back(ToBeMergedCol);
                }
                if(_midLevelStep) sigStepDone(_midLevelStep,_pageIndex, toBlockVector(ApprovedColumns), SmallColumns);
                continue;
            }
            _toBeMergedCols = filter<ColumnPtr_t>(_toBeMergedCols, [&](const ColumnPtr_t& _col){return Used.find(_col.get()) == Used.end();});
            if(_updateSmallCols)
                for(const auto& Col : _toBeMergedCols)
                    SmallColumns.push_back(Col);

            if(_midLevelStep) sigStepDone(_midLevelStep,_pageIndex, toBlockVector(ApprovedColumns), SmallColumns);

            if(AdjacentColsMergeChangeOccurred == false)
                break;
        }

        if(_midLevelStep) sigStepDone(_midLevelStep,_pageIndex, toBlockVector(ApprovedColumns), SmallColumns);
    };

    /************************************************************************/
    /************************************************************************/
    /************************************************************************/
    /************************************************************************/
    for(size_t MergeTries = 0; MergeTries < MAX_MERGE_TRIES; ++MergeTries) {
        if(MergeTries > 1) debugShow("MergeTry: "<<MergeTries);
        /// Step 1
        std::tie(ChangeOccurred, ApprovedColumns) = mergeOverlappedItems(_pageIndex, _pageMargins, _pageSize, ApprovedColumns, enuDirection::Vertical, _midLevelStep);
        std::tie(ChangeOccurred, ApprovedColumns) = mergeOverlappedItems(_pageIndex, _pageMargins, _pageSize, ApprovedColumns, enuDirection::Horizontal, _midLevelStep);

        /// break if fixed
        if(ChangeOccurred == false)
            break;
    }

    ColumnPtrVector_t OldApprovedCols;
    for(size_t MainTries = 0; MainTries < MAX_MERGE_TRIES; ++MainTries) {
        if(MainTries > 1) debugShow("MainTry: "<<MainTries);
        ChangeOccurred = false;
        // ////////////////////////////////////////////////////
        // Iteratively merge and create big columns
        // ////////////////////////////////////////////////////

        for(size_t MergeTries = 0; MergeTries < MAX_MERGE_TRIES; ++MergeTries) {
            if(MergeTries > 1) debugShow("MergeTry: "<<MergeTries);


            /// Step 2
            sort(ApprovedColumns, [](const ColumnPtr_t& _a, const ColumnPtr_t& _b){ return  _a->width() > _b->width() || _a->x0() < _b->x0();});
            expandMargins(ApprovedColumns, false);
            expandMargins(ApprovedColumns, true);

            /// Step 3
            mergeSameExpandedWidthColumns(ApprovedColumns);

            /// break if fixed
            if(hasChanged(OldApprovedCols, ApprovedColumns) == false)
                break;

            OldApprovedCols = ApprovedColumns;

            for(const auto& Col : RemainingColumns)
                ApprovedColumns.push_back(Col);

            RemainingColumns.clear();
        }

        for(size_t MainSmallColsMergeTries = 0; MainSmallColsMergeTries < MAX_MERGE_TRIES; ++MainSmallColsMergeTries) {
            MainSmallColsChangeOccurred = false;
            if(MainSmallColsMergeTries > 1) debugShow("MainSmallColsMergeTries: "<<MainSmallColsMergeTries);

            /// Step 1: Merge small columns with approved columns horizontally
            mergeAdjacentColumns(RemainingColumns, ApprovedColumns, enuDirection::Horizontal);

            /// Step 2: Merge columns with small inner columns horizontally
            mergeAdjacentColumns(ApprovedColumns,
                                 ApprovedColumns,
                                 enuDirection::Horizontal,
                                 [&](const ColumnPtr_t& _col){
                return  _col->MergedColCount > 3 ||
                        (_col->MergedColCount > 2 && (
                             _col->MinMergedColWidth < minAcceptableColWidth(_contentWidth) ||
                             (_col->SumMergedColsWidth / _col->MergedColCount) < minAcceptableColWidth(_contentWidth)));},
            false
            );

            /// Step 3: Merge remaining small columns with each other horizontally
            mergeAdjacentColumns(RemainingColumns, RemainingColumns, enuDirection::Horizontal);

            /// Step 4: Merge remaining small columns with each other vertically
            mergeAdjacentColumns(RemainingColumns, RemainingColumns, enuDirection::Vertical);

            /// Step 5: Merge remaining small columns with approved vertically
            // mergeAdjacentColumns(RemainingColumns, ApprovedColumns, enuDirection::Vertical);

            if(MainSmallColsChangeOccurred == false)
                break;
        }


        // ////////////////////////////////////////////////////
        if(ChangeOccurred == false)
            break;

        for(const auto& Col : RemainingColumns)
            ApprovedColumns.push_back(Col);

        RemainingColumns.clear();
    }

    // ////////////////////////////////////////////////////
    // convert remaining small columns to separate columns
    // ////////////////////////////////////////////////////
    for(const auto& Col : RemainingColumns)
        ApprovedColumns.push_back(Col);

    _cols = ApprovedColumns;
}

clsLayoutStorage::clsLayoutStorage(const stuConfigsPtr_t &_configs, const PdfLineSegmentPtrVector_t &_lines, bool _allowBackgrounds) :
    Configs(_configs),
    AllowBackgroundImages(_allowBackgrounds)
{
    this->Lines = _allowBackgrounds ?
                      _lines :
                      filter<PdfLineSegmentPtr_t>(_lines, [](const PdfLineSegmentPtr_t& _line){return _line->type() != enuPdfItemType::Background;});
    for(const auto& Line : this->Lines)
        this->unionWith_(Line);
}

clsLayoutStorage::~clsLayoutStorage()
{ }

ColumnPtrVector_t clsLayoutStorage::xyCutColumn(int _pageIndex){
    ColumnPtrVector_t InnerColumns;
    applyXyCut(_pageIndex, this->Configs, *this, enuDirection::Horizontal, this->AllowBackgroundImages);
    appendToItemList(*this, InnerColumns, this->width());
    return InnerColumns;
}

void clsLayoutStorage::updateChildrenByLayout(int _pageIndex, const ColumnPtrVector_t& _columns, const char* _stepName)
{
    std::vector<LayoutChildren_t> NewChildrenCombination;
    std::set<clsPdfItem*> Used;
    PdfItemPtrVector_t Dummy;

    for(const auto& Col : _columns){
        NewChildrenCombination.push_back(std::make_shared<clsLayoutStorage>(this->Configs));
        auto LastChildren = NewChildrenCombination.back();

        LastChildren->Space2Left = Col->SpaceToLeft;
        LastChildren->Space2Right = Col->SpaceToRight;
        LastChildren->setX0(Col->x0());
        LastChildren->setX1(Col->x1());
        LastChildren->setY0(Col->y0());
        LastChildren->setY1(Col->y1());
        LastChildren->InnerColCount = static_cast<uint8_t>(Col->MergedColCount);

        std::function<void(const LayoutChildren_t&)> addChildrenLines = [&](const LayoutChildren_t& _child){
            for(const auto& Child : _child->Children)
                addChildrenLines(Child);
            if(_child->x0() + 1.f > Col->x0() &&
               _child->x1() - 1.f < Col->x1() &&
               _child->y0() + 1.f > Col->y0() &&
               _child->y1() - 1.f < Col->y1()) {
                for(const auto& Line : _child->Lines){
                    if(_stepName) Dummy.push_back(Line);
                    LastChildren->Lines.push_back(Line);
                }
                Used.insert(_child.get());
            }
        };

        for(const auto& Child : this->Children)
            addChildrenLines(Child);
    }
    this->Children = NewChildrenCombination;
    if(_stepName) sigStepDone(_stepName, _pageIndex, toBlockVector(_columns), Dummy);
}

void clsLayoutStorage::identifyMainColumns(int _pageIndex, const stuPageMargins& _pageMargins, const stuSize &_pageSize)
{
    auto MainColumns = this->xyCutColumn(_pageIndex);
#ifdef TARGOMAN_SHOW_DEBUG
    if(this->Configs->MidStepDebugLevel > 0) sigStepDone("onColumnsIdentified", _pageIndex, MainColumns);
    mergeCols(_pageIndex, _pageMargins, _pageSize, MainColumns, this->width(), true, this->Configs->MidStepDebugLevel > 0 ? "onColumnsIdentified" : nullptr);
#else
    mergeCols(_pageIndex, _pageMargins, _pageSize, MainColumns, this->width(), true, nullptr);
#endif
    this->updateChildrenByLayout(_pageIndex, MainColumns, "onColumnsIdentified");
}

void clsLayoutStorage::identifyInnerColumns(int _pageIndex, const stuPageMargins& _pageMargins, const stuSize &_pageSize, const stuConfigsPtr_t& _configs)
{
    auto MainColumns = this->xyCutColumn(_pageIndex);
#ifdef TARGOMAN_SHOW_DEBUG
    if(this->Configs->MidStepDebugLevel > 0) sigStepDone("onInnerBlocks", _pageIndex, MainColumns);
    mergeCols(_pageIndex, _pageMargins, _pageSize, MainColumns, _pageSize.Width - _pageMargins.Left - _pageMargins.Right, false, this->Configs->MidStepDebugLevel > 0 ? "onInnerBlocks" : nullptr);
    this->updateChildrenByLayout(_pageIndex, MainColumns, _configs->MidStepDebugLevel > 0 ? "onInnerBlocks" : nullptr);
#else
    mergeCols(_pageIndex, _pageMargins, _pageSize, MainColumns, _pageSize.Width - _pageMargins.Left - _pageMargins.Right, false, nullptr);
    this->updateChildrenByLayout(_pageIndex, MainColumns, nullptr);
#endif
}

void clsLayoutStorage::appendLine(const PdfLineSegmentPtr_t &_line)
{
    this->Lines.push_back(_line);
    this->unionWith_(_line);
}


}
}
}
