add_library(mm mm.cpp page_alloc.cpp)
target_compile_options(mm PUBLIC ${KERNEL_COMPILE_OPTIONS})
target_include_directories(mm PUBLIC ${KERNEL_INCLUDE_DIRS})
