include(FetchContent)

FetchContent_Declare(
    Unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG v2.6.1
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(Unity)

if(NOT DEFINED PICO_SDK_PATH)
    add_subdirectory(core)
endif()

# Only add pico tests if building for Pico platform
if(PICO_SDK_PATH)
    # add_subdirectory(pico)
endif()
