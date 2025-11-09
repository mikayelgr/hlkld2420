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
    const uint8_t *rx_buffer,
    const uint8_t rx_buffer_size)
{
    // If the provided buffer is NULL or smaller than the minimum RX packet size, there's
    // definitely an error.
    if (rx_buffer == NULL || rx_buffer_size < LD2420_MIN_RX_PACKET_SIZE || rx_buffer_size > LD2420_MAX_RX_PACKET_SIZE)
        return LD2420_ERROR_INVALID_BUFFER_SIZE | LD2420_ERROR_INVALID_BUFFER;

    // Making sure that the buffer actually starts with the expected header for the LD2420 module.
    int is_same = memcmp(rx_buffer, LD2420_BEG_COMMAND_PACKET, sizeof(LD2420_BEG_COMMAND_PACKET)) == 0;
    if (!is_same)
        return LD2420_ERROR_INVALID_HEADER;

    // Verify that the footer matches the expected footer bytes.
    is_same = memcmp(rx_buffer, rx_buffer + rx_buffer_size - sizeof(LD2420_END_COMMAND_PACKET),
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
    const uint8_t *rx_buffer,
    const uint8_t rx_buffer_size,
    uint8_t *out_intra_frame_data,
    uint16_t *out_intra_frame_data_size)
{
    // It doesn't make sense to proceed if the output pointers are NULL.
    if (out_intra_frame_data_size == NULL || out_intra_frame_data == NULL)
        return LD2420_ERROR_INVALID_ARGUMENTS;

    // Start populating the packet structure by verifying the header and extracting
    // the frame size.
    __ld2420_extract_packet_metadata__(
        rx_buffer,
        out_intra_frame_data_size);

    // After we have successfully obtained the frame size and its actually written to
    // its destination, we can start further validating the packet.
    ld2420_status_t status = __ld2420_validate_packet_metadata__(
        rx_buffer,
        rx_buffer_size,
        *out_intra_frame_data_size);
    if (status != LD2420_OK)
        return status;

    // Now that we have the frame size, we can validate the overall packet size as well
    // as extract the actual data from the intra-frame data segment to a user-friendly
    // interface.
    status = __ld2420_rx_validate_packet_metadata__(rx_buffer, rx_buffer_size);
    if (status != LD2420_OK)
        return status;

    *out_intra_frame_data = rx_buffer[sizeof(LD2420_BEG_COMMAND_PACKET) +
                                      sizeof(uint16_t) /* frame size */];
    return LD2420_OK;
}

// ld2420_status_t ld2420_create_tx_command_packet(
//     const ld2420_command_t cmd,
//     const unsigned char *frame_data,
//     const unsigned short frame_size,
//     ld2420_command_packet_t *out_packet)
// {
//     // Validate input parameters
//     if (out_packet == NULL)
//     {
//         return LD2420_ERROR_INVALID_PACKET;
//     }

//     if (frame_size > 0 && frame_data == NULL)
//     {
//         return LD2420_ERROR_INVALID_BUFFER;
//     }

//     // Initialize header and footer
//     __initialize_packet_header_and_footer__(out_packet);

//     // Set command
//     out_packet->cmd = cmd;

//     // Frame size is the size of the command (2 bytes) + any additional frame_data
//     // Per HLK protocol, minimum frame_size is 2 (just the command)
//     out_packet->frame_size = sizeof(out_packet->cmd) + frame_size;

//     return LD2420_OK;
// }

// ld2420_status_t ld2420_serialize_command_packet(
//     const ld2420_command_packet_t *packet,
//     const unsigned char *frame_data,
//     const unsigned short frame_data_size,
//     unsigned char *out_buffer,
//     const size_t buffer_capacity,
//     size_t *out_size)
// {
//     // Validate input parameters
//     if (packet == NULL || out_buffer == NULL || out_size == NULL)
//     {
//         return LD2420_ERROR_INVALID_PACKET;
//     }

//     if (frame_data_size > 0 && frame_data == NULL)
//     {
//         return LD2420_ERROR_INVALID_BUFFER;
//     }

//     // Calculate required buffer size
//     size_t packet_size = sizeof(packet->HEADER) + sizeof(packet->frame_size) +
//                          sizeof(packet->cmd) + frame_data_size + sizeof(packet->FOOTER);

//     if (packet_size > buffer_capacity)
//     {
//         return LD2420_ERROR_BUFFER_TOO_SMALL;
//     }

//     // Serialize packet
//     size_t offset = 0;
//     memcpy(out_buffer + offset, packet->HEADER, sizeof(packet->HEADER));
//     offset += sizeof(packet->HEADER);

//     memcpy(out_buffer + offset, &packet->frame_size, sizeof(packet->frame_size));
//     offset += sizeof(packet->frame_size);

//     memcpy(out_buffer + offset, &packet->cmd, sizeof(packet->cmd));
//     offset += sizeof(packet->cmd);

//     if (frame_data_size > 0)
//     {
//         memcpy(out_buffer + offset, frame_data, frame_data_size);
//         offset += frame_data_size;
//     }

//     memcpy(out_buffer + offset, packet->FOOTER, sizeof(packet->FOOTER));
//     offset += sizeof(packet->FOOTER);

//     *out_size = packet_size;
//     return LD2420_OK;
// }
