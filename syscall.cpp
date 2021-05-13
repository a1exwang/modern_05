#include <syscall.h>
#include <kernel.h>
#include <irq.hpp>

void handle_syscall();

Syscall::Syscall() {
  Kernel::k->irq_->Register(IRQ_SYSCALL, [](u64, u64, Context*) {
    handle_syscall();
  });
}