#pragma once
#include <common/defs.h>

struct PageRegion {
  u32 type; // EFI Type
  u64 phy_start;
  u64 virt_start;
  u64 size;
  u64 attr;
};

void page_allocator_init(SmallVec<PageRegion, 1024> &regions);

// allocate 2^i contiguous physical pages that has mapped to kernel space
// return 0 on failure
// returns virtual address of the first page
void *kernel_page_alloc(u64 i);

// release the pages
void kernel_page_free(void *);

u64 physical_page_alloc(u64 i);
void physical_page_release(u64 paddr);

constexpr u64 Log2MinSize = 16; // 64K
constexpr u64 Log2MaxSize = 32; // 4G
