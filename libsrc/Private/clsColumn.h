#ifndef TARGOMAN_PDF_PRIVATE_CLSCOLUMN_H
#define TARGOMAN_PDF_PRIVATE_CLSCOLUMN_H

#include "clsPdfItem.h"

namespace Targoman {
namespace PDF {
namespace Private {

class clsColumn;
typedef std::shared_ptr<clsColumn> ColumnPtr_t;
typedef std::vector<ColumnPtr_t> ColumnPtrVector_t;

class clsColumn : public clsPdfItem {
public:
    clsColumn(float _width, float _topImageY0, float _bottomImageY1, float _leftImage, float _rightImage);
    clsColumn(const clsColumn& _other) = delete;
    ~clsColumn();

    std::shared_ptr<clsColumn> makeCopy();
    void expandToLeft(float _newX0);
    void expandToRight(float _newX1);
    float contentWidth() const;
    void mergeWith_(const ColumnPtr_t& _other, bool _tight);

    bool topIsImage();
    bool bottomIsImage();
    bool leftIsImage();
    bool rightIsImage();

public:
    float SpaceToLeft = 0, SpaceToRight = 0;
    float SumMergedColsWidth, MinMergedColWidth;
    uint16_t MergedColCount;
    bool IsMerged = false;

private:
    float MostTopImageY0 = -1, MostBottomImageY1 = -1;
    float MostLeftImageX0 = -1, MostRightImageX1 = -1;
};

}
}
}
#endif // TARGOMAN_PDF_PRIVATE_CLSCOLUMN_H
