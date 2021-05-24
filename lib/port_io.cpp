#include <common/defs.h>

void outb(u16 address, u8 data) {
  asm volatile (
  "mov %1, %%al\n"
  "mov %0, %%dx\n"
  "outb %%al, %%dx\n"
  :
  : "r"(address), "r"(data)
  : "%dx", "%al"
  );
}
u8 inb(u16 address) {
  u8 result = 0;
  asm volatile (
  "mov %1, %%dx\n"
  "inb %%dx, %%al\n"
  "mov %%al, %0\n"
  : "=r"(result)
  : "r"(address)
  : "%dx", "%al"
  );
  return result;
}

void outl(u16 address, u32 data) {
  asm (
  "mov %1, %%eax\n"
  "mov %0, %%dx\n"
  "outl %%eax, %%dx\n"
  :
  : "r"(address), "r"(data)
  : "%dx", "%al"
  );
}

u32 inl(u16 address) {
  u32 result = 0;
  asm (
  "mov %1, %%dx\n"
  "inl %%dx, %%eax\n"
  "mov %%eax, %0\n"
  : "=r"(result)
  : "r"(address)
  : "%dx", "%al"
  );
  return result;
}
