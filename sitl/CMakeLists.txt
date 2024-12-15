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
project(sitl)

# TODO: Add one CMake variable that will hold path to Flight Controler src
# from where control and estimation algorithms will be provided to sitl. 
# Something like -DEFC_PATH=<path> 

if(DEFINED USE_GAZEBO AND USE_GAZEBO EQUAL "1")
    add_definitions(-DGAZEBO)
    message(WARNING "EFC Sitl currently doesn't support Gazebo simulator! "
    "Sitl application won't do anything!")
else()
    add_definitions(-DJMAVSIM)
    message(STATUS "=====================================")
    message(STATUS "JMavSim simulation selected for Sitl.")
    message(STATUS "=====================================")
endif()

# Change this possibly
if(NOT DEFINED MAVLINK_DIR)
    set(MAVLINK_DIR ".")
else()
    string(FIND ${MAVLINK_DIR} "/" PREFIX_POS)
    if(PREFIX_POS EQUAL 0)
        message(WARNING "Path to mavlink directory must be relative to Sitl root directory!")
    endif()
endif()

add_executable(sitl
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sitl_main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sitl_sim.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sitl_jmavsim_sim.cpp
)

target_include_directories(sitl PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/${MAVLINK_DIR}
)

target_precompile_headers(sitl PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sitl_pch.h
)