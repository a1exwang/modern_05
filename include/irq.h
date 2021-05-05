#pragma once

#define IRQ_SYSCALL 42

void irq_init();
extern "C" void return_from_syscall(struct ThreadContext*);
