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
add_subdirectory(src core)

# Pico SDK setup
if(DEFINED PICO_SDK_PATH)
    message(STATUS "PICO_SDK_PATH is ${PICO_SDK_PATH}")
    add_subdirectory(platform/pico)
else()
    message(WARNING "Pico SDK path not found. Skipping Pico build.")
endif()
