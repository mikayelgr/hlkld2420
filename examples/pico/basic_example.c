#include <stdio.h>
#include "pico/stdlib.h"
#include "ld2420/platform/pico/ld2420_pico.h"

int main()
{
    // Initialize stdio for USB
    stdio_init_all();

    printf("LD2420 Pico Example Starting...\n");

    // TODO: Initialize your LD2420 sensor here
    // Example:
    // ld2420_t sensor;
    // ld2420_pico_config_t config = {
    //     .uart_id = uart0,
    //     .tx_pin = 0,
    //     .rx_pin = 1,
    //     .baud_rate = 115200
    // };
    // ld2420_pico_init(&sensor, &config);

    printf("LD2420 initialized successfully!\n");

    while (true)
    {
        // Your sensor reading logic here
        printf("Reading sensor data...\n");
        sleep_ms(1000);
    }

    return 0;
}
