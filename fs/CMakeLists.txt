add_library(fs initramfs.cpp tree.cpp)
target_compile_options(fs PUBLIC ${KERNEL_COMPILE_OPTIONS})
target_include_directories(fs PUBLIC ${KERNEL_INCLUDE_DIRS})
