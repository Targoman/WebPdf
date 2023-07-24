#ifndef TARGOMAN_PDF_COMMON_H
#define TARGOMAN_PDF_COMMON_H

#include <memory>
#include <vector>
#include <map>
#include <tuple>
#include <functional>
#include <cmath>
#include <algorithm>
#include <limits>
#include <cassert>
#include <string>

#ifdef TARGOMAN_SHOW_DEBUG
#include <iostream>
#include <sstream>
#endif



#define IN
#define OUT

#ifdef TARGOMAN_SHOW_DEBUG
extern int gPageIndex;
extern int gMaxStep;
#endif

class CFX_Matrix;

namespace Targoman {
namespace PDF {

constexpr float F_AUTO = std::numeric_limits<float>::min();

struct stuPoint{
    float X, Y;
    stuPoint(float _x = F_AUTO, float _y = F_AUTO) :X(_x), Y(_y) {}
    stuPoint(double _x, double _y) :X(static_cast<float>(_x)), Y(static_cast<float>(_y)) {}
};

struct stuRawBuffer {
    const char* Buff;
    size_t Size;
};

struct stuConfigs{
    bool RemoveWatermarks;
    bool DiscardTablesAndCaptions;
    bool DiscardImagesAndCaptions;
    bool DiscardHeaders;
    bool DiscardFooters;
    bool DiscardSidebars;
    bool DiscardFootnotes;
    bool AssumeLastAloneLineAsFooter;
    bool AssumeFirstAloneLineAsHeader;
    bool DiscardItemsIn2PercentOfPageMargin;
    bool MarkSubcripts;
    bool MarkSuperScripts;
    bool MarkBullets;
    bool MarkNumberings;
    int8_t ASCIIOffset;
#ifdef TARGOMAN_SHOW_DEBUG
    uint8_t MidStepDebugLevel;
#endif
    stuConfigs(
            bool _removeWatermarks = true,
            bool _discardTablesAndCaptions = true,
            bool _discardImagesAndCaptions = true,
            bool _discardHeaders = true,
            bool _discardFooters = true,
            bool _discardSidebars = true,
            bool _discardFootnotes = true,
            bool _assumeLastAloneLineAsFooter = true,
            bool _assumeFirstAloneLineAsHeader = true,
            bool _discardItemsIn2PercentOfPageMargin = true,
            bool _markSubcripts = true,
            bool _markSuperScripts = true,
            bool _markBullets = true,
            bool _markNumberings = true,
            int8_t _asciiOffset = 0
#ifdef TARGOMAN_SHOW_DEBUG
            , uint8_t _midStepDebugLevel = 0
#endif
            ) :
        RemoveWatermarks(_removeWatermarks),
        DiscardTablesAndCaptions(_discardTablesAndCaptions),
        DiscardImagesAndCaptions(_discardImagesAndCaptions),
        DiscardHeaders(_discardHeaders),
        DiscardFooters(_discardFooters),
        DiscardSidebars(_discardSidebars),
        DiscardFootnotes(_discardFootnotes),
        AssumeLastAloneLineAsFooter(_assumeLastAloneLineAsFooter),
        AssumeFirstAloneLineAsHeader(_assumeFirstAloneLineAsHeader),
        DiscardItemsIn2PercentOfPageMargin(_discardItemsIn2PercentOfPageMargin),
        MarkSubcripts(_markSubcripts),
        MarkSuperScripts(_markSuperScripts),
        MarkBullets(_markBullets),
        MarkNumberings(_markNumberings),
        ASCIIOffset(_asciiOffset)
#ifdef TARGOMAN_SHOW_DEBUG
        , MidStepDebugLevel(_midStepDebugLevel)
#endif
    {}

    std::string toStr() const {
#ifdef TARGOMAN_SHOW_DEBUG
        std::stringstream ss;
        ss<<
                "\tRemoveWatermarks"<<RemoveWatermarks<<"\n"<<
                "\tDiscardTablesAndCaptions"<<DiscardTablesAndCaptions<<"\n"<<
                "\tDiscardImagesAndCaptions"<<DiscardImagesAndCaptions<<"\n"<<
                "\tDiscardHeaders"<<DiscardHeaders<<"\n"<<
                "\tDiscardFooters"<<DiscardFooters<<"\n"<<
                "\tDiscardSidebars"<<DiscardSidebars<<"\n"<<
                "\tDiscardFootnotes"<<DiscardFootnotes<<"\n"<<
                "\tAssumeLastAloneLineAsFooter"<<AssumeLastAloneLineAsFooter<<"\n"<<
                "\tAssumeFirstAloneLineAsHeader"<<AssumeFirstAloneLineAsHeader<<"\n"<<
                "\tDiscardItemsIn2PercentOfPageMargin"<<DiscardItemsIn2PercentOfPageMargin<<"\n"<<
                "\tMarkSubcripts"<<MarkSubcripts<<"\n"<<
                "\tMarkSuperScripts"<<MarkSuperScripts<<"\n"<<
                "\tMarkBullets"<<MarkBullets<<"\n"<<
                "\tMarkNumberings"<<MarkNumberings<<"\n";
             return ss.str();
#else
        return "";
#endif
    }
};


constexpr wchar_t OBJECT_CHAR = L'\uFFFC';
constexpr wchar_t SUPER_SCR_START = L'\u207D';
constexpr wchar_t SUPER_SCR_END = L'\u207E';
constexpr wchar_t SUB_SCR_START = L'\u208D';
constexpr wchar_t SUB_SCR_END = L'\u208E';
constexpr wchar_t LIST_START = L'\u02E1';
constexpr wchar_t LIST_END = L'\u2097';

typedef std::vector<uint8_t> ImageBuffer_t;

enum class enuLoadError{
    None,
    File,
    Format,
    Password,
    Handler,
    Cert
};

enum class enuLocation{
    Main,
    Header,
    Footer,
    SidebarLeft,
    SidebarRight,
    Footnote,
    Watermark,
    None
};

extern std::vector<enuLocation> PdfLocations;

enum class enuContentType {
    None,
    Image,
    Table,
    TableCaption,
    ImageCaption,
    ImageText,
    List,
    Text
};

struct stuSize {
    float Width;
    float Height;

    stuSize(float _w =0, float _h = 0) : Width(_w), Height(_h) {}
};

struct stuPDFInfo {
    std::wstring FileName;
    std::wstring Title;
    std::wstring Author;
    std::wstring Subject;
    std::wstring Keywords;
    std::wstring CreationDate;
    std::wstring ModificationDate;
    std::wstring Creator;
    std::wstring PDFProducer;
    std::wstring PDFVersion;
    size_t  PageCount;
    stuSize PageSize;
    size_t  FileSize;
};

struct stuActiveSentenceSpecs {
    int16_t PageIndex, RealPageIndex, ParIndex, SntIndex;
    enuLocation Location;

    stuActiveSentenceSpecs(int16_t _pageIndex = -1,
                           int16_t _realPageIndex = -1,
                           int16_t _parIndex = -1,
                           int16_t _sntIndex = -1,
                           enuLocation _loc = enuLocation::None):
        PageIndex(_pageIndex), RealPageIndex(_realPageIndex), ParIndex(_parIndex), SntIndex(_sntIndex), Location(_loc)
    {}

    inline bool isValid(){
        return  Location != enuLocation::None;
    }
};

template<typename T>
class clsExplicitlySharedDataPointer : public std::shared_ptr<T> {
public:
    clsExplicitlySharedDataPointer() : std::shared_ptr<T>() { }
    clsExplicitlySharedDataPointer(T* _pointer) :
        std::shared_ptr<T>(_pointer)
    {
        assert(_pointer != nullptr);
    }

    clsExplicitlySharedDataPointer& operator = (const clsExplicitlySharedDataPointer& _other)
    {
         std::shared_ptr<T>::operator=(_other);
         return *this;
    }

    void reset(T* _pointer) {
        assert(_pointer != nullptr);
        std::shared_ptr<T>::reset(_pointer);
    }

public:
    void detach() {
        if(this->use_count() > 1) {
            T* Copy = new T(*this->get());
            this->reset(Copy);
        }
    }
};

void ltrim(std::wstring &s);
void rtrim(std::wstring &s);
void trim(std::wstring &s);

template<typename T, typename Compare_t>
inline void sort(std::vector<T>& _v, Compare_t _compare) {
    for(int i = 1; i < static_cast<int>(_v.size()); ++i) {
        int j = 0, k = i;
        while(j < k) {
            int l = (j + k) / 2;
            if(_compare(_v.at(i), _v.at(l)))
                k = l;
            else {
                if(j == l) {
                    if(_compare(_v.at(i), _v.at(k)))
                        k = l;
                    else
                        j = k;
                } else
                    j = l;
            }
        }
        assert(j == k);
        while(j < i && _compare(_v.at(i), _v.at(j)) == false)
            ++j;
        if(j != i) {
            T CurrentValue = _v.at(i);
            for(size_t m = static_cast<size_t>(i); m > static_cast<size_t>(j); --m)
                _v.at(m) = _v.at(m - 1);
            _v.at(j) = CurrentValue;
        }
    }
}

template<typename T>
inline void sort(std::vector<T>& _v) {
    sort(_v, [] (const T& a, const T& b) { return a < b; });
}

typedef clsExplicitlySharedDataPointer<stuConfigs> stuConfigsPtr_t;

}
}

#endif // TARGOMAN_PDF_COMMON_H
