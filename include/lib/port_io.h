#pragma once

#include <common/defs.h>

void outb(u16 address, u8 data);
u8 inb(u16 address);

void outl(u16 address, u32 data);
u32 inl(u16 address);
