add_library(common unwind.cpp)
target_compile_options(common PUBLIC ${KERNEL_COMPILE_OPTIONS})
target_include_directories(common PUBLIC ${KERNEL_INCLUDE_DIRS})
