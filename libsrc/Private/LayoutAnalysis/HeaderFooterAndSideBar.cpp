#include "HeaderFooterAndSideBar.h"

namespace Targoman {
namespace PDF {
namespace Private {
namespace LA {
void sigHeaderAndFooter(int _pageIndex, const PdfLineSegmentPtrVector_t& _stripes, const stuSize& _pageSize){
    PdfBlockPtrVector_t MarkedBlocks;
    PdfItemPtrVector_t MarkedItems;
    PdfBlockPtr_t Block;
    auto markedBlock = [&](enuLocation _loc) {
        for(const auto& Block : MarkedBlocks)
            if(Block->location() == _loc)
                return Block;
        MarkedBlocks.push_back(std::make_shared<clsPdfBlock>());
        return MarkedBlocks.back();
    };

    for(const auto& Stripe : _stripes){
        if(Stripe->isHeader()){
            Block = markedBlock(enuLocation::Header);
            Block->setLocation(enuLocation::Header);
        } else if (Stripe->isFooter()){
            Block = markedBlock(enuLocation::Footer);
            Block->setLocation(enuLocation::Footer);
        } else if (Stripe->isSidebar()){
            if(Stripe->x1() < SIDEBAR_PAGE_MARGIN_THRESHOLD * _pageSize.Width){
                Block = markedBlock(enuLocation::SidebarLeft);
                Block->setLocation(enuLocation::SidebarLeft);
            } else {
                Block = markedBlock(enuLocation::SidebarRight);
                Block->setLocation(enuLocation::SidebarRight);
            }
        } else
            continue;

        MarkedItems.push_back(Stripe);
        Block->unionWith_(Stripe);
    }

    sigStepDone("onMarkHeaderAndFooters",
                _pageIndex,
                MarkedBlocks,
                MarkedItems
                );
}

}
std::tuple<float, float> clsLayoutAnalyzer::markHeadersAndFooters(int _pageIndex, const stuPreprocessedPage &_page, const stuSize& _pageSize) const
{
    using namespace LA;
    float HeaderBottom = 0, FooterTop = _pageSize.Height;

    enum class enuPageNumberType {
        NotIdentified,
        None,
        Separator,
        Digits,
        Roman
    };

    auto getItemPageNumberType = [](const PdfItemPtr_t& _item, enuPageNumberType _pageNumberType) {
        auto isRoman = [](const clsPdfChar& _char){
            return  _char.code() == L'I' ||
                    _char.code() == L'V' ||
                    _char.code() == L'X' ||
                    _char.code() == L'L';
        };

        if(_item->type() != enuPdfItemType::Char)
            return enuPageNumberType::None;
        auto Char = _item->as<clsPdfChar>();
        if(Char->isDecimalDigit()){
            if(_pageNumberType != enuPageNumberType::Digits && _pageNumberType != enuPageNumberType::NotIdentified)
                return enuPageNumberType::None;
            return enuPageNumberType::Digits;
        }else if(isRoman(*Char)){
            if(_pageNumberType != enuPageNumberType::Roman && _pageNumberType != enuPageNumberType::NotIdentified)
                return enuPageNumberType::None;
            return enuPageNumberType::Roman;
        }else if(Char->isDot() == false &&
                 Char->isSpace() == false &&
                 Char->code() != L'-')
            return enuPageNumberType::None;
        if(_pageNumberType == enuPageNumberType::NotIdentified)
            return enuPageNumberType::None;
        return enuPageNumberType::Separator;
    };

    auto canBePageNumber = [&](const PdfLineSegmentPtr_t& _line, size_t _firstIndex, size_t _lastIndex){
        if(_line->items().size() == 0 || _lastIndex - _firstIndex > MAX_PAGE_NUMBER_SIZE) return  false;
        assert(_firstIndex < _line->items().size() && _lastIndex <= _line->items().size());
        auto PageNumberType = getItemPageNumberType(_line->items().at(_firstIndex), enuPageNumberType::NotIdentified);
        if(PageNumberType == enuPageNumberType::None)
            return false;
        for(size_t i=_firstIndex; i < _lastIndex; ++i){
            const auto& Item = _line->items().at(i);
            if(getItemPageNumberType(Item, PageNumberType) == enuPageNumberType::None)
                return false;
        }
        return true;
    };

    PdfLineSegmentPtr_t MostBottomStripe;
    PdfLineSegmentPtr_t MostTopStripe;
    PdfLineSegmentPtr_t LastProcessedStripe, BeforeLastProcessedStripe;
    float SumGapBetweenStripes = 0;
    float CountGapBetweenStripes = 0;
    bool LastStripWasFooter = false;
    float Gap = -2.f, PevGap = -2.f;

    for(const auto& Stripe : _page.Stripes) {
        if(Stripe->type() == enuPdfItemType::Char && LastProcessedStripe.get()){
            Gap = LastProcessedStripe->vertOverlap(Stripe);
            if(Gap < MIN_ITEM_SIZE && Gap > -30.f){
                SumGapBetweenStripes -= Gap;
                ++CountGapBetweenStripes;
            }
            if(BeforeLastProcessedStripe.get())
                PevGap = LastProcessedStripe->vertOverlap(BeforeLastProcessedStripe);
        }

        if(Stripe->y0() > (1 - HEADER_FOOTER_PAGE_MARGIN_THRESHOLD) * _pageSize.Height){
            if(this->Configs->AssumeLastAloneLineAsFooter
               && (MostBottomStripe.get() == nullptr || MostBottomStripe->y0() < Stripe->y0()))
                MostBottomStripe = Stripe;
        } else if(Stripe->y1() < HEADER_FOOTER_PAGE_MARGIN_THRESHOLD * _pageSize.Height){
            if(this->Configs->AssumeFirstAloneLineAsHeader
               && (MostTopStripe.get() == nullptr || MostTopStripe->y0() > Stripe->y0()))
                MostTopStripe = Stripe;
        }else {
            BeforeLastProcessedStripe = LastProcessedStripe;
            LastProcessedStripe = Stripe;
            continue;
        }

        if(Stripe->type() != enuPdfItemType::Char){
            BeforeLastProcessedStripe = LastProcessedStripe;
            LastProcessedStripe = Stripe;
            continue;
        }

        //@note To prevent repeated table captions to be identified as footer
        if(LastStripWasFooter == false
           && Stripe->y0() > (1 - HEADER_FOOTER_PAGE_MARGIN_THRESHOLD) * _pageSize.Height
           && (
               (LastProcessedStripe->isHorizontalRuler() && PevGap > -2.f)
               || (LastProcessedStripe->isHorizontalRuler() == false && Gap > -2.f)
               )
           ) {
            BeforeLastProcessedStripe = LastProcessedStripe;
            LastProcessedStripe = Stripe;
            continue;
        }

        LastProcessedStripe = Stripe;

        int32_t RepetitionID = std::numeric_limits<int32_t>::max();
        size_t FirstIndex = Stripe->items().size(), LastIndex = 0;

        auto PageNumberType = enuPageNumberType::NotIdentified;
        size_t CountNotRepeated = 0;
        std::wstring ToBeComparedString;
        std::wstring SkippedNumbers;
        for(size_t i = 0; i < Stripe->items().size(); ++i) {
            auto Item = Stripe->items().at(i);
            if((PageNumberType = getItemPageNumberType(Item, PageNumberType)) != enuPageNumberType::None) {
                if(FirstIndex == Stripe->items().size() || LastIndex <= i){
                    if(Item->isRepeated())
                        SkippedNumbers += Item->as<clsPdfChar>()->code();
                    continue;
                }
            }
            if(SkippedNumbers.size() > MAX_PAGE_NUMBER_SIZE || FirstIndex < Stripe->items().size())
                ToBeComparedString+=SkippedNumbers;

            SkippedNumbers.clear();

            if(Item->isRepeated() == false) {
                /*FirstIndex = LastIndex = 0;
                break;*/
                ++CountNotRepeated;
                continue;
            }

            PageNumberType = enuPageNumberType::NotIdentified;

            RepetitionID = Item->repetitionPageOffset();
            if(FirstIndex == Stripe->items().size())
                FirstIndex = i;
            LastIndex = i + 1;
            ToBeComparedString += Item->as<clsPdfChar>()->code();
        }

        if(
           RepetitionID == std::numeric_limits<int32_t>::max()
           || ToBeComparedString.size() < 1.5 * CountNotRepeated
           || (CountNotRepeated > 3 && ToBeComparedString.size() < 6)
           || LastIndex - FirstIndex < 2 ||
           (
               Stripe->items().at(FirstIndex)->isHeader() == false
               && Stripe->items().at(FirstIndex)->isFooter() == false
               && Stripe->items().at(FirstIndex)->isSidebar() == false
               )
           )
            continue;

        if(isTableCaption(ToBeComparedString))
            continue;

        std::set<int> RepeatedPageIndices;
        for(size_t i = FirstIndex; i < LastIndex; ++i){
            const auto& Item = Stripe->items().at(i);
            if(Item->isRepeated())
                RepeatedPageIndices.insert(_pageIndex + Item->repetitionPageOffset());
        }

        bool Found = false;
        bool SkipHeader = false;
        for(auto PageIndex : RepeatedPageIndices){

            auto OtherPage = this->getPreprocessedPage(PageIndex);
            for(auto OtherPageStripe : OtherPage.Stripes){
                if(OtherPageStripe->type() != enuPdfItemType::Char)
                    continue;
                if(SkipHeader && OtherPageStripe->y0() < HEADER_FOOTER_PAGE_MARGIN_THRESHOLD * _pageSize.Height)
                    continue;
                if(OtherPageStripe->y0() - OtherPage.Margins.Top > Stripe->y0() + _page.Margins.Bottom + 1.f)
                    break;
                if(isTableCaption(OtherPageStripe->contentString())) {
                    SkipHeader = true;
                    continue;
                }

                if(
                   std::abs((OtherPageStripe->y0() + OtherPage.Margins.Bottom) - (Stripe->y0() + _page.Margins.Bottom)) < .1f
                   || std::abs((OtherPageStripe->y0() + OtherPage.Margins.Bottom) - (Stripe->y0() - _page.Margins.Top)) < .1f
                   || std::abs((OtherPageStripe->y0() - OtherPage.Margins.Top) - (Stripe->y0() + _page.Margins.Bottom)) < .1f
                   || std::abs((OtherPageStripe->y0() - OtherPage.Margins.Top) - (Stripe->y0() - _page.Margins.Top)) < .1f
                   || std::abs(OtherPageStripe->y0() - Stripe->y0()) < .1f
                   ) {
                    size_t MatchedCount = 0, AlphabetCount = 0;
                    size_t NotMatchecdAlphabets = 0, SkippedNonRepeated = 0;
                    size_t Index = 0, LastIndex = 0;;
                    size_t FirstMatchedIndex = OtherPageStripe->items().size();
                    PageNumberType = enuPageNumberType::NotIdentified;
                    for(LastIndex=0; LastIndex< OtherPageStripe->items().size(); ++LastIndex){
                        const auto& Item = OtherPageStripe->items().at(LastIndex);


                        auto A = static_cast<wchar_t>(ToBeComparedString.at(Index));
                        auto B = static_cast<wchar_t>(Item->as<clsPdfChar>()->code());

                        if(
                           (Item->as<clsPdfChar>()->code() >= 'a' && Item->as<clsPdfChar>()->code() <= 'z')
                           || (Item->as<clsPdfChar>()->code() >= 'A' && Item->as<clsPdfChar>()->code() <= 'Z')
                           )
                            ++AlphabetCount;

                        if(Item->isRepeated() == false || Item->repetitionPageOffset() == 0) {
                            ++SkippedNonRepeated;
                            continue;
                        }

                        if(ToBeComparedString.at(Index) == Item->as<clsPdfChar>()->code()) {
                            ++MatchedCount;
                            if(FirstMatchedIndex == OtherPageStripe->items().size())
                                FirstMatchedIndex = LastIndex;
                            ++Index;
                            if(Index >= ToBeComparedString.size())
                                break;
                            else
                                continue;
                        }

                        if((PageNumberType = getItemPageNumberType(Item, PageNumberType)) != enuPageNumberType::None){
                            if(FirstMatchedIndex == OtherPageStripe->items().size())
                                continue;
                        }

                        PageNumberType = enuPageNumberType::NotIdentified;
                        ++NotMatchecdAlphabets;
                    }

                    if(MatchedCount > 3 && AlphabetCount > 3 && MatchedCount > 1.5 * NotMatchecdAlphabets) {
                        if(
                           (FirstMatchedIndex == 0 || canBePageNumber(OtherPageStripe, 0, FirstMatchedIndex))
                            && (
                               FirstMatchedIndex + Index + SkippedNonRepeated >= OtherPageStripe->size()
                               || canBePageNumber(OtherPageStripe, FirstMatchedIndex + SkippedNonRepeated + Index, OtherPageStripe->items().size())
                            )) {
                            Found = true;
                            break;
                        }
                    }
                }
            }
            if(Found)
                break;
        }

        if(Found == false)
            continue;

        if(Stripe->items().at(FirstIndex)->isHeader()) {
            HeaderBottom = std::max(Stripe->y1(), HeaderBottom);
        } else if (Stripe->items().at(FirstIndex)->isFooter()){
            FooterTop = std::min(Stripe->y0(), FooterTop);
            LastStripWasFooter = true;
        }
    }

    float MinGapToIdentifyAsAlone = CountGapBetweenStripes > 0 ? roundf(SumGapBetweenStripes / CountGapBetweenStripes) + 1.f : 6.f;

    if(this->Configs->AssumeFirstAloneLineAsHeader && MostTopStripe.get()){
        PdfLineSegmentPtr_t NearsetSibling, NearestNonLineSibling;
        if(isTableCaption(MostTopStripe->contentString()) == false
           && (MostTopStripe->isPureImage() == false || MostTopStripe->isHorizontalRuler())
           ) {
            for(const auto& Stripe : _page.Stripes){
                if(Stripe.get() == MostTopStripe.get())
                    continue;
                if(NearsetSibling.get() == nullptr || NearsetSibling->y0() > Stripe->y0())
                    NearsetSibling = Stripe;
                if(Stripe->isHorizontalRuler() == false && (NearestNonLineSibling.get() == nullptr || NearestNonLineSibling->y0() > Stripe->y0()))
                    NearestNonLineSibling = Stripe;
            }

            if(NearsetSibling) {
                if(NearsetSibling->isHorizontalRuler() == false && roundf(NearsetSibling->y0() - MostTopStripe->y1()) > MinGapToIdentifyAsAlone)
                    HeaderBottom = std::max(HeaderBottom, MostTopStripe->y1());
                else if(NearsetSibling->isHorizontalRuler()){
                    if(NearestNonLineSibling.get() && roundf(NearestNonLineSibling->y0() - NearsetSibling->y1()) > MinGapToIdentifyAsAlone) {
                        if(NearestNonLineSibling->y0() - NearsetSibling->y1() < 1.5f * (NearsetSibling->y0() - MostTopStripe->y1()))
                            HeaderBottom = std::max(HeaderBottom, MostTopStripe->y1());
                        else
                            HeaderBottom = std::max(HeaderBottom, NearsetSibling->y1());
                    }
                }
            }
        }
    }

    if(this->Configs->AssumeLastAloneLineAsFooter && MostBottomStripe.get()){
        PdfLineSegmentPtr_t NearsetSibling, NearestNonLineSibling;
        if(isImageCaption(MostBottomStripe->contentString()) == false
           && (MostBottomStripe->isPureImage() == false || MostBottomStripe->isHorizontalRuler())
           ) {
            for(const auto& Stripe : _page.Stripes){
                if(Stripe.get() == MostBottomStripe.get())
                    continue;
                if(NearsetSibling.get() == nullptr || NearsetSibling->y1() < Stripe->y1())
                    NearsetSibling = Stripe;
                if(Stripe->isHorizontalRuler() == false && (NearestNonLineSibling.get() == nullptr || NearestNonLineSibling->y1() < Stripe->y1()))
                    NearestNonLineSibling = Stripe;

            }

            if(NearsetSibling) {
                if(NearsetSibling->isHorizontalRuler() == false && roundf(MostBottomStripe->y0() - NearsetSibling->y1()) > MinGapToIdentifyAsAlone)
                    FooterTop = std::min(FooterTop, MostBottomStripe->y0());
                else if(NearsetSibling->isHorizontalRuler()){
                    if(NearestNonLineSibling.get() && roundf(NearsetSibling->y0() - NearestNonLineSibling->y1()) > MinGapToIdentifyAsAlone){
                        if(NearsetSibling->y0() - NearestNonLineSibling->y1() < 1.5f * (MostBottomStripe->y0() - NearsetSibling->y1()))
                            FooterTop = std::min(FooterTop, MostBottomStripe->y0());
                        else
                            FooterTop = std::min(FooterTop, NearsetSibling->y0());
                    }
                }
            }
        }
    }

    for (auto& Stripe : _page.Stripes){
        if(Stripe->y1() - 5.f <= HeaderBottom) {
            debugShow("HEADER >>> " << (Stripe->isHorizontalRuler() ? L"HORIZONTAL_RULER" : Stripe->contentString()));
            Stripe->markAsRepeated(0);
            Stripe->markAsHeader(true);
        } else if(Stripe->y0() + 5.f >= FooterTop) {
            debugShow("FOOTER >>> " << (Stripe->isHorizontalRuler() ? L"HORIZONTAL_RULER" : Stripe->contentString()));
            Stripe->markAsRepeated(0);
            Stripe->markAsFooter(true);
        }
    }

    for (auto& Stripe : _page.Stripes){
        if (Stripe->y1() < HEADER_FOOTER_PAGE_MARGIN_THRESHOLD * _pageSize.Height){
            if(canBePageNumber(Stripe, 0, Stripe->items().size())/* ||
               (Stripe->isHorizontalRuler() && Stripe->width() > .75f * _pageSize.Width)*/) {
                bool Approved = true;
                for(const auto& OtherStripe : _page.Stripes){
                    if(OtherStripe.get() == Stripe.get()) continue;
                    if(roundf(OtherStripe->y0() - Stripe->y1()) < MinGapToIdentifyAsAlone ||
                       OtherStripe->isHeader()) continue;
                    if(OtherStripe->y1() > HeaderBottom){
                        Approved = false;
                        break;
                    }
                }
                if(Approved){
                    Stripe->markAsRepeated(0);
                    Stripe->markAsHeader(true);
                    debugShow("HEADER >>> " << (Stripe->isHorizontalRuler() ? L"HORIZONTAL_RULER" : Stripe->contentString()));
                }
            }
        }else if (Stripe->y0() > (1 - HEADER_FOOTER_PAGE_MARGIN_THRESHOLD) * _pageSize.Height){
            if(canBePageNumber(Stripe, 0, Stripe->items().size()) /*||
               (Stripe->isHorizontalRuler() && Stripe->width() > .75f * _pageSize.Width)*/) {
                bool Approved = true;
                for(const auto& OtherStripe : _page.Stripes){
                    if(OtherStripe.get() == Stripe.get()) continue;
                    if(roundf(Stripe->y0() - OtherStripe->y1()) < MinGapToIdentifyAsAlone ||
                       OtherStripe->isFooter()) continue;
                    if(OtherStripe->y1() < FooterTop){
                        Approved = false;
                        break;
                    }
                }
                if(Approved) {
                    debugShow("FOOTER >>> " << (Stripe->isHorizontalRuler() ? L"HORIZONTAL_RULER" : Stripe->contentString()));
                    Stripe->markAsRepeated(0);
                    Stripe->markAsFooter(true);
                }
            }
        }
    }

    return std::make_tuple(HeaderBottom, FooterTop);
}

void clsLayoutAnalyzer::markSidebars(int _pageIndex, PdfLineSegmentPtrVector_t &_lines, const stuPageMargins& _pageMargins, const stuSize &_pageSize) const
{
    using namespace LA;
    (void)_pageIndex;
    (void)_pageMargins;

    float SidebarLeft = 0, SidebarRight = _pageSize.Width - _pageMargins.Right - 2.f;
    size_t CountLeft = 0, CountViolateLeft = 0, CountRight = 0, CountViolateRight = 0;
    const float LEFT_MARGIN = SIDEBAR_PAGE_MARGIN_THRESHOLD * _pageSize.Width;
    const float RIGHT_MARGIN = (1-SIDEBAR_PAGE_MARGIN_THRESHOLD) * _pageSize.Width;
    for(const auto& Line : _lines){
        if(Line->x1() < LEFT_MARGIN) {
            SidebarLeft = std::max(SidebarLeft, Line->x1());
            CountLeft++;
        } else if(Line->x0() > RIGHT_MARGIN){
            SidebarRight = std::min(SidebarRight, Line->x0());
            CountRight++;
        } else if(Line->x0() < SidebarLeft && Line->x1() > SidebarLeft)
            CountViolateLeft++;
        else if(Line->x0() < SidebarRight && Line->x1() > SidebarRight)
            CountViolateRight++;
    }

    if(CountLeft > 3 && CountLeft > 5 * CountViolateLeft)
        for(const auto& Line : _lines)
            if(Line->x1() < LEFT_MARGIN)
                Line->markAsSidebar(true);
    if(CountRight > 3 && CountRight > 5 * CountViolateRight)
        for(const auto& Line : _lines)
            if(Line->x0() > RIGHT_MARGIN)
                Line->markAsSidebar(true);
}

void clsLayoutAnalyzer::clearCache()
{
    this->CachedPreprocessedPage.clear();
    this->CachedProcessedPage.clear();
}

}
}
}
