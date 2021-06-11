#pragma once

#include <cpu_defs.h>
#include <irq.hpp>

// line statue register
constexpr u16 Serial8250RxBuffer = 0;
constexpr u16 Serial8250IRQStatus = 2;
constexpr u16 Serial8250LSR = 5;

constexpr u16 Serial8250IRQStatusNoIRQ = 1;
constexpr u16 Serial8250LSRDataReady = 1;

// PC COM serial port
class Serial8250 {
 public:
  Serial8250(u16 io_base);
  bool HandleIRQ(IrqHandlerInfo *info);
 private:
  u16 io_base;
};