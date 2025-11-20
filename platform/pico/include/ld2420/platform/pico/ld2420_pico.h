#pragma once

#include <hardware/uart.h>
#include <stdlib.h>
#include "ld2420/ld2420.h"

#ifdef __cplusplus
extern "C"
{
#endif
    /**
     * @brief Callback type for received LD2420 frames.
     *
     * @param uart_index UART instance (0 or 1)
     * @param packet Pointer to a complete LD2420 frame buffer (starts with SOF 0xF4)
     * @param packet_len Total frame length in bytes (includes SOF and length field)
     *
     * @note Packets are always complete, frame-aligned LD2420 protocol messages.
     *       The callback is invoked once per complete frame, not per byte.
     */
    typedef void (*ld2420_rx_callback_t)(
        uint8_t uart_index,
        const uint8_t *packet,
        uint16_t packet_len);

    /**
     * @brief Initialize UART for LD2420 sensor communication.
     *
     * Sets up the specified UART instance with RX interrupts, ring buffering,
     * and automatic frame assembly. The provided callback will be invoked
     * whenever a complete LD2420 frame is received.
     *
     * @param uart_instance Pointer to uart_inst_t (uart0 or uart1)
     * @param tx_pin TX pin number (must match uart_instance)
     * @param rx_pin RX pin number (must match uart_instance)
     * @param rx_callback Function to invoke when a complete frame is received
     *
     * @return LD2420_STATUS_OK on success, error code otherwise
     */
    const ld2420_status_t ld2420_pico_init(
        uart_inst_t *uart_instance,
        const uint8_t tx_pin,
        const uint8_t rx_pin,
        const ld2420_rx_callback_t rx_callback);

    /**
     * @brief Process pending incoming data and deliver complete frames.
     *
     * Drains bytes from the RX ring buffer, assembles complete LD2420 frames,
     * and invokes the registered callback for each complete frame received.
     * Call this function periodically from your main loop.
     *
     * @param uart_index UART instance (0 or 1)
     *
     * @return Number of complete frames delivered (≥0), or -1 on error
     */
    const int16_t ld2420_pico_process(uint8_t uart_index);

    /**
     * @brief Deinitialize UART for LD2420 communication.
     *
     * Disables RX interrupts, clears buffers, and resets state.
     *
     * @param uart_instance Pointer to uart_inst_t
     * @return LD2420_STATUS_OK on success, error code otherwise
     */
    const ld2420_status_t ld2420_pico_deinit(uart_inst_t *uart_instance);

    /**
     * @brief Send data to LD2420 sensor (thread-safe).
     *
     * Sends the provided buffer to the sensor via the specified UART instance.
     * Uses an internal mutex to ensure thread-safe access.
     *
     * @param uart_instance Pointer to uart_inst_t
     * @param data Pointer to data buffer
     * @param length Number of bytes to send
     *
     * @return LD2420_STATUS_OK on success, error code otherwise
     */
    const ld2420_status_t ld2420_pico_send_safe(uart_inst_t *uart_instance, const uint8_t *data, const uint16_t length);

#ifdef __cplusplus
}
#endif