#include <cpu_defs.h>
#include <kernel.h>
#include <lib/string.h>
#include <lib/utils.h>
#include <mm/mm.h>
#include <mm/page_alloc.h>
#include <process.h>
#include <syscall.h>
#include <irq.h>
#include <elf.h>

u64 current_pid = 0;

constexpr u64 MAX_PROCESS = 1024;
constexpr u64 THREAD_KERNEL_STACK_SIZE = 16*PAGE_SIZE;
constexpr u64 PROCESS_MAX_USER_PAGES = 512*512;

constexpr u64 USER_STACK_SIZE = 1024*1024;
constexpr u64 USER_IMAGE_SIZE = 1024*1024;

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

class Process {
 public:

  explicit Process(u64 id, u64 start_phy) :id(id), start_phy(start_phy) {
    auto pts_log2size = log2(sizeof(PageTabletSructures)) + 1;
    pts_paddr = physical_page_alloc(pts_log2size);
    pts = (PageTabletSructures*)(pts_paddr + KERNEL_START);
    memset(pts, 0, sizeof(PageTabletSructures));

    // reuse kernel space map
    pts->pml4t[256].p = 1;
    pts->pml4t[256].rw = 1;
    pts->pml4t[256].base_addr = get_kernel_pdpt_phy_addr() >> 12;

    // create a new user space map
    pts->pml4t[0].p = 1;
    pts->pml4t[0].rw = 1;
    pts->pml4t[0].us = 1;
    pts->pml4t[0].base_addr = (pts_paddr + ((u64)&pts->pdpt[0] - (u64)pts)) >> 12;

    // only the first pdpt is used
    pts->pdpt[0].p = 1;
    pts->pdpt[0].rw = 1;
    pts->pdpt[0].us = 1; // allow userspace
    pts->pdpt[0].base_addr = (pts_paddr + ((u64)&pts->pdt[0] - (u64)pts)) >> 12;

    for (u64 i = 0; i < sizeof(pts->pdt) / sizeof(pts->pdt[0]); i++) {
      pts->pdt[i].p = 1;
      pts->pdt[i].rw = 1;
      pts->pdt[i].us = 1;
      pts->pdt[i].base_addr = (pts_paddr + ((u64)&pts->pt[i * PAGES_PER_TABLE] - (u64)pts)) >> 12;
    }

    // 1MiB ~ 2MiB are for exec image
    user_image = (u8*)(1UL*1024UL*1024UL);
    user_image_phy_addr = physical_page_alloc(log2(USER_IMAGE_SIZE));
    map_user_addr((u64)user_image, user_image_phy_addr, user_image_size / PAGE_SIZE);

    // 32MiB ~ 33MiB are for user stack
    user_stack = (u8*)(33UL*1024UL*1024UL);
    user_stack_phy_addr = physical_page_alloc(log2(USER_STACK_SIZE));
    map_user_addr((u64)user_stack, user_stack_phy_addr, user_stack_size / PAGE_SIZE);

    context.cr3 = pts_paddr;
    context.cs = KERNEL_CODE_SELECTOR;
    context.ds = KERNEL_DATA_SELECTOR;
    context.ss = KERNEL_DATA_SELECTOR;
    context.es = KERNEL_DATA_SELECTOR;
    context.fs = KERNEL_DATA_SELECTOR;
    context.gs = KERNEL_DATA_SELECTOR;
    context.rsp = (u64)kernel_stack_bottom;
    context.rip = (u64)&process_entrypoint;
    // first parameter for process_entry_point(p)
    context.rdi = (u64)this;

    print();
  }

  void print() {
    Kernel::sp() << SerialPort::IntRadix::Hex << "user image " << user_image_phy_addr << " stack 0x" << user_stack_phy_addr << "\n";
  }

  void map_user_addr(u64 vaddr, u64 paddr, u64 n_pages) {
    Kernel::sp() << SerialPort::IntRadix::Hex << "mapping 0x" << vaddr << " -> " << paddr << " " << n_pages << " pages\n";
    for (u64 i = 0; i < n_pages; i++) {
      auto vpage = (vaddr >> 12) + i;
      auto ppage = (paddr >> 12) + i;
      pts->pt[vpage].p = 1;
      pts->pt[vpage].rw = 1;
      pts->pt[vpage].us = 1;
      pts->pt[vpage].base_addr = ppage;
    }
  }

  void *load_elf_from_buffer(char *buffer, u64 size) {
    Elf64_Ehdr *ehdr = (Elf64_Ehdr*)buffer;
    const char *ElfMagic = "\x7f" "ELF";
    for (int i = 0; i < 4; i++) {
      if (ElfMagic[i] != buffer[i]) {
        Kernel::k->panic("Corrupted ELF file, invalid header magic");
      }
    }

    assert(ehdr->e_phoff, "program header table not found\n");
    Kernel::sp() << "elf has " << SerialPort::IntRadix::Dec << ehdr->e_phnum << " sections at 0x" << SerialPort::IntRadix::Hex << ehdr->e_phoff << "\n";
    Kernel::sp() << "  p_offset p_filesz p_vaddr p_memsz\n";
    for (int i = 0; i < ehdr->e_phnum; i++) {
      auto phdr = (Elf64_Phdr*)(buffer + ehdr->e_phoff + ehdr->e_phentsize * i);
      if (phdr->p_type == PT_LOAD && phdr->p_memsz > 0) {
        Kernel::sp() << SerialPort::IntRadix::Hex << "  0x" << phdr->p_offset << " 0x" << phdr->p_filesz << " 0x" << phdr->p_vaddr << " 0x" << phdr->p_memsz << "\n";
        assert(phdr->p_vaddr >= 0x100000, "Segment is at wrong location");
        assert(phdr->p_vaddr+phdr->p_memsz <= 0x200000, "Segment too big");
        if (phdr->p_filesz <= phdr->p_memsz) {
          memcpy((void*)phdr->p_vaddr, buffer + phdr->p_offset, phdr->p_filesz);
        } else {
          Kernel::k->panic("\nDon't know how to load sections that has filesize > memsize");
        }
      }
    }
    Kernel::sp() << "entrypoint at " << SerialPort::IntRadix::Hex << ehdr->e_entry << "\n";
    return (void*)ehdr->e_entry;
  }

  static void process_entrypoint(Process *p) {
    p->kernel_entrypoint();
  }

  u64 phy_addr_in_this(void *ptr) {
    return start_phy + ((u64)ptr - (u64)this);
  }

  void kernel_entrypoint() {
    Kernel::sp() << "starting process pid = "<< current_pid << " '" << (const char*)name << "'\n";
    if (tmp_start) {
      Kernel::sp() << "has tmp_start, going for it" << "\n";
      tmp_start();
    }
    halt();
  }

  // id = 0 for empty process slot
  u64 id;
  ProcessState state = ProcessState::Wait;
  char name[32];
  // phy addr of the start of this Process object
  u64 start_phy = 0;

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

  void (*tmp_start)() = 0;
};

u64 next_pid = 1;
Process *processes[MAX_PROCESS];

void create_process() {
  auto pid = next_pid++;
  // TODO: optimize with low memory consumption
  auto log2size = log2(sizeof(Process)) + 1;
  if (log2size < Log2MinSize) {
    log2size = Log2MinSize;
  }
  auto process_mem = kernel_page_alloc(log2size);
  auto phy_start = ((u64)process_mem - KERNEL_START);
  auto process = new(process_mem) Process(pid, phy_start);
  assert(processes[pid] == nullptr, "Process already created");
  processes[pid] = process;
}

void thread2_start() {
  u64 i = 0;
  while (true) {
    u64 reg_new;
    register u64 reg asm("rax");
    reg = 0x3332;

    asm volatile(
    "movq $0x2, %%rax\t\n"
    "int $42"
    :
    :
    :"%rax"
    );
    __asm__ __volatile__("mov %%rax, %0" :"=r"(reg_new));
//    Kernel::sp() << SerialPort::IntRadix::Hex << "thread2 run " << i << " " << reg_new << "\n";
    i++;
  }
}

extern "C" char _binary_user_init_start[];
extern "C" char _binary_user_init_end[];

Process *current() {
  return processes[current_pid];
}

void exec_main() {
  u64 size = _binary_user_init_end - _binary_user_init_start;
  char *start = _binary_user_init_start;
  Kernel::sp() << "init elf size = " << SerialPort::IntRadix::Hex << size << ", start bytes 0x" << (u64)start[0] << " 0x" << (u64)start[1] << "\n";

  auto proc = current();
  auto start_addr = proc->load_elf_from_buffer(start, size);

  Kernel::sp() << "ELF file loaded\n";

  // goto user space
  {
    auto &c = proc->context;

    c.cs = USER_CODE_SELECTOR;
    c.ds = USER_DATA_SELECTOR;
    c.ss = USER_DATA_SELECTOR;
    c.es = USER_DATA_SELECTOR;
    // TODO: user real selector
    c.fs = 0;
    c.gs = 0;
    c.rsp = (u64)proc->user_stack + proc->user_stack_size;
    c.rip = (u64)start_addr;

    return_from_syscall(&c);
  }
}

void main_start() {
  Kernel::sp() << "main process started\n";

  Kernel::sp() << "main process creating process 2\n";
  create_process();
  auto new_proc = processes[next_pid-1];
  new_proc->tmp_start = thread2_start;
  new_proc->name[0] = '2';

  Kernel::sp() << "main process loading\n";

  exec_main();

  Kernel::k->panic("Should not reach here\n");
//  u64 i = 0;
//  while (true) {
//    u64 reg_new;
//    register u64 reg asm("rax");
//    reg = (u64)0x2223;
//    do_syscall();
//    __asm__ __volatile__("movq %%rax, %0" :"=r"(reg_new));
////    Kernel::sp() << SerialPort::IntRadix::Hex << "main process run " << i << " " << reg_new << "\n";
//    i++;
//  }
}

void process_init() {
  memset(processes, 0, sizeof(processes));
  next_pid = 1;
  current_pid = 1;
  create_process();

  auto main_process = processes[1];
  main_process->tmp_start = main_start;
}

void schedule() {
  processes[current_pid]->state = ProcessState::Wait;

  while (true) {
    if (current_pid == next_pid) {
      current_pid = 1;
    } else {
      current_pid++;
    }

    auto process = processes[current_pid];
    if (process && process->id && process->state == ProcessState::Wait) {
      process->state = ProcessState::Running;
      break;
    }
  }
}

void store_current_thread_context(Context *context) {
  memcpy(
      &processes[current_pid]->context,
      context,
      sizeof(Context)
  );
}
Context *current_context() {
  return &processes[current_pid]->context;
}

using SyscallFunc = void (*)();

void _sys_exit() {
  Kernel::sp() << "process " << SerialPort::IntRadix::Dec << current_pid << " successfully exit, not implemented so halt\n";
  halt();
}

void _sys_yield() {

}

void _sys_write() {
  const char *p = (const char*)current()->context.rdi;
  Kernel::sp() << "write() from pid " << current_pid << " content '" << p << "'\n";
}

extern "C" {

// NOTE: keep these two tables in sync
enum {
  SYSCALL_NR_EXIT = 1,
  SYSCALL_NR_YIELD,
  SYSCALL_NR_WRITE,
  SYSCALL_NR_MAX
};

// NOTE: keep these two tables in sync
SyscallFunc syscall_table[] = {
    nullptr,
    &_sys_exit,
    &_sys_yield,
    &_sys_write,
};

}

// Copied from Linux source code
//  * Registers on entry:
// * rax  system call number
// * rcx  return address
// * r11  saved rflags (note: r11 is callee-clobbered register in C ABI)
// * rdi  arg0
// * rsi  arg1
// * rdx  arg2
// * r10  arg3 (needs to be moved to rcx to conform to C ABI)
// * r8   arg4
// * r9   arg5
void handle_syscall() {
  auto &c = current()->context;
  auto syscall_num = c.rax;
  if (syscall_num == 0) {
    Kernel::k->panic("Invalid syscall 0");
  } else if (syscall_num >= SYSCALL_NR_MAX) {
    Kernel::k->panic("Invalid syscall >= max");
  }

  syscall_table[syscall_num]();
}