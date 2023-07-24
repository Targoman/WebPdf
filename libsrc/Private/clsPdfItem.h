#ifndef TARGOMAN_PDF_PRIVATE_CLSPDFITEM_H
#define TARGOMAN_PDF_PRIVATE_CLSPDFITEM_H

#include <memory>
#include <vector>
#include "common.h"

namespace Targoman {
namespace PDF {
namespace Private {

enum class enuPdfItemType {
    None,
    Image,
    Path,
    VirtualLine,
    Char,
    Background
};

constexpr float MAX_RULER_SIZE = 2.f;

class clsPdfItemData;
class clsPdfItem;
typedef std::shared_ptr<clsPdfItem> PdfItemPtr_t;
typedef std::vector<PdfItemPtr_t> PdfItemPtrVector_t;

class clsPdfItem
{
private:
    clsExplicitlySharedDataPointer<clsPdfItemData> Data;

protected:
    void detachPdfItemData();

protected:
    void setType(const enuPdfItemType& _type);
    virtual void detach();

public:
    clsPdfItem();
    clsPdfItem(float _left, float _top, float _right, float _bottom, enuPdfItemType _type);
    clsPdfItem(const clsPdfItem& _o) { this->Data = _o.Data; }
    virtual ~clsPdfItem();

public:
    enuPdfItemType type() const;
    bool isWatermark() const;
    void markAsWatermark(bool _state);
    bool isHeader() const;
    void markAsHeader(bool _state);
    bool isFooter() const;
    void markAsFooter(bool _state);
    bool isSidebar() const;
    void markAsSidebar(bool _state);
    bool isRepeated() const;
    void markAsRepeated(int32_t _pageOffset);
    bool isMainContent() const;
    void markAsMainContent(bool _state);
    int32_t repetitionPageOffset() const;
    void resetWatermarkRepetition() const;

    float x0() const;
    float y0() const;
    float x1() const;
    float y1() const;

    int x0i() const;
    int y0i() const;
    int x1i() const;
    int y1i() const;

    float x0e() const;
    float y0e() const;
    float x1e() const;
    float y1e() const;

    void setX0(float _value);
    void setY0(float _value);
    void setX1(float _value);
    void setY1(float _value);

    float tightTop() const;
    float tightBottom() const;
    void setTightTop(float _value);
    void setTightBottom(float _value);

    float width() const;
    float height() const;
    float tightHeight() const;

    float centerX() const;
    float centerY() const;

    float area() const;

public:
    void unionWith_(const clsPdfItem& _other);
    void unionWith_(const PdfItemPtr_t& _other);

    clsPdfItem unionWith(const clsPdfItem& _other) const;
    clsPdfItem unionWith(const PdfItemPtr_t& _other) const;

    void intersectWith_(const clsPdfItem& _other);
    void intersectWith_(const PdfItemPtr_t& _other);

    clsPdfItem intersectWith(const clsPdfItem& _other) const;
    clsPdfItem intersectWith(const PdfItemPtr_t& _other) const;

public:
    bool isEmpty() const;
    bool isVeryThin() const;
    bool isVeryFat() const;
    bool isVerticalRuler() const;
    virtual bool isHorizontalRuler() const;
    bool doesContain(const clsPdfItem& _other, float _margin = -1.f) const;
    bool doesContain(const PdfItemPtr_t& _other, float _margin = -1.f) const;
    bool doesTouch(const clsPdfItem& _other) const;
    bool doesTouch(const PdfItemPtr_t& _other) const;
    virtual bool isSame(const stuPageMargins& _currItemPageMargins, const PdfItemPtr_t& _other, const stuPageMargins& _otherItemPageMargins) const;
    virtual bool isSame(const stuPageMargins& _currItemPageMargins, const clsPdfItem& _other, const stuPageMargins& _otherItemPageMargins) const;

public:
    virtual float horzJitterThreshold() const;
    virtual bool conformsToLine(float _ascent, float _descent, float _baseline) const;

public:
    float horzOverlap(const clsPdfItem &_other) const;
    float horzOverlap(const PdfItemPtr_t &_other) const;

    float vertOverlap(const clsPdfItem &_other) const;
    float vertOverlap(const PdfItemPtr_t &_other) const;

    float tightVertOverlap(const clsPdfItem &_other) const;
    float tightVertOverlap(const PdfItemPtr_t &_other) const;

    clsPdfItem horzSpacer(const clsPdfItem &_other) const;
    clsPdfItem horzSpacer(const PdfItemPtr_t &_other) const;

    clsPdfItem vertSpacer(const clsPdfItem &_other) const;
    clsPdfItem vertSpacer(const PdfItemPtr_t &_other) const;

public:
    template<typename T, typename = std::enable_if_t<std::is_base_of<clsPdfItem, T>::value>>
    T* as() {
//        static_assert (std::is_base_of<clsPdfItem, T>::value);
        return static_cast<T*>(this);
    }

    template<typename T, typename = std::enable_if_t<std::is_base_of<clsPdfItem, T>::value>>
    const T* as() const {
//        static_assert (std::is_base_of<clsPdfItem, T>::value);
        return static_cast<const T*>(this);
    }

    void resetFlags();
};

class clsPdfImage : public clsPdfItem {
public:
    clsPdfImage();
    clsPdfImage(float _left, float _top, float _right, float _bottom, enuPdfItemType _type);
    clsPdfImage(const clsPdfImage& _o) : clsPdfItem(_o) {}
    ~clsPdfImage();
};

template<typename T>
std::vector<T> naiveSortItemsByReadingOrder(const std::vector<T> &_v)
{
    std::vector<T> VerticallySorted = _v;
    sort(VerticallySorted, [] (const T& a, const T& b) {
        return a->y0() < b->y0();
    });
    std::vector<bool> Used(VerticallySorted.size(), false);
    std::vector<T> Result;

    for(size_t i = 0; i < VerticallySorted.size(); ++i){
        if(Used[i])
            continue;
        std::vector<int> Overlapping;
        Overlapping.push_back(static_cast<int>(i));
        for(size_t j = i + 1; j < VerticallySorted.size(); ++j) {
            if(Used[j])
                continue;
            float Y0 = std::max(VerticallySorted[j]->y0() - 2, VerticallySorted[i]->y0() - 2);
            float Y1 = std::min(VerticallySorted[j]->y1() + 2, VerticallySorted[i]->y1() + 2);
            if(Y1 - Y0 > MIN_ITEM_SIZE)
                Overlapping.push_back(static_cast<int>(j));
        }
        sort(Overlapping, [&] (int a, int b) {
            if(VerticallySorted[a]->y0() > VerticallySorted[b]->centerY())
                return false;
            else if(VerticallySorted[a]->y1() < VerticallySorted[b]->centerY())
                return true;
            return VerticallySorted[a]->x0() < VerticallySorted[b]->x0() - 0.1f * VerticallySorted[b]->horzJitterThreshold();
        });
        for(int j : Overlapping) {
            Used[static_cast<size_t>(j)] = true;
            Result.push_back(VerticallySorted[j]);
        }
    }

    return Result;
}

#ifdef TARGOMAN_SHOW_DEBUG
inline std::wostream& operator<<(std::wostream& _os, const clsPdfItem& _pdfItem){
    std::wstring Type;
    switch (_pdfItem.type()) {
        case enuPdfItemType::None: Type= L"None"; break;
        case enuPdfItemType::Image: Type= L"Image"; break;
        case enuPdfItemType::Path: Type= L"Path"; break;
        case enuPdfItemType::VirtualLine: Type= L"VirtualLine"; break;
        case enuPdfItemType::Char: Type= L"Char"; break;
        case enuPdfItemType::Background: Type= L"Background"; break;
    };
    _os<<Type<<L"("<<_pdfItem.x0()<<L","<<_pdfItem.y0()<<L")-("<<_pdfItem.x0()<<L","<<_pdfItem.y0()<<L")";
    return _os;
}
#endif

}
}
}

#endif // TARGOMAN_PDF_PRIVATE_CLSPDFITEM_H
