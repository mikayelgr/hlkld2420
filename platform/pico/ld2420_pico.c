#include <ld2420/platform/pico/ld2420_pico.h>
#include <hardware/uart.h>
#include <hardware/gpio.h>
#include <hardware/irq.h>
#include <pico/mutex.h>

/**
 * @brief Validates if the provided RX and TX pins correspond to the specified UART instance.
 */
static inline bool validate_uart_pin_pair_instance(
    const uint8_t rx_pin,
    const uint8_t tx_pin,
    const uart_inst_t *uart_instance)
{
    if (rx_pin == tx_pin)
        return false;
    return ((rx_pin == 0 && tx_pin == 1 && uart_instance == uart0) ||   // UART0
            (rx_pin == 4 && tx_pin == 5 && uart_instance == uart1) ||   // UART1
            (rx_pin == 8 && tx_pin == 9 && uart_instance == uart1) ||   // UART1
            (rx_pin == 12 && tx_pin == 13 && uart_instance == uart1) || // UART1
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
static ld2420_uart_rx_t uart_rx[2];
static inline void __init_uart_rx_buffer__(uint8_t idx)
{
    uart_rx[idx].head = 0;
    uart_rx[idx].tail = 0;
    uart_rx[idx].overflow = 0;
}

// rx callback functions for uart0 and uart1
static ld2420_rx_callback_t rx_cbs[2] = {NULL, NULL};

static void uart0_rx_irq_handler(void)
{
    ld2420_uart_rx_t *rb = &uart_rx[0];
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

static void uart1_rx_irq_handler(void)
{
    ld2420_uart_rx_t *rb = &uart_rx[1];
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

void ld2420_pico_process(uint8_t uart_index)
{
    if (uart_index > 1)
        return;
    // TODO: Implement processing logic to read from the ring buffer and invoke the callback
    // This function should read available data from the ring buffer for the specified UART index,
    // assemble complete packets, and call the registered rx_callback with the received packet data.
}

#ifdef __cplusplus
extern "C"
{
#endif
    /**
     * A mutex to protect UART TX operations, ensuring thread-safe access
     * when multiple threads attempt to send data simultaneously.
     *
     * Refer to https://www.raspberrypi.com/documentation/pico-sdk/high_level.html#macro-definition-documentation-5
     * for more information regarding this macro.
     */
    auto_init_mutex(ld2420_uart_tx_mutex);

    ld2420_status_t ld2420_pico_send_safe(
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

        for (uint16_t i = 0; i < buffer_size; i++)
            uart_putc_raw(uart_instance, buffer[i]);
        mutex_exit(&ld2420_uart_tx_mutex);
        return LD2420_OK;
    }

    ld2420_status_t ld2420_pico_init(
        uart_inst_t *uart_instance,
        const uint8_t rx_pin,
        const uint8_t tx_pin,
        const ld2420_rx_callback_t rx_callback)
    {
        if (!validate_uart_pin_pair_instance(rx_pin, tx_pin, uart_instance))
        {
            return LD2420_ERROR_INVALID_ARGUMENTS;
        }

        int8_t idx = decide_uart_instance_number(uart_instance);
        if (idx < 0)
            return LD2420_ERROR_INVALID_ARGUMENTS;

        __init_uart_rx_buffer__(idx);
        rx_cbs[idx] = rx_callback; // Set the RX callback

        // Initialize UART first with the baud rate
        uart_init(uart_instance, LD2420_BAUD_RATE);

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
            return LD2420_ERROR_INVALID_ARGUMENTS | LD2420_ERROR_UNKNOWN;
        }

        // Set the GPIO pin mux to the UART
        // TX and RX pins need to be configured for UART function
        gpio_set_function(tx_pin, GPIO_FUNC_UART);
        gpio_set_function(rx_pin, GPIO_FUNC_UART);
        return LD2420_OK;
    }

    ld2420_status_t ld2420_pico_deinit(uart_inst_t *uart_instance)
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
        rx_cbs[idx] = NULL;
        return LD2420_OK;
    }

#ifdef __cplusplus
}
#endif