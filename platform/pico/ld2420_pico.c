#include "ld2420/platform/pico/ld2420_pico.h"
#include <hardware/uart.h>
#include <hardware/gpio.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif
    ld2420_status_t ld2420_pico_init(
        ld2420_pico_t *config,
        unsigned char rx_pin,
        unsigned char tx_pin,
        uart_inst_t *uart_instance)
    {
        // Basic parameter validation
        if (uart_instance == NULL)
        {
            return LD2420_ERROR_INVALID_ARGUMENTS;
        }

        config->rx_pin = rx_pin;
        config->tx_pin = tx_pin;
        config->uart_instance = uart_instance;

        // Initialize UART first with the baud rate
        uart_init(uart_instance, LD2420_BAUD_RATE);

        // Set the GPIO pin mux to the UART
        // TX and RX pins need to be configured for UART function
        gpio_set_function(tx_pin, GPIO_FUNC_UART);
        gpio_set_function(rx_pin, GPIO_FUNC_UART);

        return LD2420_OK;
    }

#ifdef __cplusplus
}
#endif