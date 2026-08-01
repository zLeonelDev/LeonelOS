cmake_minimum_required(VERSION 3.20)

add_executable(leonelos-img
    image_tool.c
)

target_include_directories(leonelos-img PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/kernel
)

set_target_properties(leonelos-img PROPERTIES
    OUTPUT_NAME "leonelos-img"
)

add_custom_target(image-tool DEPENDS leonelos-img)