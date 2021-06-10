set(USER_COMPILE_OPTIONS -nostdlib -fno-exceptions -fno-builtin -fno-pie)
set(USER_LINK_OPTIONS -fno-builtin -nostdlib -fno-exceptions -Wl,--no-relax -static -Wl,-T -Wl,${CMAKE_CURRENT_SOURCE_DIR}/user.ld)
set(USER_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/sdk/include/sdk/include")

add_executable(user_init programs/init.cpp)
target_compile_options(user_init PRIVATE ${USER_COMPILE_OPTIONS})
target_link_options(user_init PRIVATE ${USER_LINK_OPTIONS})
target_include_directories(user_init PRIVATE ${USER_INCLUDE_DIRS})

add_custom_target(bundled_user_programs
        COMMAND ld -r -b binary -o ${CMAKE_CURRENT_BINARY_DIR}/user_init.bin.o user_init
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS user_init
        VERBATIM
        )

add_custom_target(bundled_busybox
        COMMAND ld -r -b binary -o ${CMAKE_CURRENT_BINARY_DIR}/busybox.bin.o busybox
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/3rdparty/busybox
        VERBATIM
        )
