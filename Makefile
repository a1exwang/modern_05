ARCH            = $(shell uname -m | sed s,i[3456789]86,ia32,)

OBJS            = boot_loader.o
TARGET          = boot_loader.efi

EFIINC          = /usr/include/efi
EFIINCS         = -I$(EFIINC) -I$(EFIINC)/$(ARCH) -I$(EFIINC)/protocol
LIB             = /usr/lib
EFILIB          = /usr/lib
EFI_CRT_OBJS    = $(EFILIB)/crt0-efi-$(ARCH).o
EFI_LDS         = $(EFILIB)/elf_$(ARCH)_efi.lds

CFLAGS          = $(EFIINCS) -Iinclude -fno-stack-protector -fpic \
          -fshort-wchar -mno-red-zone -Wall
ifeq ($(ARCH),x86_64)
  CFLAGS += -DEFI_FUNCTION_WRAPPER
endif

LDFLAGS         = -nostdlib -znocombreloc -T $(EFI_LDS) -shared \
          -Bsymbolic -L $(EFILIB) -L $(LIB) $(EFI_CRT_OBJS)

all: $(TARGET)

boot_loader.so: $(OBJS)
	ld $(LDFLAGS) $(OBJS) -o $@ -lefi -lgnuefi


.FORCE:

# use cmake to enable IDE
kernel: .FORCE
	make -C cmake-build-debug kernel
	ln -sf cmake-build-debug/kernel kernel
#	g++ -nostdlib -c -o kernel.o kernel.cpp
#	ld -static -T kernel.ld -o kernel kernel.o

boot_loader.efi: boot_loader.so
	objcopy -j .text -j .sdata -j .data -j .dynamic \
        -j .dynsym  -j .rel -j .rela -j .reloc \
        --target=efi-app-$(ARCH) boot_loader.so boot_loader.efi


disk.raw:
#	dd if=/dev/zero of=disk.raw bs=1M count=64
# a gpt partition table and create an fat32 partion(esp) and 1 ext4 partition


sda.vdi: boot_loader.efi disk.raw startup.nsh kernel
	$(eval loop_device := $(shell udisksctl loop-setup -f disk.raw | python3 -c "import sys; print(sys.stdin.read().split()[-1][:-1])"))
	$(info loop device ${loop_device})
	$(eval mount_path := $(shell udisksctl mount -b $(loop_device)p1 | python3 -c "import sys; print(sys.stdin.read().split()[-1])"))
	rm -rf ${mount_path}/EFI ${mount_path}/boot_loader.efi ${mount_path}/startup.nsh
	mkdir -p ${mount_path}/EFI/Shell
	cp /usr/share/edk2-shell/x64/Shell_Full.efi ${mount_path}/EFI/shell/bootx64.efi
	mkdir -p ${mount_path}/EFI/Boot
	cp boot_loader.efi ${mount_path}/EFI/Boot/bootx64.efi
	cp boot_loader.efi ${mount_path}/
	cp kernel ${mount_path}/
	cp startup.nsh ${mount_path}/startup.nsh
	udisksctl unmount -b ${loop_device}p1
	udisksctl loop-delete -b ${loop_device}
	rm -f sda.vdi
	VBoxManage convertdd disk.raw sda.vdi --format VDI

clean:
	rm *.o *.so *.efi sdb.vdi
