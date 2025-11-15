#include <stdio.h>
#include <pico/stdlib.h>
#include <ld2420/platform/pico/ld2420_pico.h>

#define UART_TX_PIN 0
#define UART_RX_PIN 1

static const uint8_t OPEN_COMMAND_MODE[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0xFF, 0x00, 0x01, 0x00, 0x04, 0x03, 0x02, 0x01};

void rx_callback(uint8_t uart_index, const uint8_t *data, uint16_t len)
{
    printf("Received %d bytes\n", len);
}

int main(void)
{
    stdio_init_all();

    // Wait for USB connection
    while (!stdio_usb_connected())
    {
        tight_loop_contents();
    }

    printf("Initializing LD2420...\n");
    ld2420_status_t result = ld2420_pico_init(uart0, UART_TX_PIN, UART_RX_PIN, rx_callback);

    if (result != LD2420_OK)
    {
        printf("Init failed!\n");
        return -1;
    }

    for (;;)
    {
        ld2420_pico_process(0);
    }

    return 0;
}