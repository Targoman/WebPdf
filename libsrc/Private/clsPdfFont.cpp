#include "clsPdfFont.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <fpdfapi/fpdfapi.h>
#include <fpdfapi/fpdf_module.h>
#include <fpdfapi/fpdf_page.h>
#include <fxcodec/fx_codec.h>
#pragma GCC diagnostic pop

#include "./clsBezierCurve.h"

namespace Targoman {
namespace PDF {
namespace Private {

constexpr uint16_t MASK_ITALIC = 0x0001;
constexpr uint16_t MASK_FORMULA = 0x0002;

class PACKED clsPdfFontData {
public:
    clsPdfFontData(CPDF_Font* _handle) :
        Handle(_handle),
        UnicodeReplacementMap(new std::map<long, wchar_t>),
        Flags(0)
    {;}
    ~clsPdfFontData(){delete UnicodeReplacementMap;}
    wchar_t getReplacement(FX_DWORD _charCode) const;
    std::string familyName() const {return std::string(this->Handle->m_Font.GetFamilyName());}
    void setFontWeight(uint16_t _weight) { this->Weight = _weight; }
    int weight() const { return this->Weight; }
    const CPDF_Font* handle() const {return  Handle;}

    FLAG_PROPERTY(Italic, MASK_ITALIC)
    FLAG_PROPERTY(Formula, MASK_FORMULA)

private:
    CPDF_Font* Handle;
    std::map<long, wchar_t>* UnicodeReplacementMap;
    uint16_t Weight;
    uint16_t Flags;
};

std::tuple<float, float> getPathVerticalBounds(const std::unique_ptr<CPDF_PathData>& _path) {
    float X0 = std::numeric_limits<float>::max(),
          Y0 = std::numeric_limits<float>::max(),
          X1 = -std::numeric_limits<float>::max(),
          Y1 = -std::numeric_limits<float>::max();
    auto Points = _path->GetPoints();
    for(size_t i = 0; i < static_cast<size_t>(_path->GetPointCount()); ++i) {
        switch(Points[i].m_Flag) {
        case FXPT_MOVETO:
        case FXPT_LINETO:
        case (FXPT_LINETO + FXPT_CLOSEFIGURE):
            X0 = std::min(X0, Points[i].m_PointX);
            Y0 = std::min(Y0, Points[i].m_PointY);
            X1 = std::max(X1, Points[i].m_PointX);
            Y1 = std::max(Y1, Points[i].m_PointY);
            break;
        case FXPT_BEZIERTO:
            if(true) {
                auto Bezier = clsBezierCurve(
                            Points[i - 1].m_PointX, Points[i - 1].m_PointY,
                            Points[i].m_PointX, Points[i].m_PointY,
                            Points[i + 1].m_PointX, Points[i + 1].m_PointY,
                            Points[i + 2].m_PointX, Points[i + 2].m_PointY
                            );
                float BX0, BY0, BX1, BY1;
                std::tie(BX0, BY0, BX1, BY1) = Bezier.bounds();
                X0 = std::min(X0, BX0);
                Y0 = std::min(Y0, BY0);
                X1 = std::max(X1, BX1);
                Y1 = std::max(Y1, BY1);
            }
            i += 3;
            break;
        }
    }
    return std::make_tuple(Y0, Y1);
}

float getMinXAtY(const std::unique_ptr<CPDF_PathData>& _path, float _y) {
    float Result = std::numeric_limits<float>::max();
    auto Points = _path->GetPoints();
    for(size_t i = 0; i < static_cast<size_t>(_path->GetPointCount()); ++i) {
        switch(Points[i].m_Flag) {
        case (FXPT_LINETO + FXPT_CLOSEFIGURE):
            if(true) {
                float Y0 = std::min(Points[i].m_PointY, Points[0].m_PointY);
                float Y1 = std::max(Points[i].m_PointY, Points[0].m_PointY);
                float Candidate = NAN;
                if(_y >= Y0 && _y <= Y1) {
                    if(std::abs(Y0 - Y1) < 1e-5f)
                        Candidate = std::min(Points[i].m_PointX, Points[0].m_PointX);
                    else
                        Candidate = (Points[i].m_PointX - Points[0].m_PointX) * (_y - Points[0].m_PointY) / (Points[i].m_PointY - Points[0].m_PointY) + Points[0].m_PointX;
                }
                if(!isnan(Candidate) && Candidate < Result)
                    Result = Candidate;
            }
            [[fallthrough]];
        case FXPT_LINETO:
            if(true) {
                float Y0 = std::min(Points[i - 1].m_PointY, Points[i].m_PointY);
                float Y1 = std::max(Points[i - 1].m_PointY, Points[i].m_PointY);
                float Candidate = NAN;
                if(_y >= Y0 && _y <= Y1) {
                    if(std::abs(Y0 - Y1) < 1e-5f)
                        Candidate = std::min(Points[i - 1].m_PointX, Points[i].m_PointX);
                    else
                        Candidate = (Points[i - 1].m_PointX - Points[i].m_PointX) * (_y - Points[i].m_PointY) / (Points[i - 1].m_PointY - Points[i].m_PointY) + Points[i].m_PointX;
                }
                if(!isnan(Candidate) && Candidate < Result)
                    Result = Candidate;
            }
            break;
        case FXPT_BEZIERTO:
            if(true) {
                auto Bezier = clsBezierCurve(
                            Points[i - 1].m_PointX, Points[i - 1].m_PointY,
                            Points[i].m_PointX, Points[i].m_PointY,
                            Points[i + 1].m_PointX, Points[i + 1].m_PointY,
                            Points[i + 2].m_PointX, Points[i + 2].m_PointY
                            );
                float Candidate = Bezier.computeMinXAtY(_y);
                if(!isnan(Candidate) && Candidate < Result)
                    Result = Candidate;
            }
            i += 3;
            break;
        }
    }
    return Result;
}

bool isFormulaFont(CPDF_Font* _font) {
    float Ascent = _font->GetTypeAscent(), Descent = _font->GetTypeDescent();
    return -Descent >= 0.75f * Ascent;
}

float estimateFontItalicAngle(CPDF_Font* _font) {
    return 0.f;
    //TODO this code was buggy so discraded temporarily
    //TODO maybe font is not italic but matrix skew converts it to italic
    const wchar_t GLYPHS_TO_ESTIMATE_FROM[] = L"lIBDEFHKLNPRbh";
    for(size_t i = 0; i < sizeof GLYPHS_TO_ESTIMATE_FROM / sizeof GLYPHS_TO_ESTIMATE_FROM[0]; ++i) {
        FX_DWORD Code = _font->CharCodeFromUnicode(GLYPHS_TO_ESTIMATE_FROM[i]);
        std::unique_ptr<CFX_PathData> Path(_font->LoadGlyphPath(Code));
        if(Path == nullptr)
            continue;
        float Y0, Y1;
        std::tie(Y0, Y1) = getPathVerticalBounds(Path);
        float X0 = getMinXAtY(Path, 0.67f * Y0 + 0.33f * Y1);
        float X1 = getMinXAtY(Path, 0.67f * Y1 + 0.33f * Y0);

        return (3.14159265358979f/2.f) - std::atan2(0.34f * (Y1 - Y0), X1 - X0);
    }
    return 0.f;
}

bool isHyphen(const CFX_PathData* _path, float _ascent) {
    if(_path == nullptr)
        return false;
    auto PointCount = _path->GetPointCount();
    if(PointCount > 10) return false;
    auto Points = _path->GetPoints();
    float Y0 = Points[0].m_PointY * 1000.f, Y1 = Points[0].m_PointY * 1000.f;
    float X0 = Points[0].m_PointX * 1000.f, X1 = Points[0].m_PointX * 1000.f;
    for(size_t i = 1; i < static_cast<size_t>(PointCount); ++i) {
        Y0 = std::max(Y0, Points[i].m_PointY * 1000.f);
        Y1 = std::min(Y1, Points[i].m_PointY * 1000.f);
        X0 = std::min(X0, Points[i].m_PointX * 1000.f);
        X1 = std::max(X1, Points[i].m_PointX * 1000.f);
    }
    if(
       Y0 < _ascent * 2.f / 3.f && Y1 > _ascent / 7.f &&
       Y0 - Y1 < _ascent / 5.f && Y0 - Y1 >= .9f &&
       X1 - X0 > 4.f * std::max(2.f, (Y0 - Y1))
    )
        return true;
    return false;
}

clsPdfFont::clsPdfFont(void *_fontHandle) :
    Data(new clsPdfFontData(reinterpret_cast<CPDF_Font*>(_fontHandle)))
{
    auto PdfFontPtr = reinterpret_cast<CPDF_Font*>(_fontHandle);
    auto FamilyName = this->Data->familyName();
    this->Data->markAsItalic(std::abs(estimateFontItalicAngle(PdfFontPtr)) > (10.f * 3.1415f / 180.f) ||
                          (FamilyName.size() &&
                            (FamilyName.rfind("-I") == FamilyName.size() -  1)
                           ));
    this->Data->setFontWeight(
                (FamilyName.size() &&
                  (FamilyName.rfind("-B") == FamilyName.size() - 1)
                 ) ?
                    600 :
                    300
                );
    this->Data->markAsFormula(isFormulaFont(PdfFontPtr));
}

std::tuple<CFX_FloatRect, CFX_FloatRect> clsPdfFont::getGlyphBoxInfo(wchar_t _char) const
{
    FX_DWORD CharCode = const_cast<CPDF_Font*>(this->Data->handle())->CharCodeFromUnicode(_char);
    return this->getGlyphBoxInfoForCode(CharCode, 1000.f);
}

std::tuple<CFX_FloatRect, CFX_FloatRect> clsPdfFont::getGlyphBoxInfoForCode(FX_DWORD _charCode, float _fontSize) const
{
    CFX_FloatRect CharBox, AdvanceBox;
    FX_RECT GlyphRect;
    const_cast<CPDF_Font*>(this->Data->handle())->GetCharBBox(_charCode, GlyphRect);

    CharBox.left = GlyphRect.left * _fontSize / 1000.f;
    CharBox.top = GlyphRect.top * _fontSize / 1000.f;
    CharBox.right = GlyphRect.right * _fontSize / 1000.f;
    CharBox.bottom = GlyphRect.bottom * _fontSize / 1000.f;

    AdvanceBox.left = 0;
    AdvanceBox.top = CharBox.top;
    AdvanceBox.right = const_cast<CPDF_Font*>(this->Data->handle())->GetCharWidthF(_charCode) * _fontSize / 1000.f;
    AdvanceBox.bottom = CharBox.bottom;

    return std::make_tuple(CharBox, AdvanceBox);
}

float clsPdfFont::getTypeAscent(float _fontSize) const
{
    float Ascent = 0;
    for(wchar_t CharWithAscender : std::vector<wchar_t> {L'd', L'l', L'I', L'L'}) {
        FX_RECT RefBoundsRect;
        auto CharCode = this->Data->handle()->CharCodeFromUnicode(CharWithAscender);
        if(CharCode != 0) {
            const_cast<CPDF_Font*>(this->Data->handle())->GetCharBBox(CharCode, RefBoundsRect);
            Ascent = RefBoundsRect.top * _fontSize / 1000.f;
            break;
        }
    }
    if(std::abs(Ascent) < MIN_ITEM_SIZE) {
        FX_RECT RefBoundsRect;
        this->Data->handle()->GetFontBBox(RefBoundsRect);
        Ascent = RefBoundsRect.top * _fontSize / 1000.f;
    }
    Ascent = std::min(Ascent, this->Data->handle()->GetTypeAscent() * _fontSize / 1000.f);
    return 1.1f * Ascent;
}

float clsPdfFont::getTypeDescent(float _fontSize) const
{
    float Descent = 0;
    for(wchar_t CharWithDescender : std::vector<wchar_t> {L'g', L'j', L'p', L'q', L'y'}) {
        FX_RECT RefBoundsRect;
        auto CharCode = this->Data->handle()->CharCodeFromUnicode(CharWithDescender);
        if(CharCode != 0) {
            const_cast<CPDF_Font*>(this->Data->handle())->GetCharBBox(CharCode, RefBoundsRect);
            Descent = RefBoundsRect.bottom * _fontSize / 1000.f;
            break;
        }
    }
    if(std::abs(Descent) < MIN_ITEM_SIZE) {
        FX_RECT RefBoundsRect;
        this->Data->handle()->GetFontBBox(RefBoundsRect);
        Descent = RefBoundsRect.bottom * _fontSize / 1000.f;
    }
    Descent = std::max(Descent, this->Data->handle()->GetTypeDescent() * _fontSize / 1000.f);
    return 1.1f * Descent;
}

CFX_WideString clsPdfFont::getUnicodeFromCharCode(FX_DWORD _charCode) const
{
    auto isInvalidUnicode = [] (wchar_t _code) { return _code < L' '; };

    auto Result = const_cast<CPDF_Font*>(this->Data->handle())->UnicodeFromCharCode(_charCode);
    if(Result.GetLength() == 0 || (Result.GetLength() == 1 && isInvalidUnicode(Result.GetAt(0)))) {
        auto Replacement = this->Data->getReplacement(_charCode);
        if(Replacement != 0) {
            if(Result.GetLength() == 0)
                Result.Insert(0, FX_WCHAR(Replacement));
            else
                Result.SetAt(0, FX_WCHAR(Replacement));
        }
    }
    if(Result.GetLength() == 0)
        Result.Insert(0, FX_WCHAR(_charCode));
    return Result;
}

float clsPdfFont::getCharOffset(FX_DWORD _charCode, float _fontSize) const
{
    FX_RECT BBox;
    const_cast<CPDF_Font*>(this->Data->handle())->GetCharBBox(_charCode, BBox);
    return BBox.left * _fontSize / 1000.f;
}

float clsPdfFont::getCharAdvancement(FX_DWORD _charCode, float _fontSize) const
{
    return const_cast<CPDF_Font*>(this->Data->handle())->GetCharWidthF(_charCode) * _fontSize / 1000.f;
}

std::string clsPdfFont::familyName() const
{
    return this->Data->familyName();
}

bool clsPdfFont::isItalic() const
{
    return this->Data->isItalic();
}

bool clsPdfFont::isFormula() const
{
    return this->Data->isFormula();
}

int clsPdfFont::weight() const
{
    return this->Data->weight();
}

wchar_t clsPdfFontData::getReplacement(FX_DWORD _charCode) const
{
    auto GlyphIndex = this->Handle->GlyphFromCharCode(_charCode);
    auto CorrectionIter = this->UnicodeReplacementMap->find(GlyphIndex);
    if(CorrectionIter != this->UnicodeReplacementMap->end())
        return  CorrectionIter->second;

    float TypeAscent = this->Handle->GetTypeAscent();
    auto Path = this->Handle->LoadGlyphPathByIndex(GlyphIndex);
    wchar_t WChar = 0;
    if(isHyphen(Path, TypeAscent))
        WChar = L'-';

    (*(const_cast<clsPdfFontData*>(this)->UnicodeReplacementMap))[GlyphIndex] = WChar;

    return WChar;
}

}
}
}
