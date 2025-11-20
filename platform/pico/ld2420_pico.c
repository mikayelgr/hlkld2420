#include <ld2420/platform/pico/ld2420_pico.h>
#include <hardware/uart.h>
#include <hardware/gpio.h>
#include <hardware/irq.h>
#include <pico/mutex.h>
#include <stdio.h>

/**
 * @brief Validates if the provided RX and TX pins correspond to the specified UART instance.
 */
static inline bool validate_uart_pin_pair_instance(
    const uint8_t tx_pin,
    const uint8_t rx_pin,
    const uart_inst_t *uart_instance)
{
    if (rx_pin == tx_pin)
        return false;
    return ((tx_pin == 0 && rx_pin == 1 && uart_instance == uart0) ||   // UART0
            (tx_pin == 4 && rx_pin == 5 && uart_instance == uart1) ||   // UART1
            (tx_pin == 8 && rx_pin == 9 && uart_instance == uart1) ||   // UART1
            (tx_pin == 12 && rx_pin == 13 && uart_instance == uart1) || // UART1
            (tx_pin == 16 && rx_pin == 17 && uart_instance == uart0)    // UART0
    );
}

/**
 * @brief Determines the UART instance number (0 or 1) based on the provided uart_inst_t pointer.
 */
static inline int8_t decide_uart_instance_number(const uart_inst_t *uart_instance)
{
    if (uart_instance == uart0)
        return 0;
    if (uart_instance == uart1)
        return 1;

    return -1;
}

// We are setting the ringbuf size to 512 bytes to accommodate larger packets
// and ensure that we have enough space to handle incoming data without overflow.
#define LD2420_UART_RINGBUF_SIZE 512u

// Maximum frame size for LD2420 sensor (header + data + checksum).
// Typical frames are 9–27 bytes, but we allow up to 256 for safety.
#define LD2420_MAX_FRAME_SIZE 256u

// LD2420 protocol frame start-of-frame marker
#define LD2420_SOF 0xF4u

/**
 * @brief Frame assembly state machine states.
 */
typedef enum
{
    LD2420_FRAME_STATE_AWAITING_SOF = 0, // Waiting for start-of-frame marker
    LD2420_FRAME_STATE_ACCUMULATING = 1  // Accumulating frame bytes after SOF
} ld2420_frame_state_t;

/**
 * @brief Structure to hold UART RX ring buffer information.
 */
typedef struct
{
    volatile uint8_t buf[LD2420_UART_RINGBUF_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
    volatile uint16_t overflow;
} ld2420_uart_rx_t;

/**
 * @brief Frame assembly state for incoming LD2420 protocol data.
 *
 * Accumulates bytes from the ring buffer into complete frames before
 * delivering them to the user callback. Frames are identified by the
 * LD2420 SOF marker (0xF4) and contain a length field at byte [1].
 */
typedef struct
{
    uint8_t buf[LD2420_MAX_FRAME_SIZE];
    uint16_t len;               // Current frame byte count
    ld2420_frame_state_t state; // Current assembly state
    uint16_t expected_len;      // Total frame length (including SOF + length byte)
} ld2420_frame_assembler_t;

/**
 * @brief RX ring buffers for UART0 and UART1
 *
 * One buffer per UART instance:
 *  - Index 0 maps to `uart0`
 *  - Index 1 maps to `uart1`
 *
 * Concurrency and ownership model:
 *  - Written in interrupt context by the corresponding RX ISR
 *    (`uart0_rx_irq_handler` / `uart1_rx_irq_handler`), which stores bytes
 *    at `head` and advances it.
 *  - Drained in `ld2420_pico_process(uart_index)` on the main thread, which
 *    reads bytes from `tail` and advances it, forwarding data to the
 *    registered callback in `rx_callbacks`.
 *  - Fields are declared `volatile` to ensure ISR/main-core visibility; short
 *    compiler barriers are placed around updates to prevent reordering.
 *
 * Capacity and overflow behavior:
 *  - Each ring holds `LD2420_UART_RINGBUF_SIZE` bytes (currently 512).
 *  - When the buffer is full, the incoming byte is dropped and the
 *    `overflow` counter is incremented (old data is preserved).
 *
 * Lifecycle:
 *  - Buffers are cleared by `__init_uart_rx_buffer__()` during init/deinit
 *    and before enabling IRQs to avoid mixing stale data with new frames.
 *
 * Rationale:
 *  - File-static, contiguous storage avoids dynamic allocation and keeps the
 *    ISR fast and predictable.
 */
static ld2420_uart_rx_t uart_rx_buffers[2];

/**
 * @brief Frame assemblers for UART0 and UART1
 *
 * One assembler per UART instance (index 0 for uart0, index 1 for uart1).
 * Accumulates bytes from the ring buffer into complete LD2420 frames before
 * delivering them to the user callback. This ensures the callback receives
 * complete, frame-aligned packets rather than individual bytes.
 */
static ld2420_frame_assembler_t frame_assemblers[2];

/**
 * @brief Callback function pointers for UART receive operations
 *
 * This array stores callback function pointers for handling received data on UART interfaces.
 * The callbacks are invoked when data is received on the corresponding UART port.
 *
 * @details
 * - Index 0: Callback function for UART0 receive operations
 * - Index 1: Callback function for UART1 receive operations
 *
 * Each callback is of type ld2420_rx_callback_t and can be registered by the application
 * to process incoming data from the LD2420 sensor connected to the respective UART port.
 *
 * @note
 * - Callbacks are initialized to NULL and must be set before UART receive operations begin
 * - The callback function signature is defined by ld2420_rx_callback_t type
 * - Callbacks are typically invoked in interrupt context, so they should be kept short
 *   and avoid blocking operations
 *
 * @see ld2420_rx_callback_t
 * @see UART receive configuration and initialization functions
 */
// rx callback functions for uart0 and uart1
static ld2420_rx_callback_t rx_callbacks[2] = {NULL, NULL};

static inline void __init_uart_rx_buffer__(uint8_t idx)
{
    uart_rx_buffers[idx].head = 0;
    uart_rx_buffers[idx].tail = 0;
    uart_rx_buffers[idx].overflow = 0;

    // Also reset the frame assembler
    frame_assemblers[idx].len = 0;
    frame_assemblers[idx].state = LD2420_FRAME_STATE_AWAITING_SOF;
    frame_assemblers[idx].expected_len = 0;
}

static __noinline void uart0_rx_irq_handler(void)
{
    ld2420_uart_rx_t *rb = &uart_rx_buffers[0];
    while (uart_is_readable(uart0))
    {
        uint8_t c = uart_getc(uart0);
        uint16_t h = rb->head, n = (h + 1) % LD2420_UART_RINGBUF_SIZE;
        if (n != rb->tail)
        {
            rb->buf[h] = c;
            rb->head = n;
            // Ensure write is globally visible
            __asm volatile("" ::: "memory");
        }
        else
        {
            rb->overflow++;
            // drop the bytes
        }
    }
}

/**
 * @brief UART1 RX interrupt handler
 *
 * This function is invoked when data is received on UART1. It reads incoming bytes
 * from the UART hardware and stores them in a ring buffer for later processing.
 * If the ring buffer is full, it increments an overflow counter and drops the incoming bytes.
 */
static __noinline void uart1_rx_irq_handler(void)
{
    ld2420_uart_rx_t *rb = &uart_rx_buffers[1];
    while (uart_is_readable(uart1))
    {
        uint8_t c = uart_getc(uart1);
        uint16_t h = rb->head, n = (h + 1) % LD2420_UART_RINGBUF_SIZE;
        if (n != rb->tail)
        {
            rb->buf[h] = c;
            rb->head = n;
            // Ensure write is globally visible
            __asm volatile("" ::: "memory");
        }
        else
        {
            rb->overflow++;
            // drop the bytes
        }
    }
}

#ifdef __cplusplus
extern "C"
{
#endif
    /**
     * @brief Attempt to assemble a complete LD2420 frame from available bytes.
     *
     * Implements a simple state machine:
     *  1. LD2420_FRAME_STATE_AWAITING_SOF: Scan ring buffer for 0xF4 byte
     *  2. LD2420_FRAME_STATE_ACCUMULATING: Once SOF found, byte[1] is frame length.
     *     Continue reading until frame is complete, then deliver to callback.
     *
     * @param uart_index UART instance (0 or 1)
     * @return Number of complete frames delivered, or -1 on error
     */
    static int16_t __assemble_and_deliver_frames(uint8_t uart_index)
    {
        ld2420_uart_rx_t *rb = &uart_rx_buffers[uart_index];
        ld2420_frame_assembler_t *fa = &frame_assemblers[uart_index];
        int16_t frame_count = 0;

        while (rb->tail != rb->head)
        {
            uint8_t byte = rb->buf[rb->tail];
            rb->tail = (rb->tail + 1) % LD2420_UART_RINGBUF_SIZE;
            __asm volatile("" ::: "memory");

            if (fa->state == LD2420_FRAME_STATE_AWAITING_SOF)
            {
                // Waiting for SOF marker
                if (byte == LD2420_SOF)
                {
                    fa->buf[0] = byte;
                    fa->len = 1;
                    fa->state = LD2420_FRAME_STATE_ACCUMULATING;
                    fa->expected_len = 0; // Will be set once we read byte[1]
                }
                // else: skip this byte, continue scanning
            }
            else if (fa->state == LD2420_FRAME_STATE_ACCUMULATING)
            {
                // Accumulating frame bytes
                if (fa->len < LD2420_MAX_FRAME_SIZE)
                {
                    fa->buf[fa->len] = byte;
                    fa->len++;

                    // Byte[1] is the frame length field
                    if (fa->len == 2)
                    {
                        fa->expected_len = byte + 2; // +2 for SOF and length byte itself
                    }

                    // Check if frame is complete
                    if (fa->len >= 2 && fa->len == fa->expected_len)
                    {
                        // Frame complete: deliver to callback
                        if (rx_callbacks[uart_index] != NULL)
                        {
                            rx_callbacks[uart_index](uart_index, fa->buf, fa->len);
                            frame_count++;
                        }

                        // Reset for next frame
                        fa->len = 0;
                        fa->state = LD2420_FRAME_STATE_AWAITING_SOF;
                        fa->expected_len = 0;
                    }
                }
                else
                {
                    // Frame buffer overflow: discard and resync
                    printf("WARN: Frame buffer overflow on UART%d, resyncing\n", uart_index);
                    fa->len = 0;
                    fa->state = LD2420_FRAME_STATE_AWAITING_SOF;
                    fa->expected_len = 0;
                }
            }
        }

        return frame_count;
    }

    const int16_t ld2420_pico_process(uint8_t uart_index)
    {
        if (uart_index > 1)
        {
            printf("ERROR: Invalid UART index %d\n", uart_index);
            return -1;
        }

        if (rx_callbacks[uart_index] == NULL)
        {
            printf("ERROR: No callback registered for UART %d\n", uart_index);
            return -1;
        }

        ld2420_uart_rx_t *rb = &uart_rx_buffers[uart_index];

        // Attempt to assemble and deliver complete frames
        int16_t frame_count = __assemble_and_deliver_frames(uart_index);

        if (frame_count > 0)
        {
            printf("DEBUG: Delivered %d frame(s) on UART%d\n", frame_count, uart_index);
        }

        return frame_count;
    }

    /**
     * A mutex to protect UART TX operations, ensuring thread-safe access
     * when multiple threads attempt to send data simultaneously.
     *
     * Refer to https://www.raspberrypi.com/documentation/pico-sdk/high_level.html#macro-definition-documentation-5
     * for more information regarding this macro.
     */
    auto_init_mutex(ld2420_uart_tx_mutex);

    const ld2420_status_t ld2420_pico_send_safe(
        uart_inst_t *uart_instance,
        const uint8_t *buffer,
        const uint16_t buffer_size)
    {
        mutex_enter_blocking(&ld2420_uart_tx_mutex);
        if (uart_instance == NULL || buffer == NULL || buffer_size == 0)
        {
            mutex_exit(&ld2420_uart_tx_mutex);
            return LD2420_STATUS_ERROR_INVALID_ARGUMENTS;
        }

        uart_write_blocking(uart_instance, buffer, buffer_size);
        mutex_exit(&ld2420_uart_tx_mutex);
        return LD2420_STATUS_OK;
    }

    const ld2420_status_t ld2420_pico_init(
        uart_inst_t *uart_instance,
        const uint8_t tx_pin,
        const uint8_t rx_pin,
        const ld2420_rx_callback_t rx_callback)
    {
        if (!validate_uart_pin_pair_instance(tx_pin, rx_pin, uart_instance))
        {
            printf("ERROR: Invalid TX/RX pin pair (%d, %d) for the specified UART instance\n", tx_pin, rx_pin);
            return LD2420_STATUS_ERROR_INVALID_ARGUMENTS;
        }

        int8_t idx = decide_uart_instance_number(uart_instance);
        if (idx < 0)
        {
            printf("ERROR: Unable to decide UART instance number\n");
            return LD2420_STATUS_ERROR_INVALID_ARGUMENTS;
        }

        // Disable interrupts first to prevent data from being buffered during init
        if (idx == 0)
            irq_set_enabled(UART0_IRQ, false);
        else
            irq_set_enabled(UART1_IRQ, false);

        // Initialize UART first with the baud rate
        uint baudrate = uart_init(uart_instance, LD2420_BAUD_RATE);
        printf("DEBUG: UART initialized with baud rate %u\n", baudrate);

        // Flush UART: ensure it's idle and clear any stale data
        uart_tx_wait_blocking(uart_instance); // Wait for TX to complete
        while (uart_is_readable(uart_instance))
            uart_getc(uart_instance); // Clear RX FIFO

        // Now that hardware is clean, reset the ring buffer and set callback
        __init_uart_rx_buffer__(idx);
        rx_callbacks[idx] = rx_callback;

        // We are enabling FIFO for the provided UART instance because it helps in buffering the
        // data and reduces CPU load. Additionally, it improves data integrity during communication.
        // At 115200 baud, bytes arrive approximately every 87 microseconds.
        uart_set_fifo_enabled(uart_instance, true);

        // Since the sensor module doesn't have flow control lines, we disable the hardware
        // flow control via CTS/RTS.
        uart_set_hw_flow(uart_instance, false, false);
        uart_set_format(uart_instance, 8, 1, UART_PARITY_NONE);

        // Enable RX interrupt and disable TX interrupt, because all the data that will be
        // sent from the Pico to the sensor will be done in a blocking manner.
        uart_set_irqs_enabled(uart_instance, true, false);

        // Enable the UART interrupt in the NVIC with a proper initialization process.
        switch (idx)
        {
        case 0:
            irq_set_enabled(UART0_IRQ, false);
            irq_set_exclusive_handler(UART0_IRQ, (irq_handler_t)uart0_rx_irq_handler);
            irq_set_enabled(UART0_IRQ, true);
            break;
        case 1:
            irq_set_enabled(UART1_IRQ, false);
            irq_set_exclusive_handler(UART1_IRQ, (irq_handler_t)uart1_rx_irq_handler);
            irq_set_enabled(UART1_IRQ, true);
            break;
        default:
            printf("ERROR: Unknown UART instance number %d\n", idx);
            return LD2420_STATUS_ERROR_INVALID_ARGUMENTS | LD2420_STATUS_ERROR_UNKNOWN;
        }

        // Set the GPIO pin mux to the UART
        // TX and RX pins need to be configured for UART function
        gpio_set_function(tx_pin, GPIO_FUNC_UART);
        gpio_set_function(rx_pin, GPIO_FUNC_UART);
        return LD2420_STATUS_OK;
    }

    const ld2420_status_t ld2420_pico_deinit(uart_inst_t *uart_instance)
    {
        int8_t idx = decide_uart_instance_number(uart_instance);
        if (idx == 0)
            irq_set_enabled(UART0_IRQ, false);
        else if (idx == 1)
            irq_set_enabled(UART1_IRQ, false);
        else
            return LD2420_STATUS_ERROR_INVALID_ARGUMENTS;

        uart_set_irq_enables(uart_instance, false, false);
        uart_deinit(uart_instance);

        __init_uart_rx_buffer__(idx);
        rx_callbacks[idx] = NULL;
        return LD2420_STATUS_OK;
    }

#ifdef __cplusplus
}
#endif