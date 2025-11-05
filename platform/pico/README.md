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

1. **Configure the Project**  
   Use CMake to generate the build files for the Pico target:

   ```bash
   cmake -S . -B build/pico -DCMAKE_SYSTEM_NAME=PICO -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/pico.cmake
   ```

2. **Compile the Library**  
   Build the library using the generated build files:

   ```bash
   make -C build/pico
   ```

3. **Locate the Compiled Library**  
   After a successful build, the compiled library (`libld2420_pico.[a|dll|so]`) will be located in the `build/pico/platform/pico` directory.  
   You can link this library to your application as required.
