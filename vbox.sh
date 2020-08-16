#!/bin/bash
set -xue
vboxmanage storageattach efidev --storagectl SATA --port 0 --medium none
vboxmanage closemedium sda.vdi
vboxmanage storageattach efidev --storagectl SATA --port 0 --type hdd --medium $(pwd)/sda.vdi
