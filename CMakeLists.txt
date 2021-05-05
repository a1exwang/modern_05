cmake_minimum_required(VERSION 3.8)
project(a1ex-kernel C CXX ASM)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_FLAGS "-nostdlib")
set(CMAKE_CXX_FLAGS "-nostdlib -fno-exceptions -fno-builtin")
set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/kernel.ld")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-T -Wl,${LINKER_SCRIPT} -Wl,--no-relax -static")
SET(BUILD_SHARED_LIBS OFF)
include_directories(include)

add_subdirectory(lib)
add_subdirectory(mm)

add_executable(kernel kernel.cpp irq.S irq.cpp init/init.cpp init/init.S debug.cpp process.cpp syscall.cpp)
target_link_libraries(kernel lib mm)
set_target_properties(kernel PROPERTIES LINK_DEPENDS ${CMAKE_SOURCE_DIR}/kernel.ld)
