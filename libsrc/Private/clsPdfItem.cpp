#include "clsPdfItem.h"

#include <limits>

namespace Targoman {
namespace PDF {
namespace Private {

constexpr uint32_t MASK_TYPE = 0x000F;
constexpr uint32_t MASK_WMARK = 0x0010;
constexpr uint32_t MASK_HEADER = 0x0020;
constexpr uint32_t MASK_FOOTER = 0x0040;
constexpr uint32_t MASK_SIDEBAR = 0x0080;
constexpr uint32_t MASK_REPEATED = 0x0200;
constexpr uint32_t MASK_MAIN_CONTENT = 0x0400;


class clsPdfItemData {
public:
    clsPdfItemData(
            float _left,
            float _top,
            float _right,
            float _bottom,
            enuPdfItemType _type) :
        Left(_left),
        Top(_top),
        Right(_right),
        Bottom(_bottom),
        Ascent(_top),
        Descent(_bottom),
        Flags(0)
    {
        this->setType(_type);
    }

    ~clsPdfItemData() {}

    void setType(enuPdfItemType _type) {
        assert((static_cast<uint32_t>(_type) & ~MASK_TYPE) == 0);
        this->Flags = (this->Flags & ~MASK_TYPE) | (static_cast<uint32_t>(_type) & MASK_TYPE);
    }
    enuPdfItemType type() const {
        return static_cast<enuPdfItemType>(this->Flags & MASK_TYPE);
    }

    FLAG_PROPERTY(Watermark, MASK_WMARK)
    FLAG_PROPERTY(Footer, MASK_FOOTER)
    FLAG_PROPERTY(Header, MASK_HEADER)
    FLAG_PROPERTY(Sidebar, MASK_SIDEBAR)
    FLAG_PROPERTY(Repeated, MASK_REPEATED)
    FLAG_PROPERTY(Maincontent, MASK_MAIN_CONTENT)

    void setRepetitionPageOffset(int32_t _offset) {this->RepetitionOffset = _offset;}
    int32_t repetitionPageOffset() const { return this->RepetitionOffset; }

    void resetFlags() {
        this->Flags &= MASK_TYPE;
        this->RepetitionOffset = 0;
    }

public:
    float Left, Top, Right, Bottom, Ascent, Descent;

private:
    uint32_t Flags;
    int32_t  RepetitionOffset;
};

void clsPdfItem::detachPdfItemData()
{
    this->Data.detach();
}

void clsPdfItem::setType(const enuPdfItemType &_type)
{
    this->Data->setType(_type);
}

void clsPdfItem::markAsWatermark(bool _state)
{
    this->Data->markAsWatermark(_state);
}

bool clsPdfItem::isHeader() const
{
    return  this->Data->isHeader();
}

void clsPdfItem::markAsHeader(bool _state)
{
    this->Data->markAsHeader(_state);
}

bool clsPdfItem::isFooter() const
{
    return  this->Data->isFooter();
}

void clsPdfItem::markAsFooter(bool _state)
{
    this->Data->markAsFooter(_state);
}

bool clsPdfItem::isSidebar() const
{
    return this->Data->isSidebar();
}

void clsPdfItem::markAsSidebar(bool _state)
{
    this->Data->markAsSidebar(_state);
}

bool clsPdfItem::isRepeated() const
{
    return  this->Data->isRepeated();
}

void clsPdfItem::resetFlags() {
    this->Data->resetFlags();
}

void clsPdfItem::markAsRepeated(int32_t _pageOffset)
{
    this->Data->markAsRepeated(true);
    this->Data->setRepetitionPageOffset(_pageOffset);
}

bool clsPdfItem::isMainContent() const
{
    return this->Data->isMaincontent();
}

void clsPdfItem::markAsMainContent(bool _state)
{
    this->Data->markAsMaincontent(_state);
}

int32_t clsPdfItem::repetitionPageOffset() const
{
    return  this->Data->repetitionPageOffset();
}

void clsPdfItem::resetWatermarkRepetition() const
{
    this->Data->markAsRepeated(false);
}

void clsPdfItem::detach()
{
    assert(false);
}

clsPdfItem::clsPdfItem() :
    clsPdfItem(
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max(),
        enuPdfItemType::None
        )
{}

clsPdfItem::clsPdfItem(
        float _left,
        float _top,
        float _right,
        float _bottom,
        enuPdfItemType _type) :
    Data(new clsPdfItemData(
             _left, _top, _right, _bottom, _type
             ))
{}

clsPdfItem::~clsPdfItem()
{ }

enuPdfItemType clsPdfItem::type() const
{
    return this->Data->type();
}

bool clsPdfItem::isWatermark() const
{
    return  this->Data->isWatermark();
}

float clsPdfItem::x0() const
{
    return this->Data->Left;
}

float clsPdfItem::y0() const
{
    return this->Data->Ascent;
}

float clsPdfItem::x1() const
{
    return this->Data->Right;
}

float clsPdfItem::y1() const
{
    return this->Data->Descent;
}

int clsPdfItem::x0i() const
{
    return static_cast<int>(this->x0() + 0.5f);
}

int clsPdfItem::y0i() const
{
    return static_cast<int>(this->y0() + 0.5f);
}

int clsPdfItem::x1i() const
{
    return static_cast<int>(this->x1() + 0.5f);
}

int clsPdfItem::y1i() const
{
    return static_cast<int>(this->y1() + 0.5f);
}


constexpr float EFFECT_OF_SIZE_ON_COORD_COEFF = 0.05f;
float clsPdfItem::x0e() const
{
    return this->x0() + EFFECT_OF_SIZE_ON_COORD_COEFF * this->width();
}

float clsPdfItem::y0e() const
{
    return this->y0() + EFFECT_OF_SIZE_ON_COORD_COEFF * this->height();
}

float clsPdfItem::x1e() const
{
    return this->x1() - EFFECT_OF_SIZE_ON_COORD_COEFF * this->width();
}

float clsPdfItem::y1e() const
{
    return this->y1() - EFFECT_OF_SIZE_ON_COORD_COEFF * this->height();
}

void clsPdfItem::setX0(float _value)
{
    this->Data->Left = _value;
}

void clsPdfItem::setY0(float _value)
{
    this->Data->Ascent = _value;
}

void clsPdfItem::setX1(float _value)
{
    this->Data->Right = _value;
}

void clsPdfItem::setY1(float _value)
{
    this->Data->Descent = _value;
}

float clsPdfItem::tightTop() const
{
    return this->Data->Top;
}

float clsPdfItem::tightBottom() const
{
    return this->Data->Bottom;
}

void clsPdfItem::setTightTop(float _value)
{
    this->Data->Top = _value;
}

void clsPdfItem::setTightBottom(float _value)
{
    this->Data->Bottom = _value;
}

float clsPdfItem::width() const
{
    return this->Data->Right - this->Data->Left;
}

float clsPdfItem::height() const
{
    return this->Data->Descent - this->Data->Ascent;
}

float clsPdfItem::tightHeight() const
{
    return this->Data->Bottom - this->Data->Top;
}

float clsPdfItem::centerX() const
{
    return 0.5f * (this->Data->Left + this->Data->Right);
}

float clsPdfItem::centerY() const
{
    return 0.5f * (this->Data->Top + this->Data->Bottom);
}

float clsPdfItem::area() const
{
    return this->width() * this->height();
}

void clsPdfItem::unionWith_(const clsPdfItem &_other)
{
    this->Data->Left = std::min(this->Data->Left, _other.Data->Left);
    this->Data->Top = std::min(this->Data->Top, _other.Data->Top);
    this->Data->Right = std::max(this->Data->Right, _other.Data->Right);
    this->Data->Bottom = std::max(this->Data->Bottom, _other.Data->Bottom);
    this->Data->Ascent = std::min(this->Data->Ascent, _other.Data->Ascent);
    this->Data->Descent = std::max(this->Data->Descent, _other.Data->Descent);
}

void clsPdfItem::unionWith_(const PdfItemPtr_t &_other)
{
    return this->unionWith_(*_other);
}

clsPdfItem clsPdfItem::unionWith(const clsPdfItem &_other) const
{
    clsPdfItem Result(*this);
    Result.Data.detach();
    Result.unionWith_(_other);
    return Result;
}

clsPdfItem clsPdfItem::unionWith(const PdfItemPtr_t &_other) const
{
    return this->unionWith(*_other);
}

void clsPdfItem::intersectWith_(const clsPdfItem &_other)
{
    this->Data->Left = std::max(this->Data->Left, _other.Data->Left);
    this->Data->Top = std::max(this->Data->Top, _other.Data->Top);
    this->Data->Right = std::min(this->Data->Right, _other.Data->Right);
    this->Data->Bottom = std::min(this->Data->Bottom, _other.Data->Bottom);
    this->Data->Ascent = std::max(this->Data->Ascent, _other.Data->Ascent);
    this->Data->Descent = std::min(this->Data->Descent, _other.Data->Descent);
}

void clsPdfItem::intersectWith_(const PdfItemPtr_t &_other)
{
    return this->intersectWith_(*_other);
}

clsPdfItem clsPdfItem::intersectWith(const clsPdfItem &_other) const
{
    clsPdfItem Result(*this);
    Result.Data.detach();
    Result.intersectWith_(_other);
    return Result;
}

clsPdfItem clsPdfItem::intersectWith(const PdfItemPtr_t &_other) const
{
    return this->intersectWith(*_other);
}

bool clsPdfItem::isEmpty() const
{
    return this->Data->Left >= this->Data->Right || this->Data->Top >= this->Data->Bottom;
}

bool clsPdfItem::isVeryThin() const
{
    return this->tightHeight() > 4 * this->width();
}

bool clsPdfItem::isVeryFat() const
{
    return this->width() > 4 *  this->tightHeight();
}

bool clsPdfItem::isVerticalRuler() const
{
    return this->type() != enuPdfItemType::Char && this->width() < MAX_RULER_SIZE && this->height() > 8.f;
}

bool clsPdfItem::isHorizontalRuler() const
{
    return this->type() != enuPdfItemType::Char && this->height() < 2.f * MAX_RULER_SIZE && this->width() > std::max(8.f, 4.f * this->height());
}

bool clsPdfItem::doesContain(const clsPdfItem &_other, float _margin) const
{
    auto Intersection = this->intersectWith(_other);
    if (Intersection.width() < _other.width() + _margin)
        return false;
    if (Intersection.height() < _other.height() + _margin)
        return false;
    return true;
}

bool clsPdfItem::doesContain(const PdfItemPtr_t &_other, float _margin) const
{
    return this->doesContain(*_other, _margin);
}

bool clsPdfItem::doesTouch(const clsPdfItem &_other) const
{
    return this->intersectWith(_other).isEmpty() == false;
}

bool clsPdfItem::doesTouch(const PdfItemPtr_t &_other) const
{
    return this->doesTouch(*_other);
}

bool clsPdfItem::isSame(const stuPageMargins &_currItemPageMargins, const PdfItemPtr_t &_other, const stuPageMargins &_otherItemPageMargins) const
{
    return this->isSame(_currItemPageMargins, *_other, _otherItemPageMargins);
}

bool clsPdfItem::isSame(const stuPageMargins &_currItemPageMargins, const clsPdfItem &_other, const stuPageMargins &_otherItemPageMargins) const
{
    constexpr float MAX_ACCEPTABLE_OFFSET = .5f;
    return (
                (std::abs((this->x0() - _currItemPageMargins.Left) - (_other.x0() - _otherItemPageMargins.Left)) < MAX_ACCEPTABLE_OFFSET &&
                 std::abs((this->x1() - _currItemPageMargins.Left) - (_other.x1() - _otherItemPageMargins.Left)) < MAX_ACCEPTABLE_OFFSET) ||
                (std::abs(this->x0() - _other.x0()) < MAX_ACCEPTABLE_OFFSET &&
                 std::abs(this->x1() - _other.x1()) < MAX_ACCEPTABLE_OFFSET)
           ) &&
           (
                (
                    std::abs((this->y0() - _currItemPageMargins.Top) - (_other.y0() - _otherItemPageMargins.Top)) < MAX_ACCEPTABLE_OFFSET &&
                    std::abs((this->y1() - _currItemPageMargins.Top) - (_other.y1() - _otherItemPageMargins.Top)) < MAX_ACCEPTABLE_OFFSET
                ) || (
                    std::abs((this->y0() + _currItemPageMargins.Bottom) - (_other.y0() + _otherItemPageMargins.Bottom)) < MAX_ACCEPTABLE_OFFSET &&
                    std::abs((this->y1() + _currItemPageMargins.Bottom) - (_other.y1() + _otherItemPageMargins.Bottom)) < MAX_ACCEPTABLE_OFFSET
                ) || (
                    std::abs(this->y0() - _other.y0()) < MAX_ACCEPTABLE_OFFSET &&
                    std::abs(this->y1() - _other.y1()) < MAX_ACCEPTABLE_OFFSET
                )
           );

}

float clsPdfItem::horzJitterThreshold() const
{
    return this->height();
}

bool clsPdfItem::conformsToLine(float _ascent, float _descent, float _baseline) const
{
    if(this->type() == enuPdfItemType::VirtualLine)
        return false;
    if(this->y0() >= _ascent && this->y1() <= _descent)
        return true;
    float LineHeight = _descent - _ascent ;
    float Delta = std::abs(LineHeight - this->height());
    if(Delta > 0.1f * LineHeight)
        return false;
    return _baseline > this->y0() && _baseline < this->y1();
}

float clsPdfItem::horzOverlap(const clsPdfItem &_other) const
{
    float X0 = std::max(this->Data->Left, _other.Data->Left);
    float X1 = std::min(this->Data->Right, _other.Data->Right);
    return X1 - X0;
}

float clsPdfItem::horzOverlap(const PdfItemPtr_t &_other) const
{
    return this->horzOverlap(*_other);
}

float clsPdfItem::vertOverlap(const clsPdfItem &_other) const
{
    float Y0 = std::max(this->Data->Ascent, _other.Data->Ascent);
    float Y1 = std::min(this->Data->Descent, _other.Data->Descent);
    return Y1 - Y0;
}

float clsPdfItem::vertOverlap(const PdfItemPtr_t &_other) const
{
    return this->vertOverlap(*_other);
}

float clsPdfItem::tightVertOverlap(const clsPdfItem &_other) const
{
    float Y0 = std::max(this->Data->Top, _other.Data->Top);
    float Y1 = std::min(this->Data->Bottom, _other.Data->Bottom);
    return Y1 - Y0;
}

float clsPdfItem::tightVertOverlap(const PdfItemPtr_t &_other) const
{
    return this->tightVertOverlap(*_other);
}

clsPdfItem clsPdfItem::horzSpacer(const clsPdfItem& _other) const
{
    clsPdfItem Result;
    Result.Data->Left = std::min(this->Data->Right, _other.Data->Right);
    Result.Data->Right = std::max(this->Data->Left, _other.Data->Left);
    Result.Data->Top = std::min(this->Data->Top, _other.Data->Top);
    Result.Data->Bottom = std::max(this->Data->Bottom, _other.Data->Bottom);
    return Result;
}

clsPdfItem clsPdfItem::horzSpacer(const PdfItemPtr_t &_other) const
{
    return this->horzSpacer(*_other);
}

clsPdfItem clsPdfItem::vertSpacer(const clsPdfItem& _other) const
{
    clsPdfItem Result;
    Result.Data->Left = std::min(this->x0(), _other.x0());
    Result.Data->Right = std::max(this->x1(), _other.x1());
    Result.Data->Top = std::min(this->y1(), _other.y1());
    Result.Data->Bottom = std::max(this->y0(), _other.y0());
    return Result;
}

clsPdfItem clsPdfItem::vertSpacer(const PdfItemPtr_t &_other) const
{
    return this->vertSpacer(*_other);
}

clsPdfImage::clsPdfImage()
{
    this->setType(enuPdfItemType::Image);
}

clsPdfImage::clsPdfImage(float _left, float _top, float _right, float _bottom, enuPdfItemType _type):
    clsPdfItem (_left, _top, _right, _bottom, _type)
{ }

clsPdfImage::~clsPdfImage()
{ }

}
}
}
