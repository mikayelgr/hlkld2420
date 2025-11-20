# LD2420 Raspberry Pi Pico Platform

This platform library provides a complete integration of the LD2420 core library for Raspberry Pi Pico, including interrupt-driven UART communication, ring buffering, and frame assembly.

## Features

- **Interrupt-Driven UART**: Efficient RX handling with hardware interrupts
- **Ring Buffer**: Circular buffer for incoming data
- **Automatic Frame Assembly**: Transparent frame parsing using streaming parser
- **Thread-Safe Transmission**: Mutex-protected send operations
- **Dual UART Support**: Works with both uart0 and uart1

## Prerequisites

Before building for the Raspberry Pi Pico, ensure you have the following tools installed:

### Required Packages

Install the ARM cross-compilation toolchain using your package manager:

**On macOS (using Homebrew):**

```bash
brew install arm-none-eabi-gcc gcc-arm-embedded arm-none-eabi-binutils
```

**On Ubuntu/Debian:**

```bash
sudo apt-get install gcc-arm-none-eabi binutils-arm-none-eabi
```

### Pico SDK

You must have the Raspberry Pi Pico SDK installed and the `PICO_SDK_PATH` environment variable set. If you haven't set this up yet:

1. Clone the Pico SDK:

   ```bash
   git clone https://github.com/raspberrypi/pico-sdk.git
   cd pico-sdk
   git submodule update --init
   ```

2. Set the environment variable (add to your shell profile for persistence):

   ```bash
   export PICO_SDK_PATH=/path/to/pico-sdk
   ```

## Building

The Pico platform library is a standalone CMake project. It depends on the core library, which will be built automatically if not found.

Follow these steps to build from the `platform/pico/` directory:

### Debug Build

```bash
cd platform/pico
mkdir -p build/debug
cd build/debug
cmake -DCMAKE_BUILD_TYPE=Debug -DPICO_SDK_PATH=$PICO_SDK_PATH ../..
cmake --build .
```

### Release Build

```bash
cd platform/pico
mkdir -p build/release
cd build/release
cmake -DCMAKE_BUILD_TYPE=Release -DPICO_SDK_PATH=$PICO_SDK_PATH ../..
cmake --build .
```

## Using the Pico Library in Your Project (no install)

Add the Pico platform as a subdirectory (it will pull in the core if needed):

```cmake
add_subdirectory(/path/to/hlkld2420/platform/pico ld2420_pico)
add_executable(your_pico_app main.c)
target_link_libraries(your_pico_app PRIVATE ld2420_pico pico_stdlib)
```

## Output

The compiled library (`libld2420_pico.a`) will be in the build directory you created.

## Hardware Setup

### Wiring

Connect the LD2420 module to your Raspberry Pi Pico:

| LD2420 Pin | Pico Pin | Description |
|------------|----------|-------------|
| VCC | 3.3V (Pin 36) | Power supply |
| GND | GND (Pin 38) | Ground |
| TX | GP1 (default RX) | UART receive |
| RX | GP0 (default TX) | UART transmit |

**Note**: The LD2420 TX connects to Pico RX, and LD2420 RX connects to Pico TX.

### Pin Configuration

You can use any GPIO pins that support UART. Common configurations:

**UART0**: GP0 (TX), GP1 (RX), GP4 (TX), GP5 (RX), GP12 (TX), GP13 (RX), GP16 (TX), GP17 (RX)

**UART1**: GP8 (TX), GP9 (RX)

Refer to the [Pico datasheet](https://datasheets.raspberrypi.com/pico/pico-datasheet.pdf) for complete pin mapping.

## Usage Example

### Basic Initialization

```c
#include <ld2420/platform/pico/ld2420_pico.h>
#include <pico/stdlib.h>

#define UART_TX_PIN 0
#define UART_RX_PIN 1

void rx_callback(uint8_t uart_index, const uint8_t *packet, uint16_t len) {
    printf("Received %d bytes\n", len);
    // Process the complete frame
}

int main(void) {
    stdio_init_all();
    
    // Initialize LD2420
    ld2420_status_t result = ld2420_pico_init(
        uart0, 
        UART_TX_PIN, 
        UART_RX_PIN, 
        rx_callback
    );
    
    if (result != LD2420_STATUS_OK) {
        printf("Init failed: %d\n", result);
        return -1;
    }
    
    // Main loop
    while (1) {
        ld2420_pico_process(0);  // Process pending frames
        sleep_ms(10);
    }
}
```

### Sending Commands

```c
// Open configuration mode
const uint8_t open_config[] = {
    0xFD, 0xFC, 0xFB, 0xFA,  // Header
    0x04, 0x00,              // Length
    0xFF, 0x00,              // Command
    0x01, 0x00,              // Data
    0x04, 0x03, 0x02, 0x01   // Footer
};

ld2420_pico_send_safe(uart0, open_config, sizeof(open_config));
```

## Troubleshooting

### No Data Received

**Symptoms**: `rx_callback` is never called, no frames received.

**Solutions**:

1. **Check Wiring**: Verify TX/RX are crossed correctly (LD2420 TX → Pico RX)
2. **Check Power**: Ensure module is receiving 3.3V power
3. **Verify Baud Rate**: Should be 115200 (set automatically by library)
4. **Check GPIO Pins**: Ensure pins support UART function
5. **Module Not in Config Mode**: Some commands require entering config mode first

### Garbled or Incomplete Data

**Symptoms**: Frames are corrupted, parser returns errors.

**Solutions**:

1. **Check Ground Connection**: Ensure common ground between Pico and LD2420
2. **Cable Length**: Keep wires short to avoid signal degradation
3. **Power Stability**: Use capacitors near module if power is noisy
4. **Call Process Regularly**: Ensure `ld2420_pico_process()` is called frequently

### Build Errors

**Issue**: `PICO_SDK_PATH not found`

**Solution**:

```bash
export PICO_SDK_PATH=/path/to/pico-sdk
```

**Issue**: `arm-none-eabi-gcc not found`

**Solution**:

```bash
# macOS
brew install arm-none-eabi-gcc

# Ubuntu/Debian
sudo apt-get install gcc-arm-none-eabi
```

### Memory Issues

**Symptoms**: Crashes, stack corruption, or erratic behavior.

**Solutions**:

1. **Stack Size**: Increase stack size in CMakeLists.txt if deeply nested calls
2. **Ring Buffer Overflow**: Reduce polling interval or increase buffer size
3. **Check Buffer Sizes**: Ensure packet buffers are sized for `LD2420_MAX_RX_PACKET_SIZE`

## Performance Considerations

### Processing Frequency

Call `ld2420_pico_process()` frequently enough to prevent ring buffer overflow:

- **High Data Rate**: Call every 1-10ms
- **Low Data Rate**: Call every 10-100ms
- **Interrupt-Only**: Can be less frequent if IRQ handles most traffic

### Memory Usage

Per UART instance:

- Ring buffer: 256 bytes
- Stream parser buffer: 154 bytes (LD2420_MAX_RX_PACKET_SIZE)
- Total: ~410 bytes per UART

### CPU Usage

- **IRQ Handler**: Minimal, just copies bytes to ring buffer
- **Process Function**: Parses accumulated frames, depends on data rate
- **Callbacks**: User-defined, keep short to avoid blocking
