#include <device/ahci.hpp>
#include <cpu_defs.h>
#include <lib/port_io.h>
#include <kernel.h>
#include <device/pci.h>
#include <lib/string.h>
#include <mm/page_alloc.h>
#include <mm/mm.h>

void *port_register(void *ahci_reg_page, u8 port) {
  return ((char*)ahci_reg_page + 0x100) + 0x80 * port;
}

//https://forum.osdev.org/viewtopic.php?p=311182&sid=15cf6d354b08796bb45a1fb0fbadea0b#p311182
#define SATA_SIG_SATA    0x00000101  // SATA drive
#define SATA_SIG_ATAPI  0xEB140101  // SATAPI drive

#pragma pack(push, 1)
struct HBACapabilities {
  u8 NP : 5;
  u8 SXS : 1;
  u8 EMS : 1;
  u8 CCCS : 1;

  u8 NCS : 5;
  u8 PSC : 1;
  u8 SSC : 1;
  u8 PMD : 1;

  u8 FBSS : 1;
  u8 SPM : 1;
  u8 SAM : 1;
  u8 r0 : 1;
  u8 ISS : 4;

  u8 SCLO : 1;
  u8 SAL : 1;
  u8 SALP : 1;
  u8 SSS : 1;
  u8 SMPS : 1;
  u8 SSNTF : 1;
  u8 SNCQ : 1;
  u8 S64A : 1;
};


#define HBA_PxCMD_ST    0x0001
#define HBA_PxCMD_FRE   0x0010
#define HBA_PxCMD_FR    0x4000
#define HBA_PxCMD_CR    0x8000
typedef volatile struct tagHBA_PORT
{
  u32 clb;		// 0x00, command list base address, 1K-byte aligned
  u32 clbu;		// 0x04, command list base address upper 32 bits
  u32 fb;		// 0x08, FIS base address, 256-byte aligned
  u32 fbu;		// 0x0C, FIS base address upper 32 bits
  u32 is;		// 0x10, interrupt status
  u32 ie;		// 0x14, interrupt enable
  u32 cmd;		// 0x18, command and status
  u32 rsv0;		// 0x1C, Reserved
  u32 tfd;		// 0x20, task file data
  u32 sig;		// 0x24, signature
  u32 ssts;		// 0x28, SATA status (SCR0:SStatus)
  u32 sctl;		// 0x2C, SATA control (SCR2:SControl)
  u32 serr;		// 0x30, SATA error (SCR1:SError)
  u32 sact;		// 0x34, SATA active (SCR3:SActive)
  u32 ci;		// 0x38, command issue
  u32 sntf;		// 0x3C, SATA notification (SCR4:SNotification)
  u32 fbs;		// 0x40, FIS-based switch control
  u32 rsv1[11];	// 0x44 ~ 0x6F, Reserved
  u32 vendor[4];	// 0x70 ~ 0x7F, vendor specific
} HBA_PORT;
typedef struct tagHBA_CMD_HEADER
{
  // DW0
  u8  cfl:5;		// Command FIS length in DWORDS, 2 ~ 16
  u8  a:1;		// ATAPI
  u8  w:1;		// Write, 1: H2D, 0: D2H
  u8  p:1;		// Prefetchable

  u8  r:1;		// Reset
  u8  b:1;		// BIST
  u8  c:1;		// Clear busy upon R_OK
  u8  rsv0:1;		// Reserved
  u8  pmp:4;		// Port multiplier port

  u16 prdtl;		// Physical region descriptor table length in entries

  // DW1
  volatile
  u32 prdbc;		// Physical region descriptor byte count transferred

  // DW2, 3
  u32 ctba;		// Command table descriptor base address
  u32 ctbau;		// Command table descriptor base address upper 32 bits

  // DW4 - 7
  u32 rsv1[4];	// Reserved
} HBA_CMD_HEADER;


typedef struct tagHBA_PRDT_ENTRY
{
  u32 dba;		// Data base address
  u32 dbau;		// Data base address upper 32 bits
  u32 rsv0;		// Reserved

  // DW3
  u32 dbc:22;		// Byte count, 4M max
  u32 rsv1:9;		// Reserved
  u32 i:1;		// Interrupt on completion
} HBA_PRDT_ENTRY;

typedef struct tagHBA_CMD_TBL
{
  // 0x00
  u8  cfis[64];	// Command FIS

  // 0x40
  u8  acmd[16];	// ATAPI command, 12 or 16 bytes

  // 0x50
  u8  rsv[48];	// Reserved

  // 0x80
  HBA_PRDT_ENTRY	prdt_entry[1];	// Physical region descriptor table entries, 0 ~ 65535
} HBA_CMD_TBL;

typedef struct tagFIS_REG_H2D
{
  // DWORD 0
  u8  fis_type;	// FIS_TYPE_REG_H2D

  u8  pmport:4;	// Port multiplier
  u8  rsv0:3;		// Reserved
  u8  c:1;		// 1: Command, 0: Control

  u8  command;	// Command register
  u8  featurel;	// Feature register, 7:0

  // DWORD 1
  u8  lba0;		// LBA low register, 7:0
  u8  lba1;		// LBA mid register, 15:8
  u8  lba2;		// LBA high register, 23:16
  u8  device;		// Device register

  // DWORD 2
  u8  lba3;		// LBA register, 31:24
  u8  lba4;		// LBA register, 39:32
  u8  lba5;		// LBA register, 47:40
  u8  featureh;	// Feature register, 15:8

  // DWORD 3
  u8  countl;		// Count register, 7:0
  u8  counth;		// Count register, 15:8
  u8  icc;		// Isochronous command completion
  u8  control;	// Control register

  // DWORD 4
  u8  rsv1[4];	// Reserved
} FIS_REG_H2D;
#pragma pack(pop)

// Start command engine
void start_cmd(HBA_PORT *port)
{
  // Wait until CR (bit15) is cleared
  while (port->cmd & HBA_PxCMD_CR)
    ;

  // Set FRE (bit4) and ST (bit0)
  port->cmd = port->cmd | HBA_PxCMD_FRE;
  port->cmd = port->cmd | HBA_PxCMD_ST;
}

// Stop command engine
void stop_cmd(HBA_PORT *port) {
  // Clear ST (bit0)
  port->cmd = port->cmd & ~HBA_PxCMD_ST;

  // Clear FRE (bit4)
  port->cmd = port->cmd & ~HBA_PxCMD_FRE;

  // Wait until FR (bit14), CR (bit15) are cleared
  while(1)
  {
    if (port->cmd & HBA_PxCMD_FR)
      continue;
    if (port->cmd & HBA_PxCMD_CR)
      continue;
    break;
  }
}

//void port_rebase(HBA_PORT *port, int portno, u64 ahci_base) {
//  stop_cmd(port);	// Stop command engine
//
//  // Command list offset: 1K*portno
//  // Command list entry size = 32
//  // Command list entry maxim count = 32
//  // Command list maxim size = 32*32 = 1K per port
//  port->clb = AHCI_BASE + (portno<<10);
//  port->clbu = 0;
//  memset((void*)(port->clb), 0, 1024);
//
//  // FIS offset: 32K+256*portno
//  // FIS entry size = 256 bytes per port
//  port->fb = AHCI_BASE + (32<<10) + (portno<<8);
//  port->fbu = 0;
//  memset((void*)(port->fb), 0, 256);
//
//  // Command table offset: 40K + 8K*portno
//  // Command table size = 256*32 = 8K per port
//  HBA_CMD_HEADER *cmdheader = (HBA_CMD_HEADER*)(port->clb);
//  for (int i=0; i<32; i++)
//  {
//    cmdheader[i].prdtl = 8;	// 8 prdt entries per command table
//    // 256 bytes per command table, 64+16+48+16*8
//    // Command table offset: 40K + 8K*portno + cmdheader_index*256
//    cmdheader[i].ctba = AHCI_BASE + (40<<10) + (portno<<13) + (i<<8);
//    cmdheader[i].ctbau = 0;
//    memset((void*)cmdheader[i].ctba, 0, 256);
//  }
//
//  start_cmd(port);	// Start command engine
//}
//
#define ATA_DEV_BUSY 0x80
#define ATA_DEV_DRQ 0x08
// Find a free command list slot
int find_cmdslot(HBA_PORT *port)
{
  // If not set in SACT and CI, the slot is free
  u32 slots = (port->sact | port->ci);
  for (int i=0; i<32; i++)
  {
    if ((slots&1) == 0)
      return i;
    slots >>= 1;
  }
  Kernel::sp() << ("Cannot find free command list entry\n");
  return -1;
}

typedef enum
{
  FIS_TYPE_REG_H2D	= 0x27,	// Register FIS - host to device
  FIS_TYPE_REG_D2H	= 0x34,	// Register FIS - device to host
  FIS_TYPE_DMA_ACT	= 0x39,	// DMA activate FIS - device to host
  FIS_TYPE_DMA_SETUP	= 0x41,	// DMA setup FIS - bidirectional
  FIS_TYPE_DATA		= 0x46,	// Data FIS - bidirectional
  FIS_TYPE_BIST		= 0x58,	// BIST activate FIS - bidirectional
  FIS_TYPE_PIO_SETUP	= 0x5F,	// PIO setup FIS - device to host
  FIS_TYPE_DEV_BITS	= 0xA1,	// Set device bits FIS - device to host
} FIS_TYPE;
#define ATA_CMD_READ_DMA_EX 0x25

#define HBA_PxIS_TFES   (1 << 30)       /* TFES - Task File Error Status */

bool read(HBA_PORT *port, u32 startl, u32 starth, u32 count, u64 buf_phy_addr)
{
  port->is = (u32) -1;		// Clear pending interrupt bits
  int spin = 0; // Spin lock timeout counter
  int slot = find_cmdslot(port);
  if (slot == -1)
    return false;

  auto cmdheader_phy = (port->clb + ((u64)port->clbu<<32));
  assert(cmdheader_phy < IDENTITY_MAP_PHY_END, "");
  HBA_CMD_HEADER *cmdheader = (HBA_CMD_HEADER*) (cmdheader_phy + KERNEL_START);
  cmdheader += slot;
  cmdheader->cfl = sizeof(FIS_REG_H2D)/sizeof(u32);	// Command FIS size
  cmdheader->w = 0;		// Read from device
  cmdheader->prdtl = (u16)((count-1)>>4) + 1;	// PRDT entries count

  HBA_CMD_TBL *cmdtbl = (HBA_CMD_TBL*)phy2virt((u64)cmdheader->ctba + ((u64)cmdheader->ctbau<<32));
  memset(cmdtbl, 0, sizeof(HBA_CMD_TBL) +
      (cmdheader->prdtl-1)*sizeof(HBA_PRDT_ENTRY));

  int i = 0;
  // 8K bytes (16 sectors) per PRDT
  for (i=0; i<cmdheader->prdtl-1; i++)
  {
    cmdtbl->prdt_entry[i].dba = (u64) buf_phy_addr;
    cmdtbl->prdt_entry[i].dbau = ((u64) buf_phy_addr) >> 32;
    cmdtbl->prdt_entry[i].dbc = 8*1024-1;	// 8K bytes (this value should always be set to 1 less than the actual value)
    cmdtbl->prdt_entry[i].i = 1;
    buf_phy_addr += 4*1024*2;	// 4K words
    count -= 16;	// 16 sectors
  }
  // Last entry
  cmdtbl->prdt_entry[i].dba = (u64) buf_phy_addr;
  cmdtbl->prdt_entry[i].dbau = ((u64) buf_phy_addr) >> 32;
  cmdtbl->prdt_entry[i].dbc = (count<<9)-1;	// 512 bytes per sector
  cmdtbl->prdt_entry[i].i = 1;

  // Setup command
  FIS_REG_H2D *cmdfis = (FIS_REG_H2D*)(&cmdtbl->cfis);

  cmdfis->fis_type = FIS_TYPE_REG_H2D;
  cmdfis->c = 1;	// Command
  cmdfis->command = ATA_CMD_READ_DMA_EX;

  cmdfis->lba0 = (u8)startl;
  cmdfis->lba1 = (u8)(startl>>8);
  cmdfis->lba2 = (u8)(startl>>16);
  cmdfis->device = 1<<6;	// LBA mode

  cmdfis->lba3 = (u8)(startl>>24);
  cmdfis->lba4 = (u8)starth;
  cmdfis->lba5 = (u8)(starth>>8);

  cmdfis->countl = count & 0xFF;
  cmdfis->counth = (count >> 8) & 0xFF;

  // The below loop waits until the port is no longer busy before issuing a new command
  while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000)
  {
    spin++;
  }
  if (spin == 1000000)
  {
    Kernel::sp() << "port hung\n";
    return false;
  }

  port->ci = 1<<slot;	// Issue command

  Kernel::sp() << "waiting for completion\n";
  // Wait for completion
  while (1)
  {
    // In some longer duration reads, it may be helpful to spin on the DPS bit
    // in the PxIS port field as well (1 << 5)
    if ((port->ci & (1<<slot)) == 0)
      break;
    if (port->is & HBA_PxIS_TFES)	// Task file error
    {
      Kernel::sp() << "Read disk error\n";
      return false;
    }
  }

  // Check again
  if (port->is & HBA_PxIS_TFES)
  {
    Kernel::sp() << "Read disk error\n";
    return false;
  }

  return true;

}

void hex02(u8 value) {
  Kernel::sp() << IntRadix::Hex << (value >> 4) << (value & 0xf);
}
void hex08(u64 value) {
  for (int i = 0; i < 8; i++) {
    hex02((value >> (64-8 - i * 8)) & 0xff);
  }
}

bool isprint(char c) {
  return 0x20 <= c && c < 0x7f;
}

void hexdump(const char *ptr, u64 size, bool compact, int indent) {
  u64 cols = 16;
  u64 lines = size/cols;
  for (u64 line = 0; line < lines; line++) {
    if (!compact) {
      for (int ind = 0; ind < indent; ind++) {
        Kernel::sp() << ' ';
      }
      hex08(line * cols);
      Kernel::sp() << " | ";
    }
    for (u64 col = 0; col < cols; col++) {
      hex02(ptr[line*cols+col]);
      Kernel::sp() << " ";
    }
    if (!compact) {
      Kernel::sp() << " | ";
      for (u64 col = 0; col < cols; col++) {
        auto c=  (char)ptr[line*cols+col];
        if (isprint(c)) {
          Kernel::sp() << c;
        } else {
          Kernel::sp() << '.';
        }
      }
      Kernel::sp() << "\n";
    }
  }

  if (lines*cols < size) {
    if (!compact) {
      for (int ind = 0; ind < indent; ind++) {
        Kernel::sp() << ' ';
      }
      hex08(lines * cols);
      Kernel::sp() << " | ";
    }
    auto cols_remain = size - lines*cols;
    for (u64 col = 0; col < cols_remain; col++) {
      hex02(ptr[lines*cols+col]);
      Kernel::sp() << " ";
    }
    if (!compact) {
      Kernel::sp() << " | ";
      for (u64 col = 0; col < cols; col++) {
        auto c = (char)ptr[lines*cols+col];
        if (isprint(c)) {
          Kernel::sp() << c;
        } else {
          Kernel::sp() << '.';
        } }
      Kernel::sp() << "\n";
    }
  }
}


void ahci_port_init(char *port_control_register) {
  auto *cmd = (u32*)(port_control_register + 0x18);
  if ((*cmd & 2) == 0) {
    Kernel::sp() << "Spinning up\n";
    *cmd = *cmd | 2;
  }
  start_cmd((HBA_PORT*)port_control_register);
  // enable FIS
//  *cmd |= 1 << 4;
  Kernel::sp() << IntRadix::Hex << "  port command " << *cmd << "\n";

  auto command_list_addr = (*(u64*)(port_control_register) & 0xfffffffffffff000);
  auto fis_base_addr = (*(u64*)(port_control_register+8) & 0xfffffffffffff000);


  Kernel::sp() << IntRadix::Hex << "  cmd_list_addr: " << (u64)command_list_addr << ", fis_bas_addr: " << fis_base_addr << "\n";
  assert(command_list_addr < IDENTITY_MAP_PHY_END, "");
  assert(fis_base_addr < IDENTITY_MAP_PHY_END, "");

  auto buf = kernel_page_alloc(16);
  auto buf_phy = ((u64)buf-KERNEL_START);
  auto ok = read((HBA_PORT*)port_control_register, 1, 0, 1, buf_phy);

  Kernel::sp() << "read ok = " << (int)ok << "\n";

  hexdump((const char*)buf, 512);
}

bool AHCIDriver::Enumerate(PCIDeviceInfo *info) {
  auto config_space = info->config_space;
  abar = &config_space->bars[5];
  Init();
  return true;
}

void AHCIDriver::Init() {
  assert(abar != nullptr, "abar is null");

//  void *ahci_reg_page = kernel_page_alloc(16);
//  u32 ahci_phy = (u64)ahci_reg_page - KERNEL_START;
  // TODO: use allocate to allocate non-cachable memory range, currently hard code it
  u32 ahci_phy = 0xcabcd000;
  void *ahci_reg_page = (void*)(ahci_phy + KERNEL_START);

  *abar = ahci_phy;

  Kernel::sp() << "set ahci abar = " << IntRadix::Hex << ahci_phy << "\n";

  auto capabilities = (HBACapabilities*)((char*)ahci_reg_page + 0x00);
  Kernel::sp() << "  Capabilities: " << IntRadix::Hex << *(u32*)capabilities << "\n";

//  assert(capabilities->SSS, "SATA Device SSS not supported");

  auto *ghc = (u32*)((char*)ahci_reg_page + 0x04);
  if (((*ghc >> 31) & 1) == 0) {
    *ghc = (*ghc) | 0x80000000;
  }
  if (((*ghc >> 31) & 1) == 0) {
    // AHCI enabled
    Kernel::k->panic("AHCI not enabled\n");
  }
  if ((*ghc & 2) != 0) {
    // interrupt
    *ghc = *ghc | 2;
  }
  if ((*ghc & 2) != 0) {
    Kernel::k->panic("Interrupt not enabled\n");
  }

  u32 pi = *(u32*)((char*)ahci_reg_page + 0xc);
  Kernel::sp() << "ports implemented = " << IntRadix::Hex << pi << "\n";
  Kernel::sp() << "version = " << IntRadix::Hex << *(u32*)((char*)ahci_reg_page + 0x10) << "\n";

  for (int i = 0; i < 32; i++) {
    if ((1 << i) & pi) {
      Kernel::sp() << "port implemented 0x" << IntRadix::Hex << i;
      auto port_control_reg = port_register(ahci_reg_page, i);
      auto sstatus = *(u32*)((char*)port_control_reg + 0x28);
      auto det = sstatus & 0xf;
      auto spd = (sstatus>>4) & 0xf;
      auto ipm = (sstatus>>8) & 0xf;
      if (!(det == 3 && spd != 0 && ipm == 1)) {
        Kernel::sp() << " no device\n";
        continue;;
      }
      auto sig = *(u32*)((char*)port_control_reg+0x24);
      Kernel::sp() << " ACTIVE device, sig = 0x" << IntRadix::Hex << sig << "\n";
      if (sig == SATA_SIG_ATAPI) {
        Kernel::sp() << "  ATAPI device\n";
//        ahci_port_init((char *) port_control_reg);
      }
      if (sig == SATA_SIG_SATA) {
        Kernel::sp() << "  SATA device\n";
        ahci_port_init((char *) port_control_reg);
      }
    }
  }

}
