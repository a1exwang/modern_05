add_library(device pci.cpp apic.cpp rtl8139.cpp)
target_compile_options(device PUBLIC ${KERNEL_COMPILE_OPTIONS})
target_include_directories(device PUBLIC ${KERNEL_INCLUDE_DIRS})
