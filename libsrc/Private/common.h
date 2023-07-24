#ifndef TARGOMAN_PDF_PRIVATE_COMMON_H
#define TARGOMAN_PDF_PRIVATE_COMMON_H

#include "cmath"
#include "../common.h"
//#ifdef TARGOMAN_SHOW_DEBUG
#include <iostream>
#include <chrono>
#include <cstdint>
#include <iostream>
using namespace std::chrono;
#define debugShow(_stream) {std::wcerr<<"["<<duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count()<<"]"<<"["<<__FILE__<<":"<<__LINE__<<"]"<<_stream<<std::endl;}
#define PROFILE_START_TIMER gProfileStartTime = system_clock::now();
#define PROFILE_ELAPSED_MS duration_cast<milliseconds>(system_clock::now() - gProfileStartTime).count()
//#else
//#define debugShow(_stream) {}
//#endif
#include <cassert>

namespace Targoman {
namespace PDF {
namespace Private {

constexpr size_t MAX_CACHED_PAGES = 20;
constexpr size_t MAX_MERGE_TRIES = 20;
constexpr float DEFAULT_VERTICAL_MARGIN = -6.f;
constexpr float DEFAULT_HORIZONTAL_MARGIN = -8.f;

#ifdef TARGOMAN_SHOW_DEBUG
extern time_point<system_clock, nanoseconds> gProfileStartTime;
#endif

#define FLAG_PROPERTY(_name, _mask) \
    void markAs##_name(bool _state) { \
        this->Flags = (Flags & ~_mask) | (_state ? _mask : 0); \
    } \
    bool is##_name() const { \
        return (this->Flags & _mask) != 0; \
    }

#define PACKED __attribute__ ((__packed__))

class clsPdfItem;
class clsPdfLineSegment;
class clsPdfBlock;
class clsPdfFont;
class clsLayoutStorage;
#define DBG_LAMBDA_ARGS  \
    int _pageIndex, \
    const std::vector<std::shared_ptr<Private::clsPdfBlock>>& _blocks, \
    const std::vector<std::shared_ptr<Private::clsPdfItem>>& _items

#define DBG_CALL_ARGS _pageIndex, _blocks, _items

#ifdef TARGOMAN_SHOW_DEBUG
typedef std::function<void(const std::string& _name, DBG_LAMBDA_ARGS)> OnRenderCallback_t;
extern std::vector<std::tuple<std::string, std::string, OnRenderCallback_t>> gDebugCallBacks;
#endif

void sigStepDone(const std::string& _name, DBG_LAMBDA_ARGS);

#ifdef TARGOMAN_SHOW_DEBUG
template<typename T>
inline std::vector<std::shared_ptr<clsPdfItem>> convertToPdfItemPtrVector(std::vector<std::shared_ptr<T>> _items) {
    std::vector<std::shared_ptr<clsPdfItem>> ConvertedItems;
    for(const auto& Item : _items)
        ConvertedItems.push_back(std::make_shared<clsPdfItem>(*Item));
    return ConvertedItems;
}

template<>
inline std::vector<std::shared_ptr<clsPdfItem>> convertToPdfItemPtrVector(std::vector<std::shared_ptr<clsPdfItem>> _items) {
    return _items;
}
#endif

template<typename T, std::enable_if_t<std::is_same<clsPdfBlock, T>::value, int> = 0>
void sigStepDone(
    const std::string& _name,
    int _pageIndex,
    const std::vector<std::shared_ptr<T>>& _items
) {
#ifdef TARGOMAN_SHOW_DEBUG
    return sigStepDone(_name, _pageIndex, _items, std::vector<std::shared_ptr<clsPdfItem>> {});
#endif
}

template<typename T, std::enable_if_t<std::disjunction<std::is_same<clsPdfItem, T>, std::is_same<clsPdfBlock, T>>::value == false, int> = 0>
void sigStepDone(
    const std::string& _name,
    int _pageIndex,
    const std::vector<std::shared_ptr<clsPdfBlock>>& _blocks,
    const std::vector<std::shared_ptr<T>>& _items
) {
#ifdef TARGOMAN_SHOW_DEBUG
    return sigStepDone(_name, _pageIndex, _blocks, convertToPdfItemPtrVector(_items));
#endif
}

void sigStepDone(
    const std::string& _name,
    int _pageIndex,
    const std::vector<std::shared_ptr<clsPdfBlock>>& _blocks,
    const std::vector<std::shared_ptr<clsPdfBlock>>& _items
);

template<typename T, std::enable_if_t<std::is_same<clsPdfBlock, T>::value, int> = 0>
void sigStepDone(
    const std::string& _name,
    int _pageIndex,
    const std::vector<std::shared_ptr<clsPdfBlock>>& _blocks,
    const std::vector<std::shared_ptr<T>>& _items
) {
#ifdef TARGOMAN_SHOW_DEBUG
    return sigStepDone(_name, _pageIndex, _blocks, convertToPdfItemPtrVector(_items));
#endif
}

template<typename T, std::enable_if_t<std::is_same<clsPdfBlock, T>::value == false, int> = 0>
void sigStepDone(
    const std::string& _name,
    int _pageIndex,
    const std::vector<std::shared_ptr<T>>& _items
) {
#ifdef TARGOMAN_SHOW_DEBUG
    return sigStepDone(_name, _pageIndex, {}, convertToPdfItemPtrVector(_items));
#endif
}

struct stuPageMargins{
    float Left = 0, Top = 0, Right = 0, Bottom = 0;

    void update(const stuPageMargins& _other){
        this->Left = std::min(this->Left, _other.Left);
        this->Top = std::min(this->Top, _other.Top);
        this->Right = std::min(this->Right, _other.Right);
        this->Bottom = std::min(this->Bottom, _other.Bottom);
    }

    const stuPageMargins& sanitized(){
        this->Left = this->Left > (std::numeric_limits<float>::max() - 10) ? 0 : this->Left;
        this->Top = this->Top > (std::numeric_limits<float>::max() - 10) ? 0 : this->Top;
        this->Right = this->Right > (std::numeric_limits<float>::max() - 10) ? 0 : this->Right;
        this->Bottom = this->Bottom > (std::numeric_limits<float>::max() - 10) ? 0 : this->Bottom;
        return *this;
    }
};

struct stuFontInfo {};

inline constexpr float MIN_ITEM_SIZE = 0.01f;

template<typename T>
struct stuItemDeleter {
    static void remove(T& _item) { delete _item; }
};

template<typename T>
struct stuItemDeletionIgnorer {
    static void remove(T&) { }
};

template<
        typename itmplKey,
        typename itmplValue,
        int itmplMaxItems = 100,
        typename itmplDeleter = typename std::conditional<
            std::is_pointer<itmplValue>::value,
            stuItemDeleter<itmplValue>,
            stuItemDeletionIgnorer<itmplValue>
        >::type
>
class tmplLimitedCache{
public:
    tmplLimitedCache(){
        this->Values.reserve(itmplMaxItems);
    }
    ~tmplLimitedCache(){
        this->clear();
    }

    std::shared_ptr<itmplValue>& operator[](const itmplKey& _key){
        iterator ValueIter = this->find(_key);
        if(ValueIter != this->end())
            return ValueIter->second;

        if(Values.size() > itmplMaxItems){
            itmplDeleter::remove(*Values.front().second.get());
            Values.erase(Values.begin());
        }

        Values.push_back(std::make_pair(_key, std::make_shared<itmplValue>()));
        return Values.back().second;
    }

    typedef std::vector<std::pair<itmplKey, std::shared_ptr<itmplValue>>> Storage_t;
    typedef typename Storage_t::const_iterator const_iterator;
    typedef typename Storage_t::iterator iterator;

    const_iterator find(const itmplKey& _key) const{
        for(auto ValuesIter = this->Values.begin(); ValuesIter != this->Values.end(); ++ValuesIter)
            if(ValuesIter->first == _key)
                return  ValuesIter;
        return this->end();
    }

    iterator find(const itmplKey& _key){
        for(auto ValuesIter = this->Values.begin(); ValuesIter != this->Values.end(); ++ValuesIter)
            if(ValuesIter->first == _key)
                return  ValuesIter;
        return this->end();
    }

    const_iterator end() const{
        return  this->Values.end();
    }

    iterator end(){
        return  this->Values.end();
    }

    void clear(){
        for(auto Value : this->Values)
            itmplDeleter::remove(*Value.second.get());
        this->Values.clear();
    }

private:
    Storage_t Values;
};


class clsFloatHistogram {
public:
    typedef std::pair<float, int> HistogramItem_t;
    typedef std::vector<HistogramItem_t> HistogramStorage_t;

private:

    float Delta;
    HistogramStorage_t Items;

public:
    clsFloatHistogram() : Delta(0.f) {}
    clsFloatHistogram(const clsFloatHistogram& _other) = default;

    clsFloatHistogram(float _delta) :
        Delta(_delta)
    {}

    void reinforce(float _value) {
        for(auto& Item : this->Items) {
            if(std::abs(_value - Item.first) < this->Delta) {
                ++Item.second;
                return;
            }
        }
    }

    void insert(float _value) {
        for(auto& Item : this->Items) {
            if(std::abs(_value - Item.first) < this->Delta) {
                ++Item.second;
                return;
            }
        }
        this->Items.push_back(std::make_pair(_value, 1));
    }

    size_t size() const { return this->Items.size(); }

    float approximate(float _value) const {
        for(auto& Item : this->Items)
            if(std::abs(_value - Item.first) < this->Delta)
                return Item.first;
        return NAN;
    }

    const HistogramStorage_t& items() const { return this->Items; }
};

template<typename T0, typename T1, typename T2>
std::tuple<std::vector<T1>, std::vector<T2>> splitAndMap(
        const std::vector<T0>& _v,
        std::function<bool(const T0&)> _f,
        std::function<T1(const T0&)> _m1,
        std::function<T2(const T0&)> _m2)
{
    std::vector<T1> First;
    std::vector<T2> Second;
    for(const auto& Item : _v)
        if(_f(Item))
            First.push_back(_m1(Item));
        else
            Second.push_back(_m2(Item));
    return std::make_pair(First, Second);
}

template<typename T>
std::vector<T> filter(const std::vector<T>& _v, std::function<bool(const T&)> _f, size_t _offset = 0, size_t _size = std::numeric_limits<size_t>::max()) {
    std::vector<T> Result;
    if(_size > _v.size() - _offset)
         _size = _v.size() - _offset;
    _size += _offset;
    for(size_t i = _offset; i < _size; ++i)
        if(_f(_v[i]))
            Result.push_back(_v[i]);
    return Result;
}

template<typename T>
bool anyOf(const std::vector<T>& _v, std::function<bool(const T&)> _f, size_t _offset = 0, size_t _size = std::numeric_limits<size_t>::max()) {
    if(_size > _v.size() - _offset)
        _size = _v.size() - _offset;
    _size += _offset;
    for(size_t i = _offset; i < _size; ++i)
        if(_f(_v[i]))
            return true;
    return false;
}

inline bool anyOf(const std::vector<bool>& _v, size_t _offset = 0, size_t _size = std::numeric_limits<size_t>::max()) {
    if(_size > _v.size() - _offset)
        _size = _v.size() - _offset;
    _size += _offset;
    for(size_t i = _offset; i < _size; ++i)
        if(_v[i])
            return true;
    return false;
}

template<typename T>
bool allOf(const std::vector<T>& _v, std::function<bool(const T&)> _f, size_t _offset = 0, size_t _size = std::numeric_limits<size_t>::max()) {
    if(_size > _v.size() - _offset)
        _size = _v.size() - _offset;
    _size += _offset;
    for(size_t i = _offset; i < _size; ++i)
        if(!_f(_v[i]))
            return false;
    return true;
}

inline bool allOf(const std::vector<bool>& _v, size_t _offset = 0, size_t _size = std::numeric_limits<size_t>::max()) {
    if(_size > _v.size() - _offset)
        _size = _v.size() - _offset;
    _size += _offset;
    for(size_t i = _offset; i < _size; ++i)
        if(!_v[i])
            return false;
    return true;
}

template<typename T>
bool mostOf(const std::vector<T>& _v, std::function<bool(const T&)> _f, float _fraction = 0.5f, size_t _offset = 0, size_t _size = std::numeric_limits<size_t>::max()) {
    if(_size > _v.size() - _offset)
        _size = _v.size() - _offset;
    size_t Threshold = static_cast<size_t>(_size * _fraction + 0.5f);
    _size += _offset;
    size_t Count = 0;
    for(size_t i = _offset; i < _size; ++i)
        if(_f(_v[i])) {
            ++Count;
            if(Count >= Threshold)
                return true;
        }
    return false;
}

inline bool mostOf(const std::vector<bool>& _v, float _fraction = 0.5f, size_t _offset = 0, size_t _size = std::numeric_limits<size_t>::max()) {
    if(_size > _v.size() - _offset)
        _size = _v.size() - _offset;
    size_t Threshold = static_cast<size_t>(_size * _fraction + 0.5f);
    _size += _offset;
    size_t Count = 0;
    for(size_t i = _offset; i < _size; ++i)
        if(_v[i]) {
            ++Count;
            if(Count > Threshold)
                return true;
        }
    return false;
}

}
}
}

#endif // TARGOMAN_PDF_PRIVATE_COMMON_H
