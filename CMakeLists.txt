cmake_minimum_required(VERSION 3.8)
project(a1ex-kernel C CXX ASM)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_FLAGS "-nostdlib -fshort-wchar")
set(CMAKE_CXX_FLAGS "-nostdlib -fshort-wchar -fno-exceptions -Wno-builtin-declaration-mismatch")
set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/kernel.ld")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-T -Wl,${LINKER_SCRIPT} -static")
SET(BUILD_SHARED_LIBS OFF)
include_directories(include)

add_subdirectory(lib)

add_executable(kernel kernel.cpp mm.cpp irq.S irq.cpp init/init.cpp debug.cpp)
target_link_libraries(kernel lib)

set_target_properties(kernel PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT})
