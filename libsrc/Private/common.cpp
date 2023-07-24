#include "./common.h"

#ifdef TARGOMAN_SHOW_DEBUG
#include "clsPdfItem.h"
#include "clsPdfLineSegment.h"
#include "clsPdfBlock.h"
int gPageIndex;
int gMaxStep;
#endif

namespace Targoman {
namespace PDF {

void ltrim(std::wstring &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
}

void rtrim(std::wstring &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [] (int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

void trim(std::wstring &s)
{
    ltrim(s);
    rtrim(s);;
}

std::vector<enuLocation> PdfLocations = {
    enuLocation::Main,
    enuLocation::Footnote,
    enuLocation::Header,
    enuLocation::Footer
};

namespace Private {
#ifdef TARGOMAN_SHOW_DEBUG
time_point<system_clock, nanoseconds> gProfileStartTime;

std::vector<std::tuple<std::string, std::string, OnRenderCallback_t>> gDebugCallBacks = {
    { "Items", "onItems", nullptr },
    { "Stamp", "onStampsMarked", nullptr },
    { "Strip", "onStripsIdentified", nullptr },
    { "StExp", "onStampsExpanded", nullptr },
    { "HdrFt", "onMarkHeaderAndFooters", nullptr },
    { "DocCl", "onColumnsIdentified", nullptr },
    { "MainC", "onMainContentReady", nullptr },
    { "Block", "onRawBlocksFound", nullptr },
    { "Inner", "onInnerBlocks", nullptr},
    { "Markd", "onMarkedBlocks", nullptr},
    { "Final", "onFinalSentences", nullptr},
};

void sigStepDone(const std::string& _name, DBG_LAMBDA_ARGS) {
    int StepIndex = 1;
    for(const auto& Item : gDebugCallBacks) {
        if(std::get<1>(Item) == _name) {
            if(std::get<2>(Item) != nullptr)
                std::get<2>(Item)("Step" + std::to_string(StepIndex) + std::get<0>(Item), DBG_CALL_ARGS);
            return;
        }
        ++StepIndex;
    }
}

void sigStepDone(const std::string& _name, int _pageIndex, const std::vector<std::shared_ptr<clsPdfBlock>>& _blocks, const std::vector<std::shared_ptr<clsPdfBlock>>& _items)
{
    std::vector<std::shared_ptr<clsPdfItem>> ConvertedItems;
    std::function<void(const std::vector<std::shared_ptr<clsPdfBlock>>&)> extractLines = [&] (const std::vector<std::shared_ptr<clsPdfBlock>>& _blocks) {
        for(const auto& Block : _blocks) {
            if(Block->innerBlocks().size() > 0)
                extractLines(Block->innerBlocks());
            else
                for(const auto& Line : Block->lines())
                    ConvertedItems.push_back(std::make_shared<clsPdfItem>(*Line));
        }
    };
    extractLines(_items);
    sigStepDone(_name, _pageIndex, _blocks, ConvertedItems);
}

#else
void sigStepDone(const std::string& _name, DBG_LAMBDA_ARGS){}
void sigStepDone(const std::string& _name, int _pageIndex, const std::vector<std::shared_ptr<clsPdfBlock>>& _blocks, const std::vector<std::shared_ptr<clsPdfBlock>>& _items){}
#endif

}

}
}
