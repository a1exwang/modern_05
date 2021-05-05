#include <syscall.h>

void do_syscall() {
  __asm__ __volatile__("int $42");
}