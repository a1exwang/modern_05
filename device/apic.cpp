#include <cpu_utils.h>
#include <kernel.h>
#include <irq.h>



constexpr u16 APIC_APICID	= 0x20;
constexpr u16 APIC_APICVER	= 0x30;
constexpr u16 APIC_TASKPRIOR	= 0x80;
constexpr u16 APIC_EOI	= 0x0B0;
constexpr u16 APIC_LDR	= 0x0D0;
constexpr u16 APIC_DFR	= 0x0E0;
constexpr u16 APIC_SPURIOUS	= 0xF0;
constexpr u16 APIC_ESR	= 0x280;
constexpr u16 APIC_ICRL	= 0x300;
constexpr u16 APIC_ICRH	= 0x310;
constexpr u16 APIC_LVT_TMR	= 0x320;
constexpr u16 APIC_LVT_PERF	= 0x340;
constexpr u16 APIC_LVT_LINT0	= 0x350;
constexpr u16 APIC_LVT_LINT1	= 0x360;
constexpr u16 APIC_LVT_ERR	= 0x370;
constexpr u16 APIC_TMRINITCNT	= 0x380;
constexpr u16 APIC_TMRCURRCNT	= 0x390;
constexpr u16 APIC_TMRDIV	= 0x3E0;
constexpr u16 APIC_LAST	= 0x38F;

constexpr u32 APIC_DISABLE	= 0x10000;
constexpr u32 APIC_SW_ENABLE	= 0x100;
constexpr u32 APIC_CPUFOCUS	= 0x200;
constexpr u32 APIC_NMI	= (4<<8);
constexpr u32 TMR_PERIODIC	= 0x20000;
constexpr u32 TMR_BASEDIV	= (1<<20);

void test_apic() ;
class APIC {
 public:
  explicit APIC() {
    u64 apic_base_reg = get_msr(APIC_BASE_MSR);
    apic_base = (apic_base_reg & 0xfffffffff000);
    setup_timer();
  }

  void setup_timer() {
    auto id = read(APIC_APICID);
    Kernel::sp() << "Local APIC ID = " << id << "\n";

    // https://wiki.osdev.org/APIC_timer
    // initialize LAPIC to a well known state
    write(APIC_DFR, 0xffffffff);
    u32 ldr = read(APIC_LDR);
    ldr = (ldr & 0x00ffffff) | 1;
    write(APIC_LDR, ldr);

    write(APIC_LVT_TMR, APIC_DISABLE);
    write(APIC_LVT_PERF, APIC_NMI);
    write(APIC_LVT_LINT0, APIC_DISABLE);
    write(APIC_LVT_LINT1, APIC_DISABLE);
    write(APIC_TASKPRIOR, 0);

    // enable
    u64 apic_base_reg = get_msr(APIC_BASE_MSR);
    set_msr(APIC_BASE_MSR, apic_base_reg | (1<<11));

    write(APIC_SPURIOUS, IRQ_SPURIOUS + APIC_SW_ENABLE);
    write(APIC_LVT_TMR, 32);
    write(APIC_TMRDIV, 3);

//    pit_prepare_sleep(10000);
    // Set APIC init counter to -1
    write(APIC_TMRINITCNT, 0xFFFFFFFF);

    // Perform PIT-supported sleep
//    pit_perform_sleep();
    volatile int _i = 0;
    for (int i = 0; i < 1000 * 1000; i++) {
      _i = _i + 1;
    }

    // Stop the APIC timer
    write(APIC_LVT_TMR, read(APIC_LVT_TMR) & ~(1 << 16));

    // Now we know how often the APIC timer has ticked in 10ms
    u32 ticks = 0xFFFFFFFF - read(APIC_TMRCURRCNT);

    // Start timer as periodic on IRQ 0, divider 16, with the number of ticks we counted
    write(APIC_LVT_TMR, IRQ_TIMER | TMR_PERIODIC);
    write(APIC_TMRDIV, 0x3);
    write(APIC_TMRINITCNT, ticks);
  }


  u32 read(u16 addr) {
    return *(u32*)((u8*)apic_base+addr);
  }
  void write(u16 addr, u32 data) {
    *(u32*)((u8*)apic_base+addr) = data;
  }

  void eoi() {
    write(APIC_EOI, 0);
  }

 private:
  u64 apic_base;
};

u8 _boot_apic_space[sizeof(APIC)];
APIC *boot_apic = (APIC*)&_boot_apic_space;

void apic_init() {
  new(_boot_apic_space) APIC();
}

void test_apic() {
  while (true) {
    auto v = boot_apic->read(APIC_TMRCURRCNT);
    Kernel::sp() << "value " << v << "\n";

  }
}
