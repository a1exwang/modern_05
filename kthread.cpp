#include <kthread.h>
#include <kernel.h>
#include <lib/string.h>
#include <irq.h>
#include <syscall.h>
#include "debug.h"
#include <mm/mm.h>
#include <cpu_defs.h>
#include <mm/page_alloc.h>

u64 current_thread_id = 0;
u64 total_threads = 2;
Thread *main_thread;
Thread *thread2;
extern "C" {
u8 threads_space[32 * sizeof(Thread)];
}

constexpr u64 THREAD_KERNEL_STACK_SIZE = 16*PAGE_SIZE;
constexpr u64 PROCESS_MAX_USER_PAGES = 512*512;

constexpr u64 USER_STACK_SIZE = 1024*1024;
constexpr u64 USER_IMAGE_SIZE = 1024*1024;

PageDirectoryPointerEntry *kernel_pdpt;

class Process {
 public:
  void init(u64 id) {
    memset(this, 0, sizeof(Process));
    this->id = id;
    pml4t[256].p = 1;
    pml4t[256].rw = 1;
    pml4t[256].us = 0;
    pml4t[256].base_addr = ((u64)kernel_pdpt) >> 12;

    // only the first pdpt is used
    pdpt[0].p = 1;
    pdpt[0].rw = 1;
    pdpt[0].us = 1; // allow userspace
    pdpt[0].base_addr = ((u64)(&pdt)) >> 12;

    for (u64 i = 0; i < KERNEL_IMAGE_PAGES / PAGES_PER_TABLE; i++) {
      pdt[i].p = 1;
      pdt[i].rw = 1;
      pdt[i].us = 1;
      pdt[i].base_addr = ((u64)&pt[PAGES_PER_TABLE * i]) >> 12;
    }

    // 1MiB ~ 2MiB are for exec image
    user_image = (u8*)(1UL*1024UL*1024UL);
    user_image_phy_addr = physical_page_alloc(USER_IMAGE_SIZE);
    map_user_addr((u64)user_image, user_image_phy_addr, user_image_size);

    // 32MiB ~ 33MiB are for user stack
    user_stack_phy_addr = physical_page_alloc(USER_STACK_SIZE);
    map_user_addr(32UL*1024UL*1024UL, user_stack_phy_addr, user_stack_size);

    kernel_context.cs = KERNEL_CODE_SELECTOR;
    kernel_context.ss = KERNEL_DATA_SELECTOR;
    kernel_context.rsp = (u64)kernel_stack_bottom;
    kernel_context.rip = (u64)&process_entrypoint;
    // first parameter for process_entry_point(p)
    kernel_context.rdi = (u64)this;
  }
  void map_user_addr(u64 vaddr, u64 paddr, u64 n_pages) {
    for (u64 i = 0; i < n_pages; i++) {
      auto vpage = (vaddr >> 12) + i;
      auto ppage = (paddr >> 12) + i;
      pt[vpage].p = 1;
      pt[vpage].rw = 1;
      pt[vpage].us = 1;
      pt[vpage].base_addr = ppage;
    }
  }

  static void process_entrypoint(Process *p) {
    p->kernel_entrypoint();
  }

  void kernel_entrypoint() {
    Kernel::sp() << "starting kernel thread '" << (const char*)name << "'\n";
    halt();
  }

  u64 id;
  char name[32];

  // page table for the process
  PageMappingL4Entry pml4t[PAGES_PER_TABLE];
  PageDirectoryPointerEntry pdpt[PAGES_PER_TABLE];
  PageDirectoryEntry pdt[PAGES_PER_TABLE];
  PageTableEntry pt[PROCESS_MAX_USER_PAGES];

  u8 kernel_stack[THREAD_KERNEL_STACK_SIZE];
  u8 kernel_stack_bottom[0];

  ThreadContext kernel_context;

  u64 user_image_phy_addr;
  u8 *user_image;
  u64 user_image_size = USER_IMAGE_SIZE;

  u64 user_stack_phy_addr;
  u8 *user_stack;
  u64 user_stack_size = USER_STACK_SIZE;
};


void thread_start() {
  auto *threads = (Thread*)threads_space;
  new(&threads[1]) Thread(1, main_thread_start);
  new(&threads[2]) Thread(2, thread2_start);

  current_thread_id = 1;
}
void schedule() {
  if (current_thread_id == total_threads) {
    current_thread_id = 1;
  } else {
    current_thread_id++;
  }
}
void store_current_thread_context(ThreadContext *context) {
  Thread *t = get_thread(current_thread_id);
  t->store_context(context);
}

void main_thread_start() {
  u64 i = 0;

  Kernel::sp() << "main thread started\n";
  while (true) {
    u64 reg_new;
    register u64 reg asm("rax");
    reg = (u64)0x2223;
    do_syscall();
    while(1);
    __asm__ __volatile__("movq %%rax, %0" :"=r"(reg_new));
//    Kernel::sp() << SerialPort::IntRadix::Hex << "thread1 run " << i << " " << reg_new << "\n";
    i++;
  }
}
void thread2_start() {
  u64 i = 0;
  while (true) {
    u64 reg_new;
    register u64 reg asm("rax");
    reg = 0x3332;
    do_syscall();
    __asm__ __volatile__("mov %%rax, %0" :"=r"(reg_new));
//    Kernel::sp() << SerialPort::IntRadix::Hex << "thread2 run " << i << " " << reg_new << "\n";
    i++;
  }
}

Thread *get_thread(unsigned long id) {
  return &((Thread*)threads_space)[id];
}
void dump_thread_context(const ThreadContext &context) {
  Kernel::sp() << SerialPort::IntRadix::Hex << "rip: 0x" << context.rip << " rsp: " << context.rsp << "\n";
}

void Thread::store_context(ThreadContext *old_context) {
  memcpy(&this->context, old_context, sizeof(ThreadContext));
}
Thread::Thread(unsigned long id, ThreadFunction start) :id(id), start(start) {
  memset(&context, 0, sizeof(context));
  context.cs = KERNEL_CODE_SELECTOR;
  context.ss = KERNEL_DATA_SELECTOR;
  context.ds = KERNEL_DATA_SELECTOR;
  context.es = KERNEL_DATA_SELECTOR;
  context.fs = KERNEL_DATA_SELECTOR;
  context.gs = KERNEL_DATA_SELECTOR;
  context.rsp = (u64)stack_bottom;
  context.rip = (u64)start;
}

Thread *get_current_thread() {
  auto *threads = (Thread*)threads_space;
  return &threads[current_thread_id];
}
ThreadContext *get_current_thread_context() {
  return &get_current_thread()->context;
}
