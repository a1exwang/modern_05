#include <common/defs.h>
#include <cpu_defs.h>
#include <lib/string.h>
#include <kernel.h>
#include "debug.h"
#include <process.h>
#include <irq.hpp>
#include <atomic>
#include <common/small_vec.hpp>
#include <mm/page_alloc.h>
#include <functional>
#include <common/unwind.hpp>
#include <mm/mm.h>

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
u8 *interrupt_stack_bottom = interrupt_stack + INTERRUPT_STACK_SIZE - 128;

void handle_syscall();

void set_idt_offset(InterruptDescriptor *desc, void* ptr, bool user_mode = false) {
  desc->offset_lo16 = (u64)ptr & 0xffff;
  desc->offset_mid16 = ((u64)(ptr) >> 16) & 0xffff;
  desc->offset_hi32 = ((u64)(ptr) >> 32) & 0xffffffff;
  desc->dpl = user_mode ? 3 : 0;
}

InterruptProcessor::InterruptProcessor(Kernel *kernel) :k(kernel) { // NOLINT(cppcoreguidelines-pro-type-member-init)
  auto cs = get_cs();
  setup_idt(cs, (void*)&_default_irq_handler);
  load_idt(std::make_tuple<void*, u16>(main_idt_, 0xfff));
}

void InterruptProcessor::HandleInterrupt(unsigned long irq_num, unsigned long error_code) {
  auto context = current_context();
  if (interrupts_[irq_num].handlers_.empty()) {
    Kernel::k->serial_port_
        << "unhandled interrupt: "
        << "IRQ = 0x" << SerialPort::IntRadix::Hex << irq_num << " error = 0x" << error_code << " pid = 0x" << current_pid << "(" << processes[current_pid]->name.c_str() << ")" << "\n"
        << "rip = 0x" << context->rip << " rsp = 0x" << context->rsp << "\n";
    if (irq_num == IRQ_PAGE_FAULT) {
      Kernel::k->serial_port_
          << "page fault cr2 = " << SerialPort::IntRadix::Hex << get_cr2() << "\n";
    } else if (irq_num == IRQ_INVALID_OPCODE) {
      Kernel::k->serial_port_
          << "Invalid Opcode\n";
    }

    if (is_kernel(context->rbp)) {
      Kernel::k->stack_dump(context->rbp);
    } else {
      Kernel::sp() << "exception happens in userspace programs\n";
    }

    halt();
  }

  for (auto &handler : interrupts_[irq_num].handlers_) {
    IrqHandlerInfo info = {
        .irq_num = irq_num,
        .error_code = error_code,
        .context = context,
        .pid = current_pid,
    };

    handler(&info);
  }
}
void InterruptProcessor::setup_idt(unsigned short selector, void *default_handler) {
  memset(main_idt_, 0, sizeof(main_idt_));
  for (auto &idt_entry : main_idt_) {
    set_idt_offset(&idt_entry, default_handler);

    idt_entry.selector = selector;
    idt_entry.type = DescriptorType::LongInterruptGate;
    idt_entry.dpl = 0;
    idt_entry.p = 1;
  }

  set_idt_offset(&main_idt_[0], (void*)&_de_irq_handler);
  set_idt_offset(&main_idt_[1], (void*)&_db_irq_handler);
  set_idt_offset(&main_idt_[2], (void*)&_nmi_irq_handler);
  set_idt_offset(&main_idt_[3], (void*)&_bp_irq_handler, true);
  set_idt_offset(&main_idt_[4], (void*)&_of_irq_handler);
  set_idt_offset(&main_idt_[5], (void*)&_br_irq_handler);
  set_idt_offset(&main_idt_[6], (void*)&_ud_irq_handler);
  set_idt_offset(&main_idt_[7], (void*)&_nm_irq_handler);
  set_idt_offset(&main_idt_[8], (void*)&_df_irq_handler);
  set_idt_offset(&main_idt_[10], (void*)&_ts_irq_handler);
  set_idt_offset(&main_idt_[11], (void*)&_np_irq_handler);
  set_idt_offset(&main_idt_[12], (void*)&_ss_irq_handler);
  set_idt_offset(&main_idt_[13], (void*)&_gp_irq_handler);
  set_idt_offset(&main_idt_[14], (void*)&_pf_irq_handler);

  set_idt_offset(&main_idt_[32], (void*)&_timer_irq_handler);
  set_idt_offset(&main_idt_[37], (void*)&_spurious_irq_handler);
  set_idt_offset(&main_idt_[42], (void*)&_syscall_irq_handler, true);

  set_idt_offset(&main_idt_[0x40], (void*)&_40h_irq_handler);
  set_idt_offset(&main_idt_[0x41], (void*)&_41h_irq_handler);
  set_idt_offset(&main_idt_[0x42], (void*)&_42h_irq_handler);
  set_idt_offset(&main_idt_[0x43], (void*)&_43h_irq_handler);
  set_idt_offset(&main_idt_[0x44], (void*)&_44h_irq_handler);
  set_idt_offset(&main_idt_[0x45], (void*)&_45h_irq_handler);
  set_idt_offset(&main_idt_[0x46], (void*)&_46h_irq_handler);
  set_idt_offset(&main_idt_[0x47], (void*)&_47h_irq_handler);
  set_idt_offset(&main_idt_[0x48], (void*)&_48h_irq_handler);
  set_idt_offset(&main_idt_[0x49], (void*)&_49h_irq_handler);
  set_idt_offset(&main_idt_[0x4a], (void*)&_4ah_irq_handler);
  set_idt_offset(&main_idt_[0x4b], (void*)&_4bh_irq_handler);
  set_idt_offset(&main_idt_[0x4c], (void*)&_4ch_irq_handler);
  set_idt_offset(&main_idt_[0x4d], (void*)&_4dh_irq_handler);
  set_idt_offset(&main_idt_[0x4e], (void*)&_4eh_irq_handler);
  set_idt_offset(&main_idt_[0x4f], (void*)&_4fh_irq_handler);
  set_idt_offset(&main_idt_[0x50], (void*)&_50h_irq_handler);
  set_idt_offset(&main_idt_[0x50], (void*)&_51h_irq_handler);
  set_idt_offset(&main_idt_[0x51], (void*)&_52h_irq_handler);
  set_idt_offset(&main_idt_[0x52], (void*)&_53h_irq_handler);
  set_idt_offset(&main_idt_[0x53], (void*)&_54h_irq_handler);
  set_idt_offset(&main_idt_[0x54], (void*)&_55h_irq_handler);
  set_idt_offset(&main_idt_[0x56], (void*)&_56h_irq_handler);
  set_idt_offset(&main_idt_[0x57], (void*)&_57h_irq_handler);
}

void InterruptProcessor::Register(size_t id, InterruptHandler handler) {
  interrupts_[id].handlers_.emplace_back(std::move(handler));
}

static PER_CPU InterruptProcessor *processor_;

extern "C" void irq_handler(u64 irq_num, u64 error_code) {
  // TODO: percpu_get
  processor_->HandleInterrupt(irq_num, error_code);
}

void InterruptProcessor::Register(size_t start_id, size_t count, InterruptHandler handler) {
  for (size_t i = 0; i < count; i++) {
    Register(start_id + i, handler);
  }
}

InterruptProcessor *irq_init() {
  // TODO: percpu_allocate
  processor_ = knew<InterruptProcessor>(Kernel::k);
  return processor_;
}


//void print_idt_descriptor(SerialPort &serial_port, u16 index, const InterruptDescriptor *desc) {
//  serial_port << "Interrupt descriptor 0x" << SerialPort::IntRadix::Hex << index << " ";
//  u64 offset = desc->offset_lo16 | ((u64)desc->offset_mid16 << 16) | ((u64)desc->offset_hi32 << 32);
//  serial_port << "present " << desc->p << SerialPort::IntRadix::Hex << " offset 0x" << offset << " selector 0x" << desc->selector << " type 0x" << desc->type << " dpl " << desc->dpl << "\n";
//}
//
