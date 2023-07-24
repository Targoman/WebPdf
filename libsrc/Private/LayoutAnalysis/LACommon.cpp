#include "LACommon.h"

namespace Targoman {
namespace PDF {
namespace Private {
namespace LA {

bool isTableCaption(const std::wstring& _str){
    return _str.find(L"Table") == 0
            || _str.find(L"table") == 0
            || _str.find(L"TABLE") == 0;
}

bool isImageCaption(const std::wstring& _str){
    return _str.find(L"Fig.") == 0
            || _str.find(L"FIG.") == 0
            || _str.find(L"FIG ") == 0
            || _str.find(L"Fig ") == 0
            || _str.find(L"Figure") == 0
            || _str.find(L"figure") == 0
            || _str.find(L"Scheme") == 0
            || _str.find(L"scheme") == 0;
}

bool canBeTable(const PdfBlockPtr_t& _block, bool _lastWasLine){
    auto a = _block->horzRulers(.5f * _block->width());
    auto b = _block->vertRulers(.5f * _block->height());
    if(_block->horzRulers(.5f * _block->width()).size() > 0 ||
       _block->innerColsCountByLayout() > 1)
        return true;

    return false;
}

}
}
}
}
