#pragma once

#include <hardware/uart.h>
#include <stdlib.h>
#include "ld2420/ld2420.h"

#ifdef __cplusplus
extern "C"
{
#endif
    typedef void (*ld2420_rx_callback_t)(
        uint8_t uart_index,
        const uint8_t *packet,
        uint16_t packet_len);

    ld2420_status_t ld2420_pico_init(
        uart_inst_t *uart_instance,
        const uint8_t rx_pin,
        const uint8_t tx_pin,
        const ld2420_rx_callback_t rx_callback);

    void ld2420_pico_process(uint8_t uart_index);
    ld2420_status_t ld2420_pico_deinit(uart_inst_t *uart_instance);
    ld2420_status_t ld2420_pico_send_safe(uart_inst_t *uart_instance, const uint8_t *data, const uint16_t length);
#ifdef __cplusplus
}
#endif