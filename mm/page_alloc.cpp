#include <cpu_defs.h>
#include <kernel.h>
#include <lib/file_size.h>
#include <lib/string.h>
#include <lib/utils.h>
#include <mm/page_alloc.h>

struct Block {
  Block() = default;
  explicit Block(u8 available) :available(available) {}

  u8 valid = false;
  u8 available = true;

  Block *prev = nullptr;
  Block *next = nullptr;
};

struct Bucket {
  u64 log2size;

  // can be null to represent empty
  Block *first_block;
};

constexpr u64 BlockSize = 16 * PAGE_SIZE;
constexpr u64 MaxBlocks = 4UL * 1024UL * 1024UL * 1024UL / BlockSize;

static Block preallocated_blocks[MaxBlocks];

struct BuddyAllocator {
  BuddyAllocator(u64 addr, u64 log2size, Block *base, u64 max_blocks) :phy_start(addr), root_log2size(log2size), blocks(base), max_blocks(max_blocks) {
    // initialize buckets
    memset(buckets, 0, sizeof(buckets));
    auto root = &base[0];
    new(root) Block(true);
    root->valid = true;
    for (u64 i = 0; i < (Log2MaxSize-Log2MinSize+1); i++) {
      buckets[i].first_block = nullptr;
      buckets[i].log2size = Log2MinSize + i;
    }

    root->next = root;
    root->prev = root;
    buckets[log2size - Log2MinSize].first_block = root;
  }

  u64 get_block_offset(Block *block) const {
    return block - blocks;
  }

  void print() const {
    Kernel::sp() << "Buddy allocator at 0x" << SerialPort::IntRadix::Hex << (u64)this << "\n";
    for (const auto &bucket : buckets) {
      print_bucket(&bucket);
    }
  }

  void print_bucket(const Bucket *bucket) const {
    Kernel::sp() << "  bucket " << SerialPort::IntRadix::Dec << bucket->log2size << " size = ";
    print_file_size(1UL << bucket->log2size);
    Kernel::sp() << ": \n";
    if (bucket->first_block) {
      Block *block = bucket->first_block;
      do {
        Kernel::sp()
            << "    "
            << SerialPort::IntRadix::Hex
            << "id 0x" << get_block_offset(block) << " "
            << "addr 0x" << (phy_start+get_block_offset(block)*BlockSize) << " "
            << "buddy 0x" << (u64) get_buddy(block) << " "
            << "parent 0x" << (u64) get_parent(block) << " "
            << (block->available ? "available" : "used") << " "
            << (block->valid ? "valid" : "invalid") << " "
            << "\n";

        block = block->next;
      } while (block != bucket->first_block);
    }
  }

  Block *get_left(Block *block) const {
    u64 offset = block - blocks;
    if (2*offset+1 >= max_blocks) {
      return nullptr;
    }
    return &blocks[2 * offset + 1];
  }

  Block *get_right(Block *block) const {
    u64 offset = block - blocks;
    if (2*offset+2 >= max_blocks) {
      return nullptr;
    }
    return &blocks[2 * offset + 2];
  }

  Block *get_parent(Block *block) const {
    u64 offset = block - blocks;
    if (offset == 0) {
      return nullptr;
    }
    return &blocks[(offset-1) / 2];
  }

  Block *get_buddy(Block *block) const {
    auto offset = get_block_offset(block);
    if (offset == 0) {
      return nullptr;
    }
    if (offset % 2 == 0) {
      return &blocks[offset - 1];
    } else {
      return &blocks[offset + 1];
    }
  }

  void remove_from_available_list(Block *block) {
    // need to change list head from bucket
    auto &bucket = buckets[block_log2size(block) - Log2MinSize];
    if (bucket.first_block == block) {
      if (block->next == block) {
        bucket.first_block = nullptr;
      } else {
        bucket.first_block = block->next;
      }
    }

    auto next = block->next;
    auto prev = block->prev;

    if (prev) {
      prev->next = next;
    }
    if (next) {
      next->prev = prev;
    }
  }

  Block *alloc_block_size(u64 log2size) {
    if (log2size > root_log2size) {
      return nullptr;
    }

    auto first = buckets[log2size - Log2MinSize].first_block;
    if (first == nullptr) {
      // split a larger block
      auto parent_block = alloc_block_size(log2size + 1);
      if (parent_block == nullptr) {
        return nullptr;
      }

      auto left = get_left(parent_block);
      auto right = get_right(parent_block);

      // return left node as allocated node
      left->available = false;
      left->valid = true;
      right->available = true;
      right->valid = true;
      add_to_available_list(right);

      return left;
    } else {
      first->available = false;
      remove_from_available_list(first);
      return first;
    }
  }

  u64 allocate_pages(u64 log2size) {
    if (log2size < Log2MinSize) {
      Kernel::k->panic("Failed to allocate pages, log2size < Log2MinSize");
    }

    auto block = alloc_block_size(log2size);
    if (!block) {
      return 0;
    }

    return get_block_phy_addr(block);
  }

  u64 get_block_phy_addr(const Block *block) const {
    return phy_start + (block - blocks) * BlockSize;
  }

  Block *try_merge_with_buddy(Block *block) {
    auto parent = get_parent(block);
    if (!parent) {
      return block;
    }

    auto buddy = get_buddy(block);
    if (!buddy || !buddy->available) {
      return block;
    }

    remove_from_available_list(buddy);
    buddy->valid = false;

    assert(!parent->available, "inconsistent data structure, parent should not be available");
    assert(parent->valid, "inconsistent data structure, parent should be valid");
    parent->available = true;

    block->valid = false;
    return try_merge_with_buddy(parent);
  }

  void add_to_available_list(Block *block) {
    auto log2size = block_log2size(block);
    auto &first = buckets[log2size-Log2MinSize].first_block;
    if (!first) {
      block->next = block;
      block->prev = block;
      first = block;
    } else {
      auto last = first->prev;
      first->prev = block;
      last->next = block;

      block->next = first;
      block->prev = last;
    }
  }

  void free_pages(u64 addr) {
    auto block_offset = (addr - phy_start) / BlockSize;
    assert((addr-phy_start) % BlockSize == 0, "Failed to free pages, addr is not aligned");

    auto block = &blocks[block_offset];
    assert(!block->available, "Cannot free available pages");

    auto merged_block = try_merge_with_buddy(block);
    merged_block->available = true;
    add_to_available_list(merged_block);
  }

  u64 block_log2size(Block *block) const {
    u64 depth = log2(get_block_offset(block) + 1);
    return root_log2size - depth;
  }

  u64 phy_start;
  u64 root_log2size;
  Block *blocks;
  u64 max_blocks;
  Bucket buckets[Log2MaxSize - Log2MinSize + 1]{};
};

static char buddy_allocator_mem[sizeof(BuddyAllocator)];

BuddyAllocator *buddy_allocator;

void buddy_allocator_init(u64 max_pages_addr, u64 log2size) {
  // test buddy allocator
  buddy_allocator = new(buddy_allocator_mem) BuddyAllocator(max_pages_addr, log2size, preallocated_blocks, MaxBlocks);
  buddy_allocator->print();

  auto addr1 = buddy_allocator->allocate_pages(24);
  Kernel::sp() << "allocated page phy addr 0x" << SerialPort::IntRadix::Hex << addr1 << "\n";

  auto addr2 = buddy_allocator->allocate_pages(16);
  Kernel::sp() << "allocated page phy addr 0x" << SerialPort::IntRadix::Hex << addr2 << "\n";

  buddy_allocator->free_pages(addr1);
  buddy_allocator->free_pages(addr2);

  bool passed = true;
  for (int i = 0; i < sizeof(buddy_allocator->buckets) / sizeof(buddy_allocator->buckets[0]); i++) {
    if (i + Log2MinSize == buddy_allocator->root_log2size) {
      if (buddy_allocator->buckets[i].first_block == nullptr) {
        passed = false;
      } else if (buddy_allocator->buckets[i].first_block->next != buddy_allocator->buckets[i].first_block) {
        passed = false;
      }
    } else {
      if (buddy_allocator->buckets[i].first_block != nullptr) {
        passed = false;
      }
    }
  }

  assert(passed, "Buddy allocator test failed");
  Kernel::sp() << "Buddy allocator test passed\n";
}

class PageAllocator {

  // assume size = 512
  PageDirectoryEntry *pdt;
  // assume size = 512 * 512
  PageTableEntry *pt;
};

void page_allocator_init(PageRegion *regions, u64 n_regions) {
  // TODO: improve allocator to support non 2^n regions
  // find the largest region that is below 4G
  u64 max_pages = 0;
  u64 max_pages_addr = 0;
  for (u64 i = 0; i < n_regions; i++) {
    u64 n_pages = regions[i].n_pages;
    if (n_pages > max_pages && regions[i].start < 0x100000000UL) {
      max_pages = n_pages;
      max_pages_addr = regions[i].start;
    }
  }

  if (max_pages_addr < IDENTITY_MAP_PHY_START) {
    auto unusable = IDENTITY_MAP_PHY_START - max_pages_addr;
    max_pages_addr = IDENTITY_MAP_PHY_START;
    max_pages -= unusable/PAGE_SIZE;
  }
  if (max_pages > (1UL*1024UL*1024UL*1024UL)/PAGE_SIZE) {
    max_pages = (1UL*1024UL*1024UL*1024UL)/PAGE_SIZE;
  }
  auto log2size = log2(max_pages*PAGE_SIZE);

  Kernel::sp() << IntRadix::Hex << "max_pages_addr: " << max_pages_addr  << ", max_pages " << max_pages << "\n";

  assert(max_pages_addr >= IDENTITY_MAP_PHY_START, "region out of identity map region");
  assert(max_pages_addr+(1UL<<log2size) <= IDENTITY_MAP_PHY_END, "region out of identity map region");
  buddy_allocator_init(max_pages_addr, log2size);
}

void *kernel_page_alloc(u64 i) {
  auto phy_addr = buddy_allocator->allocate_pages(i);
  return (void*)(KERNEL_START + phy_addr);
}

void kernel_page_free(void *vaddr) {
  buddy_allocator->free_pages((u64)vaddr - KERNEL_START);
}

u64 physical_page_alloc(u64 i) {
  return buddy_allocator->allocate_pages(i);
}

void physical_page_release(u64 paddr) {
  buddy_allocator->free_pages(paddr);
}
void kfree(void *p) {
  kernel_page_free(p);
}
void *kmalloc(size_t size) {
  auto log2size = log2_ceil(size);
  if (log2size < Log2MinSize) {
    log2size = Log2MinSize;
  }
  return kernel_page_alloc(log2size);
}
