# Building for Raspberry Pi Pico

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

Follow the steps below to build the Raspberry Pi Pico target of the project from the root directory:

### Debug Build

1. **Configure the Project**  
   Use CMake presets to configure for Pico debug build:

   ```bash
   cmake --preset pico-debug
   ```

2. **Compile the Library**  
   Build the library using the preset:

   ```bash
   cmake --build --preset pico-debug
   ```

### Release Build

1. **Configure the Project**  
   Use CMake presets to configure for Pico release build (optimized):

   ```bash
   cmake --preset pico-release
   ```

2. **Compile the Library**  
   Build the library using the preset:

   ```bash
   cmake --build --preset pico-release
   ```

### Output Location

After a successful build, the compiled library (`libld2420_pico.[a|dll|so]`) will be located in:

- Debug: `build/pico-debug/platform/pico`
- Release: `build/pico-release/platform/pico`

You can link this library to your application as required.

## Interrupt-Driven UART Communication

The Raspberry Pi Pico platform implementation provides **interrupt-driven UART reception** for robust, non-blocking communication with the LD2420 sensor module.

### Features

- **Non-Blocking Operation**: Main program loop can perform other tasks while data is received via interrupts
- **Ring Buffer**: 256-byte circular buffer automatically stores incoming UART data
- **Efficient**: CPU only engages when data arrives, no polling required
- **Data Integrity**: Prevents packet loss by handling bytes immediately when received

### Basic Usage

```c
#include "ld2420/platform/pico/ld2420_pico.h"
#include "pico/stdlib.h"

int main() {
    ld2420_pico_t config;
    
    // Initialize the LD2420 with UART pins
    ld2420_pico_init(&config, RX_PIN, TX_PIN, uart0);
    
    // Enable interrupt-driven reception
    ld2420_pico_enable_interrupts(&config);
    
    // Send a command
    uint8_t cmd[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0xFE, 0x00, 0x04, 0x03, 0x02, 0x01};
    ld2420_pico_send(&config, cmd, sizeof(cmd));
    
    // Check for received data (non-blocking)
    while (ld2420_pico_bytes_available(&config) > 0) {
        uint8_t byte;
        if (ld2420_pico_read_byte(&config, &byte)) {
            // Process byte
        }
    }
    
    // Clean up
    ld2420_pico_deinit(&config);
}
```

### API Functions

- **`ld2420_pico_enable_interrupts(config)`** - Enable interrupt-driven UART RX
- **`ld2420_pico_disable_interrupts(config)`** - Disable interrupt-driven UART RX
- **`ld2420_pico_bytes_available(config)`** - Get count of buffered bytes
- **`ld2420_pico_read_byte(config, byte)`** - Read single byte from buffer
- **`ld2420_pico_read_bytes(config, buffer, length)`** - Read multiple bytes

### Examples

See the `examples/pico/` directory for complete examples:
- `basic_example.c` - Demonstrates interrupt-driven communication
- `interrupt_example.c` - Advanced interrupt handling with timeouts

## Thread Safety and Multi-Core Considerations

### Interrupt Safety

The ring buffer implementation is designed to be interrupt-safe for the single-producer (ISR), single-consumer (main loop) pattern:

- **Write operations** (in ISR): Update `write_idx` and `overflow_count`
- **Read operations** (in main loop): Update `read_idx`
- **Shared state**: The `volatile` keyword ensures variables are not cached inappropriately

### Memory Ordering

The implementation relies on the ARM Cortex-M0+ memory model guarantees:

- **Sequential consistency**: Memory operations appear in program order
- **Atomic reads/writes**: 16-bit and 32-bit aligned operations are atomic
- **No reordering**: The processor does not reorder memory operations across `volatile` accesses

For other architectures or more complex scenarios, consider using memory barriers.

### Multi-Core Usage

If using both cores on the RP2040:

1. **Same UART on both cores**: Not supported - only one core should access each UART instance
2. **Different UARTs**: Each core can safely use a different UART (uart0 on core 0, uart1 on core 1)
3. **Shared access**: Requires additional synchronization (mutexes, critical sections)

### Best Practices

- ✅ **DO**: Call read functions (`ld2420_pico_read_byte`, `ld2420_pico_read_bytes`) from main loop
- ✅ **DO**: Use `ld2420_pico_clear_buffer()` which includes interrupt disabling
- ✅ **DO**: Check `ld2420_pico_get_overflow_count()` periodically to detect data loss
- ❌ **DON'T**: Call read functions from multiple threads without synchronization
- ❌ **DON'T**: Manually modify ring buffer fields - use provided functions
- ❌ **DON'T**: Share a single LD2420 instance across cores without proper locking
