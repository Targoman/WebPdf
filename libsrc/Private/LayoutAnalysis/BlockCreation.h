#ifndef TARGOMAN_PDF_PRIVATE_LAYOUTANALYSIS_BLOCKCREATION_HPP
#define TARGOMAN_PDF_PRIVATE_LAYOUTANALYSIS_BLOCKCREATION_HPP

#include "LACommon.h"
namespace Targoman {
namespace PDF {
namespace Private {
namespace LA {

void appendLineToBlocks(PdfBlockPtrVector_t& _blocks,
                        PdfBlockPtr_t& _candidateBlock,
                        const LayoutChildren_t& _layout,
                        const PdfLineSegmentPtr_t& _line,
                        enuLocation _loc,
                        enuContentType _contentType,
                        bool _mergeOverlappingLines);

PdfBlockPtrVector_t makeBlocks(const stuConfigsPtr_t& _configs,
                                  const stuSize& _pageSize,
                                  const clsLayoutStorage& _layoutStorage,
                                  const PdfLineSegmentPtrVector_t& _lines, const FontInfo_t& _fontInfo);
}
}
}
}
#endif // BLOCKCREATION_HPP
