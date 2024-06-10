cmake_minimum_required(VERSION 3.15)
project(project4_virtual_memory C ASM)

set(CMAKE_C_STANDARD 11)

add_subdirectory(libc)

function(add_user_program PROGRAM_NAME)
    set(USER_PROGRAMS ${USER_PROGRAMS} ${PROGRAM_NAME} PARENT_SCOPE)
    add_executable(${PROGRAM_NAME} ${PROGRAM_NAME}.c)
    target_link_libraries(${PROGRAM_NAME} PUBLIC riscv_libc crt0)
    target_compile_options(${PROGRAM_NAME} PUBLIC ${GENERAL_COMPILE_OPTIONS})
    target_link_options(${PROGRAM_NAME} PUBLIC ${GENERAL_LINK_OPTIONS})
    message("-- Added user program ${PROGRAM_NAME}")
endfunction()

add_user_program(sh)
add_user_program(rw)
add_user_program(fly)
add_user_program(ps)
add_user_program(clear)
add_user_program(ls)
add_user_program(touch)
add_user_program(cat)
add_user_program(mkdir)
add_user_program(test_fs)

set(USER_PROGRAMS ${USER_PROGRAMS} PARENT_SCOPE)
