#pragma once
#include <lib/defs.h>

struct PageRegion {
  u64 start;
  u64 n_pages;
};

void page_allocator_init(PageRegion *regions, u64 n_regions);

// allocate 2^i contiguous physical pages that has mapped to kernel space
// return 0 on failure
// returns virtual address of the first page
void *kernel_page_allocate(u64 i);

// release the pages
void kernel_page_free(void *);

u64 physical_page_alloc(u64 i);
void physical_page_release(u64 paddr);
