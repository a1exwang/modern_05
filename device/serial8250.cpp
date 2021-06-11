#include <device/serial8250.hpp>
#include <kernel.h>
#include <irq.hpp>


void lapic_eoi();
Serial8250::Serial8250(u16 io_base) : io_base(io_base) {
  Kernel::k->irq_->Register(IOAPIC_ISA_IRQ_COM1, [this](IrqHandlerInfo *info) {
    this->HandleIRQ(info);
  });
}

//https://en.wikibooks.org/wiki/Serial_Programming/8250_UART_Programming
bool Serial8250::HandleIRQ(IrqHandlerInfo *info) {
  // COM1
  if (info->irq_num != IOAPIC_ISA_IRQ_COM1) {
    return false;
  }
  u8 irq_status = inb(io_base + Serial8250IRQStatus);
  if (irq_status & Serial8250IRQStatusNoIRQ) {
    return false;
  }

  // check is there any data received
  u16 lsr = inb(io_base + Serial8250LSR);
  if (lsr & Serial8250LSRDataReady) {
    u8 data = inb(io_base + Serial8250RxBuffer);
    Kernel::sp() << "Serial Port input: " << (char)data << "\n";
  }

  lapic_eoi();
  return true;
}
