#include <kthread.h>
#include <kernel.h>
#include <lib/string.h>
#include <irq.h>
#include <syscall.h>
#include "debug.h"
#include <mm/mm.h>

u64 current_thread_id = 0;
u64 total_threads = 2;
Thread *main_thread;
Thread *thread2;
extern "C" {
u8 threads_space[32 * sizeof(Thread)];
}

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
