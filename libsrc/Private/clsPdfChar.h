#ifndef TARGOMAN_PDF_PRIVATE_CLSPDFCHAR_H
#define TARGOMAN_PDF_PRIVATE_CLSPDFCHAR_H

#include "clsPdfItem.h"

namespace Targoman {
namespace PDF {
namespace Private {

class clsPdfCharData;
class clsPdfChar : public clsPdfItem
{
private:
    clsExplicitlySharedDataPointer<clsPdfCharData> Data;
    void detach() override final;

public:
    clsPdfChar();

    clsPdfChar(float _left,
               float _top,
               float _right,
               float _bottom,
               float _ascent,
               float _descent,
               float _offset,
               float _advance,
               wchar_t _code,
               float _baseline,
               void* _font,
               float _fontSize,
               float _angle
            );

    clsPdfChar(const clsPdfChar& _other) : clsPdfItem(_other) { this->Data = _other.Data; }
    ~clsPdfChar() override;


public:
    void markAsItalic(bool _state);
    bool isItalic() const;
    void markAsVirtual(bool _state);
    bool isVirtual() const;

public:
    wchar_t code() const;
    float baseline() const;
    float offset() const;
    float advance() const;
    void* font() const;
    float fontSize() const;
    float angle() const;
    bool isSame(const stuPageMargins &_currItemPageMargins, const PdfItemPtr_t& _other, const stuPageMargins &_otherItemPageMargins) const override;

public:
    bool conformsToLine(float _ascent, float _descent, float _baseline) const override;

public:
    inline bool isOneOf(const wchar_t _items[], size_t _size) const {
        for(size_t i = 0; i < _size; ++i)
            if(this->code() == _items[i])
                return true;
        return false;
    }

    inline bool isSpace() const {
        constexpr wchar_t ALL_SPACES[] = {
            L'\x0020', // SPACE
            L'\x00A0', // NO-BREAK SPACE
            L'\x1680', // OGHAM SPACE MARK
            L'\x180E', // MONGOLIAN VOWEL SEPARATOR
            L'\x2000', // EN QUAD
            L'\x2001', // EM QUAD
            L'\x2002', // EN SPACE (nut)
            L'\x2003', // EM SPACE (mutton)
            L'\x2004', // THREE-PER-EM SPACE (thick space)
            L'\x2005', // FOUR-PER-EM SPACE (mid space)
            L'\x2006', // SIX-PER-EM SPACE
            L'\x2007', // FIGURE SPACE
            L'\x2008', // PUNCTUATION SPACE
            L'\x2009', // THIN SPACE
            L'\x200A', // HAIR SPACE
            //            L'\x200B', // ZERO WIDTH SPACE
            L'\x202F', // NARROW NO-BREAK SPACE
            L'\x205F', // MEDIUM MATHEMATICAL SPACE
            L'\x3000', // IDEOGRAPHIC SPACE
            //            L'\xFEFF' // ZERO WIDTH NO-BREAK SPAC
        };
        return this->isOneOf(ALL_SPACES, sizeof ALL_SPACES / sizeof ALL_SPACES[0]);
    }

    inline bool isDot() const {
        constexpr wchar_t ALL_DOTS[] = {
            L'.', L'\xFF0E', L'\x0387', L'\x2024', L'\xFE52'
        };
        return this->isOneOf(ALL_DOTS, sizeof ALL_DOTS / sizeof ALL_DOTS[0]);
    }

    inline bool isUppercaseAlpha() const {
        return this->code() == 0x0395 || (this->code() >= L'A' && this->code() <= L'Z');
    }

    inline bool isLowercaseAlpha() const {
        return this->code() >= L'a' && this->code() <= L'z';
    }

    inline bool isDecimalDigit() const {
        return this->code() >= L'0' && this->code() <= L'9';
    }

    inline bool isAlphaNumeric() const {
        return
            (this->code() >= 'a' && this->code() <= 'z') ||
            (this->code() >= 'A' && this->code() <= 'Z') ||
            (this->code() >= '0' && this->code() <= '9');

    }

    inline bool canBeSuperOrSubscript() const {
        return
                this->code() > L' ' &&
                this->code() != L'•' &&
                this->code() != SUB_SCR_START &&
                this->code() != SUB_SCR_END &&
                this->code() != SUPER_SCR_START &&
                this->code() != SUPER_SCR_END &&
                this->code() != LIST_START &&
                this->code() != LIST_END;
    }

};

}
}
}

#endif // TARGOMAN_PDF_PRIVATE_CLSPDFCHAR_H
