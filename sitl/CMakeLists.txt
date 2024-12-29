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

set(SITL_MAIN_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/main)
set(SITL_CORE_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/core)
set(SITL_SIM_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/sim)

set(SITL_MAIN_SRC
    ${SITL_MAIN_SRC_DIR}/sitl_main.cpp
)

set(SITL_CORE_SRC
    ${SITL_CORE_SRC_DIR}/sitl_sim.cpp
    ${SITL_CORE_SRC_DIR}/sitl_efc.cpp
    ${SITL_CORE_SRC_DIR}/sitl_tcp_conn.cpp
)

set(SITL_SIM_SRC
    ${SITL_SIM_SRC_DIR}/sitl_jmavsim_sim.cpp
)

if(LINUX) 
    set(SITL_PLATFORM_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/linux)
    set(SITL_PLATFORM_SRC
        ${SITL_PLATFORM_SRC_DIR}/sitl_tcp_conn_linux.cpp
    )
elseif(WIN32)
    set(SITL_PLATFORM_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/windows)
    set(SITL_PLATFORM_SRC
        ${SITL_PLATFORM_SRC_DIR}/sitl_tcp_conn_win.cpp
    )
else()
    message(ERROR "Unsupported platform!")
endif()



add_executable(sitl
    ${SITL_MAIN_SRC}
    ${SITL_CORE_SRC}
    ${SITL_SIM_SRC}
    ${SITL_PLATFORM_SRC}
)

target_include_directories(sitl PRIVATE
    ${SITL_CORE_SRC_DIR}
    ${SITL_SIM_SRC_DIR}
    ${SITL_PLATFORM_SRC_DIR}
)

target_precompile_headers(sitl PRIVATE
    ${SITL_CORE_SRC_DIR}/sitl_pch.h
)