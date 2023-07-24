#ifndef TARGOMAN_PDF_PRIVATE_LAYOUTANALYSIS_COMMON_HPP
#define TARGOMAN_PDF_PRIVATE_LAYOUTANALYSIS_COMMON_HPP

#include <set>
#include "../clsLayoutAnalyzer.h"
#include "../clsLayoutStorage.h"
#include "../clsPdfLineSegment.h"
#include "../clsPdfChar.h"
#include "PdfLineSegmentManager.h"

namespace Targoman {
namespace PDF {
namespace Private {
namespace LA {

constexpr float HEADER_FOOTER_PAGE_MARGIN_THRESHOLD = .25f;
constexpr float SIDEBAR_PAGE_MARGIN_THRESHOLD = .065f;
constexpr size_t MAX_PAGE_NUMBER_SIZE = 5;
#define SIG_STEP_DONE(NAME, ITEMS) sigStepDone(NAME, _pageIndex, ITEMS);

bool isTableCaption(const std::wstring& _str);
bool isImageCaption(const std::wstring& _str);
bool canBeTable(const PdfBlockPtr_t& _block, bool _lastWasLine);

}
}
}
}
#endif // TARGOMAN_PDF_PRIVATE_LAYOUTANALYSIS_STAMPING_COMMON_HPP
