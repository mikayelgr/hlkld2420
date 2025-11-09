#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "ld2420/platform/pico/ld2420_pico.h"

// UART0 default pins
#define UART_ID uart0
#define BAUD_RATE 115200
#define UART_TX_PIN 12
#define UART_RX_PIN 13

// Circular buffer for received data
#define RX_BUFFER_SIZE 256
static uint8_t rx_buffer[RX_BUFFER_SIZE];
static volatile uint16_t rx_write_idx = 0;
static volatile uint16_t rx_read_idx = 0;
static volatile bool response_received = false;

// Command to open configuration mode
static uint8_t CMD_OPEN_CONFIG_MODE[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0xFF, 0x00, 0x01, 0x00, 0x04, 0x03, 0x02, 0x01};
#define CMD_OPEN_CONFIG_MODE_LEN 14

// Expected response length for open config command (adjust based on your sensor)
#define EXPECTED_RESPONSE_LEN 14

// UART RX interrupt handler
void on_uart_rx()
{
    while (uart_is_readable(UART_ID))
    {
        uint8_t ch = uart_getc(UART_ID);
        
        // Store byte in circular buffer
        uint16_t next_write_idx = (rx_write_idx + 1) % RX_BUFFER_SIZE;
        
        // Check for buffer overflow
        if (next_write_idx != rx_read_idx)
        {
            rx_buffer[rx_write_idx] = ch;
            rx_write_idx = next_write_idx;
            
            // Simple response detection: if we've received enough bytes
            uint16_t bytes_available = (rx_write_idx >= rx_read_idx) 
                ? (rx_write_idx - rx_read_idx) 
                : (RX_BUFFER_SIZE - rx_read_idx + rx_write_idx);
            
            if (bytes_available >= EXPECTED_RESPONSE_LEN)
            {
                response_received = true;
            }
        }
        else
        {
            // Buffer overflow - data lost
            printf("Warning: RX buffer overflow!\n");
        }
    }
}

// Get number of bytes available in buffer
uint16_t rx_bytes_available()
{
    if (rx_write_idx >= rx_read_idx)
    {
        return rx_write_idx - rx_read_idx;
    }
    else
    {
        return RX_BUFFER_SIZE - rx_read_idx + rx_write_idx;
    }
}

// Read a byte from the buffer
bool rx_buffer_read(uint8_t *byte)
{
    if (rx_read_idx == rx_write_idx)
    {
        // Buffer empty
        return false;
    }
    
    *byte = rx_buffer[rx_read_idx];
    rx_read_idx = (rx_read_idx + 1) % RX_BUFFER_SIZE;
    return true;
}

// Setup UART interrupt
void setup_uart_interrupt(uart_inst_t *uart)
{
    // Get the IRQ number for the UART
    int uart_irq = (uart == uart0) ? UART0_IRQ : UART1_IRQ;
    
    // Set up the interrupt handler
    irq_set_exclusive_handler(uart_irq, on_uart_rx);
    
    // Enable the UART interrupt
    irq_set_enabled(uart_irq, true);
    
    // Enable UART RX interrupts (but not TX)
    uart_set_irq_enables(uart, true, false);
}

int main()
{
    stdio_init_all();

    // Wait until tty is connected for serial monitoring
    while (!stdio_usb_connected())
    {
        tight_loop_contents();
    }

    printf("Starting LD2420 interrupt-driven example...\n");

    ld2420_pico_t ld2420_config;
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 1); // Turn on LED to indicate start

    // Initialize the LD2420
    if (ld2420_pico_init(&ld2420_config, UART_RX_PIN, UART_TX_PIN, UART_ID) == LD2420_OK)
    {
        printf("LD2420 initialized successfully.\n");
    }
    else
    {
        printf("Failed to initialize LD2420.\n");
        return -1;
    }

    // Setup interrupt handler for UART RX
    setup_uart_interrupt(UART_ID);
    printf("UART interrupt enabled.\n");

    // Send the open config command
    if (ld2420_pico_send(&ld2420_config, CMD_OPEN_CONFIG_MODE, CMD_OPEN_CONFIG_MODE_LEN) == LD2420_OK)
    {
        printf("Sent OPEN CONFIG MODE command.\n");
    }
    else
    {
        printf("Failed to send OPEN CONFIG MODE command.\n");
        return -1;
    }

    // Wait for response with timeout (non-blocking in main loop)
    absolute_time_t timeout = make_timeout_time_ms(2000); // 2 second timeout
    bool timeout_occurred = false;
    
    printf("Waiting for response...\n");
    
    while (!response_received && !timeout_occurred)
    {
        // Check for timeout
        if (absolute_time_diff_us(get_absolute_time(), timeout) <= 0)
        {
            timeout_occurred = true;
            printf("Timeout waiting for sensor response!\n");
            break;
        }
        
        // Blink LED while waiting (optional - shows we're not frozen)
        static absolute_time_t last_blink = 0;
        if (absolute_time_diff_us(last_blink, get_absolute_time()) > 250000) // 250ms
        {
            gpio_put(PICO_DEFAULT_LED_PIN, !gpio_get(PICO_DEFAULT_LED_PIN));
            last_blink = get_absolute_time();
        }
        
        // Could do other work here while waiting...
        tight_loop_contents();
    }

    // Process received data
    if (response_received)
    {
        printf("Response received! Bytes available: %d\n", rx_bytes_available());
        printf("Received data: ");
        
        uint8_t byte;
        while (rx_buffer_read(&byte))
        {
            printf("0x%02X ", byte);
        }
        printf("\n");
        
        gpio_put(PICO_DEFAULT_LED_PIN, 1); // LED on = success
    }
    else
    {
        printf("No response received.\n");
        gpio_put(PICO_DEFAULT_LED_PIN, 0); // LED off = failure
    }

    // Disable interrupts before cleanup
    int uart_irq = (UART_ID == uart0) ? UART0_IRQ : UART1_IRQ;
    irq_set_enabled(uart_irq, false);
    uart_set_irq_enables(UART_ID, false, false);

    printf("\nExample complete.\n");
    
    // Keep program running to see results
    while (true)
    {
        tight_loop_contents();
    }

    return 0;
}
