cmake_minimum_required(VERSION 3.16)
project(nvstd)

find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})

find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
include_directories(${SDL3_INCLUDE_DIRS})
include_directories(${SDL3_image_INCLUDE_DIRS})

set(CORE_SOURCES
  ${CMAKE_CURRENT_LIST_DIR}/core.c
  ${CMAKE_CURRENT_LIST_DIR}/containers.c
)

set(CORE_LIBS
  z
  m
  SDL3
  SDL3_image
)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(nvstd STATIC ${CORE_SOURCES})
target_compile_options(nvstd PRIVATE -g -Wall -Wextra -pedantic)
target_link_libraries(nvstd ${CORE_LIBS})
