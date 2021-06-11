#include <syscall.h>
#include <kernel.h>
#include <irq.hpp>
#include <process.h>
#include <kernel-abi/syscall_nr.h>
#include <mm/mm.h>

void handle_syscall(Process *p, Context *c);

void Syscall::SetupSyscall(Kernel *kernel) {
  kernel->irq_->Register(IRQ_SYSCALL, [](IrqHandlerInfo *info) {
    auto p = processes[info->pid];
    handle_syscall(p, info->context);
  });
}


using SyscallFunc = int (Syscall::*)();

void _sys_exit() {
}

extern "C" {

// NOTE: keep these two tables in sync
SyscallFunc syscall_table[] = {
    [SYSCALL_NR_NULL] = nullptr,
    [SYSCALL_NR_READ] = &Syscall::sys_read,
    [SYSCALL_NR_WRITE] = &Syscall::sys_write,
    [SYSCALL_NR_OPEN] = &Syscall::sys_open,
    [SYSCALL_NR_CLOSE] = &Syscall::sys_close,
    [SYSCALL_NR_EXIT] = &Syscall::sys_exit,
    [SYSCALL_NR_YIELD] = &Syscall::sys_yield,
    [SYSCALL_NR_ANON_ALLOCATE] = &Syscall::sys_anon_allocate,
};

}

// * rax  system call number
// * rdi  arg0
// * rsi  arg1
// * rdx  arg2
// * r10  arg3
// * r8   arg4
// * r9   arg5
void handle_syscall(Process *p, Context *c) {
  auto syscall_num = c->rax;
  if (syscall_num == SYSCALL_NR_NULL) {
    Kernel::k->panic("Invalid null syscall");
  } else if (syscall_num >= SYSCALL_NR_MAX) {
    Kernel::k->panic("Invalid syscall >= max");
  }

  auto syscall_object = p->syscall_.get();
  auto syscall_func_ptr = syscall_table[syscall_num];
  c->rax = (syscall_object->*syscall_func_ptr)();
}

int Syscall::sys_read() {
  return -1;
}
int Syscall::sys_write() {
  return -1;
}
int Syscall::sys_open() {
  return -1;
}
int Syscall::sys_close() {
  return -1;
}
int Syscall::sys_exit() {
  Kernel::sp() << "process " << SerialPort::IntRadix::Dec << current_pid << " successfully exit, not implemented so halt\n";
  halt();
  return -1;
}
int Syscall::sys_yield() {
  return 0;
}

int Syscall::sys_anon_allocate() {
  auto &c = process_->context;
  auto size = (size_t)c.rdi;
  auto ptr = (void**)c.rsi;
  auto &ret = c.rax;
  Kernel::sp() << "process " << SerialPort::IntRadix::Dec << current_pid << " anon_allocate(0x" << IntRadix::Hex << size << ", 0x" << (u64)ptr << ")\n";

  auto aligned_size = (size + (PAGE_SIZE-1)) / PAGE_SIZE * PAGE_SIZE;

  auto buf = kmalloc(aligned_size);
  auto buf_phy = kernel2phy((u64)buf);
  auto brk_start = process_->brk_start;
  process_->brk_start += aligned_size;
  if (process_->brk_start > process_->brk_end) {
    Kernel::sp() << "user brk overflow\n";
    return -1;
  }
  process_->map_user_addr(brk_start, buf_phy, aligned_size / PAGE_SIZE);

  *ptr = (void*)brk_start;

  return 0;
}