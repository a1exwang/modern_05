#pragma once

extern "C" {
  void do_syscall();
}

class Kernel;
class Process;

class Syscall {
 public:
  Syscall(Kernel *k, Process *p) :kernel_(k), process_(p) { }

  static void SetupSyscall(Kernel *kernel);

 public:
  void sys_read();
  void sys_write();
  void sys_open();
  void sys_close();
  void sys_exit();
  void sys_yield();
 private:
  Kernel *kernel_;
  Process *process_;
};
