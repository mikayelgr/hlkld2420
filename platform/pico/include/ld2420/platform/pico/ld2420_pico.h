#pragma once

#include <hardware/uart.h>
#include <stdlib.h>
#include <stdbool.h>
#include "ld2420/ld2420.h"

#ifdef __cplusplus
extern "C"
{
#endif
    /**
     * @brief Size of the ring buffer for UART RX data.
     * 
     * The buffer size is 256 bytes, which is sufficient for receiving multiple
     * complete LD2420 packets (max packet size is 154 bytes). This provides
     * adequate buffering to prevent data loss during interrupt-driven reception.
     */
    #define LD2420_PICO_RX_BUFFER_SIZE 256

    /**
     * @brief Ring buffer structure for interrupt-driven UART reception.
     * 
     * This structure implements a circular buffer for storing incoming UART bytes.
     * It's designed to be used in an interrupt service routine (ISR) for efficient
     * non-blocking data reception.
     */
    typedef struct
    {
        uint8_t buffer[LD2420_PICO_RX_BUFFER_SIZE];
        volatile uint16_t write_idx;
        volatile uint16_t read_idx;
    } ld2420_ring_buffer_t;

    /**
     * @brief LD2420 configuration structure for Raspberry Pi Pico platform.
     * 
     * This structure now includes a ring buffer for interrupt-driven UART reception,
     * enabling non-blocking communication with the LD2420 sensor module.
     */
    typedef struct
    {
        unsigned char rx_pin;
        unsigned char tx_pin;
        uart_inst_t *uart_instance;
        ld2420_ring_buffer_t rx_ring_buffer;
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

    /**
     * @brief Sends data to the LD2420 module via UART.
     * @param config Pointer to the ld2420_pico_t structure.
     * @param data Pointer to the data buffer to send.
     * @param length Length of the data buffer.
     * @return Status of the send operation as an ld2420_status_t value.
     */
    ld2420_status_t ld2420_pico_send(ld2420_pico_t *config, const unsigned char *data, const uint8_t length);

    /**
     * @brief Deinitializes the LD2420 configuration for Raspberry Pi Pico platform.
     * @param config Pointer to the ld2420_pico_t structure to deinitialize.
     * @return Status of the deinitialization as an ld2420_status_t value.
     */
    ld2420_status_t ld2420_pico_deinit(ld2420_pico_t *config);

    /**
     * @brief Enables interrupt-driven UART reception for the LD2420 module.
     * 
     * This function configures the UART RX interrupt and sets up the interrupt
     * handler. Once enabled, incoming bytes are automatically stored in the
     * ring buffer without blocking the main application.
     * 
     * @param config Pointer to the ld2420_pico_t structure.
     * @return Status of the operation as an ld2420_status_t value.
     * 
     * @note This function must be called after ld2420_pico_init().
     * @note The interrupt handler will automatically read bytes into the ring buffer.
     */
    ld2420_status_t ld2420_pico_enable_interrupts(ld2420_pico_t *config);

    /**
     * @brief Disables interrupt-driven UART reception for the LD2420 module.
     * 
     * This function disables the UART RX interrupt, stopping automatic data
     * reception into the ring buffer.
     * 
     * @param config Pointer to the ld2420_pico_t structure.
     * @return Status of the operation as an ld2420_status_t value.
     */
    ld2420_status_t ld2420_pico_disable_interrupts(ld2420_pico_t *config);

    /**
     * @brief Gets the number of bytes available in the receive ring buffer.
     * 
     * This function returns the count of bytes that have been received and are
     * waiting to be read from the ring buffer.
     * 
     * @param config Pointer to the ld2420_pico_t structure.
     * @return Number of bytes available to read (0 if buffer is empty).
     */
    uint16_t ld2420_pico_bytes_available(const ld2420_pico_t *config);

    /**
     * @brief Reads a single byte from the receive ring buffer.
     * 
     * This function retrieves one byte from the ring buffer if available.
     * It's designed to be called from the main application loop to consume
     * bytes received via interrupt-driven UART.
     * 
     * @param config Pointer to the ld2420_pico_t structure.
     * @param byte Pointer to store the read byte.
     * @return true if a byte was successfully read, false if buffer was empty.
     */
    bool ld2420_pico_read_byte(ld2420_pico_t *config, uint8_t *byte);

    /**
     * @brief Reads multiple bytes from the receive ring buffer.
     * 
     * This function retrieves up to 'length' bytes from the ring buffer.
     * It will read fewer bytes if not enough data is available.
     * 
     * @param config Pointer to the ld2420_pico_t structure.
     * @param buffer Pointer to the destination buffer.
     * @param length Maximum number of bytes to read.
     * @return Number of bytes actually read (may be less than 'length').
     */
    uint16_t ld2420_pico_read_bytes(ld2420_pico_t *config, uint8_t *buffer, uint16_t length);

#ifdef __cplusplus
}
#endif