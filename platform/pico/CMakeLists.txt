# Including and initializing the Pico SDK, which exposes all the necessary targets and libraries
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)
pico_sdk_init()

# Define the Pico implementation library
add_library(ld2420_pico ld2420_pico.c include/ld2420/platform/pico/ld2420_pico.h)
target_link_libraries(ld2420_pico PUBLIC ld2420_core)
target_include_directories(ld2420_pico PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_include_directories(ld2420_pico PRIVATE
    $ENV{PICO_SDK_PATH}/src/common/pico_stdlib_headers/include
    $ENV{PICO_SDK_PATH}/src/rp2_common/hardware_uart/include
    $ENV{PICO_SDK_PATH}/src/rp2040/pico_platform/include
)

# 1. Link the necessary platform dependencies
target_link_libraries(ld2420_pico PUBLIC
    pico_stdlib
    hardware_uart
    hardware_gpio
)
