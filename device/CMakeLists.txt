add_library(device pci.cpp pci_devices.cpp apic.cpp rtl8139.cpp efi.cpp ahci.cpp)
target_compile_options(device PUBLIC ${KERNEL_COMPILE_OPTIONS})
target_include_directories(device PUBLIC ${KERNEL_INCLUDE_DIRS})
