cmake_minimum_required(VERSION 3.13)
project(create_image LANGUAGES C CXX)

set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)

add_executable(createimage createimage.c)

add_executable(mkfs.naivefs mkfs.naivefs.c)

add_executable(mount.naivefs mount.naivefs.c)
target_link_libraries(mount.naivefs PRIVATE fuse3)
target_compile_options(mount.naivefs PRIVATE -Wall -Wextra -Wno-unused-parameter)
