#ifndef TARGOMAN_PDF_CLSPDFLINESEGMENT_H
#define TARGOMAN_PDF_CLSPDFLINESEGMENT_H

#include "pdfla/common.h"

namespace Targoman {
namespace PDF {

struct stuBox {
public:
    float width() const {return this->X1 - this->X0;}
    float height() const {return this->Y1 - this->Y0;}

    void setWidth(float v) {this->X1 = this->X0 + v;}
    void setHeight(float v) {this->Y1 = this->Y0 + v;}

    float X0, X1, Y0, Y1;

    stuBox(float _x0 = 0, float _y0 = 0, float _x1 = 0,  float _y1 = 0) :
        X0(_x0),X1(_x1),Y0(_y0), Y1(_y1)
    {;}

};

struct stuMarkableSegment;
typedef std::vector<stuMarkableSegment> SegmentVector_t;

struct stuMarkableSegment {
    int PageIndex;
    int ParIndex;
    int SntIndex;
    stuBox BoundingBox;
    enuLocation Location;
    enuContentType Type;
    SegmentVector_t InnerSegments;

    stuMarkableSegment(
            int PageIndex = -1,
            int _parIndex = -1,
            int _sntIndex = -1,
            stuBox _box = stuBox(),
            enuLocation _location = enuLocation::None,
            enuContentType _type = enuContentType::None,
            const SegmentVector_t& _innerSegments = {}
        ):
        PageIndex(PageIndex),
        ParIndex(_parIndex),
        SntIndex(_sntIndex),
        BoundingBox(_box),
        Location(_location),
        Type(_type),
        InnerSegments(_innerSegments)
    {;}
};


}
}
#endif // TARGOMAN_PDF_CLSPDFLINESEGMENT_H
