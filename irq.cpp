#include <lib/defs.h>
#include <cpu_defs.h>
#include <lib/string.h>
#include <kernel.h>
#include "debug.h"
#include <process.h>
#include <irq.h>

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
extern "C" void _pf_irq_handler();

extern "C" void _timer_irq_handler();
extern "C" void _spurious_irq_handler();
extern "C" void _syscall_irq_handler();

InterruptDescriptor main_idt[256];

extern "C" void timer_irq_handler() {
  Kernel::k->serial_port_ << "Timer IRQ " << "\n";
}

u8 interrupt_stack[INTERRUPT_STACK_SIZE];

void handle_syscall();

extern "C" void irq_handler(u64 irq_num, u64 error_code) {
  if (irq_num == IRQ_TIMER) {
    timer_irq_handler();
  } else if (irq_num == IRQ_SYSCALL) {
    handle_syscall();
  } else {
    auto context = current_context();
    Kernel::k->serial_port_
        << "unhandled exception: "
        << "IRQ = 0x" << SerialPort::IntRadix::Hex << irq_num << " error = 0x" << error_code << " thread = 0x" << current_pid << "\n"
        << "rip = 0x" << context->rip << " rsp = 0x" << context->rsp << "\n";
    if (irq_num == IRQ_PAGE_FAULT) {
      u64 cr2;
      asm volatile("mov %%cr2, %0" :"=r"(cr2));
      Kernel::k->serial_port_
          << "page fault cr2 = " << SerialPort::IntRadix::Hex << cr2 << "\n";
    }

    halt();
  }
}

void set_idt_offset(InterruptDescriptor *desc, void* ptr, bool user_mode = false) {
  desc->offset_lo16 = (u64)ptr & 0xffff;
  desc->offset_mid16 = ((u64)(ptr) >> 16) & 0xffff;
  desc->offset_hi32 = ((u64)(ptr) >> 32) & 0xffffffff;
  desc->dpl = user_mode ? 3 : 0;
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
  set_idt_offset(&main_idt[3], (void*)&_bp_irq_handler, true);
  set_idt_offset(&main_idt[4], (void*)&_of_irq_handler);
  set_idt_offset(&main_idt[5], (void*)&_br_irq_handler);
  set_idt_offset(&main_idt[6], (void*)&_ud_irq_handler);
  set_idt_offset(&main_idt[7], (void*)&_nm_irq_handler);
  set_idt_offset(&main_idt[8], (void*)&_df_irq_handler);
  set_idt_offset(&main_idt[10], (void*)&_ts_irq_handler);
  set_idt_offset(&main_idt[11], (void*)&_np_irq_handler);
  set_idt_offset(&main_idt[12], (void*)&_ss_irq_handler);
  set_idt_offset(&main_idt[13], (void*)&_gp_irq_handler);
  set_idt_offset(&main_idt[14], (void*)&_pf_irq_handler);
  set_idt_offset(&main_idt[32], (void*)&_timer_irq_handler);
  set_idt_offset(&main_idt[37], (void*)&_spurious_irq_handler);
  set_idt_offset(&main_idt[42], (void*)&_syscall_irq_handler, true);
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

}

