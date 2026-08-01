cmake_minimum_required(VERSION 3.20)

file(GLOB SDK_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

add_library(leonelos-sdk ${SDK_SOURCES})
target_include_directories(leonelos-sdk PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/kernel
    ${CMAKE_SOURCE_DIR}/src/api
    ${CMAKE_SOURCE_DIR}/src/libc
)

add_library(LeonelOS::SDK ALIAS leonelos-sdk)