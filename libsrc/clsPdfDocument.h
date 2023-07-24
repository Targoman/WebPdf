#ifndef TARGOMAN_PDF_CLSPDFDOCUMENT_H
#define TARGOMAN_PDF_CLSPDFDOCUMENT_H

#include <cmath>
#include <vector>
#include "pdfla/common.h"
#include "stuMarkableSegment.h"

namespace Targoman {
namespace PDF {


class clsPdfDocumentData;
class clsPdfDocument {
protected:
    clsExplicitlySharedDataPointer<clsPdfDocumentData> Data;

public:
    clsPdfDocument();
    ~clsPdfDocument();

public:
    enuLoadError loadPdf(const uint8_t* _buffer, int _size, const stuConfigs& _configs);
    int pageCount() const;
    stuSize pageSize(int _pageIndex) const;

    ImageBuffer_t getPageImage(int _pageIndex, int _backgroundColor, const stuSize& _renderSize) const;

    SegmentVector_t getMarkables(int16_t _pageIndex) const;
    SegmentVector_t search(const std::wstring& _criteria, int _fromPage = 0, int _fromPar =0, int _fromSnt = 0, stuPoint _fromPoint = {NAN, NAN}) const;
    void setConfigs(const stuConfigs& _configs);

    stuActiveSentenceSpecs setCurrentSentence(int16_t _pageIndex, int16_t _parIndex = -1, int16_t _sntIndex = -1, enuLocation _loc = enuLocation::Main);
    stuActiveSentenceSpecs gotoNextSentence();
    stuActiveSentenceSpecs gotoPrevSentence();
    int getSentenceVirtualPageIdx() const;
    int getSentenceRealPageIdx() const;
    int getSentenceParIdx() const;
    int getSentenceIdx() const;

    bool isEncrypted();
    void setPassword(const std::string& _password);
    uint32_t firstPageNo();
    stuPDFInfo docInfo(int _pageIndex);
    std::wstring pageLabel(int _pageIndex);
    int pageNoByLabel(const std::wstring& _label);

    enuLocation getSentenceLoc() const;

    std::wstring getSentenceContent() const;
};

}
}

#endif // TARGOMAN_PDF_CLSPDFDOCUMENT_H
