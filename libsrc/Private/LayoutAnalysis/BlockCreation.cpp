#include "BlockCreation.h"

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
                        bool _mergeOverlappingLines){
    if(_line->y0() > 335 && _line->y1() < 374 && _line->x0() > 231 && _line->x1() < 349)
        int a = 1;

    if(_candidateBlock.get() == nullptr){
        _candidateBlock = std::make_shared<clsPdfBlock>(_loc, _contentType);
        if(_layout.get() != nullptr) {
            _candidateBlock->setInnerColsCountByLayout(_layout->InnerColCount);
            _candidateBlock->setX0(_layout->x0());
            _candidateBlock->setX1(_layout->x1());
            _candidateBlock->setSpace2Left(_layout->Space2Left);
            _candidateBlock->setSpace2Right(_layout->Space2Right);
//            _candidateBlock->setY0(_layout->y0());
//            _candidateBlock->setY1(_layout->y1());
        }
        _blocks.push_back(_candidateBlock);
    }

    if(_line->isBackground()
       || (
           _line->isPureImage()
           && _line->isHorizontalRuler() == false
           && _line->isVerticalRuler() == false
           && _line->items().size() == 1
           && _line->items().front()->type() == enuPdfItemType::Path
           && _line->width() > .9f * _candidateBlock->width()
           )
       ){
        auto TopLine = std::make_shared<clsPdfLineSegment>();
        auto BottomLine = std::make_shared<clsPdfLineSegment>();
        auto TopLineItem = std::make_shared<clsPdfImage>(_line->x0(), _line->y0(), _line->x1(), _line->y0() + .4f, enuPdfItemType::VirtualLine);
        auto BottomLineItem = std::make_shared<clsPdfImage>(_line->x0(), _line->y1() - .4f, _line->x1(), _line->y1(), enuPdfItemType::VirtualLine);
        TopLine->appendPdfItem(TopLineItem);
        BottomLine->appendPdfItem(BottomLineItem);

        _candidateBlock->appendLineSegment(TopLine, false);
        _candidateBlock->appendLineSegment(BottomLine, false);
        _line->markAsBackground();
        _candidateBlock->appendLineSegment(_line, false);
    } else {
        _candidateBlock->appendLineSegment(_line, _mergeOverlappingLines);
    }
}

PdfBlockPtrVector_t makeBlocks(const stuConfigsPtr_t& _configs,
                                  const stuSize& _pageSize,
                                  const clsLayoutStorage& _layoutStorage,
                                  const PdfLineSegmentPtrVector_t& _lines,
                                  const FontInfo_t& _fontInfo){
    PdfBlockPtrVector_t Blocks;
    PdfBlockPtr_t Footer, Header, SidebarLeft, SidebarRight;

    std::set<clsPdfLineSegment*> UsedLines;
    LayoutChildren_t DummyLayout;

    for(const auto& Storage : _layoutStorage.Children){
        PdfBlockPtr_t Block;

        for (const auto& Line : _lines){
            if(UsedLines.find(Line.get()) != UsedLines.end())
                continue;

            if(Storage->y0() > 86 && Storage->y1() < 396 &&
               Line->y0() > 335 && Line->y0() < 338 && Line->y1() < 374 && Line->x0() > 231 && Line->x1() < 349)
                int a = 1;

            if(_configs->DiscardHeaders && Line->isHeader())
                appendLineToBlocks(Blocks, Header, DummyLayout, Line, enuLocation::Header, enuContentType::Text, false);
            else if(_configs->DiscardFooters && Line->isFooter())
                appendLineToBlocks(Blocks, Footer, DummyLayout, Line, enuLocation::Footer, enuContentType::Text, false);
            else if(_configs->DiscardSidebars && Line->isSidebar()) {
                if(Line->x1() < SIDEBAR_PAGE_MARGIN_THRESHOLD * _pageSize.Width)
                    appendLineToBlocks(Blocks, SidebarLeft, DummyLayout, Line, enuLocation::SidebarLeft, enuContentType::Text, false);
                else
                    appendLineToBlocks(Blocks, SidebarRight, DummyLayout, Line, enuLocation::SidebarLeft, enuContentType::Text, false);
            } else if(Storage->doesContain(Line)) {
                appendLineToBlocks(Blocks, Block, Storage, Line, enuLocation::Main, enuContentType::None, false);
            } else
                continue;

            UsedLines.insert(Line.get());
        }

        for(const auto& Line : Block->lines())
            Line->consolidateSpaces(_fontInfo, _configs);

        if(Block.get() != nullptr)
            Block->naiveSortByReadingOrder();
    }

    if(Header.get() != nullptr)       Header->naiveSortByReadingOrder();
    if(Footer.get() != nullptr)       Footer->naiveSortByReadingOrder();
    if(SidebarLeft.get() != nullptr)  SidebarLeft->naiveSortByReadingOrder();
    if(SidebarRight.get() != nullptr) SidebarRight->naiveSortByReadingOrder();
    return naiveSortItemsByReadingOrder(Blocks);
}

}
}
}
}
