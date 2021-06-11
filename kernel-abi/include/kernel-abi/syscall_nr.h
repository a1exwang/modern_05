#pragma once

enum {
  SYSCALL_NR_NULL = 0,
  SYSCALL_NR_READ,
  SYSCALL_NR_WRITE,
  SYSCALL_NR_OPEN,
  SYSCALL_NR_CLOSE,
  SYSCALL_NR_EXIT,
  SYSCALL_NR_YIELD,
  SYSCALL_NR_ANON_ALLOCATE,
  SYSCALL_NR_MAX,
};