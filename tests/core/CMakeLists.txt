add_executable(test_txrx_fns test_txrx_fns.c)

target_link_libraries(test_txrx_fns PRIVATE ld2420_iface unity)

add_test(NAME test_txrx_fns COMMAND test_txrx_fns)
