#!/bin/bash
set -xue
make disk.raw
# QEMU=../qemu/build/qemu-system-x86_64
QEMU=qemu-system-x86_64
sudo $QEMU \
  -machine q35 \
  -no-reboot \
  -m 5G \
  -vga std \
  -net nic,model=rtl8139,macaddr=0a:01:0e:0a:01:0e \
  -net bridge,br=br0 \
  -drive if=pflash,format=raw,readonly=on,file=/usr/share/edk2-ovmf/x64/OVMF_CODE.fd \
  -drive if=pflash,format=raw,file=OVMF_VARS.fd \
  -drive format=raw,file=disk.raw \
  -device intel-iommu,intremap=on \
  $@

#-serial stdio \
