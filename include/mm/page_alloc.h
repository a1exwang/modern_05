#pragma once
#include <lib/defs.h>

struct PageRegion {
  u64 start;
  u64 n_pages;
};

void page_allocator_init(PageRegion *regions, u64 n_regions);

// allocate 2^i contiguous physical pages
// return 0 on failure
// returns physical address of the first page
u64 page_alloc(u64 i);

// release the pages
void page_release(u64 phy_addr);