#pragma once
#include <functional>

#include <cpu_defs.h>
#include <common/small_vec.hpp>
#include <kernel.h>

#define IRQ_GENERAL_PROTECTION 13
#define IRQ_INVALID_OPCODE 6
#define IRQ_PAGE_FAULT 14
#define IRQ_TIMER 32
#define IRQ_SPURIOUS 37
#define IRQ_SYSCALL 42

#define INTERRUPT_STACK_SIZE (16UL * PAGE_SIZE)

constexpr size_t MaxHandlersPerInterrupt = 16;
constexpr size_t MaxInterrupts = 256;

extern "C" void return_from_syscall(struct Context*);
extern u8 interrupt_stack[];
extern u8 *interrupt_stack_bottom;

struct IrqHandlerInfo {
  u64 irq_num;
  u64 error_code;
  Context *context;
  u64 pid;
};

using InterruptHandler = std::function<void (IrqHandlerInfo *info)>;

class Interrupt {
 public:
  SmallVec<InterruptHandler, MaxHandlersPerInterrupt> handlers_;
};

extern "C" void irq_handler(u64 irq_num, u64 error_code);

class InterruptProcessor {
 public:
  explicit InterruptProcessor(Kernel *kernel);
  void Register(size_t id, InterruptHandler handler);
  void Register(size_t start_id, size_t count, InterruptHandler handler);

 private:
  void HandleInterrupt(u64 irq_num, u64 error_code);

  void setup_idt(u16 selector, void *default_handler);

  friend void irq_handler(u64, u64);
 private:
  Kernel *k;
  Interrupt interrupts_[MaxInterrupts];
  InterruptDescriptor main_idt_[MaxInterrupts];
};

InterruptProcessor *irq_init();
