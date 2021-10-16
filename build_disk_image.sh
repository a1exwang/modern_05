#!/bin/bash
set -xue
loop_device=$(udisksctl loop-setup -f disk.raw | python3 -c "import sys; print(sys.stdin.read().split()[-1][:-1])")
esp_partition=${loop_device}p1
mount_path=$(udisksctl mount -b "${esp_partition}" | python3 -c "import sys; print(sys.stdin.read().split()[-1])")
rm -rf "${mount_path}"/EFI "${mount_path}"/boot_loader.efi "${mount_path}"/startup.nsh
mkdir -p "${mount_path}"/EFI/Shell
cp /usr/share/edk2-shell/x64/Shell_Full.efi "${mount_path}"/EFI/shell/bootx64.efi
mkdir -p "${mount_path}"/EFI/Boot
cp boot_loader.efi "${mount_path}"/EFI/Boot/bootx64.efi
cp boot_loader.efi "${mount_path}"
cp kernel "${mount_path}"
cp startup.nsh "${mount_path}"/startup.nsh
udisksctl unmount -b "${esp_partition}"
udisksctl loop-delete -b "${loop_device}"
