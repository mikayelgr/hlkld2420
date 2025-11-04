# Check if the target already exists before adding it
add_library(ld2420_core ld2420.c)
target_link_libraries(ld2420_core PUBLIC ld2420_iface)
target_compile_options(ld2420_core PRIVATE -mlittle-endian)