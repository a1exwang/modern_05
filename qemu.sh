#!/bin/bash
set -xue
make disk.raw
qemu-system-x86_64 \
  -nographic \
  -no-reboot \
  -m 5G \
  -vga std \
  -drive if=pflash,format=raw,readonly=on,file=/usr/share/edk2-ovmf/x64/OVMF_CODE.fd \
  -drive if=pflash,format=raw,file=OVMF_VARS.fd \
  -drive format=raw,file=disk.raw \
  $@

