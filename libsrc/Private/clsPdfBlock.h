#ifndef TARGOMAN_PDF_PRIVATE_CLSPDFBLOCK_H
#define TARGOMAN_PDF_PRIVATE_CLSPDFBLOCK_H

#include <map>
#include "clsPdfLineSegment.h"

namespace Targoman {
namespace PDF {
namespace Private {

enum class enuJustification {
    None = 0,
    Left = 1,
    Right = 2,
    Justified = 3,
    Center = 4
};

inline enuJustification operator | (const enuJustification& _a, const enuJustification& _b) {
    return static_cast<enuJustification>(static_cast<size_t>(_a) | static_cast<size_t>(_b));
}

inline enuJustification operator & (const enuJustification& _a, const enuJustification& _b) {
    return static_cast<enuJustification>(static_cast<size_t>(_a) & static_cast<size_t>(_b));
}

inline enuJustification operator |= (enuJustification& _a, const enuJustification& _b) {
    return (_a = _a | _b);
}

inline enuJustification operator &= (enuJustification& _a, const enuJustification& _b) {
    return (_a = _a & _b);
}


class clsPdfBlock;
class clsPdfBlockData;
typedef std::shared_ptr<clsPdfBlock> PdfBlockPtr_t;
typedef std::vector<PdfBlockPtr_t> PdfBlockPtrVector_t;

class clsPdfBlock : public clsPdfItem
{
public:
    clsPdfBlock(enuLocation _location = enuLocation::None,
                enuContentType _contentType = enuContentType::None,
                enuJustification _justification = enuJustification::None
                );

    clsPdfBlock(const clsPdfBlock& _other) : clsPdfItem(_other) { this->Data = _other.Data; }
    ~clsPdfBlock() override;

public:
    PdfLineSegmentPtrVector_t lines() const;
    void appendLineSegment(const PdfLineSegmentPtr_t& _line, bool _mergeOverlappingLines);
    void clearLines();

    void replaceLines(const PdfLineSegmentPtrVector_t& _newLines);

    PdfBlockPtrVector_t& innerBlocks();
    void appendInnerBlock(PdfBlockPtr_t _block);

    enuLocation location() const;
    void setLocation(enuLocation _location);

    enuContentType contentType() const;
    void setContentType(enuContentType _type);

    enuJustification justification() const;
    void setOriginalIndex(size_t _value);

    PdfLineSegmentPtrVector_t horzRulers(float _minWidth = 0);
    PdfLineSegmentPtrVector_t vertRulers(float _minHeight = 0);
    bool mostlyImage() const;
    bool isPureImage() const;
    float approxLineHeight() const;

    uint8_t innerColsCountByLayout();
    void setInnerColsCountByLayout(uint8_t _count);

    float space2Left() const;
    float space2Right() const;

    void setSpace2Left(float _value);
    void setSpace2Right(float _value);

    float firstLineIndent() const;
    float textIndent() const;
    float textIndentPos() const;

    void setFirstLineIndent(float _value) const;
    void setTextIndent(float _value) const;

    float tightLeft() const;
    float tightRight() const;
    float tightWidth() const;

    bool certainlyFinished() const;
    void setCertainlyFinished(bool _value);

public:
    bool doesShareFontsWith(const clsPdfBlock& _other) const;
    bool doesShareFontsWith(const PdfBlockPtr_t& _other) const;

    bool doesShareFontsWith(const PdfLineSegmentPtr_t& _otherLine) const;
    float approxLineSpacing() const;
    float averageCharHeight() const;

    void mergeWith_(const clsPdfBlock& _other, bool _forceOneColumnMerge = false);
    void mergeWith_(const PdfBlockPtr_t& _other, bool _forceOneColumnMerge = false);
    void appendToFlatBlockVector(PdfBlockPtrVector_t& _flatBlockVector) const;

public:
    bool isNonText() const;
    bool isMainTextBlock() const;
    void naiveSortByReadingOrder();

public:
    void prepareLineSegments(const FontInfo_t& _fontInfo, const stuConfigsPtr_t& _configs);
    void sortLineSegmentsTopToBottom();
    void estimateParagraphParams();

private:
    clsExplicitlySharedDataPointer<clsPdfBlockData> Data;
};

}
}
}
#endif // TARGOMAN_PDF_PRIVATE_CLSPDFBLOCK_H
