#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "ld2420/platform/pico/ld2420_pico.h"

// UART0 default pins
#define UART_ID uart0
#define UART_TX_PIN 12
#define UART_RX_PIN 13

// Command to read version number
static const uint8_t CMD_READ_VERSION[] = {
    0xFD, 0xFC, 0xFB, 0xFA, // Header
    0x02, 0x00,             // Frame size (2 bytes)
    0x00, 0x00,             // Command: Read version
    0x04, 0x03, 0x02, 0x01  // Footer
};

// Expected minimum response size for LD2420 packets
#define MIN_RESPONSE_SIZE 12

int main()
{
    stdio_init_all();

    // Wait until USB is connected for serial monitoring
    while (!stdio_usb_connected())
    {
        tight_loop_contents();
    }

    printf("\n=== LD2420 Basic Example ===\n");
    printf("Demonstrates interrupt-driven UART communication\n\n");

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
    printf("LD2420 initialized (UART%d, RX=%d, TX=%d)\n",
           uart_get_index(UART_ID), UART_RX_PIN, UART_TX_PIN);

    // Enable interrupt-driven reception
    if (ld2420_pico_enable_interrupts(&ld2420_config) != LD2420_OK)
    {
        printf("ERROR: Failed to enable UART interrupts\n");
        ld2420_pico_deinit(&ld2420_config);
        return -1;
    }
    printf("Interrupt-driven RX enabled\n\n");

    // Send read version command
    printf("Sending READ VERSION command...\n");
    if (ld2420_pico_send(&ld2420_config, CMD_READ_VERSION, sizeof(CMD_READ_VERSION)) != LD2420_OK)
    {
        printf("ERROR: Failed to send command\n");
        ld2420_pico_deinit(&ld2420_config);
        return -1;
    }

    // Wait for response with timeout
    absolute_time_t timeout = make_timeout_time_ms(1000);
    bool response_received = false;

    printf("Waiting for response (timeout: 1000ms)...\n");

    while (absolute_time_diff_us(get_absolute_time(), timeout) > 0)
    {
        // Check if we have at least a minimum valid packet
        if (ld2420_pico_bytes_available(&ld2420_config) >= MIN_RESPONSE_SIZE)
        {
            response_received = true;
            break;
        }

        // Blink LED while waiting
        static absolute_time_t last_blink = 0;
        if (absolute_time_diff_us(last_blink, get_absolute_time()) > 100000) // 100ms
        {
            gpio_put(PICO_DEFAULT_LED_PIN, !gpio_get(PICO_DEFAULT_LED_PIN));
            last_blink = get_absolute_time();
        }
    }

    // Process response
    if (response_received)
    {
        uint16_t bytes_available = ld2420_pico_bytes_available(&ld2420_config);
        printf("\nResponse received (%d bytes)\n", bytes_available);
        printf("Raw data: ");

        uint8_t buffer[128];
        uint16_t bytes_read = ld2420_pico_read_bytes(&ld2420_config, buffer, sizeof(buffer));

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
        printf("\nNo response received (timeout)\n");
        gpio_put(PICO_DEFAULT_LED_PIN, 0); // LED off = failure
    }

    // Check for buffer overflows
    uint32_t overflow_count = ld2420_pico_get_overflow_count(&ld2420_config);
    if (overflow_count > 0)
    {
        printf("Warning: %lu bytes were dropped due to buffer overflow\n", overflow_count);
    }

    // Clean up
    ld2420_pico_deinit(&ld2420_config);
    printf("\n=== Example complete ===\n");

    return 0;
}
