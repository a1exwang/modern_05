# A EFI hello world
A simple EFI application to demonstrate how to write an one.
It paints your EFI background with red color.

## Build
1. Install a efi shell(on ArchLinux, it is `pacman -S edk2-shell`)
1. Create a disk.raw according to Makefile
1. Build VirtualBox vdi image
    - `make sda.vdi`
1. Create a virtual box virtual machine with the vdi file
    - if you have an virtual machine called `efidev` you can run `./vbox.sh` after changing sda.vdi, to let virtualbox know.

1. Start the virtual machine and enter the EFI Shell
    ```
    # In the efi shell
    fs0:
    hello.efi
    ```
1. You will see this

![red](docs/red.png)

## TODOs
- boot
   - ~~EFI~~
   - initramfs
- memory
   - ~~buddy page allocator~~
   - kernel dynamic memory allocator based on buddy allocator
   - ~~virtual memory(setup gdt,page table)~~
   - uncached pool

- irq
    - **close irq on interrupt handler**
    - **clock interrupt**
    - register all exception handlers/pretty exception dumper/ stack dumper
    
- process
    - ~~user space process~~
    - ~~basic syscall~~
    - smp
    - **fork**
    - **exec**
    - **load simple ELF**
    - scheduler

- fs
    - **vfs**
    - ~~simple in-memory file~~
    - stdio

- kernel library
    - **kernel string library, sprintf**

- userspace library
    - userspace syscall wrapper

- IO
    - some harddisk?
    - some NIC ?
    - socket?