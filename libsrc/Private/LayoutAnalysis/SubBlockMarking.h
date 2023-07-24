#ifndef TARGOMAN_PDF_PRIVATE_LAYOUTANALYSIS_PARAGRAPHDETECTION_HPP
#define TARGOMAN_PDF_PRIVATE_LAYOUTANALYSIS_PARAGRAPHDETECTION_HPP

#include "LACommon.h"
#include "../clsPdfBlock.h"
#include "BlockCreation.h"

namespace Targoman {
namespace PDF {
namespace Private {
namespace LA {

void splitAndMarkBlocks(const stuConfigsPtr_t& _configs,
                            int16_t _pageIndex, const stuPageMargins& _pageMargins,
                            const stuSize& _pageSize, const FontInfo_t& _fontInfo,
                            PdfBlockPtrVector_t& _blocks,
                            const stuParState& _lastPageBlockState
                        );

}
}
}
}
#endif // TARGOMAN_PDF_PRIVATE_LAYOUTANALYSIS_PARAGRAPHDETECTION_HPP
