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

cmake_minimum_required(VERSION 3.0)

project (mixer VERSION 1.0.0)

set(SOURCES 
    mixer.c    
)

# Create the library
add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_include_directories(${PROJECT_NAME}
SYSTEM PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ZEPHYR_BASE}/include
)

target_link_libraries(${PROJECT_NAME}
PUBLIC
    zephyr_interface
    esc
)