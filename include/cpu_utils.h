#pragma once
#include <tuple>
#include <common/defs.h>


// address spaces: similar to Linux's
// 0 -> kernel space
// 1 -> user space
// 2 -> per-cpu space
#define PER_CPU __attribute__((section(".data.per_cpu")))


void halt();

std::tuple<void*, u16> get_gdt();
void load_gdt(std::tuple<void *, u16> gdtr_input);
std::tuple<void*, u16> get_idt();
void load_idt(std::tuple<void*, u16> idtr_input);

#define APIC_BASE_MSR 0x1B

u64 get_msr(u32 msr);
void set_msr(u32 msr, u64 value);

void flush_tlb();

static void cli() {
  asm volatile ("cli");
}
static void sti() {
  asm volatile ("sti");
}

u16 get_cs();

u64 get_cr2();
u64 get_rbp();
