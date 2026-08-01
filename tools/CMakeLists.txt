cmake_minimum_required(VERSION 3.20)

add_subdirectory(image_tool)

add_custom_target(tools DEPENDS leonelos-img)
