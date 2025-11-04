#pragma once
#ifndef LD2420_WITH_PICO
#error "LD2420_WITH_PICO must be defined to use this header"
#endif

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

    ld2420_pico_t *ld2420_pico_init(unsigned char rx_pin, unsigned char tx_pin, uart_inst_t *uart_instance);
#ifdef __cplusplus
}
#endif