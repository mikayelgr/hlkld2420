#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "ld2420/platform/pico/ld2420_pico.h"

// UART0 default pins
#define UART_ID uart0
#define UART_TX_PIN 12
#define UART_RX_PIN 13

// Command to open configuration mode
static const uint8_t CMD_OPEN_CONFIG_MODE[] = {
    0xFD, 0xFC, 0xFB, 0xFA, // Header
    0x04, 0x00,             // Frame size (4 bytes)
    0xFF, 0x00,             // Command: Open config mode
    0x01, 0x00,             // Parameter
    0x04, 0x03, 0x02, 0x01  // Footer
};

// Expected minimum response size
#define MIN_RESPONSE_SIZE 12

int main()
{
    stdio_init_all();

    // Wait until USB is connected for serial monitoring
    while (!stdio_usb_connected())
    {
        tight_loop_contents();
    }

    printf("\n=== LD2420 Advanced Interrupt Example ===\n");
    printf("Demonstrates non-blocking operation with LED feedback\n\n");

    // Initialize LED for status indication
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 1);

    // Initialize LD2420 configuration
    ld2420_pico_t ld2420_config;
    if (ld2420_pico_init(&ld2420_config, UART_RX_PIN, UART_TX_PIN, UART_ID) != LD2420_OK)
    {
        printf("ERROR: Failed to initialize LD2420\n");
        return -1;
    }
    printf("✓ LD2420 initialized (UART%d, RX=%d, TX=%d)\n",
           uart_get_index(UART_ID), UART_RX_PIN, UART_TX_PIN);

    // Enable interrupt-driven reception using library function
    if (ld2420_pico_enable_interrupts(&ld2420_config) != LD2420_OK)
    {
        printf("ERROR: Failed to enable UART interrupts\n");
        ld2420_pico_deinit(&ld2420_config);
        return -1;
    }
    printf("Interrupt-driven RX enabled\n");
    printf("Ring buffer size: %d bytes\n\n", LD2420_PICO_RX_BUFFER_SIZE);

    // Send open config mode command
    printf("Sending OPEN CONFIG MODE command...\n");
    if (ld2420_pico_send(&ld2420_config, CMD_OPEN_CONFIG_MODE, sizeof(CMD_OPEN_CONFIG_MODE)) != LD2420_OK)
    {
        printf("ERROR: Failed to send command\n");
        ld2420_pico_deinit(&ld2420_config);
        return -1;
    }

    // Wait for response with timeout and LED blinking
    absolute_time_t timeout = make_timeout_time_ms(2000);
    bool response_received = false;
    uint32_t blink_count = 0;

    printf("Waiting for response (timeout: 2000ms)...\n");
    printf("LED blinking indicates the system is responsive\n");

    while (absolute_time_diff_us(get_absolute_time(), timeout) > 0)
    {
        // Check if we have received enough data
        uint16_t bytes_available = ld2420_pico_bytes_available(&ld2420_config);
        if (bytes_available >= MIN_RESPONSE_SIZE)
        {
            response_received = true;
            break;
        }

        // Blink LED while waiting - demonstrates non-blocking operation
        static absolute_time_t last_blink = 0;
        if (absolute_time_diff_us(last_blink, get_absolute_time()) > 250000) // 250ms
        {
            gpio_put(PICO_DEFAULT_LED_PIN, !gpio_get(PICO_DEFAULT_LED_PIN));
            last_blink = get_absolute_time();
            blink_count++;
            printf(".");
            fflush(stdout);
        }

        // Simulate doing other work in main loop
        tight_loop_contents();
    }
    printf("\n");

    // Process received data
    if (response_received)
    {
        uint16_t bytes_available = ld2420_pico_bytes_available(&ld2420_config);
        printf("\nResponse received after %lu blinks (%d bytes)\n",
               blink_count, bytes_available);

        // Read all available data using library function
        uint8_t buffer[128];
        uint16_t bytes_read = ld2420_pico_read_bytes(&ld2420_config, buffer, sizeof(buffer));

        printf("Raw data: ");
        for (uint16_t i = 0; i < bytes_read; i++)
        {
            printf("%02X ", buffer[i]);
            if ((i + 1) % 16 == 0)
                printf("\n          ");
        }
        printf("\n");

        gpio_put(PICO_DEFAULT_LED_PIN, 1); // LED on = success
    }
    else
    {
        printf("\nTimeout - no response received\n");
        gpio_put(PICO_DEFAULT_LED_PIN, 0); // LED off = failure
    }

    // Check for buffer overflows using library function
    uint32_t overflow_count = ld2420_pico_get_overflow_count(&ld2420_config);
    if (overflow_count > 0)
    {
        printf("Warning: %lu bytes were dropped due to buffer overflow\n", overflow_count);
    }
    else
    {
        printf("No buffer overflows detected\n");
    }

    // Demonstrate buffer clearing
    if (ld2420_pico_bytes_available(&ld2420_config) > 0)
    {
        printf("\nClearing remaining buffer data...\n");
        ld2420_pico_clear_buffer(&ld2420_config);
        printf("Buffer cleared (bytes available: %d)\n",
               ld2420_pico_bytes_available(&ld2420_config));
    }

    // Clean up using library function (automatically disables interrupts)
    ld2420_pico_deinit(&ld2420_config);
    printf("\n=== Example complete ===\n");
    printf("Note: This example demonstrated non-blocking interrupt-driven I/O\n");
    printf("      The main loop remained responsive throughout the operation\n");

    // Keep program running to see results
    while (true)
    {
        tight_loop_contents();
    }

    return 0;
}
