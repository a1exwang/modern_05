#pragma once
#include <memory>

// byte level allocation
void *kmalloc(size_t size);
void kfree(void *p);

template<typename T, typename... Args>
T *knew(Args... args) {
  auto buf = kmalloc(sizeof(T));
  return new (buf) T(std::forward<Args>(args)...);
}

template<typename T>
void kdelete(T *p) {
  p->~T();
  kfree(p);
}

template <typename T>
struct kdefault_delete {
  void operator()(T* p) const {
    static_assert(!std::is_void<T>::value,
                  "can't delete pointer to incomplete type");
    static_assert(sizeof(T)>0, // NOLINT(bugprone-sizeof-expression)
                  "can't delete pointer to incomplete type");
    kfree(p);
  }
};

template <typename T>
using kup = std::unique_ptr<T, kdefault_delete<T>>;

template <typename T, typename... Args>
kup<T> make_kup(Args&& ... args) {
  auto buf = kmalloc(sizeof(T) + sizeof(T*));
  auto obj = new(buf) T(std::forward<Args>(args)...);
  *(T**)((char*)buf + sizeof(T)) = obj;

  return kup<T>(obj);
}