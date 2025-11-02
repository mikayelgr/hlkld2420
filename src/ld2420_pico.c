#include "ld2420/ld2420_pico.h"
#include <hardware/gpio.h>

#ifdef __cplusplus
extern "C"
{
#endif
    /**
     * @brief Initializes the LD2420 configuration for Raspberry Pi Pico platform.
     *
     * @param rx_pin The GPIO pin number for UART RX.
     * @param tx_pin The GPIO pin number for UART TX.
     * @param uart_instance The UART instance to use (e.g., uart0 or uart1).
     * @return Pointer to the initialized ld2420_pico_t structure, or NULL on failure.
     */
    ld2420_pico_t *ld2420_pico_init(unsigned char rx_pin, unsigned char tx_pin, uart_inst_t *uart_instance)
    {
        // Basic parameter validation
        if (uart_instance == NULL || rx_pin < 0 || tx_pin < 0)
        {
            return NULL;
        }

        ld2420_pico_t *config = (ld2420_pico_t *)malloc(sizeof(ld2420_pico_t));
        if (config == NULL)
        {
            return NULL;
        }

        config->rx_pin = rx_pin;
        config->tx_pin = tx_pin;
        config->uart_instance = uart_instance;

        // Set the GPIO pin mux to the UART - pin 0 is TX, 1 is RX; note use of UART_FUNCSEL_NUM
        // for the general
        // case where the func sel used for UART depends on the pin number
        // Do this before calling uart_init to avoid losing data
        // This example has been adapted from the official Raspberry Pi Pico SDK documentation.
        gpio_set_function(tx_pin, UART_FUNCSEL_NUM(uart_instance, 0));
        gpio_set_function(rx_pin, UART_FUNCSEL_NUM(uart_instance, 1));

        // Setting the baud rate from the official documentation
        uart_init(uart_instance, LD2420_BAUD_RATE);
        return config;
    }

#ifdef __cplusplus
}
#endif