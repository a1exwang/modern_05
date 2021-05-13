#include <lib/string.h>
#include <common/defs.h>
#include <kernel.h>

void memset(void *data, int value, u64 size) {
  for (u64 i = 0; i < size; i++) {
    ((u8*)data)[i] = value;
  }
}

void memcpy(void *dst, const void *src, u64 size) {
  for (u64 i = 0; i < size; i++) {
    ((u8*)dst)[i] = ((u8*)src)[i];
  }
}

int memcmp(const void *a, const void *b, u64 size) {
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

void do_assert(int assumption, const char *s) {
  if (!assumption) {
    Kernel::k->panic(s);
  }
}
