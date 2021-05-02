#include <lib/string.h>
#include <lib/defs.h>

void memset(void *data, u8 value, u64 size) {
  for (u64 i = 0; i < size; i++) {
    ((u8*)data)[i] = value;
  }
}

void memcpy(void *dst, const void *src, u64 size) {
  for (u64 i = 0; i < size; i++) {
    ((u8*)dst)[i] = ((u8*)src)[i];
  }
}
