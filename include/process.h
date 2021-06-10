#pragma once
#include "common/defs.h"
#include <common/kstring.hpp>

#pragma pack(push, 1)
// If you update this struct, you should update _irq_handler and _return_from_syscall in irq.S
struct Context {
  u64 ds;
  u64 es;
  u64 fs;
  u64 gs;

  u64 cr3;

  u64 r15;
  u64 r14;
  u64 r13;
  u64 r12;
  u64 r11;
  u64 r10;
  u64 r9;
  u64 r8;
  u64 rbp;
  u64 rsi;
  u64 rdi;
  u64 rbx;
  u64 rdx;
  u64 rcx;
  u64 rax;

  u64 reserved1[2];
  u64 rip;
  u64 cs;
  u64 rflags;
  u64 rsp;
  u64 ss;
};
#pragma pack(pop)

void process_init();

extern "C" {

void schedule();
void store_current_thread_context(Context *context);
Context *current_context();
extern u64 current_pid;

}

constexpr u64 MAX_PROCESS = 1024;
constexpr u64 THREAD_KERNEL_STACK_SIZE = 16*PAGE_SIZE;
constexpr u64 PROCESS_MAX_USER_PAGES = 512*512;

constexpr u64 USER_STACK_SIZE = 1024*1024;
constexpr u64 USER_IMAGE_START = 1UL*1024UL*1024UL;
constexpr u64 USER_IMAGE_SIZE = 8UL * 1024*1024;

enum ProcessState {
  Wait,
  Running
};

u64 get_kernel_pdpt_phy_addr();

#pragma pack(push, 1)
struct PageTabletSructures {
  // page table for the process
  PageMappingL4Entry pml4t[PAGES_PER_TABLE] ALIGN(PAGE_SIZE);
  PageDirectoryPointerEntry pdpt[PAGES_PER_TABLE] ALIGN(PAGE_SIZE);
  PageDirectoryEntry pdt[PAGES_PER_TABLE] ALIGN(PAGE_SIZE);
  PageTableEntry pt[PROCESS_MAX_USER_PAGES] ALIGN(PAGE_SIZE);
};
#pragma pack(pop)

class Process;
extern Process *processes[];
class Process {
 public:

  explicit Process(u64 id, u64 start_phy);

  void print() {
    Kernel::sp() << SerialPort::IntRadix::Hex << "user image " << user_image_phy_addr << " stack 0x" << user_stack_phy_addr << "\n";
  }

  void map_user_addr(u64 vaddr, u64 paddr, u64 n_pages);

  void *load_elf_from_buffer(char *buffer, u64 size);

  static void process_entrypoint(Process *p) {
    p->kernel_entrypoint();
  }

  u64 phy_addr_in_this(void *ptr) {
    return start_phy + ((u64)ptr - (u64)this);
  }

  static Process *current() {
    return processes[current_pid];
  }

  void kernel_entrypoint();

  // id = 0 for empty process slot
  u64 id;
  ProcessState state = ProcessState::Wait;
  kstring name;
  // phy addr of the start of this Process object
  u64 start_phy = 0;
  int jiffies = 0;
  int max_jiffies = 10;

  PageTabletSructures *pts;
  u64 pts_paddr;

  u8 kernel_stack[THREAD_KERNEL_STACK_SIZE];
  u8 kernel_stack_bottom[0];

  Context context;

  u64 user_image_phy_addr;
  u8 *user_image;
  u64 user_image_size = USER_IMAGE_SIZE;

  u64 user_stack_phy_addr;
  u8 *user_stack;
  u64 user_stack_size = USER_STACK_SIZE;

  void *cookie;

  void (*tmp_start)(void*) = 0;
};

u64 create_kthread(const kstring &name, void (*start)(void *), void *cookie);

void kyield();