cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zfc
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
