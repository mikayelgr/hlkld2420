# LD2420 Examples

This directory contains example projects demonstrating how to use the LD2420 library on different platforms.

## Available Examples

### Pico Example (`pico/`)

A complete example for Raspberry Pi Pico showing how to interface with the LD2420 sensor.

**Key Features:**

- Fully independent build system using Pico toolchain
- Can be built separately from the main library
- Includes USB serial output for debugging
- Ready-to-flash `.uf2` files

See [`pico/README.md`](pico/README.md) for build instructions.

## Building Examples

Each example is designed to be built independently with its own toolchain. Navigate to the specific example directory and follow its README instructions.

**Note:** Examples are **not** part of the main project's CMake build. This is intentional to keep different toolchains isolated.

## Adding More Examples

Future examples for other platforms (ESP32, STM32, native Linux, etc.) can be added as separate subdirectories following the same pattern.
