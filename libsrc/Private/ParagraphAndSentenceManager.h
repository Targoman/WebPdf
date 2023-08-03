#ifndef TARGOMAN_PDF_PRIVATE_PARAGRAPHDETECTION_H
#define TARGOMAN_PDF_PRIVATE_PARAGRAPHDETECTION_H

#include "clsPdfBlock.h"

namespace Targoman {
namespace PDF {
namespace Private {

enum class enuLineParagraphRelation {
    None = 0,
    BelongsTo = 1,
    LastLineOf = 3,
    DoesNotBelog = 4
};

enum class enuContinuationState{
    WeakAbbr,
    Can,
    Cannot
};

struct stuParState {
    std::vector<int> OpenTags;
    PdfBlockPtr_t TextBlock;
    enuContinuationState ContinuationState;
    PdfBlockPtr_t  LastPageMainBlock;
    stuParState(): ContinuationState(enuContinuationState::Cannot) {}
};


struct stuPreprocessedPage{
    PdfLineSegmentPtrVector_t Stripes;
    stuPageMargins Margins;
};

struct stuProcessedPage {
    PdfBlockPtrVector_t Content;
    PdfBlockPtrVector_t SubContent;
    stuParState LastMainBlockState;
    bool IsDirty;
    stuProcessedPage(const PdfBlockPtrVector_t& _content = {}, const stuParState& _lastMainBlockState = {}) :
        Content(_content), LastMainBlockState(_lastMainBlockState), IsDirty(false) {}
};

enuLineParagraphRelation estimateLineParagraphRelation(
        const PdfBlockPtr_t& _parentBlock,
        const PdfLineSegmentPtr_t& _line,
        const PdfLineSegmentPtr_t& _prevLine,
        const PdfBlockPtr_t& _paragraph
        );

stuProcessedPage extractAndEnumerateSentences(const stuConfigsPtr_t& _configs,
        int16_t _pageIndex,
        const PdfBlockPtrVector_t& _pageBlocks, const FontInfo_t& _fontInfo,
        const stuParState& _prevPageState
);

void updateFirstParagraph(const stuConfigsPtr_t& _configs, PdfBlockPtrVector_t& _pageBlocks, const stuParState& _prevPageState);

}
}
}

#endif // TARGOMAN_PDF_PRIVATE_PARAGRAPHDETECTION_H
