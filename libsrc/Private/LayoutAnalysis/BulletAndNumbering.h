#ifndef TARGOMAN_PDF_PRIVATE_LAYOUTANALYSIS_BULLETANDNUMBERING_H
#define TARGOMAN_PDF_PRIVATE_LAYOUTANALYSIS_BULLETANDNUMBERING_H

#include "LACommon.h"

namespace Targoman {
namespace PDF {
namespace Private {
namespace LA {

enum class enuListSubType {
    None,
    Numerals,
    RomanNumerals,
    Brackets
};

struct stuListDetectionResult {
    enuListType Type;
    enuListSubType SubType;
    float TextLeft;
    size_t ListIdentifierStart;
    size_t ListIdentifierEnd;

    static stuListDetectionResult nonList(const PdfLineSegmentPtr_t& _line) {
        return  stuListDetectionResult { enuListType::None, enuListSubType::None, _line->x0(), 0, 0 };
    }
};
typedef std::vector<stuListDetectionResult> ListDetectionResultVector_t;

std::tuple<ListDetectionResultVector_t, clsFloatHistogram> identifyLineListTypes(const stuConfigsPtr_t& _configs, const PdfBlockPtr_t& _block);
void process4BulletAndNumbering(const stuConfigsPtr_t& _configs, const PdfBlockPtr_t& _block);

}
}
}
}
#endif // TARGOMAN_PDF_PRIVATE_LAYOUTANALYSIS_BULLETANDNUMBERING_H
