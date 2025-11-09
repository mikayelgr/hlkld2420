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

    if (ld2420_pico_send(&ld2420_config, CMD_CLOSE_CONFIG_MODE, sizeof(CMD_CLOSE_CONFIG_MODE)) == LD2420_OK)
    {
        printf("Sent CLOSE CONFIG MODE command.\n");
    }
    else
    {
        printf("Failed to send CLOSE CONFIG MODE command.\n");
        return -1;
    }

    while (!uart_is_readable(ld2420_config.uart_instance))
    {
        tight_loop_contents();
    }
    while (uart_is_readable(ld2420_config.uart_instance))
    {
        uint8_t ch = uart_getc(ld2420_config.uart_instance);
        printf("Received: 0x%02X\n", ch);
    }

    printf("\n");
    return 0;
}
