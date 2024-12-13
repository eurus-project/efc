# 
# This file is part of the efc project <https://github.com/eurus-project/efc/>.
# Copyright (c) 2024 The efc developers.
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(efc
    VERSION 0.1
    LANGUAGES C
)

target_sources(app
PRIVATE
    src/main.c
)

# SITL
set(SITL_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/sitl/cmake/linux-x86_64.cmake)
add_custom_target(
  sitl
  COMMAND ${CMAKE_COMMAND}
    -DCMAKE_TOOLCHAIN_FILE=${SITL_TOOLCHAIN_FILE}
    -B${CMAKE_BINARY_DIR}/sitl
    -S${CMAKE_SOURCE_DIR}/sitl
  COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/sitl
  SOURCES
    ${CMAKE_SOURCE_DIR}/sitl/CMakeLists.txt
  COMMENT "Building SITL target with custom toolchain"
)
