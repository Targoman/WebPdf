#include "clsPdfLineSegment.h"
#include <map>
#include "common.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <fpdfapi/fpdfapi.h>
#include <fpdfapi/fpdf_module.h>
#include <fpdfapi/fpdf_page.h>
#include <fxcodec/fx_codec.h>
#include <fxcrt/fx_coordinates.h>
#pragma GCC diagnostic pop

namespace Targoman {
namespace PDF {
namespace Private {

class clsPdfLineSegmentData {
public:
    float Baseline;
    enuLineSegmentAssociation Association;
    PdfLineSegmentPtr_t Relative;
    PdfItemPtrVector_t Items;
    int16_t PageIndex;
    int16_t ParIndex;
    int16_t SntIndex;
    uint8_t __PADDING__[2];
    enuListType ListType;
    float TextLeft;

    clsPdfLineSegmentData() :
        Baseline(NAN),
        Association(enuLineSegmentAssociation::None),
        PageIndex(-1),
        ParIndex(-1),
        SntIndex(-1),
        ListType(enuListType::None),
        TextLeft(0)
    {}
};

void clsPdfLineSegment::detach()
{
    this->detachPdfItemData();
    this->Data.detach();
}

float clsPdfLineSegment::findBaseline() const
{
    std::map<int, int> BaselineHistogram;
    for (const auto &Item : this->Data->Items) {
        if (Item->type() == enuPdfItemType::Char) {
            const int FloatKey = static_cast<int>(Item->as<clsPdfChar>()->baseline() * 1000.f + 0.5f);
            const auto Iterator = BaselineHistogram.find(FloatKey);
            if (Iterator == BaselineHistogram.end())
                BaselineHistogram[FloatKey] = 1;
            else
                ++(Iterator->second);
        }
    }
    int FloatKey = 0, Count = 0;
    for (const auto &Item : BaselineHistogram)
        if (Item.second > Count) {
            FloatKey = Item.first;
            Count = Item.second;
        }
    if(Count == 0)
        return -1;
    return FloatKey / 1000.0f;
}

void clsPdfLineSegment::setLineSpecs(int16_t _pageIndex, int16_t _parIndex, int16_t _sntIndex)
{
    this->Data->PageIndex = _pageIndex;
    this->Data->ParIndex = _parIndex;
    this->Data->SntIndex = _sntIndex;
}


clsPdfLineSegment::clsPdfLineSegment() :
    clsPdfItem(),
    Data(new clsPdfLineSegmentData)
{
}

clsPdfLineSegment::~clsPdfLineSegment()
{ }

bool clsPdfLineSegment::isValid() const
{
    return this->Data->Items.size() > 0;
}

float clsPdfLineSegment::baseline() const
{
    if(std::isnan(this->Data->Baseline)) {
        float Baseline = this->findBaseline();
        if(Baseline < 0)
            return this->centerY();
        const_cast<clsPdfLineSegment*>(this)->Data->Baseline = Baseline;
    }
    return this->Data->Baseline;
}

int16_t clsPdfLineSegment::pageIndex() const
{
    return this->Data->PageIndex;
}

int16_t clsPdfLineSegment::parIndex() const
{
    return this->Data->ParIndex;
}

int16_t clsPdfLineSegment::sntIndex() const
{
    return this->Data->SntIndex;
}

bool clsPdfLineSegment::isHorizontalRuler() const
{
    if(clsPdfItem::isHorizontalRuler())
        return true;
    if(this->items().size() > 5) {
        if(this->items().front()->type() != enuPdfItemType::Char)
            return false;
        auto LastChar = this->items().front()->as<clsPdfChar>()->code();
        if(LastChar != '-' && LastChar != '_')
            return false;
        for(const auto& Item : this->items())
            if(Item->type() != enuPdfItemType::Char ||
               Item->as<clsPdfChar>()->code() != LastChar)
                return false;
        return true;
    }
    return false;
}

bool clsPdfLineSegment::isSameAs(const stuPageMargins& _currPageMargins, const clsPdfLineSegment &_other, const stuPageMargins& _otherPageMargins) const
{
    auto approximatelySame = [] (const std::wstring& _s1, const std::wstring& _s2){
        size_t i;
        for(i = 0; i < std::min(_s1.size(), _s2.size()); ++i) {
            if(_s1[i] >= L'0' && _s1[i] <= L'9')
                continue;
            if(_s2[i] >= L'0' && _s2[i] <= L'9')
                continue;
            if(_s1[i] != _s2[i])
                break;
        }
        return 10 * i  > 9 * std::max(_s1.size(), _s2.size());
    };

    clsPdfItem OtherItem(_other.x0(), _other.y0(), _other.x1(), _other.y1(), enuPdfItemType::None);
    if(clsPdfItem::isSame(_currPageMargins, OtherItem, _otherPageMargins) == false)
        return false;

    auto CurrString = this->contentString();
    auto OtherString = _other.contentString();
    if(CurrString.size() < 5 || OtherString.size() < 5)
        return false;

    return approximatelySame(CurrString, OtherString) || approximatelySame(OtherString, CurrString);
}

bool clsPdfLineSegment::isSameAs(const stuPageMargins& _currPageMargins, const PdfLineSegmentPtr_t &_other, const stuPageMargins& _otherPageMargins) const
{
    return this->isSameAs(_currPageMargins, *_other, _otherPageMargins);
}

void clsPdfLineSegment::setBulletAndNumberingInfo(const stuConfigsPtr_t& _configs,
                                                  float _textLeft,
                                                  enuListType _listType,
                                                  size_t _listIdStart,
                                                  size_t _listIdEnd)
{
    auto appendListIdentifierMark = [&] (size_t i, bool _left) {
        if(_configs->MarkBullets == false && _listType == enuListType::Bulleted) return ;
        if(_configs->MarkNumberings == false && _listType == enuListType::Numbered) return ;

        const auto& Item = this->Data->Items.at(i);
        PdfItemPtr_t Identifier(new clsPdfChar(
                                    _left ? Item->x0() : Item->x1(),
                                    this->y0(),
                                    _left ? Item->x0() : Item->x1(),
                                    this->y1(),
                                    this->y0(),
                                    this->y1(),
                                    _left ? Item->x0() : Item->x1(),
                                    _left ? Item->x0() : Item->x1(),
                                    _left ? LIST_START : LIST_END,
                                    this->baseline(),
                                    nullptr,
                                    0,
                                    0
                                    ));
        Identifier->as<clsPdfChar>()->markAsVirtual(true);
        if(_left)
            this->Data->Items.insert(this->Data->Items.begin() + static_cast<int>(i), Identifier);
        else
            this->Data->Items.insert(this->Data->Items.begin() + static_cast<int>(i + 1), Identifier);
    };

    if(_listType != enuListType::None && _listIdStart < this->Data->Items.size() && _listIdEnd < this->Data->Items.size()) {
        appendListIdentifierMark(_listIdStart, true);
        appendListIdentifierMark(_listIdEnd, false);
    }

    this->Data->TextLeft = _textLeft;
    this->Data->ListType = _listType;
}

float clsPdfLineSegment::textLeft() const
{
    if(this->Data->ListType == enuListType::None)
        return this->x0();
    return this->Data->TextLeft;
}

enuListType clsPdfLineSegment::listType() const
{
    return this->Data->ListType;
}

PdfLineSegmentPtr_t clsPdfLineSegment::relative() const
{
    return this->Data->Relative;
}

enuLineSegmentAssociation clsPdfLineSegment::association() const
{
    return this->Data->Association;
}

void clsPdfLineSegment::setRelativeAndAssociation(const PdfLineSegmentPtr_t &_relative, enuLineSegmentAssociation _association)
{
    this->Data->Relative = _relative;
    this->Data->Association = _association;
}

std::wstring clsPdfLineSegment::contentString() const
{
    std::wstring Result;
    for(const auto& Item : this->Data->Items)
        if(Item->type() == enuPdfItemType::Char)
            Result += Item->as<clsPdfChar>()->code();
        else if(Item->y0() < this->baseline() && Item->y1() > this->baseline())
            Result += OBJECT_CHAR;
    return Result;
}

void clsPdfLineSegment::appendPdfItemVector(const PdfItemPtrVector_t &_items)
{
    for(const auto& Item : _items)
        this->appendPdfItem(Item);
}

void clsPdfLineSegment::appendPdfItem(const PdfItemPtr_t &_item)
{
    if(this->Data->Items.size() == 0)
        this->setType(_item->type());
    else if(this->type() != _item->type())
        this->setType(enuPdfItemType::None);
    this->Data->Items.push_back(_item);
    this->unionWith_(_item);
    this->Data->Baseline = NAN;
}

bool clsPdfLineSegment::doesShareFontsWith(const clsPdfLineSegment &_other) const
{
    auto areApproximatelySame = [] (float a, float b) {
        return a < 1.2f * b && b < 1.2f * a;
    };
    bool CharEncountered = false;
    for(const auto& Item : this->Data->Items)
        if(Item->type() == enuPdfItemType::Char && Item->as<clsPdfChar>()->isAlphaNumeric())
            for(const auto& OtherItem : _other.Data->Items)
                if(OtherItem->type() == enuPdfItemType::Char && OtherItem->as<clsPdfChar>()->isAlphaNumeric()) {
                    CharEncountered = true;
                    if(
                       Item->as<clsPdfChar>()->font() != nullptr &&
                       Item->as<clsPdfChar>()->font() == OtherItem->as<clsPdfChar>()->font() &&
                       areApproximatelySame(Item->as<clsPdfChar>()->fontSize(), OtherItem->as<clsPdfChar>()->fontSize())
                    )
                       return true;
                }
    return CharEncountered == false;
}

bool clsPdfLineSegment::doesShareFontsWith(const PdfLineSegmentPtr_t &_other) const
{
    return this->doesShareFontsWith(*_other);
}

bool clsPdfLineSegment::mergeWith_(const clsPdfLineSegment &_other)
{
    if(this == &_other)
        return true;
    if(
       !this->isValid() ||
       this->vertOverlap(_other) > MIN_ITEM_SIZE
    ) {
        this->appendPdfItemVector(_other.Data->Items);
        sort(this->Data->Items, [](const PdfItemPtr_t& _a, const PdfItemPtr_t& _b){return _a->x0() < _b->x0();});
        return true;
    }
    return false;
}

bool clsPdfLineSegment::mergeWith_(const PdfLineSegmentPtr_t &_other)
{
    return this->mergeWith_(*_other);
}

bool clsPdfLineSegment::isPureImage() const
{
    if(this->type() == enuPdfItemType::None) {
        for(const auto& Item : this->items())
            if(Item->type() == enuPdfItemType::Char)
                return false;
        return true;
    } else
        return this->type() != enuPdfItemType::Char;
}

bool clsPdfLineSegment::isBackground() const
{
    return this->type() == enuPdfItemType::Background;
}

void clsPdfLineSegment::markAsBackground()
{
    this->setType(enuPdfItemType::Background);
}

float clsPdfLineSegment::mostUsedFontSize() const
{
    if(this->isPureImage()) return -1;
    std::map<float, int> UsedFonts;
    for(const auto& Item : this->Data->Items)
        if(Item->type() == enuPdfItemType::Char) {
            if(UsedFonts.find(Item->as<clsPdfChar>()->fontSize()) != UsedFonts.end()) {
                UsedFonts[Item->as<clsPdfChar>()->fontSize()] = UsedFonts[Item->as<clsPdfChar>()->fontSize()] + 1;
             } else {
                UsedFonts[Item->as<clsPdfChar>()->fontSize()] = 1;
             }
        }

    float MostUsedFontSize = -1;
    int Count = 0;
    for (auto UsedFontIter = UsedFonts.begin();
         UsedFontIter != UsedFonts.end();
         ++UsedFontIter) {
        if(MostUsedFontSize < 0 || UsedFontIter->second > Count) {
            Count = UsedFontIter->second;
            MostUsedFontSize = UsedFontIter->first;
        }
    }

    return MostUsedFontSize;
}

size_t clsPdfLineSegment::size() const
{
    return this->Data->Items.size();
}

const PdfItemPtrVector_t &clsPdfLineSegment::items() const
{
    return this->Data->Items;
}

const PdfItemPtr_t &clsPdfLineSegment::at(size_t _index) const
{
    return this->Data->Items[_index];
}

const PdfItemPtr_t &clsPdfLineSegment::operator[](size_t _index) const
{
    return this->at(_index);
}

PdfItemPtrVector_t::const_iterator clsPdfLineSegment::begin() const
{
    return this->Data->Items.begin();
}

PdfItemPtrVector_t::const_iterator clsPdfLineSegment::end() const
{
    return this->Data->Items.end();
}

struct stuLigature {
    wchar_t Code;
    const wchar_t* Value;
};

static std::map<wchar_t, const std::wstring> LIGATURES = {
    { L'Ꜳ', L"AA" },
    { L'ꜳ', L"aa" },
    { L'Æ', L"AE" },
    { L'æ', L"ae" },
    { L'ꬱ', L"aə" },
    { L'Ꜵ', L"AO" },
    { L'ꜵ', L"ao" },
    { L'Ꜷ', L"AU" },
    { L'ꜷ', L"au" },
    { L'Ꜹ', L"AV" },
    { L'ꜹ', L"av" },
    { L'Ꜻ', L"AV" },
    { L'ꜻ', L"av" },
    { L'Ꜽ', L"AY" },
    { L'ꜽ', L"ay" },
    { L'🙰', L"et" },
    { L'ꭁ', L"əø" },
    { L'ﬀ', L"ff" },
    { L'ﬃ', L"ffi" },
    { L'ﬄ', L"ffl" },
    { L'ﬁ', L"fi" },
    { L'ﬂ', L"fl" },
    { L'Ƕ', L"Hv" },
    { L'ƕ', L"hv" },
    { L'℔', L"lb" },
    { L'Ỻ', L"lL" },
    { L'ỻ', L"ll" },
    { L'Œ', L"OE" },
    { L'œ', L"oe" },
    { L'Ꝏ', L"OO" },
    { L'ꝏ', L"oo" },
    { L'ꭢ', L"ɔe" },
    { L'ẞ', L"ſs" },
    { L'ß', L"ſz" },
    { L'ﬆ', L"st" },
    { L'ﬅ', L"ſt" },
    { L'Ꜩ', L"TZ" },
    { L'ꜩ', L"tz" },
    { L'ᵫ', L"ue" },
    { L'ꭣ', L"uo" },
    { L'Ꝡ', L"VY" },
    { L'ꝡ', L"vy" }
};

float getGlyphBoxAdvance(const clsPdfChar* _char, const FontInfo_t& _fontInfo) {
    if(_fontInfo.find(_char->font()) == _fontInfo.end())
        return (_char->advance() - _char->offset()) * 1000.f / _char->fontSize();
    return std::get<1>(_fontInfo.at(_char->font())->getGlyphBoxInfo(_char->code())).right;
}

size_t numSpacesInBetween(const PdfItemPtr_t& _prev, const clsPdfChar* _current, const FontInfo_t& _fontInfo) {
    if(_current->isSpace())
        return 0;
    if(_prev->type() == enuPdfItemType::Char) {
        auto PrevChar = _prev->as<const clsPdfChar>(), CurrChar = _current;
        if(PrevChar->isSpace())
            return 0;
        auto PrevBoxAdvance = getGlyphBoxAdvance(PrevChar, _fontInfo);
        auto CurrBoxAdvance = getGlyphBoxAdvance(CurrChar, _fontInfo);
        float DeltaX = CurrChar->offset() - PrevChar->advance();

        if(DeltaX <= 0)
            return 0;
        float SpaceWidthThreshold = std::max(PrevBoxAdvance, CurrBoxAdvance);
        if(SpaceWidthThreshold < 400.f)
            SpaceWidthThreshold /= 2.f;
        else if(SpaceWidthThreshold < 700.f)
            SpaceWidthThreshold /= 4.f;
        else if(SpaceWidthThreshold < 800.f)
            SpaceWidthThreshold /= 5.f;
        else
            SpaceWidthThreshold /= 6.f;
        if(
           (SpaceWidthThreshold > 1487.9f && SpaceWidthThreshold < 1488.1f) ||
           (SpaceWidthThreshold > 1389.99f && SpaceWidthThreshold < 1390.01f)
        )
            SpaceWidthThreshold *= 1.5f;
        if(PrevBoxAdvance > CurrBoxAdvance)
            SpaceWidthThreshold *= PrevChar->fontSize();
        else
            SpaceWidthThreshold *= CurrChar->fontSize();
        SpaceWidthThreshold /= 1000.f;
        return static_cast<size_t>(DeltaX / SpaceWidthThreshold) > 0 ? 1 : 0;
    } else {
        float DeltaX = _current->x0() - _prev->y1();
        if(DeltaX <= 0)
            return 0;
        return static_cast<size_t>(4.f * DeltaX / _current->height()) > 0 ? 1 : 0;
    }
}

void clsPdfLineSegment::consolidateSpaces(const FontInfo_t& _fontInfo, const stuConfigsPtr_t& _configs)
{
    auto OriginalString = this->contentString();

    constexpr float MAX_BASELINE_DIFF_TO_CHAR_HEIGHT_RATIO = 0.2f;

    this->Data->Baseline = NAN;
    auto Baseline = this->baseline();

    PdfItemPtrVector_t ItemsWithSpaces;
    PdfItemPtrVector_t Items = filter<PdfItemPtr_t>(
        this->Data->Items,
        [] (const PdfItemPtr_t& _item) {
            return _item->type() != enuPdfItemType::Char
                                    || _item->as<clsPdfChar>()->isVirtual() == false
                                    || _item->as<clsPdfChar>()->isSpace() == false;
        }
    );
    sort(Items, [](const PdfItemPtr_t &_a, const PdfItemPtr_t &_b) { return _a->x0() < _b->x0(); });
    this->Data->Items.clear();

    PdfItemPtr_t PrevObject;
    bool PrevCharWasSub= false, PrevCharWasSuper = false;

    for (size_t i = 0; i < Items.size(); ++i) {
        const auto& Item = Items[i];
        const clsPdfChar* Char = nullptr;
        const clsPdfChar* NextChar = nullptr;
        const clsPdfChar* PrevChar = nullptr;

        if(Item->type() == enuPdfItemType::Char)
            Char = Item->as<clsPdfChar>();
        if(i > 0 && Items.at(i - 1)->type() == enuPdfItemType::Char)
            PrevChar = Items.at(i - 1)->as<clsPdfChar>();
        if(i + 1 < Items.size() && Items.at(i + 1)->type() == enuPdfItemType::Char)
            NextChar = Items.at(i + 1)->as<clsPdfChar>();

        auto appendSubSupIdentifier = [&] (wchar_t _code) {
            if(_configs->MarkSubcripts == false && (_code == SUB_SCR_START || _code == SUB_SCR_END)) return ;
            if(_configs->MarkSuperScripts == false && (_code == SUPER_SCR_START || _code == SUPER_SCR_END)) return ;
            PdfItemPtr_t Identifier(new clsPdfChar(
                                        Item->x0(),
                                        this->y0(),
                                        Item->x0(),
                                        this->y1(),
                                        this->y0(),
                                        this->y1(),
                                        Item->x0(),
                                        Item->x0(),
                                        _code,
                                        Baseline,
                                        nullptr,
                                        0,
                                        0
                                        ));
            Identifier->as<clsPdfChar>()->markAsVirtual(true);
            ItemsWithSpaces.push_back(Identifier);
        };

        auto closeSubSup = [&](){
            if(PrevCharWasSub) {
                if(
                   (NextChar == nullptr || NextChar->code() != SUB_SCR_END) &&
                   (Char == nullptr || Char->code() != SUB_SCR_END)
                )
                    appendSubSupIdentifier(SUB_SCR_END);
                PrevCharWasSub = false;
            }
            if(PrevCharWasSuper) {
                if(
                   (NextChar == nullptr || NextChar->code() != SUPER_SCR_END) &&
                   (Char == nullptr || Char->code() != SUPER_SCR_END)
                )
                    appendSubSupIdentifier(SUPER_SCR_END);
                PrevCharWasSuper = false;
            }
        };

        if (Char != nullptr && Char->code() > 0) {
            if (PrevObject.get() != nullptr) {
                size_t NumSpaces = numSpacesInBetween(PrevObject, Char, _fontInfo);
                if(NumSpaces > 0) {
                    closeSubSup();
                    for(size_t i = 0; i < NumSpaces; ++i) {
                        PdfItemPtr_t SpaceItem(new clsPdfChar(
                                                   PrevObject->x1(),
                                                   this->y0(),
                                                   Char->x0(),
                                                   this->y1(),
                                                   this->y0(),
                                                   this->y1(),
                                                   PrevObject->x1(),
                                                   Char->x0(),
                                                   L' ',
                                                   Char->baseline(),
                                                   Char->font(),
                                                   Char->fontSize(),
                                                   Char->angle()
                                                   ));
                        SpaceItem->as<clsPdfChar>()->markAsVirtual(true);
                        ItemsWithSpaces.push_back(SpaceItem);
                    }
                }
            }

            if(Char->canBeSuperOrSubscript() && Char->baseline() < -MAX_BASELINE_DIFF_TO_CHAR_HEIGHT_RATIO * Char->tightHeight() + Baseline) {
                if(PrevCharWasSub) {
                    if(NextChar == nullptr || NextChar->code() != SUB_SCR_END)
                        appendSubSupIdentifier(SUB_SCR_END);
                    PrevCharWasSub = false;
                }
                if(PrevCharWasSuper == false) {
                    if(PrevChar == nullptr || PrevChar->code() != SUPER_SCR_START)
                        appendSubSupIdentifier(SUPER_SCR_START);
                    PrevCharWasSuper = true;
                }
            } else if(Char->canBeSuperOrSubscript() && Char->baseline() > MAX_BASELINE_DIFF_TO_CHAR_HEIGHT_RATIO * Char->tightHeight() + Baseline) {
                if(PrevCharWasSuper) {
                    if(NextChar == nullptr || NextChar->code() != SUPER_SCR_END)
                        appendSubSupIdentifier(SUPER_SCR_END);
                    PrevCharWasSuper = false;
                }
                if(PrevCharWasSub == false) {
                    if(PrevChar == nullptr || PrevChar->code() != SUB_SCR_START)
                        appendSubSupIdentifier(SUB_SCR_START);
                    PrevCharWasSub = true;
                }
            } else
                closeSubSup();

            if(LIGATURES.find(Char->code()) != LIGATURES.end()) {
                const auto& Value = LIGATURES[Char->code()];
                float X0 = Item->x0();
                float WidthPerChar = Item->width() / Value.size();
                for(size_t i = 0; i < Value.size(); ++i) {
                    PdfItemPtr_t LigItem(new clsPdfChar(
                                               X0,
                                               Char->tightTop(),
                                               X0 + WidthPerChar,
                                               Char->tightBottom(),
                                               Char->y0(),
                                               Char->y1(),
                                               X0,
                                               X0 + WidthPerChar,
                                               Value.at(i),
                                               Char->baseline(),
                                               Char->font(),
                                               Char->fontSize(),
                                                Char->angle()
                                               ));
                    ItemsWithSpaces.push_back(LigItem);
                    X0 += WidthPerChar;
                }
            } else
                ItemsWithSpaces.push_back(Item);
            PrevObject = Item;
        }else {
            closeSubSup();
            if (Item->type() != enuPdfItemType::Char)
                ItemsWithSpaces.push_back(Item);
            PrevObject = Item;
        }

        if(i == Items.size() - 1)
            closeSubSup();
    }

    bool HorizontallyKermed = false;
    bool LastWasSpace = true;
    if(ItemsWithSpaces.size() > 4){
        HorizontallyKermed = true;
        for(const auto& Item: ItemsWithSpaces) {
            if(Item->type() != enuPdfItemType::Char){
                HorizontallyKermed = false;
                break;
            }
            const clsPdfChar* Char = Item->as<clsPdfChar>();
            if(LastWasSpace) {
                if(Char->code() < L'A' || Char->code() > L'Z') {
                    HorizontallyKermed = false;
                    break;
                }
                LastWasSpace = false;
            } else {
                if(Char->code() != L' ') {
                    HorizontallyKermed = false;
                    break;
                }
                LastWasSpace = true;
            }
        }
    }

    if(HorizontallyKermed)
        this->Data->Items = filter<PdfItemPtr_t>(ItemsWithSpaces, [] (const PdfItemPtr_t& _item) { return _item->as<clsPdfChar>()->code() != L' '; });
    else
        this->Data->Items = ItemsWithSpaces;
}

}
}
}
