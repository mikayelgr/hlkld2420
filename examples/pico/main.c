#include <stdio.h>
#include <pico/stdlib.h>
#include <hardware/gpio.h>
#include <ld2420/platform/pico/ld2420_pico.h>

#define UART_TX_PIN 0
#define UART_RX_PIN 1

// Commands
static const uint8_t OPEN_CONFIG_MODE[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0xFF, 0x00, 0x01, 0x00, 0x04, 0x03, 0x02, 0x01};
static const uint8_t READ_VERSION[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0x00, 0x00, 0x04, 0x03, 0x02, 0x01};
static const uint8_t CLOSE_CONFIG_MODE[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0xFE, 0x00, 0x04, 0x03, 0x02, 0x01};

#define MAX_PACKET_SIZE 256
static uint8_t packet_buffer[MAX_PACKET_SIZE];
static volatile uint16_t packet_index = 0;

void rx_callback(uint8_t uart_index, const uint8_t *data, uint16_t len)
{
    // Collect bytes into buffer
    if (packet_index + len <= MAX_PACKET_SIZE)
    {
        for (uint16_t i = 0; i < len; i++)
        {
            packet_buffer[packet_index++] = data[i];
        }
    }
}

void print_packet(void)
{
    printf("Packet (%d bytes): ", packet_index);
    for (uint16_t i = 0; i < packet_index; i++)
    {
        printf("%02X ", packet_buffer[i]);
    }
    printf("\n");
}

int main(void)
{
    stdio_init_all();

    // Wait for USB connection
    while (!stdio_usb_connected())
    {
        tight_loop_contents();
    }

    printf("\n=== LD2420 Pico Example ===\n\n");

    // Initialize LED
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 1);

    // Initialize LD2420
    printf("Initializing LD2420...\n");
    ld2420_status_t result = ld2420_pico_init(uart0, UART_TX_PIN, UART_RX_PIN, rx_callback);

    if (result != 0)
    {
        printf("Init failed! Error code: %d\n", result);
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        return -1;
    }

    printf("LD2420 initialized successfully.\n\n");

    // Main loop - send commands and collect responses
    int command_count = 0;

    for (;;)
    {
        command_count++;
        printf("\n--- Command %d ---\n", command_count);

        // Reset packet buffer
        packet_index = 0;

        // Send command
        printf("Sending OPEN CONFIG MODE command...\n");

        ld2420_pico_send_safe(uart0, OPEN_CONFIG_MODE, sizeof(OPEN_CONFIG_MODE));
        ld2420_pico_process(0);
        if (packet_index > 0)
        {
            print_packet();
            gpio_put(PICO_DEFAULT_LED_PIN, !gpio_get(PICO_DEFAULT_LED_PIN)); // Toggle LED
        }
        else
        {
            printf("No response received.\n");
        }
    }

    return 0;
}