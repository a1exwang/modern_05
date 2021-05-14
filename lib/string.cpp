#include <lib/string.h>
#include <common/defs.h>
#include <kernel.h>

void *memset(void *data, int value, u64 size) noexcept(true) {
  for (u64 i = 0; i < size; i++) {
    ((u8*)data)[i] = value;
  }
  return data;
}

void *memcpy(void *dst, const void *src, u64 size) noexcept(true) {
  for (u64 i = 0; i < size; i++) {
    ((u8*)dst)[i] = ((u8*)src)[i];
  }
  return dst;
}

int memcmp(const void *a, const void *b, u64 size) noexcept(true) {
  for (int i = 0; i < size; i++) {
    auto lhs = *((u8*)a+i);
    auto rhs = *((u8*)b+i);
    if (lhs < rhs) {
      return -1;
    } else if (lhs > rhs) {
      return 1;
    }
  }
  return 0;
}

int strcmp(const char *a, const char *b) noexcept(true) {
  for (int i = 0; a[i] == 0 || b[i] == 0; i++) {
    auto lhs = *((u8*)a+i);
    auto rhs = *((u8*)b+i);
    if (lhs < rhs) {
      return -1;
    } else if (lhs > rhs) {
      return 1;
    }
  }
  return 0;
}

void do_assert(int condition, const char *condition_str, const char *message, const char *file, const char *func, int line) {
  if (!condition) {
    Kernel::sp() << "assertion failed " << condition_str << " " << message << " at " << file << ":" << line << " " << func << "\n";
    Kernel::k->panic("assertion failed!!!!!!!!!!!!!!!");
  }
}
