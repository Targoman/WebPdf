#ifndef TARGOMAN_PDF_PRIVATE_CLSLAYOUTSTORAGE_H
#define TARGOMAN_PDF_PRIVATE_CLSLAYOUTSTORAGE_H

#include "clsPdfLineSegment.h"
#include "clsColumn.h"

namespace Targoman {
namespace PDF {
namespace Private {

enum class enuDirection {
    None,
    Vertical,
    Horizontal
};

class clsLayoutStorage;
typedef  std::shared_ptr<clsLayoutStorage> LayoutChildren_t;
class clsLayoutStorage : public clsPdfItem {
public:
    clsLayoutStorage(const stuConfigsPtr_t &_configs, const PdfLineSegmentPtrVector_t& _lines = {}, bool _allowBackgrounds = true);
    clsLayoutStorage(const clsLayoutStorage& _other) = delete;
    ~clsLayoutStorage();

    void identifyMainColumns(int _pageIndex, const stuPageMargins& _pageMargins, const stuSize& _pageSize);
    void identifyInnerColumns(int _pageIndex, const stuPageMargins& _pageMargins, const stuSize& _pageSize, const stuConfigsPtr_t& _configs);


private:
    void appendLine(const PdfLineSegmentPtr_t& _line);
    ColumnPtrVector_t xyCutColumn(int _pageIndex);
    void updateChildrenByLayout(int _pageIndex, const ColumnPtrVector_t& _columns, const char* _stepName);

public:
    PdfLineSegmentPtrVector_t Lines;
    std::vector<LayoutChildren_t> Children;
    stuConfigsPtr_t Configs;
    enuDirection Direction;
    uint8_t InnerColCount;
    bool AllowBackgroundImages;
    uint8_t __PADDING__[2];
    float Space2Left = 0, Space2Right = 0;
};

}
}
}
#endif // TARGOMAN_PDF_PRIVATE_CLSLAYOUTSTORAGE_H
