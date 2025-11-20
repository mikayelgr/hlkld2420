# LD2420 Core Library

The core library provides platform-agnostic functionality for the HLK-LD2420 radar module. This is a standalone CMake project that can be built independently.

## Building the Core Library

Follow these steps to build the core library from the `core/` directory:

### Debug Build

```bash
cd core
mkdir -p build/debug
cd build/debug
cmake -DCMAKE_BUILD_TYPE=Debug ../..
cmake --build .
```

### Release Build

```bash
cd core
mkdir -p build/release
cd build/release
cmake -DCMAKE_BUILD_TYPE=Release ../..
cmake --build .
```

## Using the Core Library in Your Project (no install)

Add the core as a subdirectory and link the target directly:

```cmake
add_subdirectory(/path/to/hlkld2420/core ld2420_core)
add_executable(your_app main.c)
target_link_libraries(your_app PRIVATE ld2420_core)
```

## Output Location

The compiled library (`libld2420_core.a`) will be in the build directory you created.

## API Overview

The library provides two main parsing interfaces:

### 1. One-Shot Parser: `ld2420_parse_rx_buffer()`

For complete, pre-assembled frames:

```c
#include <ld2420/ld2420.h>

ld2420_status_t ld2420_parse_rx_buffer(
    const uint8_t *in_raw_rx_buffer,
    const uint8_t in_raw_rx_buffer_size,
    uint16_t *out_frame_size,
    uint16_t *out_cmd_echo,
    uint16_t *out_status);
```

**Use case**: You have the entire frame assembled in a buffer and want to parse it in one call.

### 2. Streaming Parser: `ld2420_stream_feed()`

For incremental byte-by-byte processing (ideal for UART):

```c
#include <ld2420/ld2420_stream.h>

ld2420_stream_t stream;
ld2420_stream_init(&stream);

// Feed one byte at a time
ld2420_status_t status = ld2420_stream_feed(
    &stream,
    &byte,
    1,
    on_frame_callback);
```

**Use case**: Receiving data from UART, serial ports, or any transport that delivers bytes incrementally.

## Streaming API Details

The streaming parser handles the complexity of frame assembly from a byte stream:

### Design Principles

1. **Feed one byte at a time** - Each call to `ld2420_stream_feed()` accepts exactly one byte
2. **Automatic buffer validation** - On each byte, the parser validates the buffer state
3. **Callback on valid frames** - When a complete valid frame is assembled, your callback is invoked
4. **Automatic error handling** - Corrupted frames are discarded and an error status is returned

### State Management

- **NOT_SYNCED**: Scanning for the 4-byte header (`0xFD 0xFC 0xFB 0xFA`)
- **SYNCED**: Header found, accumulating frame bytes
- **FRAME_COMPLETE**: Full frame assembled, footer validated, ready to parse

### Usage Example

```c
#include <ld2420/ld2420_stream.h>

// Frame callback - invoked when a complete valid frame arrives
static bool on_frame(
    const uint8_t *frame,
    uint16_t frame_size_bytes,
    uint16_t cmd_echo,
    uint16_t status)
{
    printf("Frame received: cmd=0x%04X, status=0x%04X, size=%u\n",
           cmd_echo, status, frame_size_bytes);
    return true;  // continue processing
}

// In your UART RX handler or polling loop
void process_uart_byte(uint8_t byte) {
    ld2420_status_t status = ld2420_stream_feed(
        &stream,
        &byte,
        1,
        on_frame);

    if (status != LD2420_STATUS_OK) {
        // Frame was corrupted and discarded; parser already resynced
        printf("Frame error: %d\n", status);
    }
}

// In main setup
int main(void) {
    ld2420_stream_t stream;
    ld2420_stream_init(&stream);
    
    // ... UART/serial setup ...
    
    // Feed bytes as they arrive
    while (1) {
        uint8_t byte = uart_read_byte();
        process_uart_byte(byte);
    }
}
```

### Return Values

- `LD2420_STATUS_OK` - Byte processed successfully (frame may or may not be complete yet)
- `LD2420_STATUS_ERROR_INVALID_ARGUMENTS` - Invalid arguments (e.g., `len != 1`)
- `LD2420_STATUS_ERROR_BUFFER_TOO_SMALL` - Frame size exceeds buffer limits
- `LD2420_STATUS_ERROR_INVALID_FOOTER` - Frame footer validation failed
- `LD2420_STATUS_ERROR_INVALID_PACKET` - Frame parsing failed

### Key Features

- **Single linear buffer** - No dynamic allocation; fixed memory footprint
- **Automatic resynchronization** - On frame corruption, automatically searches for the next valid header
- **Noise tolerant** - Handles garbage data before frames
- **Thread-unsafe by design** - Use one context per stream; synchronize if needed for multi-threaded access
- **No callbacks on partial frames** - Callback is only invoked on complete, validated frames
