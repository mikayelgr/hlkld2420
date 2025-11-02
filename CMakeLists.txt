cmake_minimum_required(VERSION 3.16)
include(pico_sdk_import.cmake)

project(ld2420 LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(LD2420_WITH_PICO "Build for Raspberry Pi Pico" OFF)
option(LD2420_WITH_HOST "Build for Host" ON)

pico_sdk_init()

add_library(ld2420 src/ld2420.c src/ld2420_pico.c)
target_include_directories(ld2420 PUBLIC include/)
target_link_libraries(ld2420 PUBLIC pico_stdlib hardware_uart)
target_compile_definitions(ld2420 PUBLIC LD2420_WITH_PICO LD2420_WITH_HOST)
