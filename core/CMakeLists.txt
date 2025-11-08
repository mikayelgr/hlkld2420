# Check if the target already exists before adding it
add_library(ld2420_core ld2420.c)

# Add this instead:
target_include_directories(ld2420_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<INSTALL_INTERFACE:include>
)

include(CheckCCompilerFlag)

# The LD2420 protocol uses little-endian byte order. This implementation currently
# relies on little-endian target architecture. We attempt to enforce this with the
# -mlittle-endian compiler flag.
check_c_compiler_flag(-mlittle-endian COMPILER_SUPPORTS_MLITTLE_ENDIAN)
if(COMPILER_SUPPORTS_MLITTLE_ENDIAN)
    message(STATUS "The compiler supports -mlittle-endian - enforcing little-endian byte order")
    target_compile_options(ld2420_core PRIVATE -mlittle-endian)
else()
    # If the compiler doesn't support the flag, we can't guarantee byte order.
    # Most modern architectures (x86, x64, ARM default) are little-endian anyway.
    message(WARNING
        "Compiler does not support -mlittle-endian flag. This library assumes the target\n"
        "architecture is little-endian. If you are targeting a big-endian system, this\n"
        "library will NOT work correctly. Please report this at:\n"
        "https://github.com/mikayelgr/hlkld2420/issues"
    )
endif()
