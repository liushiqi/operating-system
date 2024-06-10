cmake_minimum_required(VERSION 3.15)
project(project4_virtual_memory C)

file(GLOB CTYPE_SOURCES "*.c")
add_library(libc_ctype ${CTYPE_SOURCES})
target_include_directories(libc_ctype PRIVATE ../include)
target_compile_options(libc_ctype PUBLIC ${GENERAL_COMPILE_OPTIONS})
