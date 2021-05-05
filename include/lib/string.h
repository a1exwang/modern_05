#include <lib/defs.h>

void memset(void *data, int value, u64 size);
void memcpy(void *dst, const void *src, u64 size);

void do_assert(int, const char *);

#define assert(condition, message) \
  do_assert((condition), "assert(" #condition ") failed: " message);
