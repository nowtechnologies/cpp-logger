#ifndef NOWTECH_ARRAYMAP_INCLUDED
#define NOWTECH_ARRAYMAP_INCLUDED

#include <algorithm>
#include <functional>

namespace nowtech::log {

template<typename tKey, typename tValue, size_t tCapacity, typename tCompare = std::less<>>
class ArrayMap final {
private:
  inline static tCompare sCompare;
public:
  struct Elem final {
    tKey key;
    tValue value;

    Elem() noexcept = default;
    Elem(tKey const aKey) : key(aKey) {
    }

    Elem(Elem const &) noexcept = default;
    Elem(Elem &&) noexcept = delete;
    Elem& operator=(Elem const &) noexcept = default;
    Elem& operator=(Elem &&) noexcept = delete;

    bool operator<(Elem const & aOther) const noexcept {
      return sCompare(key, aOther.key);
    }
  };

private:
  Elem   mArray[tCapacity];
  size_t mCount = 0u;

public:
  ArrayMap() noexcept = default;

  size_t size() const noexcept {
    return mCount;
  }

  Elem const *begin() const noexcept {
    return const_cast<Elem const *>(mArray);
  }

  Elem const *end() const noexcept {
    return const_cast<Elem const *>(mArray + mCount);
  }

  Elem const *find(tKey const aKey) const noexcept {
    Elem const * const last = mArray + mCount;
    Elem key(aKey);
    Elem const* found = std::lower_bound(mArray, last, aKey, [](Elem const& aElem1, Elem const& aElem2){ return aElem1 < aElem2; });
    return (found != last && !tCompare()(key, found->key)) ? found : last;
  }

  bool insert(tKey const aKey, tValue const aValue) noexcept {
    bool result = false;
    if(mCount < tCapacity) {
      Elem* last = mArray + mCount;
      Elem key(aKey);
      Elem* found = std::lower_bound(mArray, last, key, [](Elem const& aElem1, Elem const& aElem2){ return aElem1 < aElem2; });
      std::copy_backward(found, last, last + 1);
      found->key = aKey;
      found->value = aValue;
      ++mCount;
      result = true;
    }
    else { // nothing to do
    }
    return result;
  }

};

}

#endif