cmake_minimum_required(VERSION 3.15)
project(project4_virtual_memory C ASM)

set(CMAKE_C_STANDARD 11)

add_library(crt0 crt/crt0.S)
target_include_directories(crt0 PRIVATE include)
target_compile_options(crt0 PRIVATE ${GENERAL_COMPILE_OPTIONS})
target_link_options(crt0 PRIVATE ${GENERAL_LINK_OPTIONS})

add_subdirectory(ctype)
add_subdirectory(string)

file(GLOB STDIO_SOURCES "stdio/*.c")

set(LIBC_SOURCES
        thread/barrier.c
        thread/condition_variable.c
        thread/mutex.c
        thread/mailbox.c
        thread/semaphore.c
        thread/spinlock.c
        atol.c
        gets.c
        rand.c
        strerror.c
        syscall.c
        syscall.S
        time.c
        printf.c)
add_library(riscv_libc STATIC ${LIBC_SOURCES})
target_include_directories(riscv_libc PUBLIC include)
target_link_libraries(riscv_libc PUBLIC libc_ctype libc_string)
target_compile_options(riscv_libc PUBLIC ${GENERAL_COMPILE_OPTIONS})
target_link_options(riscv_libc PUBLIC ${GENERAL_LINK_OPTIONS})
