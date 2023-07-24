#include <cmath>
#include <algorithm>
#include "clsBezierCurve.h"

clsBezierCurve::clsBezierCurve(float _x0, float _y0, float _x1, float _y1, float _x2, float _y2, float _x3, float _y3):
    X0(_x0),
    Y0(_y0),
    X1(_x1),
    Y1(_y1),
    X2(_x2),
    Y2(_y2),
    X3(_x3),
    Y3(_y3)
{ }

float getBezierValueForT(float _t, float _p0, float _p1, float _p2, float _p3)
{
    float OneMinusT = 1 - _t;

    return (std::pow(OneMinusT, 3.f) * _p0)
            + (3.f * std::pow(OneMinusT, 2.f) * _t * _p1)
            + (3.f * OneMinusT * std::pow(_t, 2.f) * _p2)
            + (std::pow(_t, 3.f) * _p3);
}

std::tuple<float, float> solveQuadratic(float _p0, float _p1, float _p2, float _p3) {
    float i = _p1 - _p0;
    float j = _p2 - _p1;
    float k = _p3 - _p2;

    // P'(x) = (3i - 6j + 3k)t^2 + (-6i + 6j)t + 3i
    float a = (3 * i) - (6 * j) + (3 * k);
    float b = (6 * j) - (6 * i);
    float c = (3 * i);

    float sqrtPart = (b * b) - (4 * a * c);
    bool hasSolution = sqrtPart >= 0;
    if (!hasSolution)
        return std::make_tuple(NAN, NAN);

    float t1 = (-b + std::sqrt(sqrtPart)) / (2 * a);
    float t2 = (-b - std::sqrt(sqrtPart)) / (2 * a);

    float s1 = NAN;
    float s2 = NAN;

    if (t1 >= 0 && t1 <= 1)
        s1 = getBezierValueForT(t1, _p0, _p1, _p2, _p3);

    if (t2 >= 0 && t2 <= 1)
        s2 = getBezierValueForT(t2, _p0, _p1, _p2, _p3);

    return std::make_tuple(s1, s2);
}

std::tuple<float, float, float, float> clsBezierCurve::bounds() const
{
    float SolX1, SolX2, SolY1, SolY2;
    std::tie(SolX1, SolX2) = solveQuadratic(this->X0, this->X1, this->X2, this->X3);
    std::tie(SolY1, SolY2) = solveQuadratic(this->Y0, this->Y1, this->Y2, this->Y3);

    float MinX = std::min(this->X0, this->X3);
    float MaxX = std::max(this->X0, this->X3);

    if (!std::isnan(SolX1)) {
        MinX = std::min(MinX, SolX1);
        MaxX = std::max(MaxX, SolX1);
    }

    if (!std::isnan(SolX2)) {
        MinX = std::min(MinX, SolX2);
        MaxX = std::max(MaxX, SolX2);
    }

    float MinY = std::min(this->Y0, this->Y3);
    float MaxY = std::max(this->Y0, this->Y3);

    if (!std::isnan(SolY1)) {
        MinY = std::min(MinY, SolY1);
        MaxY = std::max(MaxY, SolY1);
    }

    if (!std::isnan(SolY2)) {
        MinY = std::min(MinY, SolY2);
        MaxY = std::max(MaxY, SolY2);
    }

    return std::tie(MinX, MinY, MaxX, MaxY);
}

constexpr float EPSILON = 1e-5f;

std::tuple<float, float, float> solveCubic(float _a, float _b, float _c, float _d) {
    float S0 = NAN, S1 = NAN, S2 = NAN;
    if(_d == 0.f && _a != 0.f) {
        S0 = 0.f;
        if(_c == 0.f) {
            S1 = -_b / _a;
        } else {
            float Temp = _b * _b - 4.f * _a * _c;
            if(std::abs(Temp) < EPSILON)
                S1 = -_b / (2.f * _a);
            else if(Temp > 0) {
                Temp = std::sqrt(Temp);
                S1 = (-_b + Temp) / (2 * _a);
                S2 = (-_b - Temp) / (2 * _a);
            }
        }
    } else if(_a != 0.f) {
        float Xn = -_b / (3.f * _a);
        float Yn = ((_a * Xn + _b) * Xn + _c) * Xn + _d;
        float Delta2 = (_b * _b - 3.f * _a * _c) / (9.f * _a * _a);
        float D = Yn * Yn - 4.f * _a * _a * Delta2 * Delta2 * Delta2;
        if( ((Yn >.01f || Yn < -.01f) && std::abs(D/Yn) < EPSILON) || ((Yn <= .01f && Yn >= -.01f) && std::abs(D) < EPSILON) )
            D = 0;
        if( D > 0 ) {
            float Temp = std::sqrt(D);
            float T2 = (-Yn - Temp) / (2.f * _a);
            T2 = (T2 == 0.f) ? 0.f : (T2 < 0.f) ? -std::pow(-T2, 1.f / 3.f) : std::pow(T2, 1.f / 3.f);
            float T3 = (-Yn + Temp) / (2.f * _a);
            T3 = (T3 == 0.f) ? 0.f : (T3 < 0.f) ? -std::pow(-T3, 1.f / 3.f) : std::pow(T3, 1.f / 3.f);
            S0 = Xn + T2 + T3;
        } else if ( D < 0.f ) {
            if ( Delta2 >= 0.f ) {
                float Delta = std::sqrt(Delta2);
                float h = 2.f *_a*Delta2*Delta;
                float Temp = -Yn/h;
                if ( Temp>=-1.0001f && Temp<=1.0001f ) {
                    if ( Temp<-1.f ) Temp = -1.f; else if ( Temp>1.f ) Temp = 1.f;
                    float theta = std::acos(Temp)/3;
                    S0 = Xn+2.f*Delta*std::cos(theta);
                    S1 = Xn+2.f*Delta*std::cos(2.0943951f+theta);	/* 2*pi/3 */
                    S2 = Xn+2.f*Delta*std::cos(4.1887902f+theta);	/* 4*pi/3 */
                }
            }
        } else if ( /* d==0 && */ Delta2 != 0.f ) {
            float Delta = Yn / (2.f * _a);
            Delta = Delta == 0.f ? 0.f : Delta > 0.f ? std::pow(Delta, 1.f / 3.f) : -std::pow(-Delta, 1.f / 3.f);
            S0 = Xn + Delta;	/* this root twice, but that's irrelevant to me */
            S1 = Xn - 2.f*Delta;
        } else if ( /* d==0 && */ Delta2 == 0.f ) {
            if ( Xn>=-0.0001f && Xn<=1.0001f ) S0 = Xn;
        }
    } else if ( _a != 0.f ) {
        float D = _c * _c-4.f*_b*_d;
        if ( D < 0 && std::abs(D) < EPSILON) D = 0;
        if ( D < 0 )
            return std::make_tuple(S0, S1, S2);		/* All roots imaginary */
        D = std::sqrt(D);
        S0 = (-_c-D)/(2.f *  _b);
        S1 = (-_c+D)/(2.f * _b);
    } else if ( _c != 0.f ) {
        S0 = -_d/_c;
    } else {
        /* If it's a point then either everything is a solution, or nothing */
    }
    return std::make_tuple(S0, S1, S2);
}

float clsBezierCurve::computeMinXAtY(float _y) const
{
    float A, B, C, D;
    A = (this->Y3 - 3 * this->Y2 + 3 * this->Y1 - this->Y0);
    B = (3 * this->Y0 - 6 * this->Y1);
    C = (3 * this->Y1 - 3 * this->Y0);
    D = (this->Y0 + 3 * this->Y2 - _y);

    float Result = NAN;

    float T[3];
    std::tie(T[0], T[1], T[2]) = solveCubic(A, B, C, D);
    for(size_t i = 0; i < 3; ++i) {
        if(std::isnan(T[i]))
            continue;
        if(T[i] < 0.f || T[i] > 1.f)
            continue;
        float Candidate = getBezierValueForT(T[i], this->X0, this->X1, this->X2, this->X3);
        if(std::isnan(Result) || Result > Candidate)
            Result = Candidate;
    }
    return Result;
}


