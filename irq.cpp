#include <lib/defs.h>
#include <cpu_defs.h>
#include <lib/string.h>
#include <kernel.h>
#include "debug.h"

extern "C" void _default_irq_handler();

extern "C" void _de_irq_handler();
extern "C" void _db_irq_handler();
extern "C" void _nmi_irq_handler();
extern "C" void _bp_irq_handler();

extern "C" void _timer_irq_handler();
extern "C" void _schedule(void *);
void schedule_next_thread();

InterruptDescriptor main_idt[256];
u64 current_thread_id = 0;

extern "C" void timer_irq_handler() {
  Kernel::k->serial_port_ << "Timer IRQ " << "\n";
}

struct ThreadContext;
void store_current_thread_context(ThreadContext *context);
extern "C" void irq_handler(u64 irq_num, u64 error_code, ThreadContext *interrupt_stack_top) {
  Kernel::k->serial_port_ << "IRQ = " << irq_num << " error = " << error_code << " thread = " << current_thread_id << "\n";
  if (irq_num == 32) {
    timer_irq_handler();
    u64 esp;
    register u64 esp_val asm("esp");
    Kernel::k->serial_port_ << "esp = " << esp_val << "\n";
  } else if (irq_num == 42) {
    store_current_thread_context(interrupt_stack_top);
    schedule_next_thread();
  } else {
    Kernel::k->panic("unhandled exception");
  }
}

void set_idt_offset(InterruptDescriptor *desc, void* ptr) {
  desc->offset_lo16 = (u64)ptr & 0xffff;
  desc->offset_mid16 = ((u64)(ptr) >> 16) & 0xffff;
  desc->offset_hi32 = ((u64)(ptr) >> 32) & 0xffffffff;
}

void setup_idt(u16 selector, void *default_handler) {
  memset(main_idt, 0, sizeof(main_idt));

  for (int i = 0; i < sizeof(main_idt) / sizeof(main_idt[0]); i++) {
    set_idt_offset(&main_idt[i], default_handler);

    main_idt[i].selector = selector;
    main_idt[i].type = DescriptorType::LongInterruptGate;
    main_idt[i].dpl = 0;
    main_idt[i].p = 1;
  }
  set_idt_offset(&main_idt[0], (void*)&_de_irq_handler);
  set_idt_offset(&main_idt[1], (void*)&_db_irq_handler);
  set_idt_offset(&main_idt[2], (void*)&_nmi_irq_handler);
  set_idt_offset(&main_idt[3], (void*)&_bp_irq_handler);
  set_idt_offset(&main_idt[32], (void*)&_timer_irq_handler);
}

void print_idt_descriptor(SerialPort &serial_port, u16 index, const InterruptDescriptor *desc) {
  serial_port << "Interrupt descriptor 0x" << SerialPort::IntRadix::Hex << index << " ";
  u64 offset = desc->offset_lo16 | ((u64)desc->offset_mid16 << 16) | ((u64)desc->offset_hi32 << 32);
  serial_port << "present " << desc->p << SerialPort::IntRadix::Hex << " offset 0x" << offset << " selector 0x" << desc->selector << " type 0x" << desc->type << " dpl " << desc->dpl << "\n";
}

void irq_init() {
  u16 cs = 0;
  asm volatile("mov %%cs ,%0" : "=r" (cs));
  setup_idt(cs, (void*)&_default_irq_handler);

  load_idt(std::make_tuple<void*, u16>(main_idt, 0xfff));
//  __asm__ __volatile__("int $32");

  void* apic_base = (void*)cpuGetMSR(IA32_APIC_BASE_MSR);
  Kernel::sp() << "APIC base: 0x" << SerialPort::IntRadix::Hex << (u64)apic_base << '\n';
}

#pragma pack(push, 1)
struct ThreadContext {
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

using ThreadFunction = void (*)();
u64 total_threads = 2;


class Thread {
 public:
  Thread(u64 id, u16 code_selector, u16 data_selector, ThreadFunction start) :id(id), start(start) {
    memset(&context, 0, sizeof(context));
    context.cs = code_selector;
    context.ss = data_selector;
    context.rsp = (u64)stack_bottom;
    context.rip = (u64)start;
  }

  void store_context(ThreadContext *old_context) {
    memcpy(&this->context, old_context, sizeof(ThreadContext));
  }
 public:
  u64 id;
  ThreadFunction start;
  ThreadContext context;
  u8 stack[1024 * 1024];
  u8 stack_bottom[0];

  void schedule() {
    ::_schedule(&context);
  }
};

Thread *main_thread;
Thread *thread2;

u8 threads_space[256 * sizeof(Thread)];

Thread *get_thread(u64 id) {
  return &((Thread*)threads_space)[id];
}

void main_thread_start() {
  u64 i = 0;
  while (true) {
    Kernel::sp() << "main thread run" << "\n";
    dump_all();
    __asm__ __volatile__("int $42");
    dump_all();
    i++;
  }
}

void thread2_start() {
  u64 i = 0;
  while (true) {
    Kernel::sp() << "thread2 run" << "\n";
//    dump_all();

    __asm__ __volatile__("int $42");
//    dump_all();
    i++;
  }
}


void thread_start() {
  auto *threads = (Thread*)threads_space;
  new(&threads[1]) Thread(1, 0x38, 0x30, main_thread_start);
  new(&threads[2]) Thread(2, 0x38, 0x30, thread2_start);

  current_thread_id = 2;
  threads[2].schedule();
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
