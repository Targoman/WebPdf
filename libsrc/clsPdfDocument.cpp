#include "clsPdfDocument.h"
#include <map>
#include <limits>

#include "Private/clsPdfItem.h"
#include "Private/clsLayoutAnalyzer.h"
#include "Private/clsBezierCurve.h"
#include "Private/common.h"
#include "Private/clsPdfiumWrapper.h"

namespace Targoman {
namespace PDF {

class clsPdfDocumentData {
public:
    int16_t CurrentRealPageIndex;
    int16_t CurrentPageIndex;
    int16_t CurrentParIndex;
    int16_t CurrentSntIndex;
    int16_t CurrParInPageIndex;
    int16_t CurrLineInParIndex;
    enuLocation CurrentLocation;
    enuContentType CurrContentType;
    int8_t __PADDING[4];
    stuConfigsPtr_t Configs;
    stuPDFInfo Info;
    Private::clsPdfiumWrapper PdfiumWrapper;
    Private::clsLayoutAnalyzer LayoutAnalyzer;

public:
    int pageCount() const;

    clsPdfDocumentData(const stuConfigs& _configs) :
        CurrentRealPageIndex(-1),
        CurrentPageIndex(-1),
        CurrentParIndex(-1),
        CurrentSntIndex(-1),
        CurrentLocation(enuLocation::Main),
        CurrContentType(enuContentType::Text),
        Configs(new stuConfigs(_configs)),
        PdfiumWrapper(Configs),
        LayoutAnalyzer(PdfiumWrapper, Configs)
    {;}

    stuActiveSentenceSpecs setSentence(enuLocation _loc,
                                       enuContentType _contentType,
                                       int16_t _realPageIndex,
                                       int16_t _currParInPageIndex,
                                       int16_t _currLineInParIndex,
                                       const Private::PdfLineSegmentPtr_t &_line);

    ~clsPdfDocumentData();
};

static bool __pdfiumModulesInitialized__ = false;
clsPdfDocument::clsPdfDocument()
{
    if (__builtin_expect(__pdfiumModulesInitialized__, true) == false) {
        Private::initializePdfiumModules();
        __pdfiumModulesInitialized__ = true;
    }
}

clsPdfDocument::~clsPdfDocument()
{}

enuLoadError clsPdfDocument::loadPdf(const uint8_t* _buffer, int _size, const stuConfigs &_configs){
    this->Data.reset(new clsPdfDocumentData(_configs));

    auto LoadError = this->Data->PdfiumWrapper.loadPDF(_buffer, _size);
    if(LoadError == enuLoadError::None)
        this->Data->PdfiumWrapper.fillInfo(this->Data->Info);
    return LoadError;
}

int clsPdfDocument::pageCount() const
{
    return this->Data->pageCount();
}

stuSize clsPdfDocument::pageSize(int _pageIndex) const
{
    return this->Data->PdfiumWrapper.pageSize(_pageIndex);
}

ImageBuffer_t clsPdfDocument::getPageImage(int _pageIndex, int _backgroundColor, const stuSize& _renderSize) const
{
    return this->Data->PdfiumWrapper.getPageImage(_pageIndex, _backgroundColor, _renderSize);
}

SegmentVector_t clsPdfDocument::getMarkables(int16_t _pageIndex) const
{
    SegmentVector_t Result;
    if(_pageIndex < 0 || _pageIndex > this->Data->pageCount())
        return Result;
    const auto& PageContent = this->Data->LayoutAnalyzer.getPageContent(_pageIndex, this->Data->PdfiumWrapper.fontInfo());
    int16_t NonTextParIndex = 0;
    for(const auto& Block: PageContent) {
        SegmentVector_t Lines;
        for(const auto& Line : Block->lines())
            Lines.push_back(stuMarkableSegment(
                                Line->pageIndex(),
                                Line->parIndex(),
                                Line->sntIndex(),
                                stuBox(Line->x0(), Line->y0(), Line->x1(), Line->y1()),
                                Block->location(),
                                Block->contentType()
                                ));

        Result.push_back(stuMarkableSegment(
                             Block->lines().size() ? Block->lines().front()->pageIndex() : _pageIndex,
                             Block->lines().size() ? Block->lines().front()->parIndex() : NonTextParIndex++,
                             -1,
                             stuBox(Block->x0(), Block->y0(), Block->x1(), Block->y1()),
                             Block->location(),
                             Block->contentType(),
                             Lines
                             ));
    }

    return Result;
}

SegmentVector_t clsPdfDocument::search(const std::wstring& _criteria, int _fromPage, int _fromPar, int _fromSnt, stuPoint _fromPoint) const
{
    // TODO: Do something here! (clsPdfDocument::search)
    (void)_criteria;
    (void)_fromPage;
    (void)_fromPar;
    (void)_fromSnt;
    (void)_fromPoint;
    return SegmentVector_t();
}

void clsPdfDocument::setConfigs(const stuConfigs& _configs)
{
    *this->Data->Configs = _configs;
    this->Data->LayoutAnalyzer.clearCache();
    printf("%s", this->Data->Configs->toStr().c_str());
}

stuActiveSentenceSpecs clsPdfDocument::setCurrentSentence(int16_t _pageIndex, int16_t _parIndex, int16_t _sntIndex, enuLocation _loc)
{
    if(_pageIndex >= this->pageCount()) return stuActiveSentenceSpecs();
    if(_pageIndex < 0) _pageIndex = 0;

    int16_t RealPageIndex = _pageIndex;
    int16_t CurrParInPageIndex;
    int16_t CurrLineInParIndex;

    auto setSentence = [&](const Private::PdfBlockPtr_t _par, const Private::PdfLineSegmentPtr_t& _line) {
        return this->Data->setSentence(_par->location(),
                                       _par->contentType(),
                                       RealPageIndex,
                                       CurrParInPageIndex,
                                       CurrLineInParIndex,
                                       _line);
    };

    for(RealPageIndex = _pageIndex; RealPageIndex < this->Data->pageCount(); ++RealPageIndex){
        const auto& PageParagraphs = this->Data->LayoutAnalyzer.getPageContent(RealPageIndex, this->Data->PdfiumWrapper.fontInfo());
        if(PageParagraphs.size() == 0) {
            if(RealPageIndex > _pageIndex)
                continue;
            else
                return stuActiveSentenceSpecs();
        }

        CurrParInPageIndex = -1;
        for(const auto& Paragraph : PageParagraphs){
            ++CurrParInPageIndex;
            if(Paragraph->location() != _loc || Paragraph->lines().size() == 0)
                continue;
            if(Paragraph->lines().front()->pageIndex() != _pageIndex
               || Paragraph->lines().front()->parIndex() < _parIndex)
                continue;
            if(Paragraph->lines().front()->pageIndex() > _pageIndex)
                return stuActiveSentenceSpecs();

            CurrLineInParIndex = -1;
            for(const auto& Line : Paragraph->lines()){
                ++CurrLineInParIndex;
                if(Line->isPureImage()) continue;
                if(Line->pageIndex() > _pageIndex)
                    return stuActiveSentenceSpecs();

                if(Line->pageIndex() == _pageIndex
                   && (_parIndex < 0 || Line->parIndex() == _parIndex)
                   && (Line->sntIndex() == _sntIndex
                       ||(_sntIndex < 0
                        && (_pageIndex == 0
                            || Line->parIndex() == 0
                            || Line->sntIndex() > Paragraph->lines().front()->sntIndex())
                        )))
                    return  setSentence(Paragraph, Line);
            }
        }
        if(_loc != enuLocation::Main)
            break;
    }
    return  stuActiveSentenceSpecs();
}
bool areSameType(enuContentType _parType, enuContentType _requiredType){
    return _parType == _requiredType ||
            (_parType == enuContentType::Text &&
             _requiredType == enuContentType::List) ||
            (_parType == enuContentType::List &&
             _requiredType == enuContentType::Text);
};

stuActiveSentenceSpecs clsPdfDocument::gotoNextSentence()
{
    if(this->Data->CurrentPageIndex < 0)
        return this->setCurrentSentence(0);

    for(int16_t RealPageIndex = this->Data->CurrentRealPageIndex;
        RealPageIndex < this->Data->pageCount();
        ++RealPageIndex){
        auto PageParagraphs = this->Data->LayoutAnalyzer.getPageContent(RealPageIndex, this->Data->PdfiumWrapper.fontInfo());

        for (uint16_t ParIndex = (RealPageIndex == this->Data->CurrentRealPageIndex ?
                                static_cast<uint16_t>(this->Data->CurrParInPageIndex) : 0);
             ParIndex < PageParagraphs.size();
             ++ParIndex){
            auto Paragraph = PageParagraphs.at(ParIndex);
            if(Paragraph->location() != this->Data->CurrentLocation) continue;
            if(Paragraph->lines().size() == 0) continue;
            if(!areSameType(Paragraph->contentType(), this->Data->CurrContentType)) continue;

            for(uint16_t LineIndex = (RealPageIndex <= this->Data->CurrentRealPageIndex
                                     && ParIndex == static_cast<uint16_t>(this->Data->CurrParInPageIndex)
                                     ?  static_cast<uint16_t>(this->Data->CurrLineInParIndex) + 1 : 0);
                LineIndex < Paragraph->lines().size();
                ++LineIndex){
                auto Line = Paragraph->lines().at(LineIndex);

                if(Line->isPureImage()) continue;

                if(Line->pageIndex() > this->Data->CurrentPageIndex ||
                   Line->parIndex() > this->Data->CurrentParIndex ||
                   Line->sntIndex() > this->Data->CurrentSntIndex
                   ){
                    return this->Data->setSentence(Paragraph->location(),
                                                   Paragraph->contentType(),
                                                   RealPageIndex,
                                                   static_cast<int16_t>(ParIndex),
                                                   static_cast<int16_t>(LineIndex),
                                                   Line);
                }
            }
        }
    }

    return stuActiveSentenceSpecs();
}

stuActiveSentenceSpecs clsPdfDocument::gotoPrevSentence()
{
    if(this->Data->CurrentPageIndex < 0)
        return this->setCurrentSentence(0);


    auto gotoPrevSentenceLambda = [&](){
        for(int16_t RealPageIndex = this->Data->CurrentRealPageIndex; RealPageIndex >= 0; --RealPageIndex){
            auto PageParagraphs = this->Data->LayoutAnalyzer.getPageContent(RealPageIndex, this->Data->PdfiumWrapper.fontInfo());

            for (int16_t ParIndex = (RealPageIndex == this->Data->CurrentRealPageIndex ?
                                       static_cast<int16_t>(this->Data->CurrParInPageIndex) :
                                       static_cast<int16_t>(PageParagraphs.size() - 1));
                 ParIndex >= 0;
                 --ParIndex){
                auto Paragraph = PageParagraphs.at(static_cast<size_t>(ParIndex));


                if(Paragraph->location() != this->Data->CurrentLocation) continue;
                if(Paragraph->lines().size() == 0) continue;
                if(!areSameType(Paragraph->contentType(), this->Data->CurrContentType)) continue;

                for(int16_t LineIndex = (RealPageIndex <= this->Data->CurrentRealPageIndex
                                         && ParIndex == static_cast<int16_t>(this->Data->CurrParInPageIndex)
                                         ?  static_cast<int16_t>(this->Data->CurrLineInParIndex) - 1
                                         :  static_cast<int16_t>(Paragraph->lines().size() - 1));
                    LineIndex >= 0;
                    --LineIndex){
                    auto Line = Paragraph->lines().at(static_cast<size_t>(LineIndex));
                    if(Line->isPureImage()) continue;

                    if(Line->pageIndex() < this->Data->CurrentPageIndex ||
                       Line->parIndex() < this->Data->CurrentParIndex ||
                       Line->sntIndex() < this->Data->CurrentSntIndex
                       ){
                        return this->Data->setSentence(Paragraph->location(),
                                                       Paragraph->contentType(),
                                                       RealPageIndex, ParIndex, LineIndex, Line);
                    }
                }
            }
        }
        return stuActiveSentenceSpecs();
    };

    gotoPrevSentenceLambda();
    gotoPrevSentenceLambda();
    return this->gotoNextSentence();
}

int clsPdfDocument::getSentenceVirtualPageIdx() const
{
    return this->Data->CurrentPageIndex;
}

int clsPdfDocument::getSentenceRealPageIdx() const
{
    return this->Data->CurrentRealPageIndex;
}

int clsPdfDocument::getSentenceParIdx() const
{
    return this->Data->CurrentParIndex;
}

int clsPdfDocument::getSentenceIdx() const
{
    return this->Data->CurrentSntIndex;
}

bool clsPdfDocument::isEncrypted()
{
    return this->Data->PdfiumWrapper.isEncrypted();
}

void clsPdfDocument::setPassword(const std::string& _password)
{
    this->Data->PdfiumWrapper.setPassword(_password);
}

uint32_t clsPdfDocument::firstPageNo()
{
    return this->Data->PdfiumWrapper.firstPageNo();
}

stuPDFInfo clsPdfDocument::docInfo(int _pageIndex)
{
    this->Data->Info.PageSize = this->Data->PdfiumWrapper.pageSize(_pageIndex);
    return  this->Data->Info;
}

std::wstring clsPdfDocument::pageLabel(int _pageIndex)
{
    return this->Data->PdfiumWrapper.pageLabel(_pageIndex);
}

int clsPdfDocument::pageNoByLabel(const std::wstring& _label)
{
    return this->Data->PdfiumWrapper.pageNoByLabel(_label);
}

enuLocation clsPdfDocument::getSentenceLoc() const
{
    return this->Data->CurrentLocation;
}

std::wstring clsPdfDocument::getSentenceContent() const
{
    std::wstring Result;
    if(this->Data->CurrentPageIndex < 0){
        auto _this = const_cast<clsPdfDocument*>(this);
        if(_this->setCurrentSentence(0).isValid() == false) return L"";
    }
    auto isHyphen = [] (wchar_t _c) { return _c == 0x2D || _c == 0xAD; };

    for(int16_t RealPageIndex = this->Data->CurrentRealPageIndex;
        RealPageIndex < this->Data->pageCount();
        ++RealPageIndex){
        const auto& PageParagraphs = this->Data->LayoutAnalyzer.getPageContent(RealPageIndex, this->Data->PdfiumWrapper.fontInfo());

        bool CanContinue = true;
        for (size_t ParIndex = (RealPageIndex <= this->Data->CurrentRealPageIndex ?
                             static_cast<size_t>(this->Data->CurrParInPageIndex) : 0);
             ParIndex < PageParagraphs.size();
             ++ParIndex){
            auto Paragraph = PageParagraphs.at(ParIndex);
            if(Paragraph->location() != this->Data->CurrentLocation) continue;
            if(Paragraph->lines().size() == 0) continue;
            if(Paragraph->lines().front()->pageIndex() < this->Data->CurrentPageIndex) continue;
            if(Paragraph->lines().front()->pageIndex() > this->Data->CurrentPageIndex) {
                CanContinue = false;
                break;
            }
            if(Paragraph->lines().front()->parIndex() < this->Data->CurrentParIndex) continue;
            if(Paragraph->lines().front()->parIndex() > this->Data->CurrentParIndex) {
                CanContinue = false;
                break;
            }

            for(size_t LineIndex = (RealPageIndex <= this->Data->CurrentRealPageIndex
                                    && ParIndex == static_cast<size_t>(this->Data->CurrParInPageIndex)
                                    ?  static_cast<size_t>(this->Data->CurrLineInParIndex) : 0);
                LineIndex < Paragraph->lines().size();
                ++LineIndex) {
                auto Line = Paragraph->lines().at(LineIndex);
                if(Line->isPureImage() || Line->sntIndex() < this->Data->CurrentSntIndex)
                    continue;

                if(Line->sntIndex() == this->Data->CurrentSntIndex){
                    Result += Line->contentString();
                    if(Result.size() > 0 && isHyphen(Result.back()))
                        Result = Result.substr(0, Result.size() - 1);
                    else
                        Result += ' ';
                } else {
                    CanContinue = false;
                    break;
                }
            }
            if(CanContinue == false)
                break;
        }
        if(CanContinue == false)
            break;
    }

    trim(Result);
    return Result;
}

int clsPdfDocumentData::pageCount() const
{
    return this->PdfiumWrapper.pageCount();
}

stuActiveSentenceSpecs clsPdfDocumentData::setSentence(
        enuLocation _loc,
        enuContentType _contentType,
        int16_t _realPageIndex,
        int16_t _currParInPageIndex,
        int16_t _currLineInParIndex,
        const Private::PdfLineSegmentPtr_t& _line)
{
        this->CurrentLocation = _loc;
        this->CurrContentType = _contentType;
        this->CurrentRealPageIndex = _realPageIndex;
        this->CurrentPageIndex = _line->pageIndex();
        this->CurrentParIndex = _line->parIndex();
        this->CurrentSntIndex = _line->sntIndex();
        this->CurrParInPageIndex = _currParInPageIndex;
        this->CurrLineInParIndex = _currLineInParIndex;

        return stuActiveSentenceSpecs(
                    this->CurrentPageIndex,
                    this->CurrentRealPageIndex,
                    this->CurrentParIndex,
                    this->CurrentSntIndex,
                    this->CurrentLocation);
}

clsPdfDocumentData::~clsPdfDocumentData()
{;}

}
}
