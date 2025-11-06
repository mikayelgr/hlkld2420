# Check if the target already exists before adding it
add_library(ld2420_core ld2420.c)
target_link_libraries(ld2420_core PUBLIC ld2420_iface)

include(CheckCCompilerFlag)
check_c_compiler_flag(-mlittle-endian COMPILER_SUPPORTS_MLITTLE_ENDIAN)
if(COMPILER_SUPPORTS_MLITTLE_ENDIAN)
    message(STATUS "The compiler supports -mlittle-endian")
    target_compile_options(ld2420_core PRIVATE -mlittle-endian)
endif()
