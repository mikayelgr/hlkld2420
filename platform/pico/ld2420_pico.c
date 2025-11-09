#include "ld2420/platform/pico/ld2420_pico.h"
#include <hardware/uart.h>
#include <hardware/gpio.h>
#include <stdio.h>
#include <stdint.h>
#include <hardware/irq.h>
#include <string.h>

// Global pointer to track the config for interrupt handling
// Since we support up to 2 UART instances (uart0 and uart1), we maintain
// an array indexed by UART instance
static ld2420_pico_t *g_uart_configs[2] = {NULL, NULL};

// Forward declaration of the interrupt handler
static void ld2420_pico_uart_irq_handler(void);

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
        if (config == NULL || uart_instance == NULL)
        {
            return LD2420_ERROR_INVALID_ARGUMENTS;
        }

        config->rx_pin = rx_pin;
        config->tx_pin = tx_pin;
        config->uart_instance = uart_instance;

        // Initialize ring buffer indices
        config->rx_ring_buffer.write_idx = 0;
        config->rx_ring_buffer.read_idx = 0;
        memset((void *)config->rx_ring_buffer.buffer, 0, LD2420_PICO_RX_BUFFER_SIZE);

        // Store config pointer for interrupt handler access
        int uart_index = (uart_instance == uart0) ? 0 : 1;
        g_uart_configs[uart_index] = config;

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
        if (config == NULL)
        {
            return LD2420_ERROR_INVALID_ARGUMENTS;
        }

        // Disable interrupts first
        ld2420_pico_disable_interrupts(config);

        // Clear global config pointer
        int uart_index = (config->uart_instance == uart0) ? 0 : 1;
        g_uart_configs[uart_index] = NULL;

        uart_deinit(config->uart_instance);
        return LD2420_OK;
    }

    // UART interrupt handler
    static void ld2420_pico_uart_irq_handler(void)
    {
        // Determine which UART triggered the interrupt
        for (int i = 0; i < 2; i++)
        {
            if (g_uart_configs[i] == NULL)
                continue;

            uart_inst_t *uart = g_uart_configs[i]->uart_instance;
            
            // Read all available bytes from UART FIFO into ring buffer
            while (uart_is_readable(uart))
            {
                uint8_t ch = uart_getc(uart);
                ld2420_ring_buffer_t *rb = &g_uart_configs[i]->rx_ring_buffer;
                
                // Calculate next write position
                uint16_t next_write_idx = (rb->write_idx + 1) % LD2420_PICO_RX_BUFFER_SIZE;
                
                // Check for buffer overflow
                if (next_write_idx != rb->read_idx)
                {
                    rb->buffer[rb->write_idx] = ch;
                    rb->write_idx = next_write_idx;
                }
                // If buffer is full, byte is dropped (overflow condition)
            }
        }
    }

    ld2420_status_t ld2420_pico_enable_interrupts(ld2420_pico_t *config)
    {
        if (config == NULL)
        {
            return LD2420_ERROR_INVALID_ARGUMENTS;
        }

        // Get the IRQ number for the UART instance
        int uart_irq = (config->uart_instance == uart0) ? UART0_IRQ : UART1_IRQ;
        
        // Set up the interrupt handler
        irq_set_exclusive_handler(uart_irq, ld2420_pico_uart_irq_handler);
        
        // Enable the UART interrupt at the system level
        irq_set_enabled(uart_irq, true);
        
        // Enable UART RX interrupts (but not TX)
        uart_set_irq_enables(config->uart_instance, true, false);

        return LD2420_OK;
    }

    ld2420_status_t ld2420_pico_disable_interrupts(ld2420_pico_t *config)
    {
        if (config == NULL)
        {
            return LD2420_ERROR_INVALID_ARGUMENTS;
        }

        // Disable UART RX interrupts
        uart_set_irq_enables(config->uart_instance, false, false);
        
        // Disable the UART interrupt at the system level
        int uart_irq = (config->uart_instance == uart0) ? UART0_IRQ : UART1_IRQ;
        irq_set_enabled(uart_irq, false);

        return LD2420_OK;
    }

    uint16_t ld2420_pico_bytes_available(const ld2420_pico_t *config)
    {
        if (config == NULL)
        {
            return 0;
        }

        const ld2420_ring_buffer_t *rb = &config->rx_ring_buffer;
        
        if (rb->write_idx >= rb->read_idx)
        {
            return rb->write_idx - rb->read_idx;
        }
        else
        {
            return LD2420_PICO_RX_BUFFER_SIZE - rb->read_idx + rb->write_idx;
        }
    }

    bool ld2420_pico_read_byte(ld2420_pico_t *config, uint8_t *byte)
    {
        if (config == NULL || byte == NULL)
        {
            return false;
        }

        ld2420_ring_buffer_t *rb = &config->rx_ring_buffer;
        
        // Check if buffer is empty
        if (rb->read_idx == rb->write_idx)
        {
            return false;
        }
        
        // Read byte from buffer
        *byte = rb->buffer[rb->read_idx];
        rb->read_idx = (rb->read_idx + 1) % LD2420_PICO_RX_BUFFER_SIZE;
        
        return true;
    }

    uint16_t ld2420_pico_read_bytes(ld2420_pico_t *config, uint8_t *buffer, uint16_t length)
    {
        if (config == NULL || buffer == NULL || length == 0)
        {
            return 0;
        }

        uint16_t bytes_read = 0;
        
        while (bytes_read < length && ld2420_pico_read_byte(config, &buffer[bytes_read]))
        {
            bytes_read++;
        }
        
        return bytes_read;
    }

#ifdef __cplusplus
}
#endif