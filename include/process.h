#pragma once
#include <lib/defs.h>

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