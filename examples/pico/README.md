# LD2420 Pico Example

A complete, working example demonstrating LD2420 radar module integration with Raspberry Pi Pico. This example shows how to initialize the sensor, send commands, and process responses.

## What This Example Does

1. Initializes USB serial output for debugging
2. Configures UART communication with LD2420 module
3. Sends "Open Config Mode" command to the sensor
4. Receives and displays response packets
5. Toggles onboard LED on each successful response
6. Runs in a continuous loop

## Hardware Requirements

- Raspberry Pi Pico or Pico W
- HLK-LD2420 radar module
- USB cable for power and serial output
- Jumper wires for UART connection

## Wiring

Connect the LD2420 to your Pico:

| LD2420 Pin | Pico Pin | GPIO |
|------------|----------|------|
| VCC | 3.3V (Pin 36) | - |
| GND | GND (Pin 38) | - |
| TX | Pin 2 | GP1 |
| RX | Pin 1 | GP0 |

## Prerequisites

- Raspberry Pi Pico SDK installed
- `PICO_SDK_PATH` environment variable set
- ARM cross-compilation toolchain (`arm-none-eabi-gcc`)

### Setup Pico SDK

If not already installed:

```bash
# Clone the SDK
git clone https://github.com/raspberrypi/pico-sdk.git
cd pico-sdk
git submodule update --init

# Set environment variable (add to ~/.zshrc or ~/.bashrc)
export PICO_SDK_PATH=/path/to/pico-sdk
```

## Building

This example builds independently with its own toolchain, automatically fetching the LD2420 libraries.

```bash
# From the examples/pico directory
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Or specify `PICO_SDK_PATH` inline:

```bash
PICO_SDK_PATH=/path/to/pico-sdk cmake -B build
cmake --build build
```

### Build Outputs

The build generates several files in `build/`:

- `main.elf` - ELF executable
- `main.uf2` - UF2 format (for drag-and-drop flashing)
- `main.bin` - Binary format
- `main.hex` - Intel HEX format
- `main.dis` - Disassembly listing
- `main.map` - Linker map file

## Flashing to Pico

### Method 1: Drag-and-Drop (Easiest)

1. Hold down the **BOOTSEL** button on your Pico
2. While holding, connect Pico to computer via USB
3. Release BOOTSEL - Pico appears as a USB drive
4. Drag `build/main.uf2` onto the Pico drive
5. Pico automatically reboots and runs the program

### Method 2: Using picotool

```bash
# Install picotool if not already installed
brew install picotool  # macOS
# or build from source

# Flash the binary
picotool load build/main.uf2
picotool reboot
```

## Viewing Output

The example outputs diagnostic information via USB serial.

### Using screen

```bash
# macOS
screen /dev/tty.usbmodem* 115200

# Linux
screen /dev/ttyACM0 115200

# Exit: Press Ctrl-A, then K, then Y
```

### Using minicom

```bash
# Install minicom
brew install minicom  # macOS
sudo apt-get install minicom  # Linux

# Connect
minicom -D /dev/tty.usbmodem* -b 115200
```

### Using tio

```bash
# Install tio (modern alternative)
brew install tio
tio /dev/tty.usbmodem*
```

### Expected Output

```text
=== LD2420 Pico Example ===

Initializing LD2420...
LD2420 initialized successfully.

--- Command 1 ---
Sending OPEN CONFIG MODE command...
Packet (14 bytes): FD FC FB FA 06 00 FF 00 01 00 00 00 04 03 02 01

--- Command 2 ---
...
```

## Customization

### Modifying Commands

Edit `main.c` to send different commands. Common commands:

```c
// Read firmware version
static const uint8_t READ_VERSION[] = {
    0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 
    0x00, 0x00, 0x04, 0x03, 0x02, 0x01
};

// Close config mode
static const uint8_t CLOSE_CONFIG_MODE[] = {
    0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 
    0xFE, 0x00, 0x04, 0x03, 0x02, 0x01
};

// Read module configuration
static const uint8_t READ_CONFIG[] = {
    0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 
    0x08, 0x00, 0x04, 0x03, 0x02, 0x01
};
```

### Changing UART Pins

Modify the pin definitions in `main.c`:

```c
#define UART_TX_PIN 0  // Change to your TX pin
#define UART_RX_PIN 1  // Change to your RX pin
```

Ensure the pins support UART functionality (see Pico pinout).

### Processing Responses

Enhance the `rx_callback` function to parse response data:

```c
void rx_callback(uint8_t uart_index, const uint8_t *data, uint16_t len) {
    uint16_t frame_size, cmd_echo, status;
    
    ld2420_status_t result = ld2420_parse_rx_buffer(
        data, len, &frame_size, &cmd_echo, &status);
    
    if (result == LD2420_STATUS_OK) {
        printf("Command: 0x%04X, Status: 0x%04X\n", cmd_echo, status);
        // Process based on command echo and status
    }
}
```

## Troubleshooting

### No USB Serial Output

- Wait 2-3 seconds after connecting USB for serial to initialize
- Try different USB cable or port
- Check that `stdio_init_all()` is called before output

### No Response from LD2420

- Verify wiring (TX/RX crossed correctly)
- Check power supply (3.3V, sufficient current)
- Ensure module is powered on
- Try different baud rate if module was reconfigured

### Build Errors

**PICO_SDK_PATH not set:**

```bash
export PICO_SDK_PATH=/path/to/pico-sdk
```

**Toolchain not found:**

```bash
# macOS
brew install arm-none-eabi-gcc

# Linux
sudo apt-get install gcc-arm-none-eabi
```

### LED Not Blinking

- Check that LED is initialized properly
- Verify `PICO_DEFAULT_LED_PIN` is correct for your board
- Pico W uses different LED (connected to WiFi chip)

## Next Steps

- Implement command sequence (open config → read params → close config)
- Add distance gate configuration
- Parse and display detection data
- Implement state machine for sensor operations
- Add persistent configuration storage
