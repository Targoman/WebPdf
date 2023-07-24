#include "clsPdfiumWrapper.h"

#include "clsPdfChar.h"

namespace Targoman {
namespace PDF {
namespace Private {

clsPdfiumWrapper::clsPdfiumWrapper(const stuConfigsPtr_t& _configs) :
    Parser(new ::CPDF_Parser),
    PageLabels(nullptr),
    Configs(_configs)
{
}



clsPdfiumWrapper::~clsPdfiumWrapper()
{
    this->CachedPages.clear();
    delete this->Parser;
    if(this->PageLabels)
        delete this->PageLabels;
}

enuLoadError clsPdfiumWrapper::loadPDF(const uint8_t* _buffer, int _size)
{
    switch(this->Parser->StartParse(
                FX_CreateMemoryStream(
                    const_cast<uint8_t *>(_buffer),
                    static_cast<size_t>(_size),
                    false)
           ))
    {
    case PDFPARSE_ERROR_SUCCESS:
        this->PageLabels = new CPDF_PageLabel(this->Parser->GetDocument());
        return enuLoadError::None;
    case PDFPARSE_ERROR_FILE: return enuLoadError::File;
    case PDFPARSE_ERROR_FORMAT: return enuLoadError::Format;
    case PDFPARSE_ERROR_PASSWORD: return enuLoadError::Password;
    case PDFPARSE_ERROR_HANDLER: return enuLoadError::Handler;
    case PDFPARSE_ERROR_CERT: return enuLoadError::Cert;
    }
}

struct stuARGB{
    int A = 0, R = 0, G = 0, B = 0;
};

stuARGB getArgb(const CPDF_PageObject* pObj, bool _stroke)
{
    stuARGB ARGB;
    if(_stroke){
        if(pObj->m_ColorState.GetStrokeColor()->IsNull())
           return ARGB;
       pObj->m_ColorState.GetStrokeColor()->GetRGB(ARGB.R, ARGB.G, ARGB.B);
    } else {
        if(pObj->m_ColorState.GetFillColor()->IsNull())
           return ARGB;
       pObj->m_ColorState.GetFillColor()->GetRGB(ARGB.R, ARGB.G, ARGB.B);
    }

    ARGB.A = pObj->m_GeneralState.GetAlpha(_stroke);

    return ARGB;
}

bool isLuminantGray(const CPDF_PageObject* _obj, bool _stroke) {
    if(_obj->m_ColorState == nullptr ||
       (_stroke && _obj->m_ColorState.GetStrokeColor()->IsPattern()) ||
       (_stroke == false && _obj->m_ColorState.GetFillColor()->IsPattern())
       )
        return false;

    stuARGB ARGB = getArgb(_obj, _stroke);
    if(ARGB.A < 127)
        return true;
    if(std::abs(ARGB.R - ARGB.G) < 2 && std::abs(ARGB.R - ARGB.B) < 2 && std::abs(ARGB.B - ARGB.G) < 2)
        return ARGB.R > 240;
    return false;
}

bool isRedundantPath(const CPDF_PathObject* _path) {
    if(_path->m_FillType == 0 && _path->m_bStroke == false)
        return true;

    if(_path->m_bStroke && isLuminantGray(_path, true) == false)
        return false;
    return isLuminantGray(_path, false);
}

bool isBackgroundPath(const CPDF_PathObject* _path, const CFX_FloatRect& _boundingRect) {
    if(_boundingRect.Width() < 4.f || _boundingRect.Height() < 4.f)
        return false;

    if(_path->m_FillType == 0 && !_path->m_bStroke)
        return false;

    auto FillColor = _path->m_ColorState.GetFillColor();
    if(FillColor == nullptr || FillColor->IsPattern())
        return false;

    auto PointCount = _path->m_Path.GetObject()->GetPointCount();
    if(PointCount == 2 && _path->m_bStroke)
        return true;

    if(PointCount != 4 && PointCount != 5)
        return false;

    auto Points = _path->m_Path.GetObject()->GetPoints();
    for(size_t i = 1; i < static_cast<size_t>(PointCount); ++i) {
        if(
           std::abs(Points[i - 1].m_PointX - Points[i].m_PointX) > MIN_ITEM_SIZE &&
           std::abs(Points[i - 1].m_PointY - Points[i].m_PointY) > MIN_ITEM_SIZE)
            return false;
    }
    return true;
}

bool isUnionOfSeparatedVertHorzLines(const CPDF_PathObject* _path) {
    auto PointCount = _path->m_Path.GetObject()->GetPointCount();
    auto Points = _path->m_Path.GetObject()->GetPoints();

    if(PointCount < 3 || PointCount > 10)
        return false;

    float X0 = NAN, Y0 = NAN;
    for(size_t i = 0; i < static_cast<size_t>(PointCount); ++i) {
        if(isnan(X0)) {
            if(Points[i].m_Flag != FXPT_MOVETO)
                return false;
            X0 = Points[i].m_PointX;
            Y0 = Points[i].m_PointY;
        } else {
            if(Points[i].m_Flag != FXPT_LINETO)
                return false;
            if(
                std::abs(X0 - Points[i].m_PointX) > MIN_ITEM_SIZE &&
                std::abs(Y0 - Points[i].m_PointY) > MIN_ITEM_SIZE
            )
                return false;
            X0 = NAN;
            Y0 = NAN;
        }
    }
    return true;
}

std::vector<CFX_FloatRect> getSeparatedVertHorzLineBounds(const CPDF_PathObject* _path, const CFX_AffineMatrix *_baseTransformMatrix) {
    auto PointCount = _path->m_Path.GetObject()->GetPointCount();
    auto Points = _path->m_Path.GetObject()->GetPoints();

    auto LineWidth = _path->m_GraphState.GetObject()->m_LineWidth;

    std::vector<CFX_FloatRect> Result;
    for(size_t i = 0; i < static_cast<size_t>(PointCount); ++i) {
        if(Points[i].m_Flag == FXPT_LINETO) {
            float X0 = std::min(Points[i].m_PointX, Points[i - 1].m_PointX);
            float X1 = std::max(Points[i].m_PointX, Points[i - 1].m_PointX);
            float Y0 = std::min(Points[i].m_PointY, Points[i - 1].m_PointY);
            float Y1 = std::max(Points[i].m_PointY, Points[i - 1].m_PointY);
            if(std::abs(X0 - X1) < std::abs(Y0 - Y1)) {
                X0 -= LineWidth / 2;
                X1 += LineWidth / 2;
            } else {
                Y0 -= LineWidth / 2;
                Y1 += LineWidth / 2;
            }
            Result.push_back(CFX_FloatRect(X0, Y0, X1, Y1));
        }
    }

    CFX_AffineMatrix M = _path->m_Matrix;
    if(_baseTransformMatrix != nullptr)
        M.Concat(*_baseTransformMatrix);

    for(auto& Rect : Result)
        M.TransformRect(Rect);

    return Result;
}

static stuPageMargins DEFAULT_MAX_MARGINS  { std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };

stuPageMargins appendImageObjectReturnOffset(
        const stuConfigsPtr_t& _configs,
        const CPDF_PageObject *_object,
        const CFX_AffineMatrix *_baseTransformMatrix,
        const stuSize& _pageSize,
        PdfItemPtrVector_t &_reservoire)
{
    CFX_FloatRect BoundingRect(_object->m_Left, _object->m_Bottom, _object->m_Right, _object->m_Top);

    if (!_object->m_ClipPath.IsNull())
        BoundingRect.Intersect(_object->m_ClipPath.GetClipBox());

    if (_baseTransformMatrix)
        _baseTransformMatrix->TransformRect(BoundingRect);

    auto storeImageItem = [&_reservoire,&_pageSize,&_configs] (const CFX_FloatRect& _boundingRect, enuPdfItemType _itemType) {
        if(_boundingRect.left >= _pageSize.Width - MIN_ITEM_SIZE)
            return DEFAULT_MAX_MARGINS;
        if(_boundingRect.bottom >= _pageSize.Height - MIN_ITEM_SIZE)
            return DEFAULT_MAX_MARGINS;
        if(_boundingRect.right < MIN_ITEM_SIZE)
            return DEFAULT_MAX_MARGINS;
        if(_boundingRect.top < MIN_ITEM_SIZE)
            return DEFAULT_MAX_MARGINS;

        if(_boundingRect.Width() < .1f || _boundingRect.Height() < .1f)
            return DEFAULT_MAX_MARGINS;
        if(_boundingRect.Width() < 2.f && _boundingRect.Height() < 2.f)
            return DEFAULT_MAX_MARGINS;
        if(_boundingRect.Width() * _boundingRect.Height() > 0.9f * _pageSize.Width * _pageSize.Height)
            return DEFAULT_MAX_MARGINS;

        if(_configs->DiscardItemsIn2PercentOfPageMargin) {
            if(_boundingRect.right < .02f * _pageSize.Width
               || _boundingRect.left > .98f * _pageSize.Width
               || _boundingRect.bottom < .02f * _pageSize.Height
               || _boundingRect.top > .98f * _pageSize.Height
               )
                return DEFAULT_MAX_MARGINS;
        }

        auto Item = std::make_shared<clsPdfImage>(
                        _boundingRect.left - MIN_ITEM_SIZE,
                        _boundingRect.bottom - MIN_ITEM_SIZE,
                        _boundingRect.right + MIN_ITEM_SIZE,
                        _boundingRect.top + MIN_ITEM_SIZE,
                        _itemType);

        _reservoire.push_back(Item);
        return stuPageMargins { Item->x0(), Item->y0(), _pageSize.Width - Item->x1(), _pageSize.Height - Item->y1() };
    };

    auto ItemType = enuPdfItemType::Image;
    if(_object->m_Type == PDFPAGE_PATH) {
        ItemType = enuPdfItemType::Path;
        const CPDF_PathObject* Path = reinterpret_cast<const CPDF_PathObject*>(_object);

        if(isRedundantPath(Path))
            return DEFAULT_MAX_MARGINS;

        if(isUnionOfSeparatedVertHorzLines(Path)) {
            auto Margins = DEFAULT_MAX_MARGINS;
            for(const auto& BoundsRect : getSeparatedVertHorzLineBounds(Path, _baseTransformMatrix)) {
                auto PartMargin = storeImageItem(BoundsRect, enuPdfItemType::Path);
                Margins.update(PartMargin);
            }

            return Margins;
        }

        if(isBackgroundPath(Path, BoundingRect))
            ItemType = enuPdfItemType::Background;
    }

    return storeImageItem(BoundingRect, ItemType);
}

stuPageMargins appendCharsOfTextObjectReturnStartOffset(
        const stuConfigsPtr_t& _configs,
        const CPDF_TextObject *_textObject,
        const CFX_AffineMatrix *_baseTransformMatrix,
        const FontInfo_t& _fontInfo,
        const stuSize& _pageSize,
        PdfItemPtrVector_t &_reservoire)
{
    CFX_FloatRect WholeTextBoundRect(_textObject->m_Left, _textObject->m_Bottom, _textObject->m_Right, _textObject->m_Top);

    if (!_textObject->m_ClipPath.IsNull())
        WholeTextBoundRect.Intersect(_textObject->m_ClipPath.GetClipBox());

    if (abs(WholeTextBoundRect.Width()) < MIN_ITEM_SIZE ||
        abs(WholeTextBoundRect.Height()) < MIN_ITEM_SIZE) {
        return DEFAULT_MAX_MARGINS;
    }

    std::vector<FX_DWORD> FallbackCodes;
    std::vector<FX_FLOAT> FallbackPoses;
    int CharCount;
    FX_DWORD* CharCodes;
    FX_FLOAT* CharPoses;
    const_cast<CPDF_TextObject*>(_textObject)->GetData(CharCount, CharCodes, CharPoses);

    if(CharCount == 1) {
        FallbackCodes.push_back(static_cast<FX_DWORD>(reinterpret_cast<FX_UINTPTR>(CharCodes)));
        CharCodes = FallbackCodes.data();
    }

    if(CharPoses == nullptr) {
        FallbackPoses.resize(static_cast<size_t>(CharCount));
        CharPoses = FallbackPoses.data();
        _textObject->CalcCharPos(CharPoses);
        ++CharPoses;
    }


    CFX_AffineMatrix AffineMatrix;
    _textObject->GetTextMatrix(&AffineMatrix);
    if (_baseTransformMatrix != nullptr)
        AffineMatrix.Concat(*_baseTransformMatrix);

    float Angle = atan2(AffineMatrix.GetB(), AffineMatrix.GetA());

    auto PdfFontPtr = static_cast<void*>(_textObject->GetFont());
    const PdfFontPtr_t& Font = _fontInfo.at(PdfFontPtr);

    bool IsItalic =
            (std::abs(AffineMatrix.GetB()) > MIN_ITEM_SIZE && std::abs(AffineMatrix.GetC()) < MIN_ITEM_SIZE) ||
            (std::abs(AffineMatrix.GetC()) > MIN_ITEM_SIZE && std::abs(AffineMatrix.GetB()) < MIN_ITEM_SIZE);

    if(IsItalic == false && std::abs(Angle) > MIN_ITEM_SIZE) {
        auto Item = std::make_shared<clsPdfImage>(
                        WholeTextBoundRect.left - MIN_ITEM_SIZE,
                        WholeTextBoundRect.bottom - MIN_ITEM_SIZE,
                        WholeTextBoundRect.right + MIN_ITEM_SIZE,
                        WholeTextBoundRect.top + MIN_ITEM_SIZE,
                        enuPdfItemType::Image);
        _reservoire.push_back(Item);
        return stuPageMargins { Item->x0(), Item->y0(), _pageSize.Width - Item->x1(), _pageSize.Height - Item->y1() };
    }

    IsItalic = IsItalic || Font->isItalic();

    float FontSize = _textObject->GetFontSize();

    float BaseAscent = Font->getTypeAscent(FontSize), BaseDescent = Font->getTypeDescent(FontSize);

    float YUnit = AffineMatrix.GetYUnit();
    float ActualFontSize = YUnit * FontSize;
    /*if(ActualFontSize < 5) // Do not activate it causes bug on bi-1006
        return DEFAULT_MAX_MARGINS;*/

    auto allowedToHaveZeroSize = [] (wchar_t _char) {
        return _char == L' ' || _char == L'\t';
    };

    int minIndex = 0, maxIndex = CharCount;

    //@Note:  pre/post text spaces are being removed because they are unreliable
    while (minIndex < maxIndex) {
        FX_DWORD CharCode = CharCodes[minIndex];
        if(CharCode != static_cast<FX_DWORD>(-1)) {
            auto Unicode = Font->getUnicodeFromCharCode(CharCode);
            bool AllSpaces = true;
            for(int i = 0; i < Unicode.GetLength(); ++i)
                if (Unicode.GetAt(0) != L' ') {
                    AllSpaces = false;
                    break;
                }
            if(AllSpaces == false)
                break;
        }
        ++minIndex;
    }
    while (minIndex < maxIndex) {
        FX_DWORD CharCode = CharCodes[maxIndex - 1];
        if(CharCode != static_cast<FX_DWORD>(-1)) {
            auto Unicode = Font->getUnicodeFromCharCode(CharCode);
            bool AllSpaces = true;
            for(int i = 0; i < Unicode.GetLength(); ++i)
                if (Unicode.GetAt(0) != L' ') {
                    AllSpaces = false;
                    break;
                }
            if(AllSpaces == false)
                break;
        }
        --maxIndex;
    }

    stuPageMargins ItemBounding = DEFAULT_MAX_MARGINS;

    if(minIndex >= maxIndex) {
        int minIndex = 0, maxIndex = CharCount;

        //@Note:  pre/post text spaces are being removed because they are unreliable
        while (minIndex < maxIndex) {
            FX_DWORD CharCode = CharCodes[minIndex];
            if(CharCode != static_cast<FX_DWORD>(-1)) {
                auto Unicode = Font->getUnicodeFromCharCode(CharCode);
                bool AllSpaces = true;
                for(int i = 0; i < Unicode.GetLength(); ++i)
                    if (Unicode.GetAt(0) != L' ') {
                        AllSpaces = false;
                        break;
                    }
                if(AllSpaces == false)
                    break;
            }
            ++minIndex;
        }
        while (minIndex < maxIndex) {
            FX_DWORD CharCode = CharCodes[maxIndex - 1];
            if(CharCode != static_cast<FX_DWORD>(-1)) {
                auto Unicode = Font->getUnicodeFromCharCode(CharCode);
                bool AllSpaces = true;
                for(int i = 0; i < Unicode.GetLength(); ++i)
                    if (Unicode.GetAt(0) != L' ') {
                        AllSpaces = false;
                        break;
                    }
                if(AllSpaces == false)
                    break;
            }
            --maxIndex;
        }
    }

    for (int i = minIndex; i < maxIndex; ++i) {

        FX_DWORD CharCode = CharCodes[i];
        if(CharCode == static_cast<FX_DWORD>(-1))
            continue;

        CFX_FloatRect BoundingRect, AdvanceRect;

        std::tie(BoundingRect, AdvanceRect) = Font->getGlyphBoxInfoForCode(CharCode, FontSize);
        BoundingRect.left += i == 0 ? 0 : CharPoses[i - 1];
        BoundingRect.right += i == 0 ? 0 : CharPoses[i - 1];
        AdvanceRect.left += i == 0 ? 0 : CharPoses[i - 1];
        AdvanceRect.right += i == 0 ? 0 : CharPoses[i - 1];

        AffineMatrix.TransformRect(BoundingRect);
        AffineMatrix.TransformRect(AdvanceRect);

        if(BoundingRect.left >= _pageSize.Width - MIN_ITEM_SIZE)
            continue;
        if(BoundingRect.bottom >= _pageSize.Height - MIN_ITEM_SIZE)
            continue;
        if(BoundingRect.right < MIN_ITEM_SIZE)
            continue;
        if(BoundingRect.top < MIN_ITEM_SIZE)
            continue;

        if(_configs->DiscardItemsIn2PercentOfPageMargin) {
            if(BoundingRect.right < .02f * _pageSize.Width
               || BoundingRect.left > .98f * _pageSize.Width
               || BoundingRect.bottom < .02f * _pageSize.Height
               || BoundingRect.top > .98f * _pageSize.Height
               )
                continue;
        }

        float PosX;
        float Ascent = BaseAscent, Descent = BaseDescent, Baseline = 0;

        PosX = i == 0 ? 0 : CharPoses[i - 1];
        AffineMatrix.TransformPoint(PosX, Ascent);
        PosX = i == 0 ? 0 : CharPoses[i - 1];
        AffineMatrix.TransformPoint(PosX, Descent);
        PosX = i == 0 ? 0 : CharPoses[i - 1];
        AffineMatrix.TransformPoint(PosX, Baseline);

        auto Unicode = Font->getUnicodeFromCharCode(CharCode);
        if(Unicode.GetLength() == 0)
            Unicode.Insert(0, static_cast<wchar_t>(CharCode));

        float X0 = BoundingRect.left;
        float WidthPerItem = BoundingRect.Width() / Unicode.GetLength();
        float O0 = AdvanceRect.left;
        float AdvancePerItem = AdvanceRect.Width()  / Unicode.GetLength();

        for(int j = 0; j < Unicode.GetLength(); ++j) {
            if (allowedToHaveZeroSize(Unicode.GetAt(j)) ||
                (BoundingRect.Width() >= MIN_ITEM_SIZE && BoundingRect.Height() >= MIN_ITEM_SIZE)) {
                auto Char = std::make_shared<clsPdfChar>(
                                X0,
                                BoundingRect.bottom,
                                X0 + WidthPerItem,
                                BoundingRect.top,
                                std::min(Ascent, BoundingRect.bottom),
                                Font->isFormula() ? BoundingRect.top : std::max(Descent, BoundingRect.top),
                                O0,
                                O0 + AdvancePerItem,
                                static_cast<wchar_t>(Unicode.GetAt(j)) + _configs->ASCIIOffset,
                                Baseline,
                                PdfFontPtr,
                                ActualFontSize,
                                Angle
                                );
                X0 += WidthPerItem;
                O0 += AdvancePerItem;
                Char->markAsItalic(IsItalic);
                _reservoire.push_back(Char);
                ItemBounding.update(stuPageMargins {X0,
                                                    std::min(Ascent, BoundingRect.bottom),
                                                    _pageSize.Width - X0 + WidthPerItem,
                                                    _pageSize.Height - std::max(Descent, BoundingRect.top) });
            }
        }
    }
    return ItemBounding;
}

stuPageMargins clsPdfiumWrapper::appendObjectsToListComputingMargins(
        stuDoublyLinkedList *_objectPos,
        CFX_Matrix *_baseTransformMatrix,
        const stuSize& _pageSize,
        PdfItemPtrVector_t &Items) const
{
    stuPageMargins PageMargins = DEFAULT_MAX_MARGINS;
    while (_objectPos) {
        auto Object = static_cast<CPDF_PageObject *>(_objectPos->Data);

        switch (Object ? Object->m_Type : 0) {
        case PDFPAGE_TEXT:
            if(this->FontInfo.find(static_cast<CPDF_TextObject *>(Object)->GetFont()) == this->FontInfo.end()){
                auto _this = const_cast<clsPdfiumWrapper*>(this);
                _this->FontInfo[static_cast<CPDF_TextObject *>(Object)->GetFont()] = std::make_shared<clsPdfFont>(static_cast<CPDF_TextObject *>(Object)->GetFont());
            }
            PageMargins.update(appendCharsOfTextObjectReturnStartOffset(
                                   this->Configs,
                                   static_cast<CPDF_TextObject *>(Object),
                                   _baseTransformMatrix,
                                   this->FontInfo,
                                   _pageSize,
                                   Items));
            break;
        case PDFPAGE_PATH:
        case PDFPAGE_IMAGE:
            PageMargins.update(appendImageObjectReturnOffset(
                                   this->Configs,
                                   Object,
                                   _baseTransformMatrix,
                                   _pageSize,
                                   Items));
            break;
        case PDFPAGE_FORM:
            if (true) {
                CPDF_FormObject *FormObject = static_cast<CPDF_FormObject *>(Object);
                auto FormMatrix = FormObject->m_FormMatrix;
                if(_baseTransformMatrix != nullptr)
                    FormMatrix.Concat(*_baseTransformMatrix);
                PageMargins.update(this->appendObjectsToListComputingMargins(
                                       static_cast<stuDoublyLinkedList *>(FormObject->m_pForm->GetFirstObjectPosition()),
                                       &FormMatrix, _pageSize, Items
                                       ));
            }
            break;
            //        case PDFPAGE_SHADING:
            //        case PDFPAGE_INLINES:
            //            break;
        default:
            break;
        }

        _objectPos = _objectPos->Next;
    }


    return PageMargins.sanitized();
}

CPDF_Page *clsPdfiumWrapper::cpdfPage(int _pageIndex) const
{
    auto _this = const_cast<clsPdfiumWrapper*>(this);
    auto CachedPage = _this->CachedPages.find(_pageIndex);
    if(CachedPage != _this->CachedPages.end())
        return *(CachedPage->second);

    auto Page = new ::CPDF_Page;
    Page->Load( this->Parser->GetDocument(), this->Parser->GetDocument()->GetPage(_pageIndex) );
    Page->ParseContent();
    _this->CachedPages[_pageIndex] = std::make_shared<::CPDF_Page*>(Page);
    return Page;
}

std::tuple<PdfItemPtrVector_t, stuPageMargins> clsPdfiumWrapper::getPageItems(int _pageIndex) const
{
    PdfItemPtrVector_t Items;
    if(_pageIndex < 0)
        return  std::make_tuple(Items, stuPageMargins());

    auto Page = this->cpdfPage(_pageIndex);

    auto PageBBox = Page->GetPageBBox();
    auto PdfiumPageMatrix = Page->GetPageMatrix();

    int PageRotation = 0;
    if(std::abs(PdfiumPageMatrix.a - 1.0f) > MIN_ITEM_SIZE) {
        if(std::abs(PdfiumPageMatrix.b - -1.0f) < MIN_ITEM_SIZE)
            PageRotation = 90;
        else if(std::abs(PdfiumPageMatrix.b - 0.f) < MIN_ITEM_SIZE)
            PageRotation = 180;
        else
            PageRotation = 270;
    }

    CFX_AffineMatrix PageMatrix;
    switch(PageRotation) {
    case 90:
        PageMatrix.Set(0.f, 1.f, 1.f, 0.f, -PageBBox.bottom, -PageBBox.left);
        break;
    case 180:
        PageMatrix.Set(-1.f, 0.f, 0.f, 1.f, PageBBox.right, -PageBBox.bottom);
        break;
    case 270:
        PageMatrix.Set(0.f, -1.f, -1.f, 0.f, PageBBox.top, PageBBox.right);
        break;
    case 0:
    default:
        PageMatrix.Set(1.f, 0.f, 0.f, -1.f, -PageBBox.left, PageBBox.top);
        break;
    }

    auto ObjectPos = static_cast<stuDoublyLinkedList *>(Page->GetFirstObjectPosition());
    if (!ObjectPos)
        return std::make_tuple(Items, stuPageMargins());

    auto Margins = this->appendObjectsToListComputingMargins(ObjectPos,
                                                            &PageMatrix,
                                                            stuSize(Page->GetPageWidth(), Page->GetPageHeight()),
                                                            Items);

    return std::make_tuple(Items, Margins.sanitized());
}

std::vector<uint8_t> clsPdfiumWrapper::getPDFRawData()
{
    return this->Parser->GetRawBuffer();
}

bool clsPdfiumWrapper::isEncrypted()
{
    return this->Parser->IsEncrypted();
}

void clsPdfiumWrapper::setPassword(const std::string& _password)
{
    this->Parser->SetPassword(_password.c_str());
}

uint32_t clsPdfiumWrapper::firstPageNo()
{
    return this->Parser->GetFirstPageNo();
}

void clsPdfiumWrapper::fillInfo(stuPDFInfo& _infoStorage)
{
    CPDF_Metadata MetaData;
    MetaData.LoadDoc(this->Parser->GetDocument());

#define getMetaInfo(_storage, _bsItem) \
    { CFX_WideString WString; if(MetaData.GetString(_bsItem, WString) > 0) _infoStorage._storage = WString.GetBuffer(WString.GetLength()); }

    getMetaInfo(Title, "Title");
    getMetaInfo(Title, "title");
    getMetaInfo(Subject, "Subject");
    getMetaInfo(Subject, "description");
    getMetaInfo(Author, "Author");
    getMetaInfo(Author, "author");
    getMetaInfo(Creator, "Creator");
    getMetaInfo(Creator, "creator");
    getMetaInfo(Keywords, "Keywords");
    getMetaInfo(Keywords, "Keywords");
    getMetaInfo(PDFProducer, "Producer");
    getMetaInfo(PDFProducer, "producer");
    getMetaInfo(PDFProducer, "Creator");
    getMetaInfo(PDFProducer, "CreatorTool");
    getMetaInfo(CreationDate, "CreationDate");
    getMetaInfo(CreationDate, "CreateDate");
    getMetaInfo(ModificationDate, "ModDate");
    getMetaInfo(ModificationDate, "ModifyDate");

    _infoStorage.PageCount = static_cast<size_t>(this->pageCount());
}

int clsPdfiumWrapper::pageCount() const
{
    return this->Parser->GetDocument()->GetPageCount();
}

std::wstring clsPdfiumWrapper::pageLabel(int _pageIndex)
{
    if(this->PageLabels) {
        CFX_WideString WString = this->PageLabels->GetLabel(_pageIndex);
        if(WString.GetLength())
            return WString.GetBuffer(WString.GetLength());
    }
    return std::to_wstring(_pageIndex + 1);
}

int clsPdfiumWrapper::pageNoByLabel(std::wstring _pageLabel){
    if(this->PageLabels)
        return this->PageLabels->GetPageByLabel(_pageLabel.c_str());

    return  -1;
}

stuSize clsPdfiumWrapper::pageSize(int _pageIndex) const
{
    auto Page = this->cpdfPage(_pageIndex);
    return stuSize( Page->GetPageWidth(), Page->GetPageHeight() );
}

ImageBuffer_t clsPdfiumWrapper::getPageImage(int _pageIndex, int _backgroundColor, const stuSize& _renderSize) const
{
    ImageBuffer_t Buffer(static_cast<size_t>(_renderSize.Width * _renderSize.Height * 4));

    auto Page = this->cpdfPage(_pageIndex);

    int Width = static_cast<int>(_renderSize.Width);
    int Height = static_cast<int>(_renderSize.Height);

    auto Bitmap = new ::CFX_DIBitmap;
    Bitmap->Create(Width, Height, FXDIB_Argb);
    FX_RECT WholePageRect(0, 0, Width, Height);

    CFX_FxgeDevice Device;
    Device.Attach(Bitmap);
    Device.FillRect(&WholePageRect, static_cast<FX_DWORD>(_backgroundColor));

    CFX_AffineMatrix Matrix;
    Page->GetDisplayMatrix(Matrix, 0, 0, Width, Height, 0);
    auto Context = new CPDF_RenderContext();
    Context->Create(Page);
    Context->AppendObjectList(Page, &Matrix);
    auto Renderer = new CPDF_ProgressiveRenderer;
    Renderer->Start(Context, &Device, nullptr, nullptr);

    auto ResultScanLine = Buffer.data();
    for (int i = 0; i < Height; ++i)
    {
        auto ScanLine = Bitmap->GetScanline(i);
        for (int j = 0; j < Width; ++j)
        {
            ResultScanLine[0] = ScanLine[2];
            ResultScanLine[1] = ScanLine[1];
            ResultScanLine[2] = ScanLine[0];
            ResultScanLine[3] = ScanLine[3];
            ResultScanLine += 4;
            ScanLine += 4;
        }
    }

    delete Renderer;
    delete Context;
    delete Bitmap;

    return Buffer;
}


}
}
}
