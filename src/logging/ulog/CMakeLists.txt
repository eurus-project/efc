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

# Generate message sources
set(YAML_DIR ${CMAKE_SOURCE_DIR}/messages/ulog)
set(GENERATED_DIR ${CMAKE_BINARY_DIR}/ulog/generated)
set(GENERATOR_SCRIPT ${CMAKE_SOURCE_DIR}/tools/ulog_gen.py)

file(MAKE_DIRECTORY ${GENERATED_DIR})

file(GLOB YAML_FILES ${YAML_DIR}/*.yaml)

set(GENERATED_SOURCES "")
foreach(file ${YAML_FILES})
    get_filename_component(filename ${file} NAME)
    string(REPLACE ".yaml" ".c" generated_source ${filename})
    string(PREPEND generated_source "ulog_")
    string(PREPEND generated_source ${GENERATED_DIR}/)

    add_custom_command(
        OUTPUT ${generated_source}
        COMMAND ${VENV_PYTHON} ${GENERATOR_SCRIPT} ${file} ${GENERATED_DIR}
        DEPENDS ${file} ${GENERATOR_SCRIPT}
        COMMENT "Generating C and H files from YAML definitions"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )

    list(APPEND GENERATED_SOURCES ${generated_source})
endforeach()

# Create the library
add_library(ulog
STATIC
    ulog.c
    ${GENERATED_SOURCES}
)

target_include_directories(ulog
PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${GENERATED_DIR}
)

target_link_libraries(ulog PUBLIC zephyr_interface)
