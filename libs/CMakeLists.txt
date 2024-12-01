cmake_minimum_required(VERSION 3.0)

project (esc VERSION 1.0.0)

set(SOURCES 
    esc.c    
)

add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_include_directories(${PROJECT_NAME}
SYSTEM PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ZEPHYR_BASE}/include
)

target_link_libraries(${PROJECT_NAME}
PUBLIC
    zephyr_interface
)