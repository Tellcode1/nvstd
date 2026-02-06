cmake_minimum_required(VERSION 3.16)
project(nvstd)

set(CFLAGS -g -Wall -Wextra -Wpedantic -ggdb3)

find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
include_directories(${SDL3_INCLUDE_DIRS})
include_directories(${SDL3_image_INCLUDE_DIRS})

set(NVSTD_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)

set(CORE_SOURCES
  ${NVSTD_SRC_DIR}/containers/bitset.c
  ${NVSTD_SRC_DIR}/containers/hashmap.c
  ${NVSTD_SRC_DIR}/containers/idlist.c
  ${NVSTD_SRC_DIR}/containers/list.c
  ${NVSTD_SRC_DIR}/containers/rectpack.c
  ${NVSTD_SRC_DIR}/core.c
  ${NVSTD_SRC_DIR}/file.c
  ${NVSTD_SRC_DIR}/print.c
  ${NVSTD_SRC_DIR}/rand.c
  ${NVSTD_SRC_DIR}/strconv.c
  ${NVSTD_SRC_DIR}/stream.c
  ${NVSTD_SRC_DIR}/string.c
)

set(CORE_LIBS
  m
)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(nvstd STATIC ${CORE_SOURCES})
target_compile_options(nvstd PRIVATE ${CFLAGS})
target_link_libraries(nvstd ${CORE_LIBS})
