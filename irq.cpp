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
extern "C" void _40h_irq_handler();
extern "C" void _41h_irq_handler();
extern "C" void _42h_irq_handler();
extern "C" void _43h_irq_handler();
extern "C" void _44h_irq_handler();
extern "C" void _45h_irq_handler();
extern "C" void _46h_irq_handler();
extern "C" void _47h_irq_handler();
extern "C" void _48h_irq_handler();
extern "C" void _49h_irq_handler();
extern "C" void _4ah_irq_handler();
extern "C" void _4bh_irq_handler();
extern "C" void _4ch_irq_handler();
extern "C" void _4dh_irq_handler();
extern "C" void _4eh_irq_handler();
extern "C" void _4fh_irq_handler();
extern "C" void _50h_irq_handler();
extern "C" void _51h_irq_handler();
extern "C" void _52h_irq_handler();
extern "C" void _53h_irq_handler();
extern "C" void _54h_irq_handler();
extern "C" void _55h_irq_handler();
extern "C" void _56h_irq_handler();
extern "C" void _57h_irq_handler();

InterruptDescriptor main_idt[256];

extern "C" void timer_irq_handler();

u8 interrupt_stack[INTERRUPT_STACK_SIZE];

void handle_syscall();
void handle_pci_irqs();

extern "C" void irq_handler(u64 irq_num, u64 error_code) {
  auto context = current_context();
  if (irq_num == IRQ_TIMER) {
    timer_irq_handler();
//    Kernel::k->serial_port_ << "timer IRQ\n";
  } else if (irq_num == IRQ_SYSCALL) {
//    Kernel::k->serial_port_
//        << "syscall: "
//        << "number = 0x" << SerialPort::IntRadix::Hex << context->rax << " error = 0x" << error_code << " thread = 0x" << current_pid << "\n"
//        << "rip = 0x" << context->rip << " rsp = 0x" << context->rsp << "\n";
    handle_syscall();
  } else if (0x40 <= irq_num && irq_num < 0x58) {
    // PCI IRQs
//    Kernel::k->serial_port_ << "PCI IRQ = 0x" << SerialPort::IntRadix::Hex << irq_num << "\n";
    // TODO: handle other IRQs for IO APIC
    handle_pci_irqs();
  } else {
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
  set_idt_offset(&main_idt[0x40], (void*)&_syscall_irq_handler, true);

  set_idt_offset(&main_idt[0x40], (void*)&_40h_irq_handler);
  set_idt_offset(&main_idt[0x41], (void*)&_41h_irq_handler);
  set_idt_offset(&main_idt[0x42], (void*)&_42h_irq_handler);
  set_idt_offset(&main_idt[0x43], (void*)&_43h_irq_handler);
  set_idt_offset(&main_idt[0x44], (void*)&_44h_irq_handler);
  set_idt_offset(&main_idt[0x45], (void*)&_45h_irq_handler);
  set_idt_offset(&main_idt[0x46], (void*)&_46h_irq_handler);
  set_idt_offset(&main_idt[0x47], (void*)&_47h_irq_handler);
  set_idt_offset(&main_idt[0x48], (void*)&_48h_irq_handler);
  set_idt_offset(&main_idt[0x49], (void*)&_49h_irq_handler);
  set_idt_offset(&main_idt[0x4a], (void*)&_4ah_irq_handler);
  set_idt_offset(&main_idt[0x4b], (void*)&_4bh_irq_handler);
  set_idt_offset(&main_idt[0x4c], (void*)&_4ch_irq_handler);
  set_idt_offset(&main_idt[0x4d], (void*)&_4dh_irq_handler);
  set_idt_offset(&main_idt[0x4e], (void*)&_4eh_irq_handler);
  set_idt_offset(&main_idt[0x4f], (void*)&_4fh_irq_handler);
  set_idt_offset(&main_idt[0x50], (void*)&_50h_irq_handler);
  set_idt_offset(&main_idt[0x50], (void*)&_51h_irq_handler);
  set_idt_offset(&main_idt[0x51], (void*)&_52h_irq_handler);
  set_idt_offset(&main_idt[0x52], (void*)&_53h_irq_handler);
  set_idt_offset(&main_idt[0x53], (void*)&_54h_irq_handler);
  set_idt_offset(&main_idt[0x54], (void*)&_55h_irq_handler);
  set_idt_offset(&main_idt[0x56], (void*)&_56h_irq_handler);
  set_idt_offset(&main_idt[0x57], (void*)&_57h_irq_handler);
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

