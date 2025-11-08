#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <memory.h>

#include "ld2420/ld2420.h"



/**
 * @brief Initializes the HEADER and FOOTER fields of the given packet with
 *        the default values. The function assumes that the packet is never NULL.
 *
 * @param packet Pointer to the ld2420_command_packet_t to initialize.
 */
static void __initialize_packet_header_and_footer__(ld2420_command_packet_t *packet)
{
    memcpy(packet->HEADER, LD2420_BEG_COMMAND_PACKET, sizeof(packet->HEADER));
    memcpy(packet->FOOTER, LD2420_END_COMMAND_PACKET, sizeof(packet->FOOTER));
}

/**
 * @brief Sets the status code if the status pointer is not NULL.
 *
 * @param status_ref Pointer to status variable (can be NULL)
 * @param status The status code to set
 */
static void __set_status__(ld2420_status_t *status_ref, ld2420_status_t status)
{
    if (status_ref != NULL)
    {
        *status_ref = status;
    }
}

/**
 * @brief Validates the HEADER and FOOTER of the received packet.
 *
 * @param packet Pointer to the ld2420_command_packet_t to validate.
 * @return true if the packet is valid, false otherwise.
 */
static bool __validate_rx_packet__(const ld2420_command_packet_t *packet)
{
    int header_cmp = memcmp(packet->HEADER, LD2420_BEG_COMMAND_PACKET, sizeof(packet->HEADER));
    int footer_cmp = memcmp(packet->FOOTER, LD2420_END_COMMAND_PACKET, sizeof(packet->FOOTER));
    return (header_cmp == 0 && footer_cmp == 0);
}

ld2420_status_t ld2420_parse_rx_command_packet(
    const unsigned char *buffer,
    const size_t buffer_size,
    ld2420_command_packet_t *out_packet,
    unsigned char *out_frame_data,
    const size_t frame_data_capacity,
    size_t *out_frame_data_size)
{
    // Validate input parameters
    if (buffer == NULL || out_packet == NULL || out_frame_data_size == NULL)
    {
        return LD2420_ERROR_INVALID_BUFFER;
    }

    if (buffer_size < LD2420_MIN_RX_PACKET_SIZE)
    {
        return LD2420_ERROR_INVALID_BUFFER_SIZE;
    }

    // Parse packet fields
    size_t offset = 0;
    memcpy(out_packet->HEADER, buffer + offset, sizeof(out_packet->HEADER));
    offset += sizeof(out_packet->HEADER);

    memcpy(&out_packet->frame_size, buffer + offset, sizeof(out_packet->frame_size));
    offset += sizeof(out_packet->frame_size);

    memcpy(&out_packet->cmd, buffer + offset, sizeof(out_packet->cmd));
    offset += sizeof(out_packet->cmd);

    // Calculate additional frame data size (frame_size includes cmd, so subtract it)
    unsigned short additional_data_size = 0;
    if (out_packet->frame_size > sizeof(out_packet->cmd))
    {
        additional_data_size = out_packet->frame_size - sizeof(out_packet->cmd);
    }

    // Copy frame data if present
    *out_frame_data_size = additional_data_size;
    if (additional_data_size > 0)
    {
        if (out_frame_data == NULL)
        {
            return LD2420_ERROR_INVALID_BUFFER;
        }

        if (additional_data_size > frame_data_capacity)
        {
            return LD2420_ERROR_BUFFER_TOO_SMALL;
        }

        memcpy(out_frame_data, buffer + offset, additional_data_size);
        offset += additional_data_size;
    }

    // Copy footer
    memcpy(out_packet->FOOTER, buffer + offset, sizeof(out_packet->FOOTER));
    offset += sizeof(out_packet->FOOTER);

    // Validate header and footer
    if (!__validate_rx_packet__(out_packet))
    {
        return LD2420_ERROR_INVALID_HEADER_OR_FOOTER;
    }

    return LD2420_OK;
}

ld2420_status_t ld2420_create_tx_command_packet(
    const ld2420_command_t cmd,
    const unsigned char *frame_data,
    const unsigned short frame_size,
    ld2420_command_packet_t *out_packet)
{
    // Validate input parameters
    if (out_packet == NULL)
    {
        return LD2420_ERROR_INVALID_PACKET;
    }

    if (frame_size > 0 && frame_data == NULL)
    {
        return LD2420_ERROR_INVALID_BUFFER;
    }

    // Initialize header and footer
    __initialize_packet_header_and_footer__(out_packet);

    // Set command
    out_packet->cmd = cmd;

    // Frame size is the size of the command (2 bytes) + any additional frame_data
    // Per HLK protocol, minimum frame_size is 2 (just the command)
    out_packet->frame_size = sizeof(out_packet->cmd) + frame_size;

    return LD2420_OK;
}

ld2420_status_t ld2420_serialize_command_packet(
    const ld2420_command_packet_t *packet,
    const unsigned char *frame_data,
    const unsigned short frame_data_size,
    unsigned char *out_buffer,
    const size_t buffer_capacity,
    size_t *out_size)
{
    // Validate input parameters
    if (packet == NULL || out_buffer == NULL || out_size == NULL)
    {
        return LD2420_ERROR_INVALID_PACKET;
    }

    if (frame_data_size > 0 && frame_data == NULL)
    {
        return LD2420_ERROR_INVALID_BUFFER;
    }

    // Calculate required buffer size
    size_t packet_size = sizeof(packet->HEADER) + sizeof(packet->frame_size) +
                         sizeof(packet->cmd) + frame_data_size + sizeof(packet->FOOTER);

    if (packet_size > buffer_capacity)
    {
        return LD2420_ERROR_BUFFER_TOO_SMALL;
    }

    // Serialize packet
    size_t offset = 0;
    memcpy(out_buffer + offset, packet->HEADER, sizeof(packet->HEADER));
    offset += sizeof(packet->HEADER);

    memcpy(out_buffer + offset, &packet->frame_size, sizeof(packet->frame_size));
    offset += sizeof(packet->frame_size);

    memcpy(out_buffer + offset, &packet->cmd, sizeof(packet->cmd));
    offset += sizeof(packet->cmd);

    if (frame_data_size > 0)
    {
        memcpy(out_buffer + offset, frame_data, frame_data_size);
        offset += frame_data_size;
    }

    memcpy(out_buffer + offset, packet->FOOTER, sizeof(packet->FOOTER));
    offset += sizeof(packet->FOOTER);

    *out_size = packet_size;
    return LD2420_OK;
}

