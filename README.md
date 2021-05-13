# unnamed OS
## A modern OS prototype using up-to-date technology (AMD64, UEFI, Multi-Core CPU, PCI Express, ACPI/SATA, ZFS, Modern C++)

## Principles
- Only support up-to-date technology. Don't even consider backward compatibility.
- Roughly explore different concepts of an OS rather than doing one and do it best.
- Solve the most valuable problem first and add TODOs or asserts for the rest.
- Keep the code short and straight-forward at the cost of robustness (for the error cases, just let it kernel panic if the user can prevent them).

## Prerequisites
- AMD64 CPU (this OS only supports AMD64 architecture, no x86 support)
- GCC 10 (any version should be fine as long as you can compile my code)
- An environment for the OS to run, either of the three
  - qemu 6.0 + TianoCore EDK 2 (For EFI support in qemu, this OS only support EFI, no BIOS support)
  - VirtualBox 6.x (VirtualBox has native EFI support, but VirtualBox does not support RTL8139, so you do not have a working NIC driver, probably)
  - physical machine (AMD64 CPU, a motherboard with Q35 chipset, RTL8139 NIC connected via PCI, AHCI SATA controller and a SATA HDD)
- Some common Linux utilities (CMake, Make, udisksctl)

## Build

### QEMU

``` 
mkdir cmake-build-debug && cd cmake-build-debug
cmake ..
cd -
./qemu
```

### Launch QEMU and use gdb to debug the kernel

```
$ ./qemu.sh -S -s 
$ gdb
>>> target remote :1234
>>> file kernel
>>> c
```

### VirtualBox

``` 
./vbox.sh
```

## TODOs
- boot
  - ~~EFI~~
  - initramfs, embed in kernel image, depends on: VFS

- memory
  - ~~buddy page allocator~~
  - kernel dynamic memory allocator based on buddy allocator (maybe slab)
  - ~~virtual memory(setup gdt,page table)~~
  - uncached pool, depends on page allocator/slab allocator

- irq
  - ~~close irq on interrupt handler~~
  - ~~clock interrupt~~
  - register all exception handlers
  - pretty exception dumper, depends on C++ Exception/libunwind support
  - **irq register/unregister, general irq device polling**

- process
  - ~~user space process~~
  - ~~basic syscall~~
  - smp
  - **fork**
  - **exec**
  - **load simple ELF**, depends on VFS and a file system
  - scheduler, currently we have a simple round-robin scheduler
  - **spinlock**
  - **sleep**

- fs
  - **vfs**
  - ~~simple in-memory file~~
  - GPT partition table, depends on blockdev
  - ext2, depends on GPT and blockdev
  - stdio
  - zfs, too complicated (maybe in later versions)

- kernel library
  - **kernel string library, sprintf**
  - libunwind port/write a libunwind-like library  

- userspace library
  - userspace syscall wrapper
  
- bus driver
  - ~~PCI~~
    - MSI/MSI-X (no such device to test)
  - PCI express
  - **USB**

- network
  - ~~RTL8139 NIC driver~~
  - **general L2 driver**
  - **ARP**
  - IP, IPv6
  - UDP
  - TCP (maybe we won't implement a full-fledged TCP, implement just enough for SSH and HTTP)
  - BSD sockets

- block device
  - ~~AHCI read sector~~
  - AHCI write sector
  - AHCI DMA mode
  - general block device/in memory block dev

- devices
  - VGA
  - keyboard
  - ~~serial port~~

- TTY
  - port my terminal emulator to implement TTY

- infrastructure
  - C++ exception, depends on libunwind
  - static initialization \(maybe we need __attribute to put different var in different section, to init them in different stages), may depend on elf?
  - malloc/free/constructor/destructor, depends on slab allocator
  - debug symbol, depends on libunwind and elf
  - stack dumper, depends on libunwind

- misc
  - use gdb to debug qemu with gdbserver to debug kernel 😒
  - fix compiler warnings
  - compiler attributes: iomem, address_space(x), etc.

- docs
  - add a working example, maybe screencast
  - write a script to set up disk image
  - can we run this in a browser (seems not practical since there seems to be no complete AMD64 emulator in browser, no device emulation, no IOMMU, etc)

## Sample output for QEMU

```text
Shell> fs0:                                                                                                                                                                                                         
FS0:\> boot_loader.efi                                                                                                                                                                                              
TextOut->OutputString()                                                                                                                                                                                             
Hello, world!                                                                                                                                                                                                       
cs: 0038                                                                                                                                                                                                            
Allocated 0x2100 pages starting from 0x79E36000                                                                                                                                                                     
Descriptor version/size/count: 0x1/0x30/0x7B                                                                                                                                                                        
Conventional memory regions:                                                                                                                                                                                        
  Type Pad PhysicalStart VirtualStart NPages Attr                                                                                                                                                                   
  0x03 0x00000000 0x0000000000000000 0x0000000000000000 0x00000001 0x0000000F                                                                                                                                       
  0x07 0x00000000 0x0000000000001000 0x0000000000000000 0x0000009F 0x0000000F                                                                                                                                       
  0x07 0x00000000 0x0000000000100000 0x0000000000000000 0x00000700 0x0000000F                                                                                                                                       
  0x0A 0x00000000 0x0000000000800000 0x0000000000000000 0x00000008 0x0000000F                                                                                                                                       
  0x07 0x00000000 0x0000000000808000 0x0000000000000000 0x00000008 0x0000000F                                                                                                                                       
  0x0A 0x00000000 0x0000000000810000 0x0000000000000000 0x000000F0 0x0000000F
...

Kernel entry: 55 48 89 E5 48 83 EC 10 48 89 7D F8 E8 E2 0C 01                                                                                                                                                       
kernel loaded at virt: 0xFFFF800000100000, phy: 0x7A036000                                                                                                                                                          
Found ACPI 2.0 RSDP at 7FB7E014                                                                                                                                                                                     
ACPI OEM string 'BOCHS ', version 2, rsdt = 0x7FB7D074, xsdt = 0x7FB7D0E8                                                                                                                                           
hello serial from bootloader                                                                                                                                                                                        
before setting cr3 to 7d36c000                                                                                                                                                                                      
pml4e 0 7fc02023                                                                                                                                                                                                    
  pdpe 0 7fc03023                                                                                                                                                                                                   
  pdpe 1 7fc04023                                                                                                                                                                                                   
  pdpe 2 7fc05023
...

kernel page table setup done                                                                                                                                                                                        
Hello from kernel!                                                                                                                                                                                                  
cs = 0x38                                                                                                                                                                                                           
ds = 0x30                                                                                                                                                                                                           
es = 0x30                                                                                                                                                                                                           
fs = 0x30                                                                                                                                                                                                           
gs = 0x30                                                                                                                                                                                                           
ss = 0x30                                                                                                                                                                                                           
rax = 0xffff800000505f30                                                                                                                                                                                            
rcx = 0x3f8                                                                                                                                                                                                         
rdx = 0x1                                                                                                                                                                                                           
rbx = 0x7ff17488                                                                                                                                                                                                    
rsi = 0xa                                                                                                                                                                                                           
rdi = 0x3f8                                                                                                                                                                                                         
rbp = 0xffff800000505ed0                                                                                                                                                                                            
rsp = 0xffff800000505e00                                                                                                                                                                                            
rflags = 0x46                                                                                                                                                                                                       
cr0 = 0x80010033                                                                                                                                                                                                    
cr3 = 0x7d36c000                                                                                                                                                                                                    
cr4 = 0x668                                                                                                                                                                                                         
GDT address = 0x7f9ee698                                                                                                                                                                                            
Segment descriptor 0x38  dpl 0 long_mode 1 default_operand_size 0                                                                                                                                                   
IDT address = 0x7f274018 limit 0xfff                                                                                                                                                                                
cr3 7d36c000                                                                                                                                                                                                        
pml4e 0 7fc02023                                                                                                                                                                                                    
pdpe 0 7fc03023                                                                                                                                                                                                     
pml4e 100 7d36d023                                                                                                                                                                                                  
total pages: 0x40000                                                                                                                                                                                                
setting cs = 0x10                                                                                                                                                                                                   
setting all data selectors to 0x18                                                                                                                                                                                  
setting up user code/data selectors                                                                                                                                                                                 
loading tr 0x30                                                                                                                                                                                                     
moving page table to kernel, new cr3 = 0x7a455000                                                                                                                                                                   
Memory segments:                                                                                                                                                                                                    
Available memory sections = a, size = 5159196KiB                                                                                                                                                                    
max_pages_addr: 4000000, max_pages 40000                                                                                                                                                                            
Buddy allocator at 0xffff800000d29040                                                                                                                                                                               
  bucket 16 size = 64KiB:                                                                                                                                                                                           
  bucket 17 size = 128KiB:                                                                                                                                                                                          
  bucket 18 size = 256KiB:                                                                                                                                                                                          
  bucket 19 size = 512KiB:                                                                                                                                                                                          
  bucket 20 size = 1MiB:                                                                                                                                                                                            
  bucket 21 size = 2MiB:                                                                                                                                                                                            
  bucket 22 size = 4MiB:  
...
Local APIC ID = 0 base = 0xffff8000fee00000                                                                                                                                                                         
ACPI extended system description table size = 84, entries = 6                                                                                                                                                       
  ACPI table 0 FACP                                                                                                                                                                                                 
  ACPI table 1 APIC                                                                                                                                                                                                 
  ACPI table 2 HPET                                                                                                                                                                                                 
  ACPI table 3 MCFG                                                                                                                                                                                                 
...
disabling 8259 PIC                                                                                                                                                                                                  
IO APIC 0x0 0xfec00000 0x0                                                                                                                                                                                          
IOAPIC0 version 0x20 0x18                                                                                                                                                                                           
    IOAPIC 0, 2040 0                                                                                                                                                                                                
    IOAPIC 1, 2041 0                                                                                                                                                                                                
    IOAPIC 2, 2042 0                                                                                                                                                                                                
...
Enumerating PCI devices:                                                                                                                                                                                            
Segment group b0000000 00 ff                                                                                                                                                                                        
  bus 0x0 slot 0x0                                                                                                                                                                                                  
    function 0x0 vendor 0x8086 device 0x29c0 header 0x0 class 0x6 subclass 0x0 IRQ line 0xff IRQ pin 0x0                                                                                                            
  bus 0x0 slot 0x1                                                                                                                                                                                                  
    function 0x0 vendor 0x1234 device 0x1111 header 0x0 class 0x3 subclass 0x0 IRQ line 0xff IRQ pin 0x0                                                                                                            
  bus 0x0 slot 0x2                                                                                                                                                                                                  
    function 0x0 vendor 0x10ec device 0x8139 header 0x0 class 0x2 subclass 0x0 IRQ line 0xb IRQ pin 0x1                                                                                                             
      BAR[0] = 0x6000, size = 0x100 IO space                                                                                                                                                                        
      BAR[1] = 0xc1041000, size = 0x100 Memory space                     
...
user image 8000000 stack 0x8010000                                                                                                                                                                                  
starting process pid = 1 ''                                                                                                                                                                                         
has tmp_start, going for it                                                                                                                                                                                         
main process started                                                                                                                                                                                                
main process creating process 2                                                                                                                                                                                     
user image 8020000 stack 0x80b0000                                                                                                                                                                                  
main process loading                                                                                                                                                                                                
init elf size = 1900, start bytes 0x7f 0x45                                                                                                                                                                         
elf has 5 sections at 0x40                                                                                                                                                                                          
  p_offset p_filesz p_vaddr p_memsz                                                                                                                                                                                 
  0x1000 0xc4 0x110000 0xc4                                                                                                                                                                                         
entrypoint at 110000                                                                                                                                                                                                
ELF file loaded                                                                                                                                                                                                     
write() from pid 1 content 'hello from uSeR5p@ze'                                                                                                                                                                   
starting process pid = 2 '2!'                                                                                                                                                                                       
has tmp_start, going for it                                                                                                                                                                                         
thread2 started                                                                                                                                                                                                     
RTL8139 rx 0x0 0xd9 capr 0xfff0 cbr e0                                                                                                                                                                              
    0000000000000000 | 01 00 5e 7f ff fa ea a3 3e 38 ca 4d 08 00 45 00  | ..^.....>8.M..E.                                                                                                                          
    0000000000000010 | 00 c7 14 73 40 00 01 11 c8 a3 ac 14 00 01 ef ff  | ...s@...........                                                                                                                          
    0000000000000020 | ff fa ec 8b 07 6c 00 b3 ab 9f 4d 2d 53 45 41 52  | .....l....M-SEAR                                                                                                                          
    0000000000000030 | 43 48 20 2a 20 48 54 54 50 2f 31 2e 31 0d 0a 48  | CH * HTTP/1.1..H                                                                                                                          
    0000000000000040 | 4f 53 54 3a 20 32 33 39 2e 32 35 35 2e 32 35 35  | OST: 239.255.255                                                                                                                          
    0000000000000050 | 2e 32 35 30 3a 31 39 30 30 0d 0a 4d 41 4e 3a 20  | .250:1900..MAN:                                                                                                                           
    0000000000000060 | 22 73 73 64 70 3a 64 69 73 63 6f 76 65 72 22 0d  | "ssdp:discover".                                                                                                                          
    0000000000000070 | 0a 4d 58 3a 20 31 0d 0a 53 54 3a 20 75 72 6e 3a  | .MX: 1..ST: urn:                                                                                                                          
    0000000000000080 | 64 69 61 6c 2d 6d 75 6c 74 69 73 63 72 65 65 6e  | dial-multiscreen                                                                                                                          
    0000000000000090 | 2d 6f 72 67 3a 73 65 72 76 69 63 65 3a 64 69 61  | -org:service:dia                                                                                                                          
    00000000000000a0 | 6c 3a 31 0d 0a 55 53 45 52 2d 41 47 45 4e 54 3a  | l:1..USER-AGENT:                                                                                                                          
    00000000000000b0 | 20 47 6f 6f 67 6c 65 20 43 68 72 6f 6d 65 2f 39  |  Google Chrome/9                                                                                                                          
    00000000000000c0 | 30 2e 30 2e 34 34 33 30 2e 39 33 20 4c 69 6e 75  | 0.0.4430.93 Linu                                                                                                                          
    00000000000000d0 | 78 0d 0a 0d 0a f0 26 d5 9f  | x.....&.........                                                                                                                                               
                                                                                                                                                                                                                    
RTL8139 rx 0xe0 0x40 capr 0xd0 cbr 124                                                                                                                                                                              
    0000000000000000 | 0a 01 0e 0a 01 0e ea a3 3e 38 ca 4d 08 06 00 01  | ........>8.M....                                                                                                                          
    0000000000000010 | 08 00 06 04 00 02 ea a3 3e 38 ca 4d ac 14 00 01  | ........>8.M....                                                                                                                          
    0000000000000020 | 0a 01 0e 0a 01 0e ac 14 00 03 00 00 00 00 00 00  | ................                                                                                                                          
    0000000000000030 | 00 00 00 00 00 00 00 00 00 00 00 00 59 f2 c3 42  | ............Y..B                                                                                                                          
                                                                                                                                                                                                                    
RTL8139 tx ok                                                                                           
```