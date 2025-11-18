cmake_minimum_required(VERSION 3.16)
project(ld2420_core VERSION 1.0.0 LANGUAGES C)

# Core library
add_library(ld2420_core ld2420.c)

# Include directories
target_include_directories(ld2420_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# The LD2420 protocol uses little-endian byte order for all multi-byte values.
# This implementation uses explicit byte-order conversion and works on any architecture.
include(TestBigEndian)
test_big_endian(IS_BIG_ENDIAN)
if(IS_BIG_ENDIAN)
    message(STATUS "LD2420: Building for big-endian architecture '${CMAKE_SYSTEM_PROCESSOR}'")
    message(STATUS "LD2420: Using explicit little-endian byte conversion for protocol compliance")
else()
    message(STATUS "LD2420: Building for little-endian architecture '${CMAKE_SYSTEM_PROCESSOR}'")
endif()

# Adding tests if testing is enabled
if(CMAKE_TESTING_ENABLED)
    # Making sure test dependencies are enabled
    include(CTest)
    include(FetchContent)
    FetchContent_Declare(
        Unity
        GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
        GIT_TAG v2.6.1
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(Unity)

    # Adding the test executable
    add_executable(ld2420_test ld2420_test.c)
    # Linking against unity framework and the core library
    target_link_libraries(ld2420_test PRIVATE ld2420_core unity)
    # Registering within CTest
    add_test(NAME ld2420_test COMMAND ld2420_test)
endif()
