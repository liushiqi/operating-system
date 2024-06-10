cmake_minimum_required(VERSION 3.15)
project(project6_file_system C ASM)

set(CMAKE_C_STANDARD 11)

set(CMAKE_SYSTEM_NAME none)
set(CMAKE_SYSTEM_PROCESSOR riscv64)

set(CMAKE_C_COMPILER_TARGET riscv64-unknown-elf)
set(CMAKE_ASM_COMPILER_TARGET riscv64-unknown-elf)

set(BOOTLOADER_ENTRY_POINT 0x10200000)
set(BOOTLOADER_SECOND_STAGE_ENTRY_POINT 0x10201000)
set(KERNEL_ENTRY_POINT 0xffffffc020000000)

set(GENERAL_COMPILE_OPTIONS -Wall -Wextra -mcmodel=medany -nostdinc -fvar-tracking -ffreestanding)
set(GENERAL_LINK_OPTIONS -nostdlib)

set(MOUNT_POINT /media/liu/naivefs)

add_executable(bootloader boot/bootloader.S)
target_include_directories(bootloader PRIVATE include)
target_compile_options(bootloader PRIVATE ${GENERAL_COMPILE_OPTIONS})
target_link_options(bootloader PRIVATE ${GENERAL_LINK_OPTIONS} -Wl,-T${CMAKE_SOURCE_DIR}/linker.ld -Wl,-Ttext=${BOOTLOADER_ENTRY_POINT} -Wl,--entry=_start)

add_subdirectory(tool)
add_subdirectory(test)

set(KERNEL_SOURCES
        init/init.S
        init/init.c
        interrupt/entry.S
        interrupt/interrupt.c
        interrupt/syscall.c
        driver/fdt.c
        driver/screen.c
        fs/naivefs.c
        fs/dirname.c
        string/memchr.c
        string/memcmp.c
        string/memcpy.c
        string/memmove.c
        string/memrchr.c
        string/memset.c
        string/stpcpy.c
        string/stpncpy.c
        string/strchr.c
        string/strchrnul.c
        string/strcmp.c
        string/strcpy.c
        string/strcspn.c
        string/strlen.c
        string/strncmp.c
        string/strncpy.c
        string/strcat.c
        string/strnlen.c
        string/strrchr.c
        string/strspn.c
        string/strstr.c
        string/strtok.c
        printf/printf.c
        futex/futex.c
        futex/lock.c
        memory/memory.c
        scheduler/scheduler.c
        scheduler/timer.c
        scheduler/unblock.c
        scheduler/sleep.c
        scheduler/exit.c
        scheduler/kill.c
        scheduler/wait_pid.c
        scheduler/get_process_info.c
        scheduler/block.c
        scheduler/exec.c)
add_executable(kernel ${KERNEL_SOURCES})
target_include_directories(kernel PUBLIC include)
target_compile_options(kernel PUBLIC ${GENERAL_COMPILE_OPTIONS})
target_link_options(kernel PUBLIC ${GENERAL_LINK_OPTIONS} -Wl,-T${CMAKE_SOURCE_DIR}/linker.ld -Wl,-Ttext=${KERNEL_ENTRY_POINT})

set(KERNEL_FIRST_STAGE_SOURCE
        boot/second_stage.c
        boot/second_stage.S
        boot/entry.S
        printf/printf.c
        fs/naivefs.c
        fs/dirname.c
        string/memchr.c
        string/memcmp.c
        string/memcpy.c
        string/memmove.c
        string/memrchr.c
        string/memset.c
        string/stpcpy.c
        string/stpncpy.c
        string/strchr.c
        string/strchrnul.c
        string/strcmp.c
        string/strcpy.c
        string/strcspn.c
        string/strlen.c
        string/strncmp.c
        string/strncpy.c
        string/strcat.c
        string/strnlen.c
        string/strrchr.c
        string/strspn.c
        string/strstr.c
        string/strtok.c)
add_executable(main ${KERNEL_FIRST_STAGE_SOURCE})
target_include_directories(main PUBLIC include)
target_compile_options(main PUBLIC ${GENERAL_COMPILE_OPTIONS})
target_link_options(main PUBLIC ${GENERAL_LINK_OPTIONS} -Wl,-T${CMAKE_SOURCE_DIR}/linker.ld -Wl,--entry=_start -Wl,-Ttext=${BOOTLOADER_SECOND_STAGE_ENTRY_POINT})

add_custom_target(image ALL
        COMMAND $<TARGET_FILE:createimage> --extended ${CMAKE_BINARY_DIR}/bootloader ${CMAKE_BINARY_DIR}/main
        DEPENDS createimage bootloader main ${USER_PROGRAMS} kernel
        COMMENT "Creating image file"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

set(DISK /dev/sdc)
add_custom_target(floppy ALL
        COMMAND bash -c \"fdisk -l ${DISK} && dd if=${CMAKE_BINARY_DIR}/image of=${DISK}2 conv=notrunc\"
        DEPENDS image
        COMMENT "Write image to sd card"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

set_property(DIRECTORY PROPERTY ADDITIONAL_MAKE_CLEAN_FILES createimage image)
