#pragma once

#define KERNEL_CODE_SEGMENT_INDEX 2
#define KERNEL_DATA_SEGMENT_INDEX 3
#define USER_CODE_SEGMENT_INDEX 4
#define USER_DATA_SEGMENT_INDEX 5
#define TSS_INDEX 6
// TSS is 16 bytes, so 7 is always occupied

#define KERNEL_CODE_SELECTOR (KERNEL_CODE_SEGMENT_INDEX*8)
#define KERNEL_DATA_SELECTOR (KERNEL_DATA_SEGMENT_INDEX*8)
#define USER_CODE_SELECTOR (USER_CODE_SEGMENT_INDEX*8 + 3)
#define USER_DATA_SELECTOR (USER_DATA_SEGMENT_INDEX*8 + 3)
#define TSS_SELECTOR (TSS_INDEX*8)

void mm_init();
void map_user_addr(u64 vaddr, u64 paddr, u64 n_pages);

u64 kernel2phy(u64 kernel_vaddr);
bool is_kernel(u64 vaddr);
