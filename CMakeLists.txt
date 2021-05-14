cmake_minimum_required(VERSION 3.13)
project(a1ex-kernel C CXX ASM)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 20)
set(BUILD_SHARED_LIBS OFF)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # For libc++, std::map is header only
    set(KERNEL_COMPILER_OPTIONS -stdlib=libc++)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # Link libstdc++, we only need the Rb_tree functions. Rely on the linker to remove other useless functions.
#    set(KERNEL_LINK_LIBRARIES -lstdc++)
else()
    message(FATAL_ERROR "Compiler ${CMAKE_CXX_COMPILER} is not supported")
endif()

set(KERNEL_LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/kernel.ld")
set(KERNEL_COMPILE_OPTIONS ${KERNEL_COMPILE_OPTIONS} -nostdlib -fno-exceptions -fno-builtin -fno-rtti -fno-stack-protector)
set(KERNEL_LINK_OPTIONS ${KERNEL_LINK_OPTIONS} -fno-builtin -nostdlib -fno-exceptions -Wl,-T -Wl,${KERNEL_LINKER_SCRIPT} -Wl,--no-relax -static)
#set(KERNEL_LINK_LIBRARIES ${KERNEL_LINK_LIBRARIES})
set(KERNEL_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/include")

add_subdirectory(cxx)
add_subdirectory(common)
add_subdirectory(fs)
add_subdirectory(lib)
add_subdirectory(mm)
add_subdirectory(user)
add_subdirectory(device)

get_target_property(BUNDLED_USER_PROGRAMS_BINARY_DIR bundled_user_programs BINARY_DIR)
message("output ${BUNDLED_USER_PROGRAMS_BINARY_DIR}")

add_executable(kernel
        kernel.cpp irq.S irq.cpp init/init.cpp init/init.S debug.cpp process.cpp syscall.cpp
        )
target_compile_options(kernel PUBLIC ${KERNEL_COMPILE_OPTIONS})
target_link_options(kernel PUBLIC ${KERNEL_LINK_OPTIONS})
target_link_libraries(kernel
        cxx common fs lib mm device
        ${BUNDLED_USER_PROGRAMS_BINARY_DIR}/user_init.bin.o
        ${KERNEL_LINK_LIBRARIES}
        )
target_include_directories(kernel PUBLIC ${KERNEL_INCLUDE_DIRS})
add_dependencies(kernel bundled_user_programs)
set_target_properties(kernel PROPERTIES LINK_DEPENDS ${CMAKE_SOURCE_DIR}/kernel.ld)
