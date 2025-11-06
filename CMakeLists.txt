cmake_minimum_required(VERSION 3.16)

project(hlkld2420 C CXX ASM)

add_library(ld2420_iface INTERFACE)
target_include_directories(ld2420_iface INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include> # Used when the library is installed
)

install(
    DIRECTORY include/
    DESTINATION include/
)

# Adding the src directory as core library when compiling
add_subdirectory(core)

# Pico platform support
if(CMAKE_SYSTEM_NAME STREQUAL "PICO")
    if(NOT DEFINED PICO_SDK_PATH)
        set(PICO_SDK_PATH $ENV{PICO_SDK_PATH} CACHE PATH "Path to the Pico SDK" FORCE)
    endif()

    if(NOT DEFINED PICO_SDK_PATH)
        message(FATAL_ERROR "PICO_SDK_PATH environment variable is not set. Please set it to the path of the Pico SDK.")
    endif()
    message(STATUS "PICO_SDK_PATH is ${PICO_SDK_PATH}")
    add_subdirectory(platform/pico)
else()
    message(STATUS "Pico SDK path not set. Skipping Pico build.")
endif()

if(BUILD_TESTING)
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif()
