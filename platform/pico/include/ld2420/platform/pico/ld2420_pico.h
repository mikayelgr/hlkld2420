#pragma once

#include <hardware/uart.h>
#include <stdlib.h>
#include "ld2420/ld2420.h"

#ifdef __cplusplus
extern "C"
{
#endif
    /**
     * @brief LD2420 configuration structure for Raspberry Pi Pico platform.
     */
    typedef struct
    {
        unsigned char rx_pin;
        unsigned char tx_pin;
        uart_inst_t *uart_instance;
    } ld2420_pico_t;

    /**
     * @brief Initializes the LD2420 configuration for Raspberry Pi Pico platform.
     *
     * This function sets up the LD2420 configuration structure with the specified
     * UART pins and instance. It prepares the device for communication with the
     * default baud rate defined by the documentation for the LD2420 module.
     *
     * @param config Pointer to the ld2420_pico_t structure to initialize.
     * @param rx_pin The GPIO pin number for UART RX.
     * @param tx_pin The GPIO pin number for UART TX.
     * @param uart_instance The UART instance to use (e.g., uart0 or uart1).
     * @return Status of the initialization as an ld2420_status_t value.
     */
    ld2420_status_t ld2420_pico_init(ld2420_pico_t *config, unsigned char rx_pin, unsigned char tx_pin, uart_inst_t *uart_instance);
#ifdef __cplusplus
}
#endif