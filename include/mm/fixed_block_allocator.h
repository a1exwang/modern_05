#pragma once
#include <lib/defs.h>
#include <utility>

template <typename T, T* (T::*NextPtr)>
class FixedBlockAllocator {
 public:
  FixedBlockAllocator(T *buffer, u64 max_size)
      :buffer(buffer), max_size(max_size) {

    if (max_size == 0) {
      Kernel::k->panic("FixedBlockAllocator max_size must > 0");
    }

    T *last = &buffer[0];
    for (u64 i = 1; i < max_size; i++) {
      next_ptr(last) = &buffer[i];
      last = &buffer[i];
    }

    next_ptr(&buffer[max_size-1]) = nullptr;

    head = &buffer[0];
    tail = &buffer[max_size - 1];
  }

  T *&next_ptr(T *p) {
    return p->*NextPtr;
  }

  T *allocate() {
    if (head == tail) {
      return nullptr;
    }
    auto old_head = head;
    auto new_head = next_ptr(head);

    next_ptr(old_head) = nullptr;
    head = new_head;
    return old_head;
  }

  template <typename... Args>
  T *create(Args&&...  args) {
    if (head == tail) {
      return nullptr;
    }
    auto old_head = head;
    auto new_head = next_ptr(head);

    next_ptr(old_head) = nullptr;
    head = new_head;

    auto ret = old_head;
    if (ret != nullptr) {
      new(ret) T(std::forward<Args>(args)...);
    }
    return ret;
  }

  void free(T *p) {
    auto new_head = p;
    next_ptr(new_head) = head;
    head = new_head;
  }

 private:
  T *buffer;
  u64 max_size;
  // head must be valid as a sentry
  // head == tail for emtpy list
  T *head;
  T *tail;
};
