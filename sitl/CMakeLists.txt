cmake_minimum_required(VERSION 3.20.0)
project(sitl)

if(DEFINED USE_GAZEBO AND USE_GAZEBO EQUAL "1")
    add_definitions(-DGAZEBO)
else()
    add_definitions(-DJMAVSIM)
endif()

add_executable(sitl
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sitl_main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sim/sitl_sim.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sim/sitl_jmavsim_sim.cpp
)

target_include_directories(sitl PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)