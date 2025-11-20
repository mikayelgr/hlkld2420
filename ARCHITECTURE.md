# Architecture Documentation

This document describes the architectural design of the HLK-LD2420 library, including design principles, component organization, and implementation details.

## Design Philosophy

### Core Principles

**Platform Agnostic**: The core library contains no platform-specific code, making it portable across any system that supports C99.

**Zero Dependencies**: Core functionality requires only the C standard library, ensuring minimal integration complexity.

**Fixed Memory**: No dynamic memory allocation. All buffers are statically sized based on protocol limits.

**Explicit Error Handling**: All functions return status codes. No exceptions, no hidden failures.

**Single Responsibility**: Each module has a clear, focused purpose with minimal coupling.

## System Architecture

### Layer Architecture

The library is organized into three distinct layers:

```text
┌─────────────────────────────────────────────────┐
│           Application Code                      │
│  (User's embedded application or tool)          │
└────────────────┬────────────────────────────────┘
                 │
                 ↓
┌─────────────────────────────────────────────────┐
│        Platform Layer                           │
│  ld2420_pico, ld2420_esp32, etc.                │
│  - UART hardware abstraction                    │
│  - Interrupt handling                           │
│  - Ring buffering                               │
│  - Platform-specific initialization             │
└────────────────┬────────────────────────────────┘
                 │
                 ↓
┌─────────────────────────────────────────────────┐
│           Core Library                          │
│         ld2420_core                             │
│  - Protocol parsing                             │
│  - Frame validation                             │
│  - Streaming parser                             │
│  - Command/response structures                  │
└────────────────┬────────────────────────────────┘
                 │
                 ↓
┌─────────────────────────────────────────────────┐
│      LD2420 Protocol Specification              │
│  (Hardware communication protocol)              │
└─────────────────────────────────────────────────┘
```

### Component Breakdown

#### Core Library (`core/`)

**Purpose**: Platform-independent protocol implementation.

**Components**:

- `ld2420.c/h` - One-shot frame parser
- `ld2420_stream.c/h` - Incremental streaming parser

**Responsibilities**:

- Parse complete frames into structured data
- Validate frame integrity (header, footer, length)
- Handle little-endian byte order conversion
- Provide error detection and reporting
- Maintain parser state for streaming

**Does NOT**:

- Perform I/O operations
- Manage hardware resources
- Handle threading or synchronization
- Allocate dynamic memory

#### Platform Layer (`platform/`)

**Purpose**: Hardware abstraction and integration.

**Components** (example: Pico):

- `ld2420_pico.c/h` - Pico-specific implementation
- UART initialization and configuration
- Interrupt service routines
- Ring buffer management

**Responsibilities**:

- Initialize and configure UART hardware
- Handle RX interrupts and buffer incoming data
- Provide thread-safe transmission
- Integrate core parser with hardware I/O
- Manage platform-specific resources

**Does NOT**:

- Implement protocol parsing (delegates to core)
- Make assumptions about application structure
- Perform business logic

#### Examples (`examples/`)

**Purpose**: Demonstrate complete integration patterns.

**Components**:

- Complete working applications
- Hardware wiring diagrams
- Build configurations
- Usage patterns

## Core Library Design

### Frame Structure

The LD2420 protocol defines frames with the following structure:

```text
┌──────────┬────────┬─────────────────┬──────────┐
│  Header  │ Length │   Payload       │  Footer  │
│ (4 bytes)│(2 bytes)│  (N bytes)      │(4 bytes) │
└──────────┴────────┴─────────────────┴──────────┘
```

**Header**: Always `0xFD 0xFC 0xFB 0xFA`

**Length**: 16-bit little-endian payload size

**Payload**: Variable length command/response data

**Footer**: Always `0x04 0x03 0x02 0x01`

### Parser Implementation

#### One-Shot Parser

**Function**: `ld2420_parse_rx_buffer()`

**Use Case**: Complete frames already assembled in memory.

**Flow**:

1. Validate buffer pointer and size
2. Verify header bytes
3. Extract and validate length field
4. Compute expected total size
5. Verify footer bytes
6. Extract command echo and status
7. Return parsed values

**Complexity**: O(1) - Fixed number of operations

**Memory**: No additional allocation, operates on input buffer

#### Streaming Parser

**Function**: `ld2420_stream_feed()`

**Use Case**: Incremental byte-by-byte processing.

**State Machine**:

```text
┌──────────────┐
│ NOT_SYNCED   │ ← Initial state, searching for header
└──────┬───────┘
       │ Header found
       ↓
┌──────────────┐
│   SYNCED     │ ← Accumulating frame bytes
└──────┬───────┘
       │ Complete frame received
       ↓
┌──────────────┐
│FRAME_COMPLETE│ ← Validate and parse
└──────┬───────┘
       │ Process or Error
       ↓
   (Reset to NOT_SYNCED)
```

**State Transitions**:

- **NOT_SYNCED → SYNCED**: Header pattern matched
- **SYNCED → FRAME_COMPLETE**: Received expected bytes
- **SYNCED → NOT_SYNCED**: Error detected, resync
- **FRAME_COMPLETE → NOT_SYNCED**: Frame processed

**Resynchronization**: On error, preserve up to 3 trailing bytes to catch headers split across errors.

**Memory**: Single buffer of `LD2420_MAX_RX_PACKET_SIZE` (154 bytes)

### Endianness Handling

The LD2420 protocol uses little-endian byte order for all multi-byte fields.

**Implementation**:

```c
// Read 16-bit little-endian value
static inline uint16_t read_le16(const uint8_t *buf) {
    return (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
}

// Write 16-bit little-endian value
static inline void write_le16(uint8_t *buf, uint16_t val) {
    buf[0] = val & 0xFF;
    buf[1] = (val >> 8) & 0xFF;
}
```

**Platform Detection**:

- CMake tests system endianness at build time
- Big-endian systems get explicit conversion
- Little-endian systems can optimize (but use same code for clarity)

### Error Handling

**Status Codes**:

```c
typedef enum {
    LD2420_STATUS_OK = 0,
    LD2420_STATUS_ERROR_UNKNOWN,
    LD2420_STATUS_ERROR_INVALID_PACKET,
    LD2420_STATUS_ERROR_INVALID_BUFFER,
    LD2420_STATUS_ERROR_INVALID_BUFFER_SIZE,
    LD2420_STATUS_ERROR_INVALID_FRAME,
    LD2420_STATUS_ERROR_INVALID_FRAME_SIZE,
    LD2420_STATUS_ERROR_BUFFER_TOO_SMALL,
    LD2420_STATUS_ERROR_INVALID_HEADER,
    LD2420_STATUS_ERROR_INVALID_FOOTER,
    LD2420_STATUS_ERROR_INVALID_ARGUMENTS,
    LD2420_STATUS_ERROR_ALREADY_INITIALIZED,
} ld2420_status_t;
```

**Error Propagation**:

- Functions return status codes
- Callers check and handle appropriately
- No global error state
- Thread-safe by design

## Platform Layer Design

### Raspberry Pi Pico Implementation

#### Components

**UART Management**:

- Hardware UART0 or UART1
- Configurable TX/RX pins
- Fixed baud rate: 115200

**Interrupt Handler**:

```c
void uart_irq_handler(void) {
    while (uart_is_readable(uart_inst)) {
        uint8_t byte = uart_getc(uart_inst);
        ring_buffer_push(byte);
    }
}
```

**Ring Buffer**:

- Fixed size: 256 bytes
- Circular buffer implementation
- Lock-free single producer, single consumer
- Overflow detection

**Frame Processing**:

```c
int16_t ld2420_pico_process(uint8_t uart_index) {
    // Drain ring buffer
    // Feed bytes to streaming parser
    // Invoke callback on complete frames
    // Return frame count
}
```

#### Threading Model

**Single-Threaded Design**:

- IRQ handler: Minimal work, just buffer bytes
- Main loop: Call `process()` to drain buffer
- Callbacks: Executed in main loop context

**No Mutexes Required**: Simple producer-consumer pattern with lock-free ring buffer.

**Send Operations**: Optional mutex for multi-threaded applications.

#### Memory Layout

Per UART instance:

```text
┌──────────────────────────┐
│  Ring Buffer (256 bytes) │
├──────────────────────────┤
│  Stream Parser           │
│  - Buffer (154 bytes)    │
│  - State variables       │
├──────────────────────────┤
│  Callback pointer        │
├──────────────────────────┤
│  UART instance pointer   │
└──────────────────────────┘
Total: ~420 bytes per UART
```

### Porting to New Platforms

**Required Implementations**:

1. **Initialization Function**
   - Configure UART hardware
   - Set up interrupts or polling
   - Initialize streaming parser
   - Register callback

2. **Send Function**
   - Transmit bytes over UART
   - Thread-safe if needed
   - Error handling

3. **Receive Handling**
   - IRQ handler or polling loop
   - Buffer incoming bytes
   - Feed to streaming parser

4. **Process Function**
   - Drain receive buffer
   - Feed bytes to parser
   - Invoke callbacks on frames

**Platform Checklist**:

- [ ] UART initialization at 115200 baud
- [ ] RX data collection (IRQ or polling)
- [ ] TX data transmission
- [ ] Integration with streaming parser
- [ ] Callback mechanism for complete frames
- [ ] Resource cleanup/deinitialization
- [ ] Thread safety (if applicable)
- [ ] Error handling

## Protocol Details

### Command Frame Format

**TX (Host → LD2420)**:

```text
Header (4) | Length (2) | Command (2) | Data (N×6) | Footer (4)
```

**RX (LD2420 → Host)**:

```text
Header (4) | Length (2) | Status (2) | CmdEcho (2) | Data (N×4) | Footer (4)
```

### Packet Size Constraints

**Minimum RX Packet**: 14 bytes

```text
4 (header) + 2 (length) + 4 (minimal data) + 4 (footer) = 14
```

**Maximum RX Packet**: 154 bytes

```text
4 + 2 + 2 + 2 + (35 × 4) + 4 = 154
```

**Minimum TX Packet**: 12 bytes

```text
4 (header) + 2 (length) + 2 (command) + 4 (footer) = 12
```

**Maximum TX Packet**: 222 bytes

```text
4 + 2 + 2 + (35 × 6) + 4 = 222
```

### Command Types

**Configuration Commands**:

- `0xFF` - Enter configuration mode
- `0xFE` - Exit configuration mode
- `0x07` - Set configuration parameters
- `0x08` - Read configuration parameters

**System Commands**:

- `0x00` - Read firmware version
- `0x68` - Reboot module

## Performance Considerations

### Memory Usage

**Core Library**:

- Streaming parser: 154 bytes buffer + state variables
- One-shot parser: No additional memory
- Total per stream context: ~160 bytes

**Platform Layer (Pico)**:

- Ring buffer: 256 bytes
- Stream context: 160 bytes
- Metadata: ~20 bytes
- Total per UART: ~436 bytes

### CPU Usage

**Interrupt Handler**: O(1) per byte, minimal work

**Processing Loop**: O(N) where N is bytes in ring buffer

**Frame Parsing**: O(1) per frame (fixed validation steps)

**Callback Overhead**: User-defined, should be kept minimal

### Throughput

At 115200 baud:

- Theoretical max: ~11,520 bytes/second
- Frame overhead: 14 bytes minimum
- Practical max: ~800 frames/second (for minimal frames)

## Testing Strategy

### Unit Tests

**Core Library Tests**:

- Valid frame parsing
- Invalid header/footer detection
- Buffer overflow protection
- Endianness conversion
- Streaming state machine
- Error condition handling

**Test Framework**: Unity (ThrowTheSwitch)

**Coverage**: Aim for >90% code coverage

### Integration Tests

**Platform Tests**:

- UART loopback tests
- Interrupt handling validation
- Ring buffer stress tests
- Multi-frame processing

**Hardware-in-Loop**: Test with actual LD2420 modules

### Continuous Integration

**Automated Builds**:

- Core library on native targets
- Platform libraries with cross-compilers
- Examples compilation verification
- Unit test execution
- Documentation generation

## Future Enhancements

### Planned Features

- Command builder API
- Configuration parameter helpers
- Additional platform support
- Configuration tooling

### Architecture Evolution

Core APIs are stabilizing. Breaking changes will follow semantic versioning.

## References

**LD2420 Documentation**:

- [Sensor Description](https://www.hlktech.net/index.php?id=1291)
- [Protocol Specification](https://drive.google.com/drive/folders/1IggDH6ejNSOs8EklQbAXcqUI7KENSZLt)

**Standards**:

- C99 Standard
- Semantic Versioning 2.0.0
- Markdown CommonMark Spec

**Dependencies**:

- CMake 3.13+
- Unity Test Framework
- Pico SDK (for Pico platform)
