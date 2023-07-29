#include <pdfla/clsPdfDocument.h>
#include <emscripten.h>
#include <emscripten/bind.h>
#include <codecvt>
#include <locale>

using namespace emscripten;
using namespace Targoman::PDF;

val getPageImage(clsPdfDocument* _this, int _pageIndex, int _backgroundColor, const stuSize& _renderSize) {
    auto PageImageData = _this->getPageImage(_pageIndex, _backgroundColor, _renderSize);
    auto Offset = reinterpret_cast<uint64_t>(PageImageData.data());
    return val::module_property("HEAPU8")["buffer"].call<val>("slice", static_cast<int>(Offset), static_cast<int>(Offset + PageImageData.size()));
}

std::vector<stuSize> getAllPageSizes(clsPdfDocument* _this) {
    std::vector<stuSize> Result;
    for(int i = 0; i < _this->pageCount(); ++i)
        Result.push_back(_this->pageSize(i));
    return Result;
}

clsPdfDocument* constructPdfDocument() {
    return new clsPdfDocument();
}

enuLoadError loadPdfDoc(clsPdfDocument* _this, int _bufferStartPos, int _bufferSize, stuConfigs _configs){
    return _this->loadPdf(reinterpret_cast<const uint8_t*>(_bufferStartPos), _bufferSize, _configs);
}

std::wstring string2wstring(const char* _buff, int _size){
    //setup converter
    using convert_type = std::codecvt_utf8<typename std::wstring::value_type>;
    std::wstring_convert<convert_type, typename std::wstring::value_type> converter;

    //use converter (.to_bytes: wstr->str, .from_bytes: str->wstr)
    return converter.from_bytes(_buff, _buff + _size);
}

int pageNoByLabel(clsPdfDocument* _this, int _bufferStartPos, int _bufferSize){
    return  _this->pageNoByLabel(string2wstring(reinterpret_cast<const char*>(_bufferStartPos), _bufferSize));
}


EMSCRIPTEN_BINDINGS(tgpdf)
{
    value_object<stuConfigs>("stuConfigs")
            .field("RemoveWatermarks", &stuConfigs::RemoveWatermarks)
            .field("AssumeFirstAloneLineAsHeader", &stuConfigs::AssumeFirstAloneLineAsHeader)
            .field("DiscardHeaders", &stuConfigs::DiscardHeaders)
            .field("AssumeLastAloneLineAsFooter", &stuConfigs::AssumeLastAloneLineAsFooter)
            .field("DiscardFooters", &stuConfigs::DiscardFooters)
            .field("DiscardItemsIn2PercentOfPageMargin", &stuConfigs::DiscardItemsIn2PercentOfPageMargin)
            .field("DiscardSidebars", &stuConfigs::DiscardSidebars)
            .field("DiscardFootnotes", &stuConfigs::DiscardFootnotes)
            .field("DiscardTablesAndCaptions", &stuConfigs::DiscardTablesAndCaptions)
            .field("DiscardImagesAndCaptions", &stuConfigs::DiscardImagesAndCaptions)
            .field("MarkSubcripts", &stuConfigs::MarkSubcripts)
            .field("MarkSuperScripts", &stuConfigs::MarkSuperScripts)
            .field("MarkBullets", &stuConfigs::MarkBullets)
            .field("MarkNumberings", &stuConfigs::MarkNumberings)
            .field("ASCIIOffset", &stuConfigs::ASCIIOffset);

    value_object<stuSize>("stuSize")
            .field("Width", &stuSize::Width)
            .field("Height", &stuSize::Height);

    enum_<enuLocation>("enuLocation")
            .value("None", enuLocation::None)
            .value("Main", enuLocation::Main)
            .value("Header", enuLocation::Header)
            .value("Footnote", enuLocation::Footnote)
            .value("Footer", enuLocation::Footer)
            .value("Watermark", enuLocation::Watermark)
            .value("SidebarLeft", enuLocation::SidebarLeft)
            .value("SidebarRight", enuLocation::SidebarRight);

    enum_<enuLoadError>("enuLoadError")
            .value("None", enuLoadError::None)
            .value("File", enuLoadError::File)
            .value("Format", enuLoadError::Format)
            .value("Handler", enuLoadError::Handler)
            .value("Password", enuLoadError::Password)
            .value("Cert", enuLoadError::Cert);

    value_object<stuPDFInfo>("stuPDFInfo")
            .field("FileName", &stuPDFInfo::FileName)
            .field("FileSize", &stuPDFInfo::FileSize)
            .field("Title", &stuPDFInfo::Title)
            .field("Author", &stuPDFInfo::Author)
            .field("Subject", &stuPDFInfo::Subject)
            .field("Keywords", &stuPDFInfo::Keywords)
            .field("CreationDate", &stuPDFInfo::CreationDate)
            .field("ModificationDate", &stuPDFInfo::ModificationDate)
            .field("Creator", &stuPDFInfo::Creator)
            .field("PDFProducer", &stuPDFInfo::PDFProducer)
            .field("PDFVersion", &stuPDFInfo::PDFVersion)
            .field("PageCount", &stuPDFInfo::PageCount)
            .field("PageSize", &stuPDFInfo::PageSize)
            ;

    value_object<stuBox>("stuBox")
            .field("X0", &stuBox::X0)
            .field("Y0", &stuBox::Y0)
            .field("X1", &stuBox::X1)
            .field("Y1", &stuBox::Y1)
            .field("Width", &stuBox::width, &stuBox::setWidth)
            .field("Height", &stuBox::height, &stuBox::setHeight);

    enum_<enuContentType>("enuContentType")
            .value("None", enuContentType::None)
            .value("Text", enuContentType::Text)
            .value("Image", enuContentType::Image)
            .value("Table", enuContentType::Table)
            .value("ImageText", enuContentType::ImageText)
            .value("ImageCaption", enuContentType::ImageCaption)
            .value("TableCaption", enuContentType::TableCaption);

    value_object<stuMarkableSegment>("stuMarkable")
            .field("PageIndex", &stuMarkableSegment::PageIndex)
            .field("ParIndex", &stuMarkableSegment::ParIndex)
            .field("SntIndex", &stuMarkableSegment::SntIndex)
            .field("BoundingBox", &stuMarkableSegment::BoundingBox)
            .field("Location", &stuMarkableSegment::Location)
            .field("Type", &stuMarkableSegment::Type)
            .field("InnerSegments", &stuMarkableSegment::InnerSegments);

    value_object<stuActiveSentenceSpecs>("stuActiveSentenceSpecs")
            .field("PageIndex", &stuActiveSentenceSpecs::PageIndex)
            .field("RealPageIndex", &stuActiveSentenceSpecs::RealPageIndex)
            .field("ParIndex", &stuActiveSentenceSpecs::ParIndex)
            .field("SntIndex", &stuActiveSentenceSpecs::SntIndex)
            .field("Location", &stuActiveSentenceSpecs::Location);



    register_vector<stuMarkableSegment>("MarkableVector_t");
    register_vector<stuSize>("SizeVector_t");

    class_<clsPdfDocument>("clsPdfDocument")
            .class_function("new", constructPdfDocument, allow_raw_pointers())
            .property("PageCount", &clsPdfDocument::pageCount)
            .function("loadPdf", &loadPdfDoc, allow_raw_pointers())
            .function("pageSize", &clsPdfDocument::pageSize)
            .function("getAllPageSizes", &getAllPageSizes, allow_raw_pointers())
            .function("getPageImage", &getPageImage, allow_raw_pointers())
            .function("getPDFDocInfo", &clsPdfDocument::docInfo)
            .function("pageLabel", &clsPdfDocument::pageLabel)
            .function("pageNoByLabel", &clsPdfDocument::pageNoByLabel)
            //.function("getOutline", )
            //.function("getAttachments", )
            //.function("setPassword", )
            .function("getMarkables", &clsPdfDocument::getMarkables)
            .function("search", &clsPdfDocument::search)
            .function("setConfigs", &clsPdfDocument::setConfigs)
            .function("setCurrentSentence", &clsPdfDocument::setCurrentSentence)
            .function("gotoNextSentence", &clsPdfDocument::gotoNextSentence)
            .function("gotoPrevSentence", &clsPdfDocument::gotoPrevSentence)
            .function("getSentenceContent", &clsPdfDocument::getSentenceContent)
            .property("SentenceVirtualPageIdx", &clsPdfDocument::getSentenceVirtualPageIdx)
            .property("SentenceRealPageIdx", &clsPdfDocument::getSentenceRealPageIdx)
            .property("SentenceParIdx", &clsPdfDocument::getSentenceParIdx)
            .property("SentenceIdx", &clsPdfDocument::getSentenceIdx);

}
