#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "ld2420/platform/pico/ld2420_pico.h"

// UART0 default pins
#define UART_ID uart0
#define BAUD_RATE 115200
#define UART_TX_PIN 12
#define UART_RX_PIN 13

// Command to open configuration mode
static uint8_t CMD_OPEN_CONFIG_MODE[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0xFF, 0x00, 0x01, 0x00, 0x04, 0x03, 0x02, 0x01};
#define CMD_OPEN_CONFIG_MODE_LEN 14

// Command to close configuration mode
static uint8_t CMD_CLOSE_CONFIG_MODE[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0xFE, 0x00, 0x04, 0x03, 0x02, 0x01};
#define CMD_CLOSE_CONFIG_MODE_LEN 12

// Command to read version
static uint8_t CMD_READ_VERSION[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0x00, 0x00, 0x04, 0x03, 0x02, 0x01};
#define CMD_READ_VERSION_LEN 12

int main()
{
    stdio_init_all();

    // Wait until tty is connected for serial monitoring and run the program
    while (!stdio_usb_connected())
    {
        tight_loop_contents();
    }

    printf("Starting LD2420 interrupt-driven example...\n");

    ld2420_pico_t ld2420_config;
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 1); // Turn on LED to indicate start

    if (ld2420_pico_init(&ld2420_config, UART_RX_PIN, UART_TX_PIN, UART_ID) == LD2420_OK)
    {
        printf("LD2420 initialized successfully.\n");
    }
    else
    {
        printf("Failed to initialize LD2420.\n");
        return -1;
    }

    // Enable interrupt-driven UART reception
    if (ld2420_pico_enable_interrupts(&ld2420_config) == LD2420_OK)
    {
        printf("UART interrupts enabled.\n");
    }
    else
    {
        printf("Failed to enable UART interrupts.\n");
        return -1;
    }

    if (ld2420_pico_send(&ld2420_config, CMD_CLOSE_CONFIG_MODE, sizeof(CMD_CLOSE_CONFIG_MODE)) == LD2420_OK)
    {
        printf("Sent CLOSE CONFIG MODE command.\n");
    }
    else
    {
        printf("Failed to send CLOSE CONFIG MODE command.\n");
        return -1;
    }

    // Wait for response with timeout (non-blocking)
    absolute_time_t timeout = make_timeout_time_ms(2000); // 2 second timeout
    
    printf("Waiting for response...\n");
    
    while (absolute_time_diff_us(get_absolute_time(), timeout) > 0)
    {
        // Check if we have received data
        if (ld2420_pico_bytes_available(&ld2420_config) >= sizeof(CMD_CLOSE_CONFIG_MODE))
        {
            break;
        }
        
        // Could do other work here while waiting...
        tight_loop_contents();
    }

    // Read and display received data
    uint16_t bytes_available = ld2420_pico_bytes_available(&ld2420_config);
    if (bytes_available > 0)
    {
        printf("Received %d bytes:\n", bytes_available);
        
        uint8_t byte;
        while (ld2420_pico_read_byte(&ld2420_config, &byte))
        {
            printf("0x%02X ", byte);
        }
        printf("\n");
    }
    else
    {
        printf("No response received (timeout).\n");
    }

    // Clean up
    ld2420_pico_deinit(&ld2420_config);
    printf("\nExample complete.\n");
    
    return 0;
}
