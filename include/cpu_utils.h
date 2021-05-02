#pragma once
#include <tuple>
#include <lib/defs.h>

void halt();

std::tuple<void*, u16> get_gdt();
std::tuple<void*, u16> get_idt();
void load_idt(std::tuple<void*, u16> idtr_input);

#define IA32_APIC_BASE_MSR 0x1B

u64 cpuGetMSR(u32 msr);

void cpuSetMSR(u32 msr, u64 value);
