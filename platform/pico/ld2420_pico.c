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

// uart rx structures for uart0 and uart1
static ld2420_uart_rx_t uart_rx_buffers[2];
static inline void __init_uart_rx_buffer__(uint8_t idx)
{
    uart_rx_buffers[idx].head = 0;
    uart_rx_buffers[idx].tail = 0;
    uart_rx_buffers[idx].overflow = 0;
}

// rx callback functions for uart0 and uart1
static ld2420_rx_callback_t rx_callbacks[2] = {NULL, NULL};

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
        printf("DEBUG: head=%d, tail=%d, overflow=%d\n", rb->head, rb->tail, rb->overflow);

        // Drain available bytes from ring buffer and pass to callback
        int byte_count = 0;
        while (rb->tail != rb->head)
        {
            uint8_t byte = rb->buf[rb->tail];
            rb->tail = (rb->tail + 1) % LD2420_UART_RINGBUF_SIZE;
            __asm volatile("" ::: "memory");
            rx_callbacks[uart_index](uart_index, &byte, 1);
            byte_count++;
        }

        if (byte_count > 0)
        {
            printf("DEBUG: Processed %d bytes\n", byte_count);
            return byte_count;
        }
        else
        {
            printf("DEBUG: No bytes in buffer\n");
            return 0;
        }
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
            return LD2420_ERROR_INVALID_ARGUMENTS;
        }

        uart_write_blocking(uart_instance, buffer, buffer_size);
        mutex_exit(&ld2420_uart_tx_mutex);
        return LD2420_OK;
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
            return LD2420_ERROR_INVALID_ARGUMENTS;
        }

        int8_t idx = decide_uart_instance_number(uart_instance);
        if (idx < 0)
        {
            printf("ERROR: Unable to decide UART instance number\n");
            return LD2420_ERROR_INVALID_ARGUMENTS;
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
            return LD2420_ERROR_INVALID_ARGUMENTS | LD2420_ERROR_UNKNOWN;
        }

        // Set the GPIO pin mux to the UART
        // TX and RX pins need to be configured for UART function
        gpio_set_function(tx_pin, GPIO_FUNC_UART);
        gpio_set_function(rx_pin, GPIO_FUNC_UART);
        return LD2420_OK;
    }

    const ld2420_status_t ld2420_pico_deinit(uart_inst_t *uart_instance)
    {
        int8_t idx = decide_uart_instance_number(uart_instance);
        if (idx == 0)
            irq_set_enabled(UART0_IRQ, false);
        else if (idx == 1)
            irq_set_enabled(UART1_IRQ, false);
        else
            return LD2420_ERROR_INVALID_ARGUMENTS;

        uart_set_irq_enables(uart_instance, false, false);
        uart_deinit(uart_instance);

        __init_uart_rx_buffer__(idx);
        rx_callbacks[idx] = NULL;
        return LD2420_OK;
    }

#ifdef __cplusplus
}
#endif