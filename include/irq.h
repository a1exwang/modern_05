#pragma once
#include <cpu_defs.h>

#define IRQ_SYSCALL 42
#define IRQ_GENERAL_PROTECTION 13
#define IRQ_PAGE_FAULT 14
#define INTERRUPT_STACK_SIZE (16UL * PAGE_SIZE)

void irq_init();
extern "C" void return_from_syscall(struct Context*);

extern u8 interrupt_stack[];
