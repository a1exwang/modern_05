#pragma once

extern "C" {
  void do_syscall();
}

class Syscall {
 public:
  Syscall();
};
