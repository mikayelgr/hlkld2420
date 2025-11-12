# Including and initializing the Pico SDK, which exposes all the necessary targets and libraries
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)
pico_sdk_init()

# Define the Pico implementation library
add_library(ld2420_pico ld2420_pico.c include/ld2420/platform/pico/ld2420_pico.h)
target_link_libraries(ld2420_pico PUBLIC ld2420_core)
target_include_directories(ld2420_pico PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 1. Link the necessary platform dependencies
target_link_libraries(ld2420_pico PUBLIC
    pico_stdlib
    hardware_uart
    hardware_gpio
    hardware_irq
)
