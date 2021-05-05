#pragma once
#include <lib/defs.h>

#pragma pack(push, 1)
struct ThreadContext {
  u64 ds;
  u64 es;
  u64 fs;
  u64 gs;

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

void dump_thread_context(const ThreadContext &context);

using ThreadFunction = void (*)();


class Thread {
 public:
  Thread(u64 id, ThreadFunction start);

  void store_context(ThreadContext *old_context);
 public:
  u64 id;
  ThreadFunction start;
  ThreadContext context;
  u8 stack[4 * 1024];
  u8 stack_bottom[0];
};

Thread *get_thread(u64 id);

void main_thread_start();
void thread2_start();

void thread_start();

extern "C" {
  void schedule();
  void store_current_thread_context(ThreadContext *context);
  Thread *get_current_thread();
  ThreadContext *get_current_thread_context();
}

extern u64 current_thread_id;
extern u64 total_threads;
extern Thread *main_thread;
extern Thread *thread2;
