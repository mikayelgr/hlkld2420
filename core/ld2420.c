#include <stddef.h>
#include <stdbool.h>
#include <memory.h>

#include "ld2420/ld2420.h"

/**
 * Helper function to read a 16-bit little-endian value from a buffer.
 * This function explicitly reads bytes in little-endian order regardless of host endianness.
 *
 * The LD2420 protocol uses little-endian byte order for all multi-byte values:
 * - Byte 0: LSB (least significant byte)
 * - Byte 1: MSB (most significant byte)
 *
 * Example: bytes [0xFF, 0x01] represent 0x01FF in little-endian.
 */
static inline uint16_t read_le16(const uint8_t *buffer)
{
    return ((uint16_t)buffer[0]) |     // LSB
           ((uint16_t)buffer[1] << 8); // MSB
}

/**
 * Helper function to write a 16-bit value to a buffer in little-endian byte order.
 * This ensures correct byte order when sending commands to the LD2420 module.
 */
static inline void write_le16(uint8_t *buffer, uint16_t value)
{
    buffer[0] = (uint8_t)(value & 0xFF);        // LSB
    buffer[1] = (uint8_t)((value >> 8) & 0xFF); // MSB
}

/**
 * Given an RX buffer and its size received from the LD2420 module, this function
 * validates the basic metadata of the packet such as the header and footer bytes
 * as well as size constraints.
 */
static inline const ld2420_status_t __ld2420_rx_validate_packet_metadata__(
    const uint8_t *raw_rx_buffer,
    const uint8_t raw_rx_buffer_size)
{
    // If the provided buffer is NULL or smaller than the minimum RX packet size, there's
    // definitely an error.
    if (raw_rx_buffer == NULL || raw_rx_buffer_size < LD2420_MIN_RX_PACKET_SIZE || raw_rx_buffer_size > LD2420_MAX_RX_PACKET_SIZE)
        return LD2420_ERROR_INVALID_BUFFER_SIZE | LD2420_ERROR_INVALID_BUFFER;

    // Making sure that the buffer actually starts with the expected header for the LD2420 module.
    int is_same = memcmp(raw_rx_buffer, LD2420_BEG_COMMAND_PACKET, sizeof(LD2420_BEG_COMMAND_PACKET)) == 0;
    if (!is_same)
        return LD2420_ERROR_INVALID_HEADER;

    // Verify that the footer matches the expected footer bytes.
    is_same = memcmp(raw_rx_buffer, raw_rx_buffer + raw_rx_buffer_size - sizeof(LD2420_END_COMMAND_PACKET),
                     sizeof(LD2420_END_COMMAND_PACKET)) == 0;
    if (!is_same)
        return LD2420_ERROR_INVALID_FOOTER;

    return LD2420_OK;
}

/**
 * This function is responsible for taking the raw buffer data received from the LD2420 module
 * as bytes and starting the initial phase of the population of the library-native command packet.
 * The function verifies the integrity of the packet by parsing the header and the received data
 * size fields.
 *
 * @note The assumptions made inside this function will need to be refined as the implementation
 *       matures further.
 */
static inline const ld2420_status_t __get_frame_size__(
    const uint8_t *buffer,
    uint16_t *out_intra_frame_data_size)
{
    // Extract frame size using little-endian byte order (LD2420 protocol standard).
    // Frame size is located immediately after the 4-byte header.
    *out_intra_frame_data_size = read_le16(buffer + sizeof(LD2420_BEG_COMMAND_PACKET));

    // An additional check to make sure that the extracted frame size is a valid strictly
    // positive integer.
    if (*out_intra_frame_data_size == 0)
    {
        return LD2420_ERROR_INVALID_FRAME_SIZE;
    }

    return LD2420_OK;
}

/**
 * This function makes sure that the packet metadata such as the overall size of the packet
 * matches the expected size based on the frame size extracted earlier.
 */
static inline const ld2420_status_t __validate_packet__(
    const uint8_t *buffer,
    const uint8_t buffer_size,
    const uint16_t intra_frame_data_size)
{
    uint8_t expected_buffer_size = sizeof(LD2420_BEG_COMMAND_PACKET) +
                                   sizeof(intra_frame_data_size) + // 2 bytes
                                   intra_frame_data_size +
                                   sizeof(LD2420_END_COMMAND_PACKET);
    // Verify that the footer matches the expected footer bytes.
    if (buffer_size != expected_buffer_size)
        return LD2420_ERROR_INVALID_BUFFER_SIZE;

    // Making sure that the header matches the expected header bytes.
    if (memcmp(buffer, LD2420_BEG_COMMAND_PACKET, sizeof(LD2420_BEG_COMMAND_PACKET)) != 0)
        return LD2420_ERROR_INVALID_HEADER;

    // Verify that the footer matches the expected footer bytes.
    if (memcmp(buffer +
                   sizeof(LD2420_BEG_COMMAND_PACKET) +
                   sizeof(intra_frame_data_size) +
                   intra_frame_data_size,
               LD2420_END_COMMAND_PACKET,
               sizeof(LD2420_END_COMMAND_PACKET)) != 0)
        return LD2420_ERROR_INVALID_FOOTER;

    return LD2420_OK;
}

ld2420_status_t ld2420_parse_rx_buffer(
    const uint8_t *in_raw_rx_buffer,
    const uint8_t in_raw_rx_buffer_size,

    uint16_t *out_frame_size,
    uint16_t *out_cmd_echo,
    uint16_t *out_status)
{
    // We need to make sure that the input we are dealing with is correct.
    if (in_raw_rx_buffer == NULL || in_raw_rx_buffer_size < 0)
        return LD2420_ERROR_INVALID_BUFFER | LD2420_ERROR_INVALID_ARGUMENTS | LD2420_ERROR_INVALID_BUFFER_SIZE;

    // It doesn't make sense to proceed if the output pointers are NULL.
    if (out_frame_size == NULL || out_cmd_echo == NULL || out_status == NULL)
        return LD2420_ERROR_INVALID_ARGUMENTS;

    // Start populating the packet structure by verifying the header and extracting
    // the frame size.
    ld2420_status_t status = __get_frame_size__(in_raw_rx_buffer, out_frame_size);
    if (status != LD2420_OK)
        return status;

    // After we have successfully obtained the frame size and its actually written to
    // its destination, we can start further validating the packet.
    status = __validate_packet__(in_raw_rx_buffer, in_raw_rx_buffer_size, *out_frame_size);
    if (status != LD2420_OK)
        return status;

    // Verify that intra-frame data contains at least cmd_echo (2 bytes) and status (2 bytes).
    if (*out_frame_size < 4)
        return LD2420_ERROR_INVALID_FRAME_SIZE;

    // Packet layout (all multi-byte values in little-endian):
    // [0..3]   Header (4 bytes)
    // [4..5]   Frame Size (2 bytes, little-endian)
    // [6..7]   Command Echo (2 bytes, little-endian)
    // [8..9]   Status (2 bytes, little-endian)
    // [10..]   Payload data (variable length)
    // [end-3..end] Footer (4 bytes)

#define CMD_ECHO_OFFSET (sizeof(LD2420_BEG_COMMAND_PACKET) + 2)
#define STATUS_OFFSET (CMD_ECHO_OFFSET + 2)

    // Extract command echo using little-endian byte order. Making sure to discard
    // any higher-order bits by casting to uint8_t. This makes sure that the last
    // 1 appended to the command is ignored.
    *out_cmd_echo = (uint8_t)read_le16(in_raw_rx_buffer + CMD_ECHO_OFFSET);

    // Extract status using little-endian byte order.
    *out_status = read_le16(in_raw_rx_buffer + STATUS_OFFSET);
    return LD2420_OK;
}
