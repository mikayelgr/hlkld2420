include(FetchContent)

FetchContent_Declare(
    Unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG v2.6.1
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(Unity)
add_subdirectory(core)
