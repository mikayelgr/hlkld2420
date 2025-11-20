/*
 * LD2420 one-shot buffer parser
 * -----------------------------
 * Parses a fully assembled packet (header..footer) into key metadata fields.
 * The streaming counterpart assembles frames and then calls this routine.
 */

#include <stddef.h>
#include <stdbool.h>
#include <memory.h>

#include "ld2420/ld2420.h"

#define PACKET_CMD_ECHO_OFFSET (sizeof(LD2420_BEG_COMMAND_PACKET) + 2)
#define PACKET_STATUS_OFFSET (PACKET_CMD_ECHO_OFFSET + 2)

/**
 * When compiling for a big-endian platform, we need to use helper routines
 * to read/write little-endian multi-byte values.
 */
#ifdef LD2420_PLATFORM_BE
/**
 * Read a 16-bit little-endian value from a buffer regardless of host endianness.
 * Example: bytes [0xFF, 0x01] represent 0x01FF in little-endian.
 */
static inline uint16_t read_le16(const uint8_t *b)
{
    return (uint16_t)b[0] | ((uint16_t)b[1] << 8);
}

/** Write a 16-bit value to a buffer in little-endian byte order. */
static inline void write_le16(uint8_t *buffer, uint16_t value)
{
    buffer[0] = (uint8_t)(value & 0xFF);        // LSB
    buffer[1] = (uint8_t)((value >> 8) & 0xFF); // MSB
}
#endif

/**
 * Validate basic packet metadata (header/footer/size constraints).
 * Returns LD2420_OK when the buffer plausibly represents an LD2420 packet.
 */
static inline const ld2420_status_t __ld2420_rx_validate_packet_metadata__(
    const uint8_t *raw_rx_buffer,
    const uint8_t raw_rx_buffer_size)
{
    // If the provided buffer is NULL or smaller than the minimum RX packet size, there's
    // definitely an error.
    if (raw_rx_buffer == NULL || raw_rx_buffer_size < LD2420_MIN_RX_PACKET_SIZE || raw_rx_buffer_size > LD2420_MAX_RX_PACKET_SIZE)
        return LD2420_STATUS_ERROR_INVALID_BUFFER_SIZE | LD2420_STATUS_ERROR_INVALID_BUFFER;

    // Making sure that the buffer actually starts with the expected header for the LD2420 module.
    int is_same = memcmp(raw_rx_buffer, LD2420_BEG_COMMAND_PACKET, sizeof(LD2420_BEG_COMMAND_PACKET)) == 0;
    if (!is_same)
        return LD2420_STATUS_ERROR_INVALID_HEADER;

    // Verify that the footer matches the expected footer bytes.
    is_same = memcmp(raw_rx_buffer, raw_rx_buffer + raw_rx_buffer_size - sizeof(LD2420_END_COMMAND_PACKET),
                     sizeof(LD2420_END_COMMAND_PACKET)) == 0;
    if (!is_same)
        return LD2420_STATUS_ERROR_INVALID_FOOTER;

    return LD2420_STATUS_OK;
}

/**
 * Extract the intra-frame data size from a packet using little-endian order.
 * Validates that the size is strictly positive.
 */
static inline const ld2420_status_t __get_frame_size__(
    const uint8_t *buffer,
    uint16_t *out_intra_frame_data_size)
{
#ifdef LD2420_PLATFORM_BE
    // Extract frame size using little-endian byte order (LD2420 protocol standard).
    // Frame size is located immediately after the 4-byte header.
    *out_intra_frame_data_size = read_le16(buffer + sizeof(LD2420_BEG_COMMAND_PACKET));
#else
    *out_intra_frame_data_size = *(uint16_t *)(buffer + sizeof(LD2420_BEG_COMMAND_PACKET));
#endif

    // An additional check to make sure that the extracted frame size is a valid strictly
    // positive integer.
    if (*out_intra_frame_data_size == 0)
    {
        return LD2420_STATUS_ERROR_INVALID_FRAME_SIZE;
    }

    return LD2420_STATUS_OK;
}

/**
 * Validate that the overall packet size matches the expected value derived from
 * the intra-frame length, and that the header/footer match the protocol markers.
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
    // Verify that the buffer size matches the expected size.
    if (buffer_size != expected_buffer_size)
        return LD2420_STATUS_ERROR_INVALID_BUFFER_SIZE;

    // Making sure that the header matches the expected header bytes.
    if (memcmp(buffer, LD2420_BEG_COMMAND_PACKET, sizeof(LD2420_BEG_COMMAND_PACKET)) != 0)
        return LD2420_STATUS_ERROR_INVALID_HEADER;

    // Verify that the footer matches the expected footer bytes.
    if (memcmp(buffer +
                   sizeof(LD2420_BEG_COMMAND_PACKET) +
                   sizeof(intra_frame_data_size) +
                   intra_frame_data_size,
               LD2420_END_COMMAND_PACKET,
               sizeof(LD2420_END_COMMAND_PACKET)) != 0)
        return LD2420_STATUS_ERROR_INVALID_FOOTER;

    return LD2420_STATUS_OK;
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
        return LD2420_STATUS_ERROR_INVALID_BUFFER | LD2420_STATUS_ERROR_INVALID_ARGUMENTS | LD2420_STATUS_ERROR_INVALID_BUFFER_SIZE;

    // It doesn't make sense to proceed if the output pointers are NULL.
    if (out_frame_size == NULL || out_cmd_echo == NULL || out_status == NULL)
        return LD2420_STATUS_ERROR_INVALID_ARGUMENTS;

    // Start populating the packet structure by verifying the header and extracting
    // the frame size.
    ld2420_status_t status = __get_frame_size__(in_raw_rx_buffer, out_frame_size);
    if (status != LD2420_STATUS_OK)
        return status;

    // After we have successfully obtained the frame size and its actually written to
    // its destination, we can start further validating the packet.
    status = __validate_packet__(in_raw_rx_buffer, in_raw_rx_buffer_size, *out_frame_size);
    if (status != LD2420_STATUS_OK)
        return status;

    // Verify that intra-frame data contains at least cmd_echo (2 bytes) and status (2 bytes).
    if (*out_frame_size < 4)
        return LD2420_STATUS_ERROR_INVALID_FRAME_SIZE;

// Echoed command and the status will be extracted directly from the buffer as separate
// values. This will ensure that the command doesn't contain the status bits.
#ifdef LD2420_PLATFORM_BE
    *out_cmd_echo = (uint8_t)read_le16(in_raw_rx_buffer + PACKET_CMD_ECHO_OFFSET);
    *out_status = (uint8_t)(in_raw_rx_buffer + PACKET_STATUS_OFFSET);
#else
    *out_cmd_echo = *(uint8_t *)(in_raw_rx_buffer + PACKET_CMD_ECHO_OFFSET);
    *out_status = *(uint8_t *)(in_raw_rx_buffer + PACKET_STATUS_OFFSET);
#endif
    return LD2420_STATUS_OK;
}
