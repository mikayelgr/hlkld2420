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

### Output Location

The compiled library (`libld2420_pico.a`) will be in the build directory you created.
