#ifndef TARGOMAN_PDF_PRIVATE_CLSPDFLINESEGMENT_H
#define TARGOMAN_PDF_PRIVATE_CLSPDFLINESEGMENT_H

#include <cmath>
#include <algorithm>
#include <map>
#include "clsPdfChar.h"
#include "clsPdfFont.h"

namespace Targoman {
namespace PDF {
namespace Private {

enum class enuLineSegmentAssociation {
    None,
    IsCaptionOf,
    HasAsCaption,
    InsideTextOf,
    HasAsInsideText
};

enum class enuListType {
    None,
    Bulleted,
    Numbered
};

class clsPdfLineSegment;
typedef std::shared_ptr<clsPdfLineSegment> PdfLineSegmentPtr_t;
typedef std::vector<PdfLineSegmentPtr_t> PdfLineSegmentPtrVector_t;

class LineSpecUpdater;
class clsPdfLineSegmentData;
class clsPdfLineSegment : public clsPdfItem
{
public:
    clsExplicitlySharedDataPointer<clsPdfLineSegmentData> Data;

private:
//    clsExplicitlySharedDataPointer<clsPdfLineSegmentExData> Data;
    void detach() override final;

    float findBaseline() const;
    void setLineSpecs(int16_t _pageIndex, int16_t _parIndex, int16_t _sntIndex);

public:
    clsPdfLineSegment();
    clsPdfLineSegment(const clsPdfLineSegment& _other) : clsPdfItem(_other) { this->Data = _other.Data; }
    ~clsPdfLineSegment() override;

    bool isValid() const;

    float baseline() const;

    int16_t pageIndex() const;
    int16_t parIndex() const;
    int16_t sntIndex() const;

    bool isHorizontalRuler() const override;

    bool isSameAs(const stuPageMargins &_currPageMargins, const clsPdfLineSegment& _other, const stuPageMargins &_otherPageMargins) const;
    bool isSameAs(const stuPageMargins& _currPageMargins, const PdfLineSegmentPtr_t &_other, const stuPageMargins& _otherPageMargins) const;

    void setBulletAndNumberingInfo(const stuConfigsPtr_t& _configs,
                                   float _textLeft,
                                   enuListType _listType,
                                   size_t _listIdStart = std::numeric_limits<size_t>::max(),
                                   size_t _listIdEnd = std::numeric_limits<size_t>::max());

    float textLeft() const;
    enuListType listType() const;

    PdfLineSegmentPtr_t relative() const;
    enuLineSegmentAssociation association() const;

    void setRelativeAndAssociation(const PdfLineSegmentPtr_t& _relative, enuLineSegmentAssociation _association);

public:
    std::wstring contentString() const;

public:
    void appendPdfItemVector(const PdfItemPtrVector_t& _items);
    void appendPdfItem(const PdfItemPtr_t& _item);

    bool doesShareFontsWith(const clsPdfLineSegment& _other) const;
    bool doesShareFontsWith(const PdfLineSegmentPtr_t& _other) const;

    bool mergeWith_(const clsPdfLineSegment& _other);
    bool mergeWith_(const PdfLineSegmentPtr_t& _other);

    bool isPureImage() const;
    bool isBackground() const;
    void markAsBackground();

    float mostUsedFontSize() const;

public:
    size_t size() const;

    const PdfItemPtrVector_t& items() const;

    const PdfItemPtr_t& at(size_t _index) const;
    const PdfItemPtr_t& operator[] (size_t _index) const;

    PdfItemPtrVector_t::const_iterator begin() const;
    PdfItemPtrVector_t::const_iterator end() const;

public:
    void consolidateSpaces(const FontInfo_t &_fontInfo, const stuConfigsPtr_t& _configs);

private:
    friend class LineSpecUpdater;
};

}
}
}

#endif // TARGOMAN_PDF_PRIVATE_CLSPDFLINESEGMENTEXTENDED_H
