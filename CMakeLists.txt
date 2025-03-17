cmake_minimum_required(VERSION 3.16)
project(nvstd)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(nvstd STATIC ${CMAKE_CURRENT_LIST_DIR}/core.c)
target_compile_options(nvstd PRIVATE -g -Wall -Wextra -pedantic)
target_link_libraries(nvstd SDL2)