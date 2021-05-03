#include <lib/defs.h>
#include <cpu_defs.h>
#include <lib/string.h>
#include <kernel.h>
#include "debug.h"
#include <kthread.h>

extern "C" void _default_irq_handler();

extern "C" void _de_irq_handler();
extern "C" void _db_irq_handler();
extern "C" void _nmi_irq_handler();
extern "C" void _bp_irq_handler();

extern "C" void _of_irq_handler();
extern "C" void _br_irq_handler();
extern "C" void _ud_irq_handler();
extern "C" void _nm_irq_handler();

extern "C" void _df_irq_handler();
extern "C" void _ts_irq_handler();
extern "C" void _np_irq_handler();

extern "C" void _ss_irq_handler();
extern "C" void _gp_irq_handler();

extern "C" void _timer_irq_handler();
extern "C" void _syscall_irq_handler();

InterruptDescriptor main_idt[256];

extern "C" void timer_irq_handler() {
  Kernel::k->serial_port_ << "Timer IRQ " << "\n";
}

u8 interrupt_stack[4096];
extern "C" constexpr u8 *_interrupt_stack_bottom = &interrupt_stack[sizeof(interrupt_stack)];

extern "C" void irq_handler(u64 irq_num, u64 error_code, ThreadContext *context) {
  if (irq_num == 32) {
    timer_irq_handler();
    u64 esp;
    register u64 esp_val asm("esp");
    Kernel::k->serial_port_ << "esp = " << esp_val << "\n";
  } else if (irq_num == 42) {
    store_current_thread_context(context);
    schedule_next_thread();
  } else {
    Kernel::k->serial_port_
        << "IRQ = " << irq_num << " error = " << error_code << " thread = " << current_thread_id << "\n"
        << "rip = " << context->rip << " rsp = " << context->rsp << "\n";

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
  set_idt_offset(&main_idt[4], (void*)&_of_irq_handler);
  set_idt_offset(&main_idt[5], (void*)&_br_irq_handler);
  set_idt_offset(&main_idt[6], (void*)&_ud_irq_handler);
  set_idt_offset(&main_idt[7], (void*)&_nm_irq_handler);
  set_idt_offset(&main_idt[8], (void*)&_df_irq_handler);
  set_idt_offset(&main_idt[10], (void*)&_ts_irq_handler);
  set_idt_offset(&main_idt[11], (void*)&_np_irq_handler);
  set_idt_offset(&main_idt[12], (void*)&_ss_irq_handler);
  set_idt_offset(&main_idt[13], (void*)&_gp_irq_handler);
  set_idt_offset(&main_idt[32], (void*)&_timer_irq_handler);
  set_idt_offset(&main_idt[42], (void*)&_syscall_irq_handler);
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
