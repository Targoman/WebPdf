#ifndef TARGOMAN_PDF_PRIVATE_CLSPDFIUMWRAPPER_H
#define TARGOMAN_PDF_PRIVATE_CLSPDFIUMWRAPPER_H


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <fpdfapi/fpdfapi.h>
#include <fpdfapi/fpdf_module.h>
#include <fpdfapi/fpdf_page.h>
#include <fxcodec/fx_codec.h>
#include <fxcrt/fx_coordinates.h>
#include <fpdfdoc/fpdf_doc.h>
#pragma GCC diagnostic pop

#include "clsPdfItem.h"
#include "clsPdfFont.h"
#include "common.h"

namespace Targoman {
namespace PDF {
namespace Private {

inline void initializePdfiumModules()
{
    CPDF_ModuleMgr::Create();
    CFX_GEModule::Create();

    auto Codec = CCodec_ModuleMgr::Create();
    CFX_GEModule::Get()->SetCodecModule(Codec);
    CPDF_ModuleMgr::Get()->SetCodecModule(Codec);

    CPDF_ModuleMgr::Get()->InitPageModule();
    CPDF_ModuleMgr::Get()->InitRenderModule();
    CPDF_ModuleMgr::Get()->SetDownloadCallback(nullptr);
}

struct stuDoublyLinkedList
{
    stuDoublyLinkedList *Next;
    stuDoublyLinkedList *Prev;
    void *Data;
};

class clsPdfiumWrapper
{
public:
    clsPdfiumWrapper(const stuConfigsPtr_t& _configs);
    ~clsPdfiumWrapper();

    enuLoadError loadPDF(const uint8_t *_buffer, int _size);

    std::tuple<PdfItemPtrVector_t, stuPageMargins> getPageItems(int _pageIndex) const;
    std::vector<uint8_t> getPDFRawData();
    bool isEncrypted();
    void setPassword(const std::string& _password);
    uint32_t firstPageNo();
    void fillInfo(stuPDFInfo& _infoStorage);
    int pageCount() const;
    std::wstring pageLabel(int _pageIndex);
    int pageNoByLabel(std::wstring _pageLabel);
    stuSize pageSize(int _pageIndex) const;
    ImageBuffer_t getPageImage(int _pageIndex, int _backgroundColor, const stuSize& _renderSize) const;

    const FontInfo_t& fontInfo(){
        return this->FontInfo;
    }

private:
    stuPageMargins appendObjectsToListComputingMargins(stuDoublyLinkedList* _objectPos,
                                                       CFX_AffineMatrix* _baseTransformMatrix,
                                                       const stuSize &_pageSize,
                                                       PdfItemPtrVector_t& Items
                                                       ) const;

    ::CPDF_Page* cpdfPage(int _pageIndex) const;

private:
    tmplLimitedCache<int, ::CPDF_Page*, MAX_CACHED_PAGES> CachedPages;
    CPDF_Parser* Parser;
    CPDF_PageLabel* PageLabels;
    FontInfo_t FontInfo;
    const stuConfigsPtr_t& Configs;
};

}
}
}
#endif // TARGOMAN_PDF_PRIVATE_CLSPDFIUMWRAPPER_H
