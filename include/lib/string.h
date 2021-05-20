#pragma once

#include <common/defs.h>
#include <cstddef>

extern "C" {
void *memset(void *data, int value, u64 size) noexcept(true);
void *memcpy(void *dst, const void *src, u64 size) noexcept(true);
int memcmp(const void *dst, const void *src, u64 size) noexcept(true);
int strcmp(const char *lhs, const char *rhs) noexcept(true);
size_t strlen(const char *s) noexcept(true);
void *memmove(void *dest, const void *src, size_t size);
}

void do_assert(int, const char *, const char *message, const char *file, const char *func, int line);

#define assert1(condition) \
  do_assert((condition), #condition, "", __FILE__, __FUNCTION__, __LINE__)

#define assert(condition, message) \
  do_assert((condition), #condition, (message), __FILE__, __FUNCTION__, __LINE__)

void hexdump(const char *ptr, u64 size, bool compact = false, int indent = 0);
