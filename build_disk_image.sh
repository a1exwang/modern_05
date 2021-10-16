#!/bin/bash
set -xue
EFI_SHELL=$1
loop_device=$(udisksctl loop-setup -f disk.raw | python3 -c "import sys; print(sys.stdin.read().split()[-1][:-1])")
esp_partition=${loop_device}p1
udisksctl mount -b "${esp_partition}"
mount_path=$(mount | grep "${esp_partition}" | cut -d ' ' -f 3)
rm -rf "${mount_path}"/EFI "${mount_path}"/boot_loader.efi "${mount_path}"/startup.nsh
mkdir -p "${mount_path}"/EFI/Shell
cp "${EFI_SHELL}" "${mount_path}"/EFI/shell/bootx64.efi
mkdir -p "${mount_path}"/EFI/Boot
cp boot_loader.efi "${mount_path}"/EFI/Boot/bootx64.efi
cp boot_loader.efi "${mount_path}"
cp kernel "${mount_path}"
cp startup.nsh "${mount_path}"/startup.nsh
udisksctl unmount -b "${esp_partition}"
udisksctl loop-delete -b "${loop_device}"
