cmake_minimum_required(VERSION 3.13)

# Set up the Pico SDK before the project() call
if(NOT DEFINED PICO_SDK_PATH)
    set(PICO_SDK_PATH $ENV{PICO_SDK_PATH} CACHE PATH "Path to the Pico SDK" FORCE)
endif()

if(NOT DEFINED PICO_SDK_PATH)
    message(FATAL_ERROR "PICO_SDK_PATH environment variable is not set. Please set it to the path of the Pico SDK.")
endif()

# Including and initializing the Pico SDK, which exposes all the necessary targets and libraries
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(ld2420_pico VERSION 1.0.0 LANGUAGES C CXX ASM)

pico_sdk_init()

message(STATUS "Building ld2420_core from source")
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../src ${CMAKE_CURRENT_BINARY_DIR}/ld2420_core)

# Define the Pico implementation library
add_library(ld2420_pico
    ld2420_pico.c
    include/ld2420/platform/pico/ld2420_pico.h
)
target_link_libraries(ld2420_pico PUBLIC ld2420_core)
target_include_directories(ld2420_pico PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# Link the necessary platform dependencies
target_link_libraries(ld2420_pico PUBLIC
    pico_stdlib
    pico_sync
    hardware_uart
    hardware_gpio
    hardware_irq
)
