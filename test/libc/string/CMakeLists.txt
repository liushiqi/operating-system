cmake_minimum_required(VERSION 3.15)
project(project4_virtual_memory C)

file(GLOB STRING_SOURCES "*.c")
add_library(libc_string ${STRING_SOURCES})
target_include_directories(libc_string PRIVATE ../include)
target_compile_options(libc_string PUBLIC ${GENERAL_COMPILE_OPTIONS})
