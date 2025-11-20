# LD2420 Core Library

The core library provides platform-agnostic functionality for the HLK-LD2420 radar module. This is a standalone CMake project that can be built independently and integrated into any platform.

## Overview

The core library handles:

- **Protocol Parsing**: Complete LD2420 protocol implementation
- **Frame Validation**: Header/footer verification and checksum handling
- **Endianness**: Little-endian byte order handling on any architecture
- **Memory Safety**: Fixed-size buffers with no dynamic allocation
- **Error Handling**: Comprehensive status codes for all operations

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

## Output

The compiled library (`libld2420_core.a`) will be in the build directory you created.

## Running Tests

The core library includes comprehensive unit tests using the Unity framework:

```bash
cd core
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DLD2420_CORE_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Tests cover:

- Frame parsing with valid and invalid data
- Streaming parser state transitions
- Error handling and edge cases
- Endianness conversion
- Buffer overflow protection

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

## Streaming Parser Deep Dive

The streaming parser is designed for incremental processing of UART/serial data where bytes arrive asynchronously.

### Design Principles

1. **Single-Byte Processing**: Each call to `ld2420_stream_feed()` accepts exactly one byte
2. **Automatic Validation**: Buffer state is validated on each byte
3. **Frame-Complete Callbacks**: Callback invoked only when a complete valid frame is assembled
4. **Automatic Recovery**: Corrupted frames are discarded and parser resyncs to next header
5. **Zero Allocation**: Fixed-size buffer with predictable memory usage

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

- **Single Linear Buffer**: No dynamic allocation; fixed memory footprint
- **Automatic Resynchronization**: On frame corruption, automatically searches for the next valid header
- **Noise Tolerant**: Handles garbage data before frames
- **Thread-Unsafe by Design**: Use one context per stream; synchronize if needed for multi-threaded access
- **No Partial Frame Callbacks**: Callback is only invoked on complete, validated frames

### Common Use Cases

**UART Interrupt Handler:**

```c
static ld2420_stream_t stream;

void uart_irq_handler(void) {
    while (uart_is_readable(uart0)) {
        uint8_t byte = uart_getc(uart0);
        ld2420_stream_feed(&stream, &byte, 1, on_frame);
    }
}
```

**Polling Loop:**

```c
ld2420_stream_t stream;
ld2420_stream_init(&stream);

while (1) {
    if (uart_is_readable(uart0)) {
        uint8_t byte = uart_getc(uart0);
        ld2420_status_t status = ld2420_stream_feed(&stream, &byte, 1, on_frame);
        if (status != LD2420_STATUS_OK) {
            printf("Frame error: %d\n", status);
        }
    }
}
```

**Serial Port Reading (Native):**

```c
int fd = open("/dev/ttyUSB0", O_RDWR);
ld2420_stream_t stream;
ld2420_stream_init(&stream);

uint8_t byte;
while (read(fd, &byte, 1) > 0) {
    ld2420_stream_feed(&stream, &byte, 1, on_frame);
}
```
