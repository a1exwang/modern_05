add_library(cxx exceptions.cpp)
target_compile_options(cxx PUBLIC ${KERNEL_COMPILE_OPTIONS})
target_include_directories(cxx PUBLIC ${KERNEL_INCLUDE_DIRS})
