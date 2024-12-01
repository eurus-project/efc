# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(BOARD_ROOT ${CMAKE_SOURCE_DIR})

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zfc
    VERSION 0.1
    LANGUAGES C
)

target_sources(app
PRIVATE
    src/main.c
)

add_subdirectory(libs)

target_link_libraries(app
    PRIVATE
    # Libs
    esc
)

