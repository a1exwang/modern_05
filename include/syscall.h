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
  int sys_read();
  int sys_write();
  int sys_open();
  int sys_close();
  int sys_exit();
  int sys_yield();
  int sys_anon_allocate();
 private:
  Kernel *kernel_;
  Process *process_;
};
