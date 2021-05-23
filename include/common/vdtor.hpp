#pragma once

#define VIRTUAL_DTOR(cls) \
  void operator delete(cls* p, std::destroying_delete_t) { \
    p->~cls(); \
    kfree(p); \
  } \
  virtual ~cls() = default

