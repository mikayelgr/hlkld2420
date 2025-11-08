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

### Building the Library

When building the library, you are going to have two platform choices for now:

- **Native (default)** - Sets the target platform as your machine
- **Raspberry Pi Pico** - When you want to call this library from your Pico project

The project uses CMake presets for simplified building. To see all available presets:

```bash
# List all available configure presets
cmake --list-presets

# List all available build presets
cmake --build --list-presets
```

Then configure and build using your chosen preset:

```bash
# Example: Native release build
cmake --preset native-release
cmake --build --preset native-release
```

Detailed build steps for each of the supported platforms is documented in the corresponding files which you can find in the table.

| Target | Build Process Reference |
| ------ | ------------- |
| Native | [Documentation](./core/README.md) |
| Raspberry Pi Pico | [Documentation](./platform/pico/README.md) |

## API Documentation

The API is fully documented in the source code headers:

- Core API: [`include/ld2420/ld2420.h`](include/ld2420/ld2420.h)
- Raspberry Pi Pico implementation: [`platform/pico/include/ld2420/platform/pico/ld2420_pico.h`](platform/pico/include/ld2420/platform/pico/ld2420_pico.h)

### Constraints

To ensure efficient memory usage, the library exposes a few useful constants which are computed from the official protocol documentation. They are documented below.

#### Minimum Receive (RX) Packet Size

- `LD2420_MIN_RX_PACKET_SIZE` - core constant

The minimum size for a packet received (RX) from the sensor is **14 bytes**. This minimum size occurs for simple acknowledgment packets, such as "Response successful", which contain no parameter data. Based on the "Receive Command Component" structure defined in the protocol[cite: 6], the packet is composed of the following:

- **Packet Header**: 4 bytes
- **Data Length**: 2 bytes
- **In-frame Data**: 4 bytes (This is the minimal payload: a 2-byte original command echo + 2-byte return value/status)
- **End of Packet**: 4 bytes

The total size is calculated as:

$$
\underbrace{4 \text{ bytes}}_{\text{Header}} +
\underbrace{2 \text{ bytes}}_{\text{Length}} +
\underbrace{4 \text{ bytes}}_{\text{Min. RX Data}} +
\underbrace{4 \text{ bytes}}_{\text{Footer}} = 14 \text{ bytes}
$$

## Adding the Library to Another Project

To use the HLK-LD2420 library in your own project, you can fetch it directly from the GitHub repository using CMake's `FetchContent` module.

> **Note:** This section describes integration patterns that are still being refined. Please report any issues or suggestions for improvement.

### CMake Example

Here's how to integrate the library into your `CMakeLists.txt` for a Raspberry Pi Pico project:

```cmake
include(FetchContent)

FetchContent_Declare(hlkld2420
    GIT_REPOSITORY https://github.com/mikayelgr/hlkld2420.git
    GIT_TAG main  # or specify a specific version/tag
)

FetchContent_MakeAvailable(hlkld2420)

# For your Pico project, add this to your executable
add_executable(your_project main.c)
target_link_libraries(your_project PRIVATE ld2420_pico pico_stdlib)
target_include_directories(your_project PRIVATE ${hlkld2420_SOURCE_DIR}/include)
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
