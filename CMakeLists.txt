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
    set(KERNEL_LINK_LIBRARIES -lstdc++)
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
add_subdirectory(device)
add_subdirectory(fs)
add_subdirectory(lib)
add_subdirectory(mm)
add_subdirectory(net)
add_subdirectory(user)

get_target_property(BUNDLED_USER_PROGRAMS_BINARY_DIR bundled_user_programs BINARY_DIR)

add_library(kernellib
        kernel.cpp irq.S irq.cpp init/init.cpp init/init.S debug.cpp process.cpp syscall.cpp)
target_compile_options(kernellib PUBLIC ${KERNEL_COMPILE_OPTIONS})
target_link_options(kernellib PUBLIC ${KERNEL_LINK_OPTIONS})
target_include_directories(kernellib PUBLIC ${KERNEL_INCLUDE_DIRS})
add_dependencies(kernellib bundled_user_programs bundled_busybox)

# These object files cannot be used. Just leave them here.
# The crtbeginT versions are for static executables
execute_process(COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=crtbeginT.o OUTPUT_VARIABLE CRTBEGIN_O ERROR_VARIABLE CRTBEGIN_ERR OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=crtend.o OUTPUT_VARIABLE CRTEND_O ERROR_VARIABLE CRTEND_ERR OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=crti.o OUTPUT_VARIABLE CRTI_O ERROR_VARIABLE CRTI_ERR OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=crtn.o OUTPUT_VARIABLE CRTN_O ERROR_VARIABLE CRTN_ERR OUTPUT_STRIP_TRAILING_WHITESPACE)

add_executable(kernel dummy.cpp)
# NODE: Don't change the order of these crt objs
# https://wiki.osdev.org/Calling_Global_Constructors
target_link_libraries(kernel
        kernellib cxx device common fs lib mm net
        ${BUNDLED_USER_PROGRAMS_BINARY_DIR}/user_init.bin.o
        ${BUNDLED_USER_PROGRAMS_BINARY_DIR}/busybox.bin.o
        ${KERNEL_LINK_LIBRARIES}
        )
set_target_properties(kernel PROPERTIES LINK_DEPENDS ${CMAKE_SOURCE_DIR}/kernel.ld)
