#include "SubBlockMarking.h"
#include "BulletAndNumbering.h"
#include "../ParagraphAndSentenceManager.h"

namespace Targoman {
namespace PDF {
namespace Private {
namespace LA {


PdfBlockPtrVector_t splitBlockBasedOnJustification(const PdfBlockPtr_t &_block)
{
    PdfBlockPtrVector_t SplittedBlocks;

    assert(_block->innerBlocks().size() == 0);

    _block->estimateParagraphParams();

    auto Justification = _block->justification();
    PdfBlockPtr_t CurrentBlock = std::make_shared<clsPdfBlock>(_block->location(), _block->contentType(), Justification);

    enuLineParagraphRelation Relation = enuLineParagraphRelation::None;
    auto storeCurrentBlock = [&] () {
        if(CurrentBlock->lines().size() > 0) {
            CurrentBlock->setCertainlyFinished(Relation == enuLineParagraphRelation::LastLineOf);
            SplittedBlocks.push_back(CurrentBlock);
            CurrentBlock = std::make_shared<clsPdfBlock>(_block->location(), _block->contentType(), Justification);
        }
    };

    PdfLineSegmentPtr_t PrevLine;
    for(const auto& Line : _block->lines()) {
        if(Line->association() == enuLineSegmentAssociation::InsideTextOf)
            Relation = enuLineParagraphRelation::BelongsTo;
        else {
            Relation = estimateLineParagraphRelation(_block, Line, PrevLine, CurrentBlock);
        }
        PrevLine = Line;

        if(Relation == enuLineParagraphRelation::DoesNotBelog)
            storeCurrentBlock();

        CurrentBlock->appendLineSegment(Line, false);

        if(Relation == enuLineParagraphRelation::LastLineOf)
            storeCurrentBlock();
    }
    storeCurrentBlock();

    for(const auto& Block : SplittedBlocks) {
        Block->setX0(_block->x0());
        Block->setX1(_block->x1());
        Block->setSpace2Left(_block->space2Left());
        Block->setSpace2Right(_block->space2Right());
    }

    return SplittedBlocks;
}

void splitToInnerBlocks(const stuConfigsPtr_t& _configs,
                        int _pageIndex,
                        const stuPageMargins& _pageMargins,
                        const stuSize &_pageSize,
                        const FontInfo_t& _fontInfo,
                        PdfBlockPtr_t& _block){
    (void)_pageSize;

    auto BlockLines = _block->lines();
    sort(BlockLines, [] (const PdfLineSegmentPtr_t& _a, const PdfLineSegmentPtr_t& _b) { return _a->y0() < _b->y0(); });

    clsLayoutStorage LayoutStorage(_configs, _block->lines(), false);
    LayoutStorage.identifyInnerColumns(_pageIndex, _pageMargins, _pageSize, _configs);

    float MinLeft = _pageSize.Width, MaxRight = 0;
    for(const auto& Storage : LayoutStorage.Children) {
        MinLeft = std::min(MinLeft, Storage->x0() + Storage->Space2Left);
        MaxRight = std::max(MaxRight, Storage->x1() - Storage->Space2Right);
    }

    for(const auto& Storage : LayoutStorage.Children) {
        if(Storage->x0() + Storage->Space2Left <= MinLeft + 1.f) {
            Storage->Space2Left = MinLeft - _block->x0();
            Storage->setX0(_block->x0());
        }
        if(Storage->x1() - Storage->Space2Right >= MaxRight - 1.f) {
            Storage->Space2Right = _block->x1() - MaxRight;
            Storage->setX1(_block->x1());
        }
    }

    for(const auto& Storage : LayoutStorage.Children) {
        PdfBlockPtr_t InnerBlock;
        for(size_t i = 0; i < BlockLines.size(); ++i) {
            const auto Line = BlockLines.at(i);
            if(Storage->doesContain(Line))
                appendLineToBlocks(_block->innerBlocks(),
                                   InnerBlock,
                                   Storage,
                                   Line,
                                   _block->location(),
                                   _block->contentType(), true);
        }
    }

    _block->naiveSortByReadingOrder();
    _block->clearLines();

    std::function<void(const PdfBlockPtr_t& _block)> consolidateSpacesOfLines = [&](const PdfBlockPtr_t& _block){
        if(_block->innerBlocks().size())
            for(const auto& IB : _block->innerBlocks())
                consolidateSpacesOfLines(IB);
        else
            for(const auto& Line : _block->lines())
                Line->consolidateSpaces(_fontInfo, _configs);
    };

    consolidateSpacesOfLines(_block);
}

void splitAndMarkBlocks(const stuConfigsPtr_t& _configs,
                        int16_t _pageIndex,
                        const stuPageMargins& _pageMargins,
                        const stuSize &_pageSize,
                        const FontInfo_t& _fontInfo,
                        PdfBlockPtrVector_t& _blocks,
                        const stuParState& _lastPageBlockState) {

    float FootNoteTop = _pageSize.Height;

    for (const auto& Block : _blocks) {
        if(Block->location() != enuLocation::Main) continue;

        if (Block->y0() > (1 - HEADER_FOOTER_PAGE_MARGIN_THRESHOLD) * _pageSize.Height) {
            if(Block->lines().front()->isHorizontalRuler()
               && Block->lines().front()->x0() - Block->x0() < 10.f)
                if(Block->horzRulers(.05f * _pageSize.Width).size() == 1)
                    FootNoteTop = Block->y0();
        }
    }

    PdfBlockPtr_t TableCandidate;
    PdfBlockPtr_t ImageCaptionCandidate;
    bool TableCandidateEndedWithHorzRuler = false;
    clsPdfItem ImageMargins(_pageSize.Width, _pageSize.Height, 0, 0, enuPdfItemType::None);
    clsPdfItem TableMargins(_pageSize.Width, _pageSize.Height, 0, 0, enuPdfItemType::None);

    std::set<clsPdfBlock*> Used;

    auto resetMargins = [&](clsPdfItem& _margins) {
        _margins = clsPdfItem(_pageSize.Width, _pageSize.Height, 0, 0, enuPdfItemType::None);
    };

    auto runOnAllBlocks = [&](std::function<bool(PdfBlockPtr_t& _candidate)> _method, bool _reverse = false) {
        if(_reverse) {
            for(auto Iter = _blocks.rbegin(); Iter != _blocks.rend(); ++Iter) {
                if((*Iter).get() == nullptr) continue;
                if((*Iter)->lines().size()) {
                    if(_method((*Iter)) == false)
                        return;
                }else
                    for(
                        auto Iter2 = (*Iter)->innerBlocks().rbegin(); Iter2 != (*Iter)->innerBlocks().rend(); ++Iter2) {
                        if((*Iter2).get() == nullptr) continue;
                        if((*Iter2)->innerBlocks().size()) {
                            for(auto& Par : (*Iter2)->innerBlocks())
                                if(Par.get() && _method((Par)) == false)
                                    return;
                        } else
                            if(_method((*Iter2)) == false)
                                return;
                    }
            }
        } else {
            for(auto Iter = _blocks.begin(); Iter != _blocks.end(); ++Iter) {
                if((*Iter).get() == nullptr) continue;
                if((*Iter)->lines().size()) {
                    if(_method((*Iter)) == false)
                        return;
                } else
                    for(
                        auto Iter2 = (*Iter)->innerBlocks().begin(); Iter2 != (*Iter)->innerBlocks().end(); ++Iter2) {
                        if((*Iter2).get() == nullptr) continue;
                        if((*Iter2)->innerBlocks().size()) {
                            for(auto& Par : (*Iter2)->innerBlocks())
                                if(Par.get() && _method((Par)) == false)
                                    return;
                        } else
                            if(_method((*Iter2)) == false)
                                return;
                    }
            }
        }

    };

    auto markIBAs = [&](PdfBlockPtr_t& _captionCandidate, clsPdfItem& _margins, bool _isImage) {

        PdfBlockPtr_t ParentBlock;

        auto setBlockAsType = [&](PdfBlockPtr_t& _candidate){
            if(Used.find(_candidate.get()) != Used.end()) {
                return true;
            }
            if(_margins.doesContain(_candidate, DEFAULT_HORIZONTAL_MARGIN)){
                Used.insert(_candidate.get());
                if(_candidate.get() == _captionCandidate.get())
                    _candidate->setContentType(_isImage ? enuContentType::ImageCaption : enuContentType::Table);
                else
                    _candidate->setContentType(_isImage ? enuContentType::Image : enuContentType::Table);

                if(ParentBlock.get() == nullptr)
                    ParentBlock = _candidate;
            }
            return true;
        };

        auto mergeBlocks = [&](PdfBlockPtr_t& _candidate){
            if(_candidate.get() == ParentBlock.get() || _candidate.get() == nullptr || _margins.doesContain(_candidate, DEFAULT_HORIZONTAL_MARGIN) == false)
                return true;
            if(_candidate->contentType() == (_isImage ? enuContentType::Image : enuContentType::Table)){
                for(const auto& Line : _candidate->lines())
                    ParentBlock->appendLineSegment(Line, false);
                if(Used.find(_candidate.get()) != Used.end())
                    Used.erase(_candidate.get());
                _candidate = nullptr;
            }
            return true;
        };

        if(_margins.y1() < _margins.y0()) {
            _captionCandidate = nullptr;
            return;
        }

        runOnAllBlocks(setBlockAsType);
        assert(ParentBlock.get() != nullptr);
        runOnAllBlocks(mergeBlocks);

        resetMargins(_margins);
        _captionCandidate = nullptr;
    };

    PdfBlockPtrVector_t NotSplitted;

    auto splitIB2SubIBs = [] (const PdfBlockPtr_t& IB) {
        PdfBlockPtrVector_t SIBs = splitBlockBasedOnJustification(IB);
        if(SIBs.size()) {
            for (const auto& SIB : SIBs)
                IB->appendInnerBlock(SIB);
            IB->clearLines();
        } else
            SIBs.push_back(IB);

        return SIBs;
    };

#ifdef TARGOMAN_SHOW_DEBUG
    if(_configs->MidStepDebugLevel > 0) sigStepDone("onInnerBlocks", _pageIndex, _blocks);
#endif
    int BlockIndex = -1;
    for (auto& Block : _blocks) {
        if(Block.get() == nullptr) continue;

        if(Block->y0() >= FootNoteTop) {
            Block->setLocation(enuLocation::Footnote);
            Block->setContentType(enuContentType::Text);
        }

#ifdef TARGOMAN_SHOW_DEBUG
        if(_configs->MidStepDebugLevel > 0) sigStepDone("onInnerBlocks", _pageIndex, PdfBlockPtrVector_t { Block });
#endif
        splitToInnerBlocks(_configs, _pageIndex, _pageMargins, _pageSize, _fontInfo, Block);

#ifdef TARGOMAN_SHOW_DEBUG
        if(_configs->MidStepDebugLevel > 0) sigStepDone("onInnerBlocks", _pageIndex, Block->innerBlocks());
#endif
        if(Block->location() != enuLocation::Main)
            continue;
        ++BlockIndex;

        auto ColumnApproxLineHeight = Block->approxLineHeight();
        int IBIndex = -1;
        for(const auto& IB : Block->innerBlocks()) {
            if(IB.get() == nullptr) continue;

            if(IB->y0() >= FootNoteTop) {
                IB->setLocation(enuLocation::Footnote);
                IB->setContentType(enuContentType::Text);
                continue;
            }

            PdfBlockPtrVector_t SubIBs;

            auto IBIsTableLike = canBeTable(IB, false);
            auto IBIsMostlyImage = IB->mostlyImage();

#ifdef TARGOMAN_SHOW_DEBUG
            if(_configs->MidStepDebugLevel > 0) sigStepDone("onInnerBlocks", _pageIndex, PdfBlockPtrVector_t { IB }, PdfBlockPtrVector_t { IB });
#endif

            if(IBIsTableLike == false && IBIsMostlyImage == false) {
                SubIBs = splitIB2SubIBs(IB);
            } else {
                SubIBs.push_back(IB);
                NotSplitted.push_back(IB);
            }

#ifdef TARGOMAN_SHOW_DEBUG
            if(_configs->MidStepDebugLevel > 0) sigStepDone("onInnerBlocks", _pageIndex, SubIBs, SubIBs);
#endif
            ++IBIndex;
            int SIBIndex = -1;

            for (const auto& SubIB : SubIBs) {

                if(SubIB->y0() >= FootNoteTop) {
                    SubIB->setLocation(enuLocation::Footnote);
                    SubIB->setContentType(enuContentType::Text);
                    continue;
                }

                ++SIBIndex;
                auto canBeTableCaption = [&](const PdfLineSegmentPtr_t& _line){
                    return isTableCaption(_line->contentString())
                            && (std::abs(_line->x0() - SubIB->x0()) < 2.f
                                || canBeTable(SubIB, false)
                                || anyOf<PdfLineSegmentPtr_t>(SubIB->lines(),
                                                                [&](const PdfLineSegmentPtr_t& _otherLine) {
                        return _otherLine->type() == enuPdfItemType::Char
                                && _otherLine->x0() < _line->x0()
                                && _otherLine->vertOverlap(_line) > MIN_ITEM_SIZE; }) == false);
                };

                auto canBeImageCaption = [&](const PdfLineSegmentPtr_t& _line){
                    return isImageCaption(_line->contentString())
                            && (std::abs(_line->x0() - SubIB->x0()) < 2.f
                                || SubIB->mostlyImage()
                                || anyOf<PdfLineSegmentPtr_t>(SubIB->lines(),
                                                                [&](const PdfLineSegmentPtr_t& _otherLine) {return _otherLine->type() == enuPdfItemType::Char && _otherLine->x0() < _line->x0();}) == false);
                };


                auto Lines = SubIB->lines();
                sort(Lines, [](const PdfLineSegmentPtr_t& _a, const PdfLineSegmentPtr_t& _b) { return _a->y0() < _b->y0() && _a->x0() < _b->x0(); });

                auto setMargins = [&](clsPdfItem& _margins, const PdfBlockPtr_t& _candidate){
                    _margins.setX0(std::min(SubIB->x0(),(std::min(_margins.x0(), _candidate->x0()))));
                    _margins.setY0(std::min(SubIB->y0(),(std::min(_margins.y0(), _candidate->y0()))));
                    _margins.setX1(std::max(SubIB->x1(),(std::max(_margins.x1(), _candidate->x1()))));
                    _margins.setY1(std::max(SubIB->y1(),(std::max(_margins.y1(), _candidate->y1()))));
                    _margins.setTightTop(_margins.y0());
                    _margins.setTightBottom(_margins.y1());
                };

                if(TableCandidate.get() == nullptr
                   && BlockIndex == 0
                   && IBIndex == 0
                   && SIBIndex == 0
                   && _lastPageBlockState.LastPageMainBlock.get()
                   && _lastPageBlockState.LastPageMainBlock->contentType() == enuContentType::Table)
                    TableCandidate = _lastPageBlockState.LastPageMainBlock;

                if(TableCandidate.get() != nullptr) {
                    auto MaxDistance = (TableCandidateEndedWithHorzRuler
                                        && TableCandidate->height() > 4 * ColumnApproxLineHeight
                                        ? -.5f : -1.9f) * ColumnApproxLineHeight;

                    auto a = TableCandidate->vertOverlap(SubIB);
                    auto b = canBeTable(SubIB, TableCandidateEndedWithHorzRuler);
                    if(TableCandidate->vertOverlap(SubIB) > MaxDistance
                       && canBeTable(SubIB, TableCandidateEndedWithHorzRuler)) {
                        SubIB->setContentType(enuContentType::Table);
                        TableCandidate->setContentType(enuContentType::Table);
                        TableCandidate = SubIB;
                        if(Lines.back()->isHorizontalRuler())
                            TableCandidateEndedWithHorzRuler = true;
                        else
                            TableCandidateEndedWithHorzRuler = false;

                        setMargins(TableMargins, TableCandidate);

                    } else
                        markIBAs(TableCandidate, TableMargins, false);
                }

                if(canBeTableCaption(Lines.front())){
                    if(canBeTable(SubIB, false)) {
                        SubIB->setContentType(enuContentType::Table);
                        setMargins(TableMargins, SubIB);
                    }
                    TableCandidate = SubIB;
                    if(Lines.back()->isHorizontalRuler()) TableCandidateEndedWithHorzRuler = true;
                } else if (canBeImageCaption(Lines.back()) || canBeImageCaption(Lines.front())) {
                    bool ImageFound = false;
                    ImageCaptionCandidate = SubIB;

                    auto setImageMargins = [&](const PdfBlockPtr_t& _candidate){
                        setMargins(ImageMargins, _candidate);
                        if(_candidate->mostlyImage())
                            ImageFound = true;
                    };

                    auto confirmSubIBIsImageCaption = [&](const PdfBlockPtr_t& _imgCandidate, bool _isCaptionCandidate) {
                        if(_imgCandidate->mostlyImage()
                           || (_isCaptionCandidate == false
                               && (_imgCandidate->height() < 1.8f * ColumnApproxLineHeight
                                   || _imgCandidate->approxLineHeight() < .6f * ColumnApproxLineHeight
                                   )
                               )
                           ){
                            setImageMargins(_imgCandidate);
                        } else if (_isCaptionCandidate == false)
                            return false;
                        return true;
                    };

                    bool Continue2LookForImages = true;
                    for(int i = SIBIndex; i >= 0; --i) {
                        auto ImgCandidate = SubIBs[static_cast<size_t>(i)];
                        if(confirmSubIBIsImageCaption(ImgCandidate, i == SIBIndex) == false) {
                            Continue2LookForImages = false;
                            break;
                        }
                    }

                    if (Continue2LookForImages) {
                        for (int i= IBIndex - 1; i >= 0; --i) {
                            auto ImgBlockCandidate = Block->innerBlocks()[static_cast<size_t>(i)];
                            if(ImgBlockCandidate.get() == nullptr
                               || ImgBlockCandidate->location() != enuLocation::Main
                               || Used.find(ImgBlockCandidate.get()) != Used.end())
                                continue;
                            if(ImgBlockCandidate->innerBlocks().size()) {
                                auto SIBIter = ImgBlockCandidate->innerBlocks().end();
                                do {
                                    --SIBIter;
                                    if(SIBIter->get() == nullptr
                                       || Used.find(SIBIter->get()) != Used.end()
                                       || confirmSubIBIsImageCaption(*SIBIter, false) == false){
                                        Continue2LookForImages = false;
                                        break;
                                    }
                                }while(SIBIter != ImgBlockCandidate->innerBlocks().begin());
                            } else if(confirmSubIBIsImageCaption(ImgBlockCandidate, false) == false){
                                Continue2LookForImages = false;
                                break;
                            }
                            if(Continue2LookForImages == false)
                                break;
                        }
                    }

                    auto updateImageMargin = [&](const PdfBlockPtr_t& _candidate, bool _horizontalOverlap){
                        if(_candidate->location() != enuLocation::Main
                           || _candidate.get() == SubIB.get()
                           || Used.find(_candidate.get()) != Used.end()
                           )
                            return true;
                        auto hasHorzOverlap = _candidate->horzOverlap(SubIB) > _candidate->width() - 2.f;
                        auto hasVertOverlap = _candidate->vertOverlap(SubIB) > SubIB->height() - 2.f;
                        if(
                           (
                               (_horizontalOverlap
                                && hasHorzOverlap
                                && _candidate->width() < SubIB->width() + MIN_ITEM_SIZE
                                && _candidate->y0() < SubIB->y0()
                                ) ||
                               (_horizontalOverlap == false
                                && hasVertOverlap
                                )
                               )){
                            if(_candidate->mostlyImage()
                               || (hasHorzOverlap
                                   && _candidate->y1() > ImageMargins.y0() + DEFAULT_VERTICAL_MARGIN - 2.f
                                   && (_candidate->height() < 1.8f * ColumnApproxLineHeight
                                       || _candidate->approxLineHeight() < .6f * ColumnApproxLineHeight
                                       )
                                   )
                               ) {
                                setImageMargins(_candidate);
                                return true;
                            } else if (_candidate->height() < 1.8f * ColumnApproxLineHeight
                                       || _candidate->approxLineHeight() < .6f * ColumnApproxLineHeight)
                                return true;
                            else
                                return false;
                        }
                        return true;
                    };

                    if (Continue2LookForImages) {
                        runOnAllBlocks([&](PdfBlockPtr_t& _block) { return updateImageMargin(_block, true); }, true);
                        if (ImageFound == false) {
                            resetMargins(ImageMargins);
                            runOnAllBlocks([&](PdfBlockPtr_t& _block) { return updateImageMargin(_block, false); }, true);
                        }
                    }

                    if(ImageFound)
                        markIBAs(ImageCaptionCandidate, ImageMargins, true);

                } else if(Lines.front()->y0() > (1 - HEADER_FOOTER_PAGE_MARGIN_THRESHOLD) * _pageSize.Height
                          && Lines.front()->isHorizontalRuler()
                          ){
                    size_t HorzRulersBelow = 0;
                    for (const auto& Line : Block->horzRulers(10.f * Block->width()))
                        if(Line->y0() > Lines.front()->y1())
                            ++HorzRulersBelow;
                    if(HorzRulersBelow == 0) {
                        FootNoteTop = SubIB->y0();
                        SubIB->setLocation(enuLocation::Footnote);
                        if(SubIB->contentType() == enuContentType::None)
                            SubIB->setContentType(enuContentType::Text);
                    }
                }
            }
        }
    }

    if(TableCandidate.get() != nullptr)
        markIBAs(TableCandidate, TableMargins, false);

    std::function<void(const PdfBlockPtr_t&)> recursivelyMarkInnerBlocks = [&](const PdfBlockPtr_t& _block) {
        if(_block.get() == nullptr) return;
        if(_block->lines().size() && _block->contentType() == enuContentType::None) {
            _block->setContentType(enuContentType::Text);
            if(
               _block->lines().size() == 1
               && _block->lines().front()->isHorizontalRuler()
               && _block->lines().front()->width() < 0.25f * _block->width()
               )
                _block->setContentType(enuContentType::Text);
            else
                _block->setContentType(_block->isPureImage() ?  enuContentType::Image : enuContentType::Text);
        } else
            for(const auto& IB : _block->innerBlocks())
                if(IB.get() != nullptr)
                    recursivelyMarkInnerBlocks(IB);
    };

    for(const auto& Block : _blocks)
        recursivelyMarkInnerBlocks(Block);

    PdfBlockPtrVector_t FlattenMarkedBlocks;
    for(const auto& Block : _blocks) {
        if(Block.get() == nullptr) continue;
        if(Block->lines().size())
            FlattenMarkedBlocks.push_back(Block);
        else {
            for(const auto& IB : Block->innerBlocks()) {
                if(IB.get() == nullptr) continue;
                if(IB->lines().size()) {
                    FlattenMarkedBlocks.push_back(IB);
                } else {
                    PdfBlockPtr_t LastSubIB;
                    for(const auto& SubIB : IB->innerBlocks()) {
                        if(SubIB.get() == nullptr) continue;
                        if(LastSubIB.get() == nullptr
                           || LastSubIB->contentType() != SubIB->contentType()){
                            LastSubIB = SubIB;
                            FlattenMarkedBlocks.push_back(SubIB);
                        } else {
                            LastSubIB->unionWith_(SubIB);
                            for(const auto& Line : SubIB->lines())
                                LastSubIB->appendLineSegment(Line, false);
                        }
                    }
                }
            }
        }
    }

    _blocks = FlattenMarkedBlocks;
}


}
}
}
}
