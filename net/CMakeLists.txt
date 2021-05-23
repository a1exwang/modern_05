add_library(net arp.cpp)
target_compile_options(net PUBLIC ${KERNEL_COMPILE_OPTIONS})
target_include_directories(net PUBLIC ${KERNEL_INCLUDE_DIRS})
