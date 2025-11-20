# LD2420 Examples

This directory contains complete, standalone example projects demonstrating how to use the LD2420 library on different platforms.

## Available Examples

### Raspberry Pi Pico (`pico/`)

A complete working example demonstrating LD2420 integration with Raspberry Pi Pico.

**Features:**

- Independent build system using Pico SDK
- USB serial output for debugging and monitoring
- Command sending and response handling
- Demonstrates initialization, configuration, and data processing
- Generates ready-to-flash `.uf2` files

**What it demonstrates:**

- UART initialization and configuration
- Sending commands to LD2420 module
- Receiving and processing responses
- Frame callback handling
- LED indication for activity

See [`pico/README.md`](pico/README.md) for detailed build and usage instructions.

## Building Philosophy

Each example is a **standalone project** with its own build system. This design:

- Allows independent compilation with platform-specific toolchains
- Makes examples easy to copy into your own projects
- Avoids cross-toolchain complications in the main build
- Demonstrates real-world integration patterns

## Using Examples as Templates

Examples are designed to be copied and modified for your projects:

1. Copy the example directory to your project
2. Modify paths in `CMakeLists.txt` to point to the LD2420 library
3. Customize the application code for your needs
4. Build with the platform's toolchain

## Future Examples

Planned examples for additional platforms:

- **Native/Linux**: Command-line tool for serial communication
- **ESP32**: UART integration with ESP-IDF
- **Arduino**: Platform-agnostic Arduino wrapper

Contributions welcome!
