#include <stddef.h>
#include <stdbool.h>
#include <memory.h>

#include "ld2420/ld2420.h"

/**
 * Given an RX buffer and its size received from the LD2420 module, this function
 * validates the basic metadata of the packet such as the header and footer bytes
 * as well as size constraints.
 */
static const ld2420_status_t __ld2420_rx_validate_packet_metadata__(
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
 */
static void __ld2420_extract_packet_metadata__(
    const uint8_t *buffer,
    uint16_t *out_intra_frame_data_size)
{
    // We don't need the header anymore at this point and can move forward to extracting
    // and copying the frame size into the corresponding address.
    memcpy(
        out_intra_frame_data_size,
        buffer + sizeof(LD2420_BEG_COMMAND_PACKET),
        sizeof(*out_intra_frame_data_size));
}

/**
 * This function makes sure that the packet metadata such as the overall size of the packet
 * matches the expected size based on the frame size extracted earlier.
 */
static const ld2420_status_t __ld2420_validate_packet_metadata__(
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

    return LD2420_OK;
}

ld2420_status_t ld2420_parse_raw_rx_command_packet(
    const uint8_t *in_raw_rx_buffer,
    const uint8_t in_raw_rx_buffer_size,
    uint16_t *out_frame_size,
    uint16_t *out_cmd_echo,
    uint16_t *out_status)
{
    // It doesn't make sense to proceed if the output pointers are NULL.
    if (out_frame_size == NULL || out_cmd_echo == NULL || out_status == NULL)
        return LD2420_ERROR_INVALID_ARGUMENTS;

    // Start populating the packet structure by verifying the header and extracting
    // the frame size.
    __ld2420_extract_packet_metadata__(
        in_raw_rx_buffer,
        out_frame_size);

    // After we have successfully obtained the frame size and its actually written to
    // its destination, we can start further validating the packet.
    ld2420_status_t status = __ld2420_validate_packet_metadata__(
        in_raw_rx_buffer,
        in_raw_rx_buffer_size,
        *out_frame_size);
    if (status != LD2420_OK)
        return status;

    // Now that we have the frame size, we can validate the overall packet size as well
    // as extract the actual data from the intra-frame data segment to a user-friendly
    // interface.
    status = __ld2420_rx_validate_packet_metadata__(in_raw_rx_buffer, in_raw_rx_buffer_size);
    if (status != LD2420_OK)
        return status;

    // TODO: Finalize the implementation of the parser by extracting the returned parameters with their
    //       respective values. For now, the command only populates the frame size, the echoed cmd, as
    //       as well as the status code which is 2 bytes in size.
    return LD2420_OK;
}
