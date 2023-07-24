#include "clsColumn.h"

namespace Targoman {
namespace PDF {
namespace Private {

clsColumn::clsColumn(float _width, float _topImageY0, float _bottomImageY1, float _leftImage, float _rightImage) :
    SpaceToLeft(0),
    SpaceToRight(0),
    SumMergedColsWidth(_width),
    MinMergedColWidth(_width),
    MergedColCount(1),
    MostTopImageY0(_topImageY0),
    MostBottomImageY1(_bottomImageY1),
    MostLeftImageX0(_leftImage),
    MostRightImageX1(_rightImage)
{ ; }

clsColumn::~clsColumn()
{;}

std::shared_ptr<clsColumn> clsColumn::makeCopy()
{
    auto Col = std::make_shared<clsColumn>(this->width(), this->MostTopImageY0, this->MostBottomImageY1, this->MostLeftImageX0, this->MostRightImageX1);
    Col->setX0(this->x0());
    Col->setX1(this->x1());
    Col->setY0(this->y0());
    Col->setY1(this->y1());
    Col->setTightTop(this->tightTop());
    Col->setTightBottom(this->tightBottom());
    Col->setType(this->type());
    Col->SpaceToLeft = this->SpaceToLeft;
    Col->SpaceToRight = this->SpaceToRight;
    Col->SumMergedColsWidth = this->SumMergedColsWidth;
    Col->MinMergedColWidth = this->MinMergedColWidth;
    Col->MergedColCount = this->MergedColCount;
    Col->MostTopImageY0 = this->MostTopImageY0;
    Col->MostBottomImageY1 = this->MostBottomImageY1;
    Col->MostLeftImageX0 = this->MostLeftImageX0;
    Col->MostRightImageX1 = this->MostRightImageX1;
    return Col;
}

void clsColumn::expandToLeft(float _newX0)
{
    assert(_newX0 <= this->x0());
    this->SpaceToLeft += this->x0() - _newX0;
//    this->SpaceToLeft = this->x0() - _newX0;
    this->setX0(_newX0);
}

void clsColumn::expandToRight(float _newX1)
{
    assert(_newX1 >= this->x1());
    this->SpaceToRight += _newX1 - this->x1();
//    this->SpaceToRight = _newX1 - this->x1();
    this->setX1(_newX1);
}

float clsColumn::contentWidth() const
{
    return this->width() - this->SpaceToLeft - this->SpaceToRight;
}

void clsColumn::mergeWith_(const ColumnPtr_t &_other, bool _tight)
{
    if(_other->x1() < this->x0()) {
        this->SpaceToLeft = _other->SpaceToLeft;
        this->MergedColCount += _other->MergedColCount;
        this->SumMergedColsWidth += _other->SumMergedColsWidth;
        this->MinMergedColWidth = std::min(this->MinMergedColWidth, _other->MinMergedColWidth);
    } else if(_other->x0() > this->x1()) {
        this->SpaceToRight = _other->SpaceToRight;
        this->MergedColCount += _other->MergedColCount;
        this->SumMergedColsWidth += _other->SumMergedColsWidth;
        this->MinMergedColWidth = std::min(this->MinMergedColWidth, _other->MinMergedColWidth);
    } else {
        this->SpaceToLeft  = this->x0() + this->SpaceToLeft  < _other->x0() + _other->SpaceToLeft  ? this->SpaceToLeft  : _other->SpaceToLeft;
        this->SpaceToRight = this->x1() - this->SpaceToRight > _other->x1() - _other->SpaceToRight ? this->SpaceToRight : _other->SpaceToRight;
        if(_other->isHorizontalRuler() == false){
            this->MergedColCount = 1;
            this->SumMergedColsWidth = std::max(this->SumMergedColsWidth, _other->SumMergedColsWidth - _other->SpaceToLeft - _other->SpaceToRight);
            this->MinMergedColWidth = std::max(this->MinMergedColWidth, _other->MinMergedColWidth);
        }
    }

    if(_tight){
        _other->setX0(this->x0() + _other->SpaceToLeft);
        _other->setX1(this->x1() - _other->SpaceToRight);
    }
    this->unionWith_(_other);

    this->MostBottomImageY1 = std::max(this->MostBottomImageY1, _other->MostBottomImageY1);
    if(_other->MostTopImageY0 > 0.f) this->MostTopImageY0 = std::min(this->MostTopImageY0, _other->MostTopImageY0);
}

bool clsColumn::topIsImage()
{
    if(std::abs(this->MostTopImageY0 - this->y0()) < MIN_ITEM_SIZE)
        return true;
    return false;
}

bool clsColumn::bottomIsImage()
{
    if(std::abs(this->MostBottomImageY1 - this->y1()) < MIN_ITEM_SIZE)
        return true;
    return false;
}

bool clsColumn::leftIsImage()
{
    if(std::abs(this->MostLeftImageX0 - (this->x0() + this->SpaceToLeft))  < MIN_ITEM_SIZE)
        return true;
    return false;
}

bool clsColumn::rightIsImage()
{
    if(std::abs(this->MostRightImageX1 - (this->x1() - this->SpaceToRight)) < MIN_ITEM_SIZE)
        return true;
    return false;
}

}
}
}
