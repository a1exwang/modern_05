#pragma once
#include <lib/serial_port.h>

static void print_file_size(u64 size) {
  const char* sizes[] = { "B", "KiB", "MiB", "GiB", "TiB" };
  u64 order = 0;
  u64 remains = 0;

  while (size >= 1024 && order < sizeof(sizes)/sizeof(sizes[0]) - 1) {
    order++;
    remains = size % 1024;
    size = size / 1024;
  }

  Kernel::sp() << SerialPort::IntRadix::Dec << size;
  auto r = remains * 100 / 1024;
  if (r) {
    Kernel::sp() << SerialPort::IntRadix::Dec << (r >= 10 ? "." : ".0") << r;
  }
  Kernel::sp() << sizes[order];
}
