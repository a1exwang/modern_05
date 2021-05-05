add_library(lib
        string.cpp
        serial_port.cpp
        port_io.cpp
        cpu_utils.cpp
        )
target_compile_options(lib PUBLIC ${KERNEL_COMPILE_OPTIONS})
target_include_directories(lib PUBLIC ${KERNEL_INCLUDE_DIRS})
