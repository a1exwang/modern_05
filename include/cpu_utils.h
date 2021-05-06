#pragma once
#include <tuple>
#include <lib/defs.h>

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
