#pragma once

#include <common/defs.h>


extern "C" {
void *memset(void *data, int value, u64 size) noexcept(true);
void *memcpy(void *dst, const void *src, u64 size) noexcept(true);
int memcmp(const void *dst, const void *src, u64 size) noexcept(true);
}

void do_assert(int, const char *);

#define assert(condition, message) \
  do_assert((condition), "assert(" #condition ") failed " message);

void hexdump(const char *ptr, u64 size, bool compact = false, int indent = 0);
