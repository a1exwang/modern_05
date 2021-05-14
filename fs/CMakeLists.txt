add_library(fs mem_fs_node.cpp)
target_compile_options(fs PUBLIC ${KERNEL_COMPILE_OPTIONS})
target_include_directories(fs PUBLIC ${KERNEL_INCLUDE_DIRS})
