cmake_minimum_required(VERSION 3.20)

project(hil_bench_fw C)

set(BOARD tm4c123 CACHE STRING "Target board (directory under boards/)")
set(FW_VERSION "0.1.0")

set(CPU_FLAGS -mcpu=cortex-m4 -mthumb -mfloat-abi=soft)

add_executable(firmware.elf
    src/main.c
    src/protocol.c
    src/libc_min.c
    boards/${BOARD}/startup.c
    boards/${BOARD}/board.c
)

target_include_directories(firmware.elf PRIVATE include)
target_compile_definitions(firmware.elf PRIVATE FW_VERSION="${FW_VERSION}")

target_compile_options(firmware.elf PRIVATE
    ${CPU_FLAGS}
    -std=c11
    -ffreestanding
    # Stop GCC turning the loops in libc_min.c back into calls to memset/memcpy.
    -fno-tree-loop-distribute-patterns
    -ffunction-sections
    -fdata-sections
    -Wall
    -Wextra
    -Werror
    -Os
    -g
)

target_link_options(firmware.elf PRIVATE
    ${CPU_FLAGS}
    -nostdlib
    -T${CMAKE_CURRENT_SOURCE_DIR}/boards/${BOARD}/linker.ld
    -Wl,--gc-sections
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/firmware.map
)

target_link_libraries(firmware.elf PRIVATE gcc)

add_custom_command(TARGET firmware.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary firmware.elf firmware.bin
    COMMAND ${CMAKE_SIZE} firmware.elf
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
