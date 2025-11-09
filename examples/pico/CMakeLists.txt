cmake_minimum_required(VERSION 3.13)

# This example is built completely independently with the Pico toolchain
# It pulls in the ld2420 library as an external dependency

# Set up the Pico SDK before the project() call
if(NOT DEFINED PICO_SDK_PATH)
    set(PICO_SDK_PATH $ENV{PICO_SDK_PATH} CACHE PATH "Path to the Pico SDK" FORCE)
endif()

if(NOT DEFINED PICO_SDK_PATH)
    message(FATAL_ERROR "PICO_SDK_PATH environment variable is not set. Please set it to the path of the Pico SDK.")
endif()

# Initialize the Pico SDK
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(ld2420_pico_example C CXX ASM)

# Initialize Pico SDK
pico_sdk_init()

include(FetchContent)

# Use local hlkld2420 repository as is
set(HLKLD2420_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../..")
message(STATUS "Using local hlkld2420 repository")

FetchContent_Declare(hlkld2420
    SOURCE_DIR "${HLKLD2420_SOURCE}"
)
FetchContent_MakeAvailable(hlkld2420)

# Create the example executable
add_executable(basic_example
    basic_example.c
)

# Link to the pico platform implementation
target_link_libraries(basic_example PRIVATE
    ld2420_core
    ld2420_pico
    pico_stdlib
    pico_multicore
)

# Enable USB and UART I/O
pico_enable_stdio_usb(basic_example 1)
pico_enable_stdio_uart(basic_example 1)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(basic_example)
