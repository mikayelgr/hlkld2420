#include "ld2420/platform/pico/ld2420_pico.h"
#include <hardware/uart.h>
#include <hardware/gpio.h>
#include <stdio.h>
#include <stdint.h>
#include <hardware/irq.h>

#ifdef __cplusplus
extern "C"
{
#endif
    ld2420_status_t ld2420_pico_send(
        ld2420_pico_t *config,
        const uint8_t *buffer,
        const uint8_t buffer_size)
    {
        if (config == NULL || buffer == NULL || buffer_size == 0)
        {
            return LD2420_ERROR_INVALID_ARGUMENTS;
        }

        // uart_write_blocking will handle waiting for UART to be ready
        // and will write all bytes, blocking until complete
        uart_write_blocking(config->uart_instance, buffer, buffer_size);
        return LD2420_OK;
    }

    ld2420_status_t ld2420_pico_init(
        ld2420_pico_t *config,
        unsigned char rx_pin,
        unsigned char tx_pin,
        uart_inst_t *uart_instance)
    {
        if (uart_instance == NULL)
        {
            return LD2420_ERROR_INVALID_ARGUMENTS;
        }

        config->rx_pin = rx_pin;
        config->tx_pin = tx_pin;
        config->uart_instance = uart_instance;

        // Initialize UART first with the baud rate
        uart_init(config->uart_instance, LD2420_BAUD_RATE);

        // We are enabling FIFO for the provided UART instance because it helps in buffering the
        // data and reduces CPU load. Additionally, it improves data integrity during communication.
        // At 115200 baud, bytes arrive approximately every 87 microseconds.
        uart_set_fifo_enabled(config->uart_instance, true);

        // Since the sensor module doesn't have flow control lines, we disable the hardware
        // flow control via CTS/RTS.
        uart_set_hw_flow(config->uart_instance, false, false);
        uart_set_format(config->uart_instance, 8, 1, UART_PARITY_NONE);

        // Set the GPIO pin mux to the UART
        // TX and RX pins need to be configured for UART function
        gpio_set_function(config->tx_pin, GPIO_FUNC_UART);
        gpio_set_function(config->rx_pin, GPIO_FUNC_UART);

        // Flush and clear the UART receive FIFO to ensure clean state
        // This removes any residual data from previous operations
        while (uart_is_readable(config->uart_instance))
        {
            uart_getc(config->uart_instance);
        }

        return LD2420_OK;
    }

    ld2420_status_t ld2420_pico_deinit(ld2420_pico_t *config)
    {
        uart_deinit(config->uart_instance);
        return LD2420_OK;
    }

#ifdef __cplusplus
}
#endif