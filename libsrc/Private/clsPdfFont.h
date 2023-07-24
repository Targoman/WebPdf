#ifndef TARGOMAN_PDF_PRIVATE_CLSPDFFONT_H
#define TARGOMAN_PDF_PRIVATE_CLSPDFFONT_H

#include "./common.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <fpdfapi/fpdfapi.h>
#include <fpdfapi/fpdf_module.h>
#include <fpdfapi/fpdf_page.h>
#include <fxcodec/fx_codec.h>
#include <fxcrt/fx_coordinates.h>
#pragma GCC diagnostic pop

namespace Targoman {
namespace PDF {
namespace Private {

class clsPdfFontData;

class clsPdfFont;
typedef std::shared_ptr<clsPdfFont> PdfFontPtr_t;
typedef std::map<const void*, PdfFontPtr_t> FontInfo_t;

class clsPdfFont
{
private:
    clsExplicitlySharedDataPointer<clsPdfFontData> Data;

public:
    clsPdfFont(void *_fontHandle);

    std::tuple<CFX_FloatRect, CFX_FloatRect> getGlyphBoxInfo(wchar_t _char) const;
    std::tuple<CFX_FloatRect, CFX_FloatRect> getGlyphBoxInfoForCode(FX_DWORD _charCode, float _fontSize) const;
    float getTypeAscent(float _fontSize) const;
    float getTypeDescent(float _fontSize) const;

    CFX_WideString getUnicodeFromCharCode(FX_DWORD _charCode) const;

    float getCharOffset(FX_DWORD _charCode, float _fontSize) const;
    float getCharAdvancement(FX_DWORD _charCode, float _fontSize) const;

public:
    std::string familyName() const;
    bool isItalic() const;
    bool isFormula() const;
    int weight() const;
};

}
}
}
#endif // TARGOMAN_PDF_PRIVATE_CLSPDFFONT_H
