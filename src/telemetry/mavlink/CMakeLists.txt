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

# Generate message headers
set(XML_FILES
    ${CMAKE_SOURCE_DIR}/tools/mavlink/message_definitions/v1.0/common.xml
)
set(GENERATED_DIR ${CMAKE_BINARY_DIR}/mavlink/generated)
set(GENERATOR_SCRIPT ${CMAKE_SOURCE_DIR}/tools/mavlink/pymavlink/tools/mavgen.py)

file(MAKE_DIRECTORY ${GENERATED_DIR})

set(DIALECT_HEADERS "")
foreach(file ${XML_FILES})
    get_filename_component(dialect_name ${file} NAME_WLE)
    set(DIALECT_HEADER ${GENERATED_DIR}/${dialect_name}/${dialect_name}.h)

    add_custom_command(
        OUTPUT ${DIALECT_HEADER}
        COMMAND ${VENV_PYTHON} ${GENERATOR_SCRIPT} --lang=C --wire-protocol=2.0 -o ${GENERATED_DIR} ${file}
        DEPENDS ${file} ${GENERATOR_SCRIPT}
        COMMENT "Generating mavlink dialect from ${file}"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )

    list(APPEND DIALECT_HEADERS ${DIALECT_HEADER})
endforeach()

# Create the library
add_library(mavlink
STATIC
    mavlink.c
    ${DIALECT_HEADERS} # Only used to make the library depend on all generation steps
)

target_include_directories(mavlink
SYSTEM PUBLIC
    ${GENERATED_DIR}
)

target_compile_definitions(mavlink
PUBLIC
    # Conversions will be handled using different libraries
    MAVLINK_NO_CONVERSION_HELPERS=true
    # Good for both unreliable links and security
    MAVLINK_CHECK_MESSAGE_LENGTH=true
    # Use global buffers and statuses per channel, with helpers to point to them
    # instead of them being defined in every source file mavlink is included in
    # due to the unfortunate design of mavlink
    MAVLINK_GET_CHANNEL_STATUS=mavlink_get_channel_status
    MAVLINK_GET_CHANNEL_BUFFER=mavlink_get_channel_buffer
)

target_link_libraries(mavlink PUBLIC zephyr_interface)
