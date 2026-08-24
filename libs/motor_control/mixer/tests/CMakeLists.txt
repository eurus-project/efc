# This file is part of the efc project <https://github.com/eurus-project/efc/>.
# Copyright (c) (2024 - Present), The efc developers.
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

# Host-side unit tests for the mixer library. Not part of the Zephyr build.

cmake_minimum_required(VERSION 3.20.0)

project(mixer_tests LANGUAGES C CXX)

set(EFC_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../..)
set(DOCTEST_DIR ${EFC_ROOT_DIR}/libs/third_party/doctest)

if(NOT EXISTS ${DOCTEST_DIR}/doctest/doctest.h)
    message(FATAL_ERROR
        "doctest submodule is missing (${DOCTEST_DIR}/doctest.h not found). "
        "Run: git submodule update --init libs/third_party/doctest"
    )
endif()

add_executable(mixer_test
    mixer_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../mixer.c
)

target_include_directories(mixer_test
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${DOCTEST_DIR}
)

set_target_properties(mixer_test PROPERTIES
    C_STANDARD 99
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

target_compile_options(mixer_test PRIVATE -Wall -Wextra)

target_link_libraries(mixer_test PRIVATE m)

enable_testing()
add_test(NAME mixer COMMAND mixer_test)
