#include <cpu_utils.h>

std::tuple<void *, u16> get_idt() {
  u8 idtr[10];
  __asm__ __volatile__("sidt %0"
  :"=m"(idtr)
  :
  :"memory");

  u16 limit = *(u16*)&idtr[0];
  void* offset = (void*)*(u64*)&idtr[2];
  return std::make_tuple(offset, limit);
}

std::tuple<void *, u16> get_gdt() {
  u8 gdtr[10];
  __asm__ __volatile__("sgdt %0"
  :"=m"(gdtr)
  :
  :"memory");

  u16 limit = *(u16*)&gdtr[0];
  void* offset = (void*)*(u64*)&gdtr[2];
  return std::make_tuple(offset, limit);
}
void load_idt(std::tuple<void *, unsigned short> idtr_input) {
  u8 idtr[10];
  auto [offset, limit] = idtr_input;

  *(u16*)&idtr[0] = limit;
  *(u64*)&idtr[2] = (u64)offset;
  __asm__ __volatile__("lidt %0"
  :
  :"m"(idtr)
  :"memory");
}
u64 cpuGetMSR(unsigned int msr) {
  u64 result;
  asm volatile("rdmsr" : "=a"(*(u32*)&result), "=d"(*(u32*)((char*)&result+4)) : "c"(msr));
  return result;
}
void cpuSetMSR(unsigned int msr, unsigned long value) {
  asm volatile("wrmsr" : : "a"(value & 0xffffffff), "d"(value >> 32), "c"(msr));
}
void halt() {
  while (true) ;
  //asm volatile ("hlt");
}
