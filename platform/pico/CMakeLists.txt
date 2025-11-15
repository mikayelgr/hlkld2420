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

# Find the ld2420_core library
# First try to find it as an installed package
find_package(ld2420_core QUIET)

if(NOT ld2420_core_FOUND)
    # If not found as a package, try to add it as a subdirectory
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../core/CMakeLists.txt")
        message(STATUS "Building ld2420_core from source")
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../core ${CMAKE_CURRENT_BINARY_DIR}/ld2420_core)
    else()
        message(FATAL_ERROR "ld2420_core not found. Please install it or ensure the core directory is available.")
    endif()
endif()

# Define the Pico implementation library
add_library(ld2420_pico ld2420_pico.c include/ld2420/platform/pico/ld2420_pico.h)
target_link_libraries(ld2420_pico PUBLIC ld2420_core)
target_include_directories(ld2420_pico PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Link the necessary platform dependencies
target_link_libraries(ld2420_pico PUBLIC
    pico_stdlib
    hardware_uart
    hardware_gpio
)

# # Installation rules
# install(TARGETS ld2420_pico
#     EXPORT ld2420_picoTargets
#     LIBRARY DESTINATION lib
#     ARCHIVE DESTINATION lib
#     RUNTIME DESTINATION bin
#     INCLUDES DESTINATION include
# )

# install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
#     DESTINATION include
#     FILES_MATCHING PATTERN "*.h"
# )

# install(EXPORT ld2420_picoTargets
#     FILE ld2420_picoTargets.cmake
#     NAMESPACE ld2420::
#     DESTINATION lib/cmake/ld2420_pico
# )

# # Generate and install package configuration files
# include(CMakePackageConfigHelpers)
# write_basic_package_version_file(
#     "${CMAKE_CURRENT_BINARY_DIR}/ld2420_picoConfigVersion.cmake"
#     VERSION ${PROJECT_VERSION}
#     COMPATIBILITY AnyNewerVersion
# )

# configure_package_config_file(
#     "${CMAKE_CURRENT_SOURCE_DIR}/ld2420_picoConfig.cmake.in"
#     "${CMAKE_CURRENT_BINARY_DIR}/ld2420_picoConfig.cmake"
#     INSTALL_DESTINATION lib/cmake/ld2420_pico
# )

# install(FILES
#     "${CMAKE_CURRENT_BINARY_DIR}/ld2420_picoConfig.cmake"
#     "${CMAKE_CURRENT_BINARY_DIR}/ld2420_picoConfigVersion.cmake"
#     DESTINATION lib/cmake/ld2420_pico
# )
