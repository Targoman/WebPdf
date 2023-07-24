#include "BulletAndNumbering.h"

namespace Targoman {
namespace PDF {
namespace Private {
namespace LA {

std::tuple<ListDetectionResultVector_t, clsFloatHistogram> identifyLineListTypes(const PdfBlockPtr_t& _block) {
    auto isDecimal = [&] (const PdfLineSegmentPtr_t& _line, size_t _from, size_t _to) {
        for(size_t i = _from; i < _to; ++i) {
            if(_line->at(i)->type() != enuPdfItemType::Char)
                return false;
            if(_line->at(i)->as<clsPdfChar>()->isDecimalDigit() == false && _line->at(i)->as<clsPdfChar>()->isDot() == false)
                return false;
        }
        return true;
    };

    auto isRoman = [&] (const PdfLineSegmentPtr_t& _line, size_t _from, size_t _to) {
        bool Fresh = true, Capital = false;
        auto isRomanItem = [&] (const PdfItemPtr_t& _item) {
            if(_item->type() != enuPdfItemType::Char)
                return false;
            if(_item->as<clsPdfChar>()->isDot())
                return true;
            if(_item->as<clsPdfChar>()->code() == L'I' && (Fresh || Capital)) {
                Capital = true;
                Fresh = false;
                return true;
            }
            if(_item->as<clsPdfChar>()->code() == L'V' && (Fresh || Capital)) {
                Capital = true;
                Fresh = false;
                return true;
            }
            if(_item->as<clsPdfChar>()->code() == L'X' && (Fresh || Capital)) {
                Capital = true;
                Fresh = false;
                return true;
            }
            if(_item->as<clsPdfChar>()->code() == L'i' && (Fresh || Capital == false)) {
                Capital = false;
                Fresh = false;
                return true;
            }
            if(_item->as<clsPdfChar>()->code() == L'v' && (Fresh || Capital == false)) {
                Capital = false;
                Fresh = false;
                return true;
            }
            if(_item->as<clsPdfChar>()->code() == L'x' && (Fresh || Capital == false)) {
                Capital = false;
                Fresh = false;
                return true;
            }
            return false;
        };
        for(size_t i = _from; i < _to; ++i)
            if(isRomanItem(_line->at(i)) == false)
                return false;
        return true;
    };

    auto identifyLine = [&] (const PdfLineSegmentPtr_t& _line) {
        size_t ItemPosition = 0;
        while(
              ItemPosition < _line->size() &&
              _line->at(ItemPosition)->type() == enuPdfItemType::Char &&
              _line->at(ItemPosition)->as<clsPdfChar>()->isSpace()
              )
            ++ItemPosition;
        if(ItemPosition >= _line->size())
            return stuListDetectionResult::nonList(_line);
        size_t SpacePosition = ItemPosition;
        while(
              SpacePosition < _line->size() &&
              (
                  _line->at(SpacePosition)->type() != enuPdfItemType::Char ||
                  _line->at(SpacePosition)->as<clsPdfChar>()->isSpace() == false
                  )
              )
            ++SpacePosition;
        if(SpacePosition >= _line->size())
            return stuListDetectionResult::nonList(_line);
        size_t TextPosition = SpacePosition;
        while(
              TextPosition < _line->size() &&
              _line->at(TextPosition)->type() == enuPdfItemType::Char &&
              _line->at(TextPosition)->as<clsPdfChar>()->isSpace()
              )
            ++TextPosition;
        if(TextPosition >= _line->size())
            return stuListDetectionResult::nonList(_line);

        if(
           SpacePosition - ItemPosition == 1 &&
           (
               _line->at(ItemPosition)->type() != enuPdfItemType::Char ||
               _line->at(ItemPosition)->as<clsPdfChar>()->isAlphaNumeric() == false
               )
           )
            return stuListDetectionResult { enuListType::Bulleted, enuListSubType::None, _line->at(TextPosition)->x0(), ItemPosition, SpacePosition };

        if(
           _line->at(SpacePosition - 1)->type() == enuPdfItemType::Char &&
           (
               _line->at(SpacePosition - 1)->as<clsPdfChar>()->isDot() ||
               _line->at(SpacePosition - 1)->as<clsPdfChar>()->code() == L':'
               )
           ) {
            if(isDecimal(_line, ItemPosition, SpacePosition - 1))
                return stuListDetectionResult { enuListType::Numbered, enuListSubType::Numerals, _line->at(TextPosition)->x0(), ItemPosition, SpacePosition };
            if(isRoman(_line, ItemPosition, SpacePosition - 1))
                return stuListDetectionResult { enuListType::Numbered, enuListSubType::RomanNumerals, _line->at(TextPosition)->x0(), ItemPosition, SpacePosition };
        }

        if(
           _line->at(ItemPosition)->type() == enuPdfItemType::Char && _line->at(ItemPosition)->as<clsPdfChar>()->code() == L'[' &&
           _line->at(SpacePosition - 1)->type() == enuPdfItemType::Char && _line->at(SpacePosition - 1)->as<clsPdfChar>()->code() == L']'
           ) {
            if(isDecimal(_line, ItemPosition + 1, SpacePosition - 1))
                return stuListDetectionResult { enuListType::Numbered, enuListSubType::Brackets, _line->at(TextPosition)->x0(), ItemPosition, SpacePosition };
        }

        return stuListDetectionResult::nonList(_line);
    };

    ListDetectionResultVector_t DetectionResults;
    clsFloatHistogram Histogram(1.f);
    for(const auto& Line : _block->lines()) {
        auto IdentificationResult = identifyLine(Line);
        DetectionResults.push_back(IdentificationResult);
        if(IdentificationResult.Type != enuListType::None)
            Histogram.insert(IdentificationResult.TextLeft);
        else
            Histogram.reinforce(Line->x0());
    }

    return std::make_tuple(DetectionResults, Histogram);
}

void process4BulletAndNumbering(const stuConfigsPtr_t& _configs, const PdfBlockPtr_t& _block)
{
    std::vector<stuListDetectionResult> DetectionResults;
    clsFloatHistogram Histogram;

    std::tie(DetectionResults, Histogram) = identifyLineListTypes(_block);

    if(Histogram.size() > 0) {
        bool ValidListBlock = true;
        for(size_t i = 0; i < _block->lines().size(); ++i) {
            if(DetectionResults[i].Type != enuListType::None)
                continue;
            if(i > 0 && _block->lines().at(i)->x0() <= _block->x0() + MIN_ITEM_SIZE) {
                ValidListBlock = false;
                break;
            }
        }
        if(ValidListBlock) {
            for(size_t i = 0; i < _block->lines().size(); ++i) {
                float Approximate = Histogram.approximate(DetectionResults[i].TextLeft);
                if(std::isnan(Approximate) == false)
                    _block->lines().at(i)->setBulletAndNumberingInfo(
                                _configs,
                                Approximate,
                                DetectionResults[i].Type,
                                DetectionResults[i].ListIdentifierStart,
                                DetectionResults[i].ListIdentifierEnd
                                );
                else
                    _block->lines().at(i)->setBulletAndNumberingInfo(_configs, 0, enuListType::None);
            }
            _block->setContentType(enuContentType::List);
        } else if(DetectionResults.size() > 0) {
            float Approximate = Histogram.approximate(DetectionResults[0].TextLeft);
            if(std::isnan(Approximate) == false && DetectionResults[0].SubType == enuListSubType::Numerals)
                _block->lines().at(0)->setBulletAndNumberingInfo(
                            _configs,
                            Approximate,
                            DetectionResults[0].Type,
                        DetectionResults[0].ListIdentifierStart,
                        DetectionResults[0].ListIdentifierEnd
                        );
            for(size_t i = 0; i < _block->lines().size(); ++i) {
                if(DetectionResults[i].Type != enuListType::Bulleted)
                    continue;
                float Approximate = Histogram.approximate(DetectionResults[i].TextLeft);
                if(std::isnan(Approximate) == false)
                    _block->lines().at(i)->setBulletAndNumberingInfo(
                                _configs,
                                Approximate,
                                DetectionResults[i].Type,
                                DetectionResults[i].ListIdentifierStart,
                                DetectionResults[i].ListIdentifierEnd
                                );
                else
                    _block->lines().at(i)->setBulletAndNumberingInfo(_configs, 0, enuListType::None);
            }
        }
    }
}

}
}
}
}
