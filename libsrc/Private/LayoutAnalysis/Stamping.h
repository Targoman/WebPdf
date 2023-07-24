#ifndef TARGOMAN_PDF_PRIVATE_LAYOUTANALYSIS_STAMPING_HPP
#define TARGOMAN_PDF_PRIVATE_LAYOUTANALYSIS_STAMPING_HPP

#include "LACommon.h"

namespace Targoman {
namespace PDF {
namespace Private {
namespace LA {

void sigStampDetectionDone(int _pageIndex, const PdfItemPtrVector_t& _pageItems, bool _onlyNewOnes);

}
}
}
}

#endif // STAMPING_HPP
