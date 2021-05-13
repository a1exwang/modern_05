#pragma once

#include <cstddef>
#include <utility>
#include <lib/string.h>

template <typename T, size_t N>
class SmallVec { // NOLINT(cppcoreguidelines-pro-type-member-init)
 public:
  size_t size() const {
    return this->size_;
  }

  bool empty() const {
    return size() == 0;
  }

  T *begin() {
    return &get_buf(0);
  }

  const T *begin() const {
    return &get_buf(0);
  }

  T *end() {
    return &get_buf(size_);
  }

  const T *end() const {
    return &get_buf(size_);
  }

  void resize(size_t new_size) {
    if (new_size < size_) {
      for (size_t i = new_size; i < size_; i++) {
        get_buf(i).~T();
      }
      size_ = new_size;
    } else if (new_size > size_) {
      for (size_t i = size_; i < new_size; i++) {
        new(&get_buf(i)) T();
      }
    }
  }

  template <typename... Args>
  T &emplace_back(Args... args) {
    auto ptr = new(&get_buf(size_)) T(std::forward<Args>(args)...);
    size_++;
    return *ptr;
  }

  void pop_back() {
    assert(size_ != 0, "cannot pop empty vec");
    size_--;
    get_buf(size_).~T();
  }

  T &push_back(const T &obj) {
    T &ret = get_buf(size_);
    ret = obj;
    size_++;
    return ret;
  }

  T &back() {
    assert(size_ != 0, "SmallVec is empty");
    return get_buf(size_ - 1);
  }

  const T &back() const {
    assert(size_ != 0, "SmallVec is empty");
    return get_buf(size_ - 1);
  }

  T &operator[](size_t offset) {
    return get_buf(size_);
  }

  const T &operator[](size_t offset) const {
    return get_buf(offset);
  }

 private:
  T &get_buf(size_t offset) {
    return *reinterpret_cast<T*>(buf_ + sizeof(T)*offset);
  }
  const T &get_buf(size_t offset) const {
    return *reinterpret_cast<const T*>(buf_ + sizeof(T)*offset);
  }

  char buf_[sizeof(T) * N];
  size_t size_ = 0;
};