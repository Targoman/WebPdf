#include "ParagraphAndSentenceManager.h"
#include "LayoutAnalysis/BulletAndNumbering.h"

namespace Targoman {
namespace PDF {
namespace Private {

class LineSpecUpdater {
public:
    static void setLineSpecs(PdfLineSegmentPtr_t& _line, int16_t _pageIndex, int16_t _parIndex, int16_t _sntIndex){
        _line->setLineSpecs(_pageIndex, _parIndex, _sntIndex);
    }
};

constexpr float HORZ_POS_THRESH_TO_LINEHEIGHT_RATIO = 0.5f;

bool endsWith(const PdfLineSegmentPtr_t& _line, size_t _start, size_t _end, const std::wstring& _value) {
    for(size_t i = _end, j = _value.length(); i > _start && j > 0; --i, --j) {
        if(_line->at(i - 1)->type() != enuPdfItemType::Char)
            return false;
        if(_line->at(i - 1)->as<clsPdfChar>()->code() != _value.at(j - 1))
            return false;
    }
    return true;
}

bool isPersonsInitial(const PdfItemPtr_t& _a) {
    if(_a->type() != enuPdfItemType::Char)
        return false;
    auto CharCode = _a->as<clsPdfChar>()->code();
    if(CharCode >= '0' && CharCode <= '9')
        return false;
    return true;
}

bool endsWithStrongAbbreviation(const PdfLineSegmentPtr_t& _line, size_t _start, size_t _end) {
    const std::wstring STRONG_ABBREVIATIONS[] = {
        L"Vol", L"No", L"pp", L"ie", L"i.e", L"eg", L"e.g", L"ex", L"fig",
        L"Fig", L"eq", L"Eq", L"eqn", L"Eqn", L"tbl", L"Tbl", L"Ref", L"adj",
        L"ltd", L"Ltd", L"Dept", L"approx", L"Approx", L"Corp", L"corp", L"Figs",
        L"vol", L"etc"
    };


    if(_end >= 2 &&
       _line->at(_end - 2)->type() == enuPdfItemType::Char &&
       _line->at(_end - 1)->type() == enuPdfItemType::Char &&
       _line->at(_end - 2)->as<clsPdfChar>()->code() == L' ' &&
       isPersonsInitial(_line->at(_end - 1))
       )
        return true;
    for(size_t i = 0; i < sizeof STRONG_ABBREVIATIONS / sizeof STRONG_ABBREVIATIONS[0]; ++i)
        if(endsWith(_line, _start, _end, STRONG_ABBREVIATIONS[i]))
            return true;
    return false;
}

bool endsWithWeakAbbreviation(const PdfLineSegmentPtr_t& _line, size_t _start, size_t _end) {
    const std::wstring WEAK_ABBREVIATIONS[] = {
        L"Int", L"et al"
    };

    if(_end >= 2 &&
       _line->at(_end - 2)->type() == enuPdfItemType::Char &&
       _line->at(_end - 1)->type() == enuPdfItemType::Char &&
       _line->at(_end - 2)->as<clsPdfChar>()->code() == L' ' &&
       isPersonsInitial(_line->at(_end - 1)) == false
       )
        return true;
    for(size_t i = 0; i < sizeof WEAK_ABBREVIATIONS / sizeof WEAK_ABBREVIATIONS[0]; ++i)
        if(endsWith(_line, _start, _end, WEAK_ABBREVIATIONS[i]))
            return true;
    return false;
}

inline bool isOneOf(const PdfItemPtr_t& _item, const wchar_t _items[], size_t _size) {
    if(_item->type() != enuPdfItemType::Char)
        return false;
    return _item->as<clsPdfChar>()->isOneOf(_items, _size);
}

bool isSpace(const PdfItemPtr_t& _item) {
    if(_item->type() != enuPdfItemType::Char)
        return false;
    return _item->as<clsPdfChar>()->isSpace();
}

bool isDot(const PdfItemPtr_t& _item) {
    if(_item->type() != enuPdfItemType::Char)
        return false;
    return _item->as<clsPdfChar>()->isDot();
}

bool isUppercaseAlpha(const PdfItemPtr_t& _item) {
    if(_item->type() != enuPdfItemType::Char)
        return false;
    return _item->as<clsPdfChar>()->isUppercaseAlpha();
}

bool isLowercaseAlpha(const PdfItemPtr_t& _item) {
    if(_item->type() != enuPdfItemType::Char)
        return false;
    return _item->as<clsPdfChar>()->isLowercaseAlpha();
}

int findInArray(const wchar_t _item, const wchar_t _items[], size_t _size) {
    for(size_t i = 0; i < _size; ++i)
        if(_items[i] == _item)
            return static_cast<int>(i);
    return -1;
}

enuLineParagraphRelation estimateLineParagraphRelation(
        const PdfBlockPtr_t& _parentBlock,
        const PdfLineSegmentPtr_t& _line,
        const PdfLineSegmentPtr_t& _prevLine,
        const PdfBlockPtr_t& _paragraph
        ) {

    if(_line->listType() != enuListType::None) {
        if(_paragraph->lines().size() == 0)
            return enuLineParagraphRelation::BelongsTo;
        return enuLineParagraphRelation::DoesNotBelog;
    }

    if(
       _parentBlock->contentType() == enuContentType::List &&
       _paragraph->lines().size() > 0 &&
       _paragraph->lines().front()->listType() != enuListType::None
       ) {
        if(std::abs(_line->x0() - _paragraph->lines().front()->textLeft()) < 2.f)
            return enuLineParagraphRelation::BelongsTo;
        return enuLineParagraphRelation::DoesNotBelog;
    }

    //    if(
    //       (_paragraph->justification() == enuJustification::Left || _paragraph->justification() == enuJustification::Justified) &&
    //       _line->x0() - _parentBlock->tightLeft() > 4.f * _line->height()
    //    ) {
    //        // TODO: Move these to their proper lambdas
    //        if(_paragraph->lines().size() == 0)
    //            return enuLineParagraphRelation::LastLineOf;
    //        return enuLineParagraphRelation::DoesNotBelog;
    //    }

    if (_paragraph->lines().size() > 0) {
        // WARNING: THE MAGIC NUMBER BELOW IS RESULT OF TEDIOUS LONG HOUR WORKS, DO NOT MEDDLE
        if(_parentBlock->lines().size() > 2 && _line->baseline() - _paragraph->lines().back()->baseline() > 1.1f * _parentBlock->approxLineSpacing())
            return enuLineParagraphRelation::DoesNotBelog;
        if(_paragraph->doesShareFontsWith(_line) == false)
            return enuLineParagraphRelation::DoesNotBelog;
    }

    auto getFirstWordWidth = [&] () {
        float X0 = _line->x0(), X1 = -1;
        float BiggestCharWidth = 0;

        for(const auto& Item : _line->items())
            if(Item->type() == enuPdfItemType::Char)
                BiggestCharWidth = std::max(BiggestCharWidth, Item->width());

        for(const auto& Item : _line->items())
            if(Item->type() == enuPdfItemType::Char && Item->as<clsPdfChar>()->isSpace()) {
                X1 = Item->x0();
                break;
            }
        if(X1 < 0)
            X1 = _line->x1();
        return (X1 - X0) + (BiggestCharWidth * 2);
    };

    auto estimateLineLeftJustParagraphRelation = [&] () {
        if(std::abs(_line->x0() - _parentBlock->textIndentPos()) > 1.f * _line->height()) // INDENTATION
            return enuLineParagraphRelation::DoesNotBelog;
        float A = getFirstWordWidth();
        if(_prevLine.get() != nullptr && _parentBlock->x1() - _prevLine->x1() > getFirstWordWidth())
            return enuLineParagraphRelation::DoesNotBelog;
        return enuLineParagraphRelation::BelongsTo;
    };
    auto estimateLineRightJustParagraphRelation = [&] () {
        if(_parentBlock->tightRight() - _line->x1() > 1.f * _line->height())
            return enuLineParagraphRelation::LastLineOf;
        if(_prevLine.get() != nullptr && _prevLine->x0() - _parentBlock->x0() > getFirstWordWidth())
            return enuLineParagraphRelation::DoesNotBelog;
        return enuLineParagraphRelation::BelongsTo;
    };
    auto estimateLineJustJustParagraphRelation = [&] () {
        if(_parentBlock->tightRight() - _line->x1() > 1.f * _line->height())
            return enuLineParagraphRelation::LastLineOf;
        if(std::abs(_line->x0() - _parentBlock->textIndentPos()) > 1.f * _line->height())
            return enuLineParagraphRelation::DoesNotBelog;
        return enuLineParagraphRelation::BelongsTo;
    };

    switch(_paragraph->justification()) {
    case enuJustification::Center:
        return enuLineParagraphRelation::BelongsTo;
    case enuJustification::Left:
        return estimateLineLeftJustParagraphRelation();
    case enuJustification::Right:
        return estimateLineRightJustParagraphRelation();
    case enuJustification::Justified:
        return estimateLineJustJustParagraphRelation();
    default:
        // TODO: Issue warning
        return estimateLineJustJustParagraphRelation();
    }
}

bool canContinuePrevBlock(const PdfBlockPtr_t& _paragraph, const stuParState& _prevState) {
    auto AverageLineHeight = _paragraph->approxLineHeight();
    auto minIndentWidth = [&] () { return 1.f * AverageLineHeight; };
    auto sameWidthThreshold = [&] () { return 0.5f * AverageLineHeight; };
    auto maxSameBlockVertGap = [&] () { return 2.f * AverageLineHeight; };
    auto sameHorzPosThreshold = [&] () { return HORZ_POS_THRESH_TO_LINEHEIGHT_RATIO * AverageLineHeight;  };

    if(_prevState.TextBlock.get() == nullptr)
        return false;
    if(
       _paragraph->contentType() == enuContentType::List &&
       _prevState.TextBlock->contentType() == enuContentType::List
       )
        return _paragraph->lines().size() == 0 || _paragraph->lines().front()->listType() == enuListType::None;

    if(_paragraph->justification() != _prevState.TextBlock->justification())
        return false;

    switch(_prevState.TextBlock->justification()) {
    case enuJustification::None:
        return false;
    case enuJustification::Justified:
        if(true) {
            float HorzOverlap = _paragraph->horzOverlap(_prevState.TextBlock);
            if(HorzOverlap > MIN_ITEM_SIZE && HorzOverlap < 0.8f * std::min(_paragraph->width(), _prevState.TextBlock->width()))
                return false;
            if(HorzOverlap > 0.5f * std::min(_paragraph->width(), _prevState.TextBlock->width())
               && std::abs(_paragraph->width() - _prevState.TextBlock->width()) > sameWidthThreshold()
               )
                return false;

            if(std::abs(_paragraph->lines().front()->x0() - _paragraph->x0()) > minIndentWidth())
                return false;
        }
        break;
    case enuJustification::Left:
        if(true) {
            float HorzOverlap = _paragraph->horzOverlap(_prevState.TextBlock);
            if(HorzOverlap > MIN_ITEM_SIZE && std::abs(_paragraph->x0() - _prevState.TextBlock->x0()) > sameHorzPosThreshold())
                return false;
        }
        break;
    case enuJustification::Right:
        if(true) {
            float HorzOverlap = _paragraph->horzOverlap(_prevState.TextBlock);
            if(HorzOverlap > MIN_ITEM_SIZE && std::abs(_paragraph->x1() - _prevState.TextBlock->x1()) > sameHorzPosThreshold())
                return false;
        }
        break;
    case enuJustification::Center:
        if(std::abs(_paragraph->centerX() - _prevState.TextBlock->centerX()) > sameHorzPosThreshold())
            return false;
        break;
    }
    if(_paragraph->doesShareFontsWith(_prevState.TextBlock) == false)
        return false;

    if(
       _paragraph->horzOverlap(_prevState.TextBlock) > 0.5f * _paragraph->width() &&
       _paragraph->y0() - _prevState.TextBlock->y1() > maxSameBlockVertGap()
       )
        return false;
    return true;
};

stuParState splitParagraphSentences(const stuConfigsPtr_t& _configs, PdfBlockPtr_t &_paragraph, int16_t _pageIndex, const stuParState &_prevState) {
    PdfLineSegmentPtrVector_t RawLines = _paragraph->lines();

    stuParState State = _prevState;

    if(_paragraph->isNonText()) {
        State.ContinuationState = enuContinuationState::Cannot;
        return State;
    }

    bool ContinueFromPrev = _prevState.ContinuationState != enuContinuationState::Cannot;
    int16_t PageIndex, ParIndex, SntIndex;

    if(_prevState.TextBlock.get() != nullptr) {
        if(canContinuePrevBlock(_paragraph, _prevState) == false)
            ContinueFromPrev = false;

        PageIndex = _prevState.TextBlock->lines().back()->pageIndex();
        ParIndex  = _prevState.TextBlock->lines().back()->parIndex();
        SntIndex  = _prevState.TextBlock->lines().back()->sntIndex();

        if(ContinueFromPrev == false) {
            if(PageIndex != _pageIndex){
                ParIndex = 0;
                PageIndex = _pageIndex;
            }else
                ++ParIndex;
            SntIndex = 0;
        }
    } else {
        PageIndex = _pageIndex;
        ParIndex = 0;
        SntIndex = 0;
    }

    if(ContinueFromPrev == false)
        State.OpenTags.clear();

    auto storeCurrentSentence = [&] (const PdfLineSegmentPtr_t& _line, size_t _start, size_t _end) {
        PdfLineSegmentPtr_t NewSegment = std::make_shared<clsPdfLineSegment>();
        for(size_t i = _start; i < _end; ++i)
            NewSegment->appendPdfItem(_line->at(i));
        LineSpecUpdater::setLineSpecs(NewSegment, PageIndex, ParIndex, SntIndex);
        if(_start == 0 && _line->listType() != enuListType::None)
            NewSegment->setBulletAndNumberingInfo(_configs, _line->textLeft(), _line->listType());
        _paragraph->appendLineSegment(NewSegment, false);
    };

    auto isSentenceEnder = [&] (const PdfItemPtr_t& _item) {
        constexpr wchar_t SENTENCE_ENDERS[] = {
            L'!', L'?', L';', L':',
            L'\xFF01', // {！}
            L'\x37E', // {;}
            L'\x203C', // {‼}
            L'\x203D', // {‽}
            L'\x2047', // {⁇}
            L'\x2048', // {⁈}
            L'\x2049', // {⁉}
            L'\x204F', // {⁏}
            L'\xFE15', // {︕}
            L'\xFE16', // {︖}
            L'\xFE54', // {﹔}
            L'\xFE56', // {﹖}
            L'\xFE57', // {﹗}
            L'\xFF1B', // {；}
            L'\xFF1F', // {？}
        };
        if(isDot(_item))
            return true;
        return isOneOf(_item, SENTENCE_ENDERS, sizeof SENTENCE_ENDERS / sizeof SENTENCE_ENDERS[0]);
    };

    auto isNumbering = [&] (const PdfLineSegmentPtr_t& _line, size_t _start, size_t _index) {
        if(_start != 0 ||
           //(FirstLineOfBlock && _prevState.CanContinue) ||
           _paragraph->lines().size() != 0)
            return false;
        for(size_t i = _start; i < _index; ++i) {
            if(_line->at(i)->type() != enuPdfItemType::Char)
                return false;
            const clsPdfChar* Char = _line->at(i)->as<clsPdfChar>();
            if(Char->isDot() == false && Char->isDecimalDigit() == false)
                return false;
        }
        return true;
    };


    constexpr wchar_t OPEN_TAGS[] = L"([{";
    constexpr wchar_t CLOSE_TAGS[] = L")]}";
    static_assert(
                sizeof OPEN_TAGS / sizeof OPEN_TAGS[0] == sizeof CLOSE_TAGS / sizeof CLOSE_TAGS[0],
            "Number of `open` and `close` tags are different"
            );
    constexpr int TAG_COUNT = sizeof OPEN_TAGS / sizeof OPEN_TAGS[0];


    bool EndedWithWeakAbbr = _prevState.ContinuationState == enuContinuationState::WeakAbbr, LineEndedWithSentenceEnder = false;
    size_t Start;
    auto splitToSentences = [&] (const PdfLineSegmentPtr_t& _line) {
        auto weakAbbrEndsSentence = [&] (size_t _position) {
            return _line->size() <= _position + 1 ||
                    ( isUppercaseAlpha(_line->at(_position)) && isLowercaseAlpha(_line->at(_position + 1)) );
        };
        if(EndedWithWeakAbbr) {
            if(weakAbbrEndsSentence(0))
                ++SntIndex;
        }
        EndedWithWeakAbbr = false;
        LineEndedWithSentenceEnder = false;
        Start = 0;

        auto canBreakAtPosition = [&](size_t _index){
            return isSpace(_line->at(_index))
            || _line->at(_index)->type() != enuPdfItemType::Char
            || _line->at(_index)->as<clsPdfChar>()->isVirtual();
        };

        for(size_t Index = Start; Index < _line->size(); ++Index) {
            bool MustBreakAtThisIndex = false;
            if(
               State.OpenTags.size() == 0 &&
               isSentenceEnder(_line->at(Index))
               ) {
                if(isDot(_line->at(Index)) == false) {
                    if(Index == _line->size() - 1
                       || (Index < _line->size() - 1
                           && canBreakAtPosition(Index + 1)))
                        MustBreakAtThisIndex = true;
                } else {
                    if(endsWithStrongAbbreviation(_line, Start, Index) == false) {
                        if(endsWithWeakAbbreviation(_line, Start, Index)) {
                            EndedWithWeakAbbr = Index >= _line->size() - 3;
                            if(EndedWithWeakAbbr) {
                                storeCurrentSentence(_line, Start, _line->size());
                                Start = _line->size();
                                break;
                            } else {
                                //TODO if isSpace convert to 0x20 also for dot
                                if(canBreakAtPosition(Index + 1) && weakAbbrEndsSentence(Index + 2))
                                    MustBreakAtThisIndex = true;
                            }
                        } else {
                            if(Index == _line->size() - 1
                               || (Index < _line->size() - 1
                                   && canBreakAtPosition(Index + 1)))
                                MustBreakAtThisIndex = isNumbering(_line, Start, Index + 1) == false;
                        }
                    }
                }
            }
            if(MustBreakAtThisIndex) {
                storeCurrentSentence(_line, Start, Index + 1);
                Start = Index + 1;
                ++SntIndex;
            } else if(_line->at(Index)->type() == enuPdfItemType::Char) {
                wchar_t CharCode = _line->at(Index)->as<clsPdfChar>()->code();
                int IndexOfTag;
                IndexOfTag = findInArray(CharCode, OPEN_TAGS, TAG_COUNT);
                if(IndexOfTag >= 0)
                    State.OpenTags.push_back(IndexOfTag);
                IndexOfTag = findInArray(CharCode, CLOSE_TAGS, TAG_COUNT);
                if(
                   IndexOfTag >= 0 &&
                   State.OpenTags.size() > 0 &&
                   State.OpenTags[State.OpenTags.size() - 1] == IndexOfTag
                   )
                    State.OpenTags.pop_back();
            }
        }
        if(Start < _line->size()) {
            storeCurrentSentence(_line, Start, _line->size());
        } else
            LineEndedWithSentenceEnder = true;
    };

    _paragraph->clearLines();
    for(const auto& RawLine : RawLines)
        splitToSentences(RawLine);

    if(_paragraph->isNonText() == false) {
        State.TextBlock = _paragraph;
        State.ContinuationState = enuContinuationState::Cannot;
        if(_paragraph->certainlyFinished() == false) {
            if(EndedWithWeakAbbr)
                State.ContinuationState = enuContinuationState::WeakAbbr;
            else if (LineEndedWithSentenceEnder == false)
                State.ContinuationState = enuContinuationState::Can;
        }
    }

    return State;
}


PdfBlockPtrVector_t splitBlockToParagraphs(int _pageIndex, const PdfBlockPtr_t &_block)
{
    (void)_pageIndex;
    PdfBlockPtrVector_t SplittedBlocks;

    assert(_block->innerBlocks().size() == 0);

    _block->estimateParagraphParams();

    auto Justification = _block->justification();
    PdfBlockPtr_t CurrentBlock = std::make_shared<clsPdfBlock>(_block->location(), _block->contentType(), Justification);

    enuLineParagraphRelation Relation = enuLineParagraphRelation::None;
    auto storeCurrentBlock = [&] (bool _lastParagraph) {
        if(CurrentBlock->lines().size() > 0) {
            CurrentBlock->setCertainlyFinished(_lastParagraph == false || Relation == enuLineParagraphRelation::LastLineOf);
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
            storeCurrentBlock(false);

        CurrentBlock->appendLineSegment(Line, false);

        if(Relation == enuLineParagraphRelation::LastLineOf)
            storeCurrentBlock(Line.get() == _block->lines().back().get());
    }
    storeCurrentBlock(true);

    for(const auto& Block : SplittedBlocks) {
        Block->setX0(_block->x0());
        Block->setX1(_block->x1());
        Block->setSpace2Left(_block->space2Left());
        Block->setSpace2Right(_block->space2Right());
    }

    return SplittedBlocks;
}


stuProcessedPage extractAndEnumerateSentences(const stuConfigsPtr_t& _configs,
                                              int16_t _pageIndex,
                                              const PdfBlockPtrVector_t& _pageBlocks,
                                              const FontInfo_t& _fontInfo,
                                              const stuParState& _prevPageState) {
    (void)_configs;

    stuProcessedPage Result;
    for(const auto& Block : _pageBlocks) {
        if(Block->isMainTextBlock() == false)
            continue;
        Block->replaceLines(LA::mergeOverlappingLines(_configs, Block->lines(), _fontInfo));
        LA::process4BulletAndNumbering(_configs, Block);
        auto BlockParagraphs = splitBlockToParagraphs(_pageIndex, Block);
        for(const auto& Par : BlockParagraphs)
            Result.Content.push_back(Par);
    }

    stuParState PrevParState = _prevPageState;
    for(size_t i = 0; i < Result.Content.size(); ++i)
        PrevParState = splitParagraphSentences(_configs, Result.Content.at(i), _pageIndex, PrevParState);

    for(const auto& Block : _pageBlocks)
        if(Block->location() == enuLocation::Main) {
            if(Block->isMainTextBlock() == false)
                Result.Content.push_back(Block);
            PrevParState.LastPageMainBlock = Block;
        }

    for(const auto& Block : _pageBlocks)
        if(Block->location() != enuLocation::Main){
            Block->replaceLines(LA::mergeOverlappingLines(_configs, Block->lines(), _fontInfo));
            LA::process4BulletAndNumbering(_configs, Block);
            auto BlockParagraphs = splitBlockToParagraphs(_pageIndex, Block);
            size_t LastBlockNo = Result.Content.size() - 1;
            for(const auto& Par : BlockParagraphs)
                Result.Content.push_back(Par);

            for(size_t i = LastBlockNo; i < Result.Content.size(); ++i)
                splitParagraphSentences(_configs, Result.Content.at(i), _pageIndex, stuParState());
        }

    Result.LastMainBlockState = PrevParState;

    return Result;
}

void updateFirstParagraph(const stuConfigsPtr_t& _configs, PdfBlockPtrVector_t& _pageBlocks, const stuParState& _prevPageState)
{
    for(auto& Paragraph : _pageBlocks) {
        if(Paragraph->isMainTextBlock()){
            splitParagraphSentences(_configs, Paragraph, Paragraph->lines().front()->pageIndex(), _prevPageState);
            return;
        }
    }
}

}
}
}
