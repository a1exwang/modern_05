#!/bin/bash
set -xue
VM=efidev
SERIAL_PORT_TCP_PORT=1235

make sda.vdi
vboxmanage controlvm ${VM} poweroff || true
sleep 0.3
vboxmanage storageattach ${VM} --storagectl SATA --port 0 --medium none
vboxmanage closemedium sda.vdi
vboxmanage storageattach ${VM} --storagectl SATA --port 0 --type hdd --medium $(pwd)/sda.vdi
#VirtualBoxVM --debug --startvm ${VM}

vboxmanage modifyvm ${VM} --uart1 0x3f8 4
vboxmanage modifyvm ${VM} --uartmode1 tcpserver ${SERIAL_PORT_TCP_PORT}
vboxmanage startvm ${VM} --type headless
# VirtualBoxVM --debug --startvm ${VM}
telnet 127.0.0.1 ${SERIAL_PORT_TCP_PORT}
