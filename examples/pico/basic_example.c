#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "pico/multicore.h"

// UART0 default pins
#define UART_ID uart0
#define BAUD_RATE 115200
#define UART_TX_PIN 0
#define UART_RX_PIN 1

void core1_launch_entry()
{
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    for (;;)
    {
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        sleep_ms(200);

        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        sleep_ms(200);
    }
}

int main()
{
    stdio_init_all();

    // Wait until USB is connected to run the program
    while (!stdio_usb_connected())
    {
        sleep_ms(100);
    }

    printf("\n=== UART0 Test Program ===\n");

    // Initialize UART0 with proper baud rate
    uart_init(UART_ID, BAUD_RATE);

    // Set the TX and RX pins for UART0
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // Set UART flow control (CTS/RTS) - disabled
    uart_set_hw_flow(UART_ID, false, false);

    // Set data format: 8 data bits, 1 stop bit, no parity
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);

    // Turn on FIFO's
    uart_set_fifo_enabled(UART_ID, true);

    printf("UART0 initialized: %d baud, TX=GP%d, RX=GP%d\n", BAUD_RATE, UART_TX_PIN, UART_RX_PIN);

    // Start LED blinking on core 1
    multicore_launch_core1(core1_launch_entry);

    // Main loop - continuously check for incoming data
    printf("Waiting for UART data...\n");

    while (true)
    {
        const uint8_t test_msg[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0xFF, 0x00, 0x01, 0x00, 0x04, 0x03, 0x02, 0x01};
        uart_write_blocking(UART_ID, test_msg, sizeof(test_msg));
        printf("Sent %d bytes: ", sizeof(test_msg));
        for (int i = 0; i < sizeof(test_msg); i++)
        {
            printf("0x%02X ", test_msg[i]);
        }
        printf("\n");

        // Wait a bit and collect any response
        sleep_ms(100);

        // Check if data is available and read it byte by byte
        if (uart_is_readable(UART_ID))
        {
            printf("Received: ");
            while (uart_is_readable(UART_ID))
            {
                uint8_t byte = uart_getc(UART_ID);
                printf("0x%02X ", byte);
            }
            printf("\n");
        }
        else
        {
            printf("No data received\n");
        }
    }

    return 0;
}
