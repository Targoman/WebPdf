#ifndef TARGOMAN_PDF_PRIVATE_PDFLINESEGMENTMANAGER_H
#define TARGOMAN_PDF_PRIVATE_PDFLINESEGMENTMANAGER_H

#include "../clsPdfLineSegment.h"

namespace Targoman {
namespace PDF {
namespace Private {
namespace LA {

PdfLineSegmentPtrVector_t findVertStripes(const PdfItemPtrVector_t &_items, const stuSize& _pageSize);
PdfLineSegmentPtrVector_t findLines(const PdfItemPtrVector_t &_items);
PdfLineSegmentPtrVector_t mergeOverlappingLines(const stuConfigsPtr_t& _configs,
                                                const PdfLineSegmentPtrVector_t& _blockLines,
                                                const FontInfo_t& _fontInfo);


}
}
}
}

#endif // TARGOMAN_PDF_PRIVATE_PDFLINESEGMENTMANAGER_H
