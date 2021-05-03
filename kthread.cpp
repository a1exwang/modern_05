#include <kthread.h>
#include <kernel.h>
#include <lib/string.h>
#include <irq.h>
#include "debug.h"

u64 current_thread_id = 0;
u64 total_threads = 2;
Thread *main_thread;
Thread *thread2;
u8 threads_space[256 * sizeof(Thread)];

void thread_start() {
  auto *threads = (Thread*)threads_space;
  new(&threads[1]) Thread(1, 0x38, 0x30, main_thread_start);
  new(&threads[2]) Thread(2, 0x38, 0x30, thread2_start);

  current_thread_id = 1;
  threads[1].schedule();
}
void schedule_next_thread() {
  if (current_thread_id == total_threads) {
    current_thread_id = 1;
  } else {
    current_thread_id++;
  }

  Thread *t = get_thread(current_thread_id);
  t->schedule();
}
void store_current_thread_context(ThreadContext *context) {
  Thread *t = get_thread(current_thread_id);
  t->store_context(context);
}

void main_thread_start() {
  u64 i = 0;
  while (true) {
    u64 reg_new;
    register u64 reg asm("rax");
    reg = (u64)0x2223;
    __asm__ __volatile__("int $42");
    __asm__ __volatile__("movq %%rax, %0" :"=r"(reg_new));
    Kernel::sp() << SerialPort::IntRadix::Hex << "thread1 run " << i << " " << reg_new << "\n";
    i++;
  }
}
void thread2_start() {
  u64 i = 0;
  while (true) {
    u64 reg_new;
    register u64 reg asm("rax");
    reg = 0x3332;
    __asm__ __volatile__("int $42");
    __asm__ __volatile__("mov %%rax, %0" :"=r"(reg_new));
    Kernel::sp() << SerialPort::IntRadix::Hex << "thread2 run " << i << " " << reg << "\n";
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
void Thread::schedule() {
  Kernel::sp() << "schedule() " << id << " " << context.rax << "\n";
  ::_schedule(&context);
}
Thread::Thread(unsigned long id, unsigned short code_selector, unsigned short data_selector, ThreadFunction start) :id(id), start(start) {
  memset(&context, 0, sizeof(context));
  context.cs = code_selector;
  context.ss = data_selector;
  context.rsp = (u64)stack_bottom;
  context.rip = (u64)start;
}
