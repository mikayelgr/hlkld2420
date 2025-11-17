[![CI](https://github.com/mikayelgr/hlkld2420/actions/workflows/ci.yaml/badge.svg)](https://github.com/mikayelgr/hlkld2420/actions/workflows/ci.yaml)

# HLK-LD2420 C Library

> Important: the project is currently unstable with a lot of changes being made to the core API. Since platform specific-APIs depend on the core, you can expect everything to stabilize as the core APIs improve.

This repository provides a library for interfacing with the HLK-LD2420 radar module. The library is designed to be lightweight and easy to integrate into projects using the Raspberry Pi Pico or other platforms, not necessarily based on the RP2040 SoC.

The specific module that this library covers is outlined in the picture (LD2420). It supports 15 range gates, in 0.7 meter increments. The module covers distances from 0.2m ~ 8m, for that reason, the recommended maximum range gate is `10`. This also ensures that the gathered data contains less noise and is more accurate overall.

> Note: The official configuration/calibration tooling for these sensors is for Windows-based systems only. Once the basis of this project is finished, I may start working on a web-based platform for configuring this module with any device.

![image of the outlined module in the modules table](./docs/images/module-outlined-table.png)

| Resource Name | Resource URL |
|---|---|
|Sensor Description|<https://www.hlktech.net/index.php?id=1291>|
|Sensor Documentation, Tooling, and Protocol Description|<https://drive.google.com/drive/folders/1IggDH6ejNSOs8EklQbAXcqUI7KENSZLt>|

## Platform Support Status

| Target | Status | API Stability Status |
| ------ | --------- | --- |
| Native (defaults to host's machine) | Supported | Unstable |
|Raspberry Pi Pico | In Progress | Unstable |

> The native target is useful for developing tooling around the HLK-LD2420 sensor which would run locally on your device, such as a command-line tool for interacting with /dev/tty interfaces exposed by this device.

## Features

- Easy-to-use C API for interfacing with the HLK-LD2420 radar module
- Platform-agnostic core library with platform-specific implementations
- Raspberry Pi Pico support with UART communication
- Command packet creation and parsing utilities
- Configuration parameter management (distance, delay, trigger sensitivity, etc.)
- Both native (host machine) and embedded (Pico) compilation targets

## Prerequisites

To use this library, ensure you have the following installed:

- cmake (version 3.13 or later)
- make (version 3.81 or later)
- A supported C/C++ compiler (e.g., GCC) by default for native targets

> **Note:** Platform-specific prerequisites (such as cross-compilers for embedded targets) are documented in their respective platform README files.

### Cloning the Repository

Clone this repository and initialize any required submodules:

```bash
git clone https://github.com/mikayelgr/hlkld2420.git
cd hlkld2420
```

### Project Structure

This project is now organized as independent CMake projects:

- **core/** - Platform-agnostic core library (standalone CMake project)
- **platform/pico/** - Raspberry Pi Pico implementation (standalone CMake project)
- **examples/pico/** - Example projects demonstrating usage

Each component can be built independently or integrated into your project.

### Building the Library

Each library component is an independent CMake project:

- **Core Library** - Build from the `core/` directory
- **Raspberry Pi Pico Platform** - Build from the `platform/pico/` directory

Detailed build steps for each component are documented in the corresponding README files:

| Component | Build Documentation |
| ------ | ------------- |
| Core Library | [Documentation](./core/README.md) |
| Raspberry Pi Pico Platform | [Documentation](./platform/pico/README.md) |
| Examples | [Documentation](./examples/README.md) |

## API Documentation

The API is fully documented in the source code headers:

- Core API: [`core/include/ld2420/ld2420.h`](core/include/ld2420/ld2420.h)
- Raspberry Pi Pico implementation: [`platform/pico/include/ld2420/platform/pico/ld2420_pico.h`](platform/pico/include/ld2420/platform/pico/ld2420_pico.h)

### Constraints

To ensure efficient memory usage, the library exposes a few useful constants which are computed from the official protocol documentation. Both, upper and lower bounds are documented below for TX and RX packets, to and from the sensor.

#### Minimum Receive (RX) Packet Size

- `LD2420_MIN_RX_PACKET_SIZE` - core constant

The minimum size for a packet received (RX) from the sensor is **14 bytes**. This minimum size occurs for simple acknowledgment packets, such as "Response successful", which contain no parameter data. Based on the "Receive Command Component" structure defined in the protocol, the packet is composed of the following:

- **Packet Header**: 4 bytes
- **Data Length**: 2 bytes
- **In-frame Data**: 4 bytes (This is the minimal payload: a 2-byte original command echo + 2-byte return value/status)
- **End of Packet**: 4 bytes

The total size is calculated as:

$$$
\underbrace{4 \text{ bytes}}_{\text{Header}} +
\underbrace{2 \text{ bytes}}_{\text{Length}} +
\underbrace{4 \text{ bytes}}_{\text{Min. RX Data}} +
\underbrace{4 \text{ bytes}}_{\text{Footer}} = 14 \text{ bytes}
$$$

___

#### Maximum Receive (RX) Packet Size

- `LD2420_MAX_RX_PACKET_SIZE` - core constant

The maximum size for a packet received (RX) from the sensor is **154 bytes**. This occurs when receiving a response from the **"Read module configuration" (0x08)** command that queries all 35 available parameters.

Based on the "Receive Command Component" structure, the packet is composed of:

- **Packet Header**: 4 bytes
- **Data Length**: 2 bytes
- **Return Value (Status)**: 2 bytes
- **Return Command Value**: 2 bytes
- **Return Data**: $N \times (4 \text{ value})$ bytes
- **End of Packet**: 4 bytes

The command data payload is $35 \times 4 = 140$ bytes.

The total size is calculated as:

$$$
\underbrace{4 \text{ bytes}}_{\text{Header}} + \underbrace{2 \text{ bytes}}_{\text{Length}} + \underbrace{2 \text{ bytes}}_{\text{Status}} + \underbrace{2 \text{ bytes}}_{\text{Command}} + \underbrace{140 \text{ bytes}}_{\text{Max Data}} + \underbrace{4 \text{ bytes}}_{\text{End}} = 154 \text{ bytes}
$$$

___

#### Minimum Transmit (TX) Packet Size

- `LD2420_MIN_TX_PACKET_SIZE` - core constant

The minimum size for a packet transmitted (TX) to the sensor is **12 bytes**. This occurs when sending a simple command that does not require any parameters ($N=0$), such as "Reboot module", "Read version number", or "Module close command mode".

Based on the "Send Command Components" structure, the packet is composed of:

- **Packet Header**: 4 bytes
- **Data Length**: 2 bytes (This field will contain the value $0x0002$ to indicate the 2-byte payload)
- **Intra-frame Data**: 2 bytes (This is the minimal 2-byte command value, e.g., `0x68 0x00`)
- **End of Packet**: 4 bytes

The total size is calculated as:

$$$
\underbrace{4 \text{ bytes}}_{\text{Header}} + \underbrace{2 \text{ bytes}}_{\text{Length}} + \underbrace{2 \text{ bytes}}_{\text{Min. Data}} + \underbrace{4 \text{ bytes}}_{\text{End}} = 12 \text{ bytes}
$$$

___

#### Maximum Transmit (TX) Packet Size

- `LD2420_MAX_TX_PACKET_SIZE` - core constant

The maximum size for a packet transmitted (TX) to the sensor is **222 bytes**.

This occurs when using the **"Set-up module configuration parameters" (0x07)** command to set all possible parameters at once.

Based on the "Send Command Components" structure, the packet is composed of:

- **Packet Header**: 4 bytes
- **Data Length**: 2 bytes
- **Command Value**: 2 bytes
- **Command Data**: $N \times (2 \text{ name} + 4 \text{ value})$ bytes
- **End of Packet**: 4 bytes

The total number of parameters ($N$) from Table 2 is **35** [$1+1+1+16+16$]. The command data payload is $35 \times 6 = 210$ bytes.

The total size is calculated as:

$$$
\underbrace{4 \text{ bytes}}_{\text{Header}} + \underbrace{2 \text{ bytes}}_{\text{Length}} + \underbrace{2 \text{ bytes}}_{\text{Command}} + \underbrace{210 \text{ bytes}}_{\text{Max Data}} + \underbrace{4 \text{ bytes}}_{\text{End}} = 222 \text{ bytes}
$$$

## Adding the Library to Another Project

Since each component is an independent CMake project, the recommended approach is to build from source without installing anything.

### Option 1: Add as Subdirectories

For a Raspberry Pi Pico project, reference the libraries as subdirectories:

```cmake
# Add the core library
add_subdirectory(/path/to/hlkld2420/core ld2420_core)

# Add the Pico platform library
add_subdirectory(/path/to/hlkld2420/platform/pico ld2420_pico)

# Link to your executable
add_executable(your_project main.c)
target_link_libraries(your_project PRIVATE ld2420_pico)
```

### Option 2: Use FetchContent (for Git-based projects)

```cmake
include(FetchContent)

# Fetch core library
FetchContent_Declare(ld2420_core
    GIT_REPOSITORY https://github.com/mikayelgr/hlkld2420.git
    GIT_TAG main
    SOURCE_SUBDIR core
)

# Fetch Pico platform library
FetchContent_Declare(ld2420_pico
    GIT_REPOSITORY https://github.com/mikayelgr/hlkld2420.git
    GIT_TAG main
    SOURCE_SUBDIR platform/pico
)

FetchContent_MakeAvailable(ld2420_core ld2420_pico)

# Link to your executable
add_executable(your_project main.c)
target_link_libraries(your_project PRIVATE ld2420_pico)
```

#### Basic Usage in C with Raspberry Pi Pico

Once added, you can go ahead and include the LD2420 library as well as stdlib for Raspberry Pi Pico.

```c
#include "ld2420/platform/pico/ld2420_pico.h"
#include "pico/stdlib.h"
```

## License

This project is licensed under the MIT License. See the `LICENSE` file for details.

## Contributing

Contributions are welcome! Please open an issue or submit a pull request for any improvements or bug fixes.

## Contact

For questions or support, please contact [mikayelgr](https://github.com/mikayelgr).
