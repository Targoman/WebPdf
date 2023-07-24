#ifndef TARGOMAN_PDF_CLSBEZIERCURVE_H
#define TARGOMAN_PDF_CLSBEZIERCURVE_H

#include <tuple>

class clsBezierCurve
{
private:
    float X0, Y0, X1, Y1, X2, Y2, X3, Y3;
public:
    clsBezierCurve(float _x0, float _y0, float _x1, float _y1, float _x2, float _y2, float _x3, float _y3);
    std::tuple<float, float, float, float> bounds() const;
    float computeMinXAtY(float _y) const;
};

#endif // TARGOMAN_PDF_CLSBEZIERCURVE_H
