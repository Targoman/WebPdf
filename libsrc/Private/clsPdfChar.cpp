#include "clsPdfChar.h"

namespace Targoman {
namespace PDF {
namespace Private {

constexpr uint64_t MASK_ITALIC = 0x0000000000000001;
constexpr uint64_t MASK_VIRTUAL = 0x0000000000000002;

class clsPdfCharData {
public:
    wchar_t Code;
    float Offset, Advance;
    float Baseline;
    void* Font;
    float FontSize;
    float Angle;
    uint64_t Flags;

    clsPdfCharData(wchar_t _code, float _offset, float _advance, float _baseline, void* _font, float _fontSize, float _angle) :
        Code(_code),
        Offset(_offset),
        Advance(_advance),
        Baseline(_baseline),
        Font(_font),
        FontSize(_fontSize),
        Angle(_angle),
        Flags(0)
    {}

    FLAG_PROPERTY(Italic, MASK_ITALIC)
    FLAG_PROPERTY(Virtual, MASK_VIRTUAL)
};

void clsPdfChar::detach()
{
    this->detachPdfItemData();
    this->Data.detach();
}

clsPdfChar::~clsPdfChar()
{ }

clsPdfChar::clsPdfChar(float _left,
                       float _top,
                       float _right,
                       float _bottom,
                       float _ascent,
                       float _descent,
                       float _offset,
                       float _advance,
                       wchar_t _code,
                       float _baseline,
                       void *_font,
                       float _fontSize,
                       float _angle) :
    clsPdfItem(_offset, _top, _advance, _bottom, enuPdfItemType::Char),
    Data(new clsPdfCharData(_code, _offset, _advance, _baseline, _font, _fontSize, _angle))
{
    (void)_left;
    (void)_right;

    this->setY0(_ascent);
    this->setY1(_descent);
}

void clsPdfChar::markAsItalic(bool _state)
{
    this->Data->markAsItalic(_state);
}

bool clsPdfChar::isItalic() const
{
    return this->Data->isItalic();
}

void clsPdfChar::markAsVirtual(bool _state)
{
    this->Data->markAsVirtual(_state);
}

bool clsPdfChar::isVirtual() const
{
    return this->Data->isVirtual();
}

wchar_t clsPdfChar::code() const
{
    return this->Data->Code;
}

float clsPdfChar::baseline() const
{
    return this->Data->Baseline;
}

float clsPdfChar::offset() const
{
    return this->Data->Offset;
}

float clsPdfChar::advance() const
{
    return this->Data->Advance;
}

void *clsPdfChar::font() const
{
    return this->Data->Font;
}

float clsPdfChar::fontSize() const
{
    return this->Data->FontSize;
}

float clsPdfChar::angle() const
{
    return this->Data->Angle;
}

bool clsPdfChar::isSame(const stuPageMargins &_currItemPageMargins, const PdfItemPtr_t &_other, const stuPageMargins &_otherItemPageMargins) const
{
    return  _other->type() == enuPdfItemType::Char &&
            _other->as<clsPdfChar>()->code() == this->code() &&
            clsPdfItem::isSame(_currItemPageMargins, _other, _otherItemPageMargins);

}

bool clsPdfChar::conformsToLine(float _ascent, float _descent, float _baseline) const
{
    return std::abs(this->Data->Baseline - _baseline) < 0.3f || clsPdfItem::conformsToLine(_ascent, _descent, _baseline);
}

}
}
}
