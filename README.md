[![CI](https://github.com/mikayelgr/hlkld2420/actions/workflows/ci.yaml/badge.svg)](https://github.com/mikayelgr/hlkld2420/actions/workflows/ci.yaml)

# HLK-LD2420 C Library

A lightweight, platform-agnostic C library for interfacing with the HLK-LD2420 24GHz radar motion detection module. Designed for embedded systems with support for Raspberry Pi Pico and other microcontrollers.

> **Development Status**: This project is under active development. The core API is evolving, and breaking changes may occur. Platform-specific implementations will stabilize as the core APIs mature.

## Features

- Pure C99, zero dependencies
- Streaming parser for byte-by-byte processing
- One-shot parser for complete frames
- Little-endian byte order handling
- No dynamic allocation
- Comprehensive error handling
- Platform-agnostic core design

### Module Specifications

The LD2420 variant supports:

- **Detection Range**: 0.2m to 8m
- **Range Gates**: 15 gates at 0.7m increments
- **Recommended Max Gate**: 10 (for optimal accuracy and reduced noise)
- **Interface**: UART at 115200 baud
- **Operating Frequency**: 24GHz FMCW

![HLK-LD2420 Module](./docs/images/module-outlined-table.png)

| Resource Name | Resource URL |
|---|---|
|Sensor Description|<https://www.hlktech.net/index.php?id=1291>|
|Sensor Documentation, Tooling, and Protocol Description|<https://drive.google.com/drive/folders/1IggDH6ejNSOs8EklQbAXcqUI7KENSZLt>|

## Platform Support

| Platform | Status | Notes |
|----------|--------|-------|
| Native (Host) | Supported | Core parsing library |
| Raspberry Pi Pico | In Progress | UART layer under development |

## Quick Start

### Prerequisites

- CMake 3.13 or later
- C compiler (GCC, Clang, or MSVC)
- Platform-specific toolchains (see platform READMEs)

### Clone

```bash
git clone https://github.com/mikayelgr/hlkld2420.git
cd hlkld2420
```

### Build Documentation

Each component has detailed build instructions:

| Component | Documentation |
|-----------|---------------|
| Core Library | [core/README.md](./core/README.md) |
| Pico Platform | [platform/pico/README.md](./platform/pico/README.md) |
| Examples | [examples/README.md](./examples/README.md) |

## API Overview

### Core Parsers

**Streaming Parser** (for UART/serial):

```c
#include <ld2420/ld2420_stream.h>
ld2420_stream_feed(&stream, &byte, 1, on_frame_callback);
```

**One-Shot Parser** (for complete frames):

```c
#include <ld2420/ld2420.h>
ld2420_parse_rx_buffer(buffer, size, &frame_size, &cmd_echo, &status);
```

### Documentation

API documentation is in the header files:

- Core: [`ld2420.h`](core/include/ld2420/ld2420.h), [`ld2420_stream.h`](core/include/ld2420/ld2420_stream.h)
- Pico: [`ld2420_pico.h`](platform/pico/include/ld2420/platform/pico/ld2420_pico.h)

## Protocol Constraints

Packet size limits defined by the protocol:

- **Min RX**: 14 bytes (simple acknowledgments)
- **Max RX**: 154 bytes (full configuration read)
- **Min TX**: 12 bytes (parameter-less commands)
- **Max TX**: 222 bytes (full configuration write)

See [ARCHITECTURE.md](./ARCHITECTURE.md) for detailed protocol specifications.

## Integration

### CMake Subdirectory

```cmake
add_subdirectory(/path/to/hlkld2420/platform/pico ld2420_pico)
add_executable(my_project main.c)
target_link_libraries(my_project PRIVATE ld2420_pico)
```

### CMake FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(ld2420_pico
    GIT_REPOSITORY https://github.com/mikayelgr/hlkld2420.git
    GIT_TAG main
    SOURCE_SUBDIR platform/pico
)
FetchContent_MakeAvailable(ld2420_pico)
target_link_libraries(my_project PRIVATE ld2420_pico)
```

See [examples/](./examples/) for complete usage demonstrations.

## License

This project is licensed under the MIT License. See the `LICENSE` file for details.

## Contributing

Contributions are welcome! Please open an issue or submit a pull request for any improvements or bug fixes.

## Contact

For questions or support, please contact [mikayelgr](https://github.com/mikayelgr).
