#!/bin/bash
set -xue
VM=efidev
vboxmanage controlvm efidev poweroff || true
sleep 0.3
vboxmanage storageattach efidev --storagectl SATA --port 0 --medium none
vboxmanage closemedium sda.vdi
vboxmanage storageattach efidev --storagectl SATA --port 0 --type hdd --medium $(pwd)/sda.vdi
vboxmanage startvm ${VM}
