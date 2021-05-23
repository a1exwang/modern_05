#pragma once
#include <common/kmemory.hpp>


template <typename T>
class kallocator {
 public:
  using value_type = T;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

  pointer allocate(size_type n) {
    return reinterpret_cast<pointer>(kmalloc(n));
  }
  void deallocate(T *p, std::size_t n) {
    kfree(p);
  }

  bool operator==(const kallocator<T> &rhs) const {
    return true;
  }
  bool operator!=(const kallocator<T> &rhs) const {
    return !(*this == rhs);
  }
};