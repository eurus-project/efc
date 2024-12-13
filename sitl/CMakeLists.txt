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

add_executable(sitl
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sitl_main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sitl_sim.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sitl_jmavsim_sim.cpp
)

target_include_directories(sitl PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)