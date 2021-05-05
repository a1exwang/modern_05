#include <kernel.h>
#include <lib/serial_port.h>
#include <cpu_defs.h>

#define PRINT_REG(type, name) \
  do {                      \
    type reg_##name = 0; \
    asm volatile("mov %%" #name ",%0" : "=r" (reg_##name)); \
    Kernel::sp() << #name " = 0x" << SerialPort::IntRadix::Hex << reg_##name << "\n"; \
  } while (0)

void print_segment_descriptor(SerialPort &serial_port, u16 selector, void *gdt) {
  auto &desc = *reinterpret_cast<const SegmentDescriptor*>((char*)gdt+ (selector & 0xfff8));

  serial_port << "Segment descriptor 0x" << SerialPort::IntRadix::Hex << selector << " ";

//  serial_port << "  base_addr 0x" << SerialPort::IntRadix::Hex << base_addr;
  serial_port << " dpl " << SerialPort::IntRadix::Hex << desc._dpl << " ";

//  serial_port << "  avl 0x" << SerialPort::IntRadix::Hex << desc.dpl;
  serial_port << "long_mode " << SerialPort::IntRadix::Hex << desc._long_mode << " ";
  serial_port << "default_operand_size " << SerialPort::IntRadix::Hex << desc._default_operand_size << "\n";
//  serial_port << "  granularity 0x" << SerialPort::IntRadix::Hex << desc.granularity;
}

void dump_all() {

  PRINT_REG(u16, cs);
  PRINT_REG(u16, fs);
  PRINT_REG(u16, gs);

  PRINT_REG(u64, rax);
  PRINT_REG(u64, rcx);
  PRINT_REG(u64, rdx);
  PRINT_REG(u64, rbx);
  PRINT_REG(u64, rsi);
  PRINT_REG(u64, rdi);
  PRINT_REG(u64, rbp);
  PRINT_REG(u64, rsp);
  u64 rflags = 0;
  __asm __volatile(
  "pushf\n"
  "pop %0\n"
  :"=m"(rflags)
  :
  :"memory"
  );

  Kernel::sp() << "rflags = 0x" << SerialPort::IntRadix::Hex << rflags << "\n"; \

  PRINT_REG(u64, cr0);
  PRINT_REG(u64, cr3);
  PRINT_REG(u64, cr4);

  auto [gdt_addr, _] = get_gdt();
  Kernel::sp() << "GDT address = 0x" << SerialPort::IntRadix::Hex << (u64)gdt_addr << '\n';

  u16 cs = 0;
  asm volatile("mov %%cs ,%0" : "=r" (cs));
  print_segment_descriptor(Kernel::sp(), cs, gdt_addr);

  auto [idt_addr, idt_size] = get_idt();
  Kernel::sp() << "IDT address = 0x" << SerialPort::IntRadix::Hex << (u64)idt_addr << " limit 0x" << idt_size << '\n';
  for (u32 i = 0; i < 256; i++) {
    auto idt = reinterpret_cast<InterruptDescriptor *>(idt_addr);
    //print_idt_descriptor(serial_port_, i, &idt[i]);
  }

}

void debug_init() {
  dump_all();
}