# LD2420 Pico Example

This example demonstrates how to use the LD2420 library with a Raspberry Pi Pico.

## Prerequisites

- Raspberry Pi Pico SDK installed
- `PICO_SDK_PATH` environment variable set
- ARM cross-compilation toolchain (arm-none-eabi-gcc)

## Building

This example is built **independently** from the main project using the Pico toolchain. It automatically builds the required `ld2420_core` and `ld2420_pico` libraries from source.

### From the examples/pico directory

```bash
# Configure the build
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build the example
cmake --build build

# The output files will be in build/:
# - basic_example.elf
# - basic_example.uf2 (ready to flash to Pico)
# - basic_example.bin
# - basic_example.hex
```

### Or set PICO_SDK_PATH inline

```bash
PICO_SDK_PATH=/path/to/pico-sdk cmake -B build
cmake --build build
```

## Flashing to Pico

1. Hold down the BOOTSEL button on your Pico
2. Connect it to your computer via USB
3. Copy `build/basic_example.uf2` to the Pico's drive
4. The Pico will automatically reboot and run your program

## Viewing Output

The example is configured to output via USB. Connect to the Pico's serial port:

```bash
# macOS
screen /dev/tty.usbmodem* 115200

# Linux
screen /dev/ttyACM0 115200
```

Press `Ctrl-A` then `K` to exit screen.

## Customization

Edit `basic_example.c` to implement your specific LD2420 sensor logic.
