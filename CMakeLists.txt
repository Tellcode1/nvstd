cmake_minimum_required(VERSION 3.16)
project(nvstd)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(nvstd STATIC ${CMAKE_CURRENT_LIST_DIR}/core.c)
target_compile_options(nvstd PRIVATE -g -Wall -Wextra -pedantic)
target_link_libraries(nvstd SDL2 z)