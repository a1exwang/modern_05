#include <lib/defs.h>

void outb(u16 address, u8 data) {
  asm (
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
  asm (
  "mov %1, %%dx\n"
  "inb %%dx, %%al\n"
  "mov %%al, %0\n"
  : "=r"(result)
  : "r"(address)
  : "%dx", "%al"
  );
  return result;
}