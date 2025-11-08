#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <memory.h>

#include "ld2420/ld2420.h"

/**
 * An internal macro to set the state of the LD2420 device.
 */
#define __LD2420_FN_SET_STATE__(state_ptr, new_state) \
    if (state_ptr != NULL)                            \
    {                                                 \
        *(state_ptr) = new_state;                     \
    }

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

ld2420_command_packet_t *ld2420_parse_rx_command_packet(
    const unsigned char *buffer,
    const size_t buffer_size,
    ld2420_status_t *status_ref)
{
    if ((buffer == NULL) || (buffer_size < LD2420_MIN_RX_PACKET_SIZE))
    {
        // In case the user wants to handle the error codes manually from their side as well,
        // we are setting the error and then returning NULL as the actual command packet
        // which indicates failure.
        __LD2420_FN_SET_STATE__(status_ref, LD2420_ERROR_INVALID_BUFFER | LD2420_ERROR_INVALID_BUFFER_SIZE);
        return NULL;
    }

    ld2420_command_packet_t *packet = (ld2420_command_packet_t *)malloc(sizeof(ld2420_command_packet_t));
    if (packet == NULL)
    {
        __LD2420_FN_SET_STATE__(status_ref, LD2420_ERROR_MEMORY_ALLOCATION_FAILED);
        return NULL;
    }

    size_t offset = 0;
    memcpy(packet->HEADER, buffer + offset, sizeof(packet->HEADER));
    offset += sizeof(packet->HEADER);

    memcpy(&packet->frame_size, buffer + offset, sizeof(packet->frame_size));
    offset += sizeof(packet->frame_size);

    memcpy(&packet->cmd, buffer + offset, sizeof(packet->cmd));
    offset += sizeof(packet->cmd);

    // By default frame data is NULL
    packet->frame_data = NULL;

    // Now that we know frame_size, check if buffer is large enough (optional, if buffer length is available)
    if (packet->frame_size > 0)
    {
        // Attempting to allocate a buffer for the backets internal frame data given the
        // known frame size.
        packet->frame_data = (unsigned char *)malloc(packet->frame_size);
        if (packet->frame_data == NULL)
        {
            __LD2420_FN_SET_STATE__(status_ref, LD2420_ERROR_MEMORY_ALLOCATION_FAILED);
            free(packet);
            return NULL;
        }

        // Copying the frame data to the actual packet for a valid structure and
        // increasing the offset. This needs to be done in the end to ensure
        // correctness.
        memcpy(packet->frame_data, buffer + offset, packet->frame_size);
        offset += packet->frame_size;
    }

    // Copying the default footer and increasing the offset
    memcpy(packet->FOOTER, buffer + offset, sizeof(packet->FOOTER));
    offset += sizeof(packet->FOOTER);

    if (!__validate_rx_packet__(packet))
    {
        __LD2420_FN_SET_STATE__(status_ref, LD2420_ERROR_INVALID_HEADER_OR_FOOTER);
        // Freeing the allocated packet in case if the packet validation failed for
        // either the header or the footer.
        ld2420_free_command_packet(packet);
        return NULL;
    }

    return packet;
}

ld2420_command_packet_t *ld2420_create_tx_command_packet(
    const ld2420_command_t cmd,
    const unsigned char *frame_data,
    const unsigned short frame_size,
    ld2420_status_t *status_ref)
{
    ld2420_command_packet_t *packet = (ld2420_command_packet_t *)malloc(sizeof(ld2420_command_packet_t));
    if (packet == NULL)
    {
        __LD2420_FN_SET_STATE__(status_ref, LD2420_ERROR_MEMORY_ALLOCATION_FAILED);
        return NULL;
    }

    // Setting the default header and footer for the TX command packet.
    __initialize_packet_header_and_footer__(packet);

    packet->frame_data = NULL; // by default frame data is NULL
    packet->cmd = cmd;

    // Frame size is the size of the command (2 bytes) + any additional frame_data
    // Per HLK protocol, minimum frame_size is 2 (just the command)
    packet->frame_size = sizeof(packet->cmd) + frame_size;

    if (frame_size > 0 && frame_data != NULL)
    {
        packet->frame_data = (unsigned char *)malloc(sizeof(unsigned char) * frame_size);
        if (packet->frame_data == NULL)
        {
            __LD2420_FN_SET_STATE__(status_ref, LD2420_ERROR_MEMORY_ALLOCATION_FAILED);
            free(packet);
            return NULL;
        }

        memcpy(packet->frame_data, frame_data, frame_size);
    }

    return packet;
}

unsigned char *ld2420_serialize_command_packet(
    const ld2420_command_packet_t *packet,
    size_t *out_size,
    ld2420_status_t *status_ref)
{
    if (packet == NULL || out_size == NULL)
    {
        __LD2420_FN_SET_STATE__(status_ref, LD2420_ERROR_INVALID_PACKET);
        return NULL;
    }

    // Calculate additional frame data size (frame_size includes command, so subtract it)
    size_t additional_data_size = packet->frame_size - sizeof(packet->cmd);
    size_t packet_size = sizeof(packet->HEADER) + sizeof(packet->frame_size) +
                         sizeof(packet->cmd) + additional_data_size + sizeof(packet->FOOTER);

    unsigned char *buffer = (unsigned char *)malloc(packet_size);
    if (buffer == NULL)
    {
        __LD2420_FN_SET_STATE__(status_ref, LD2420_ERROR_MEMORY_ALLOCATION_FAILED);
        return NULL;
    }

    size_t offset = 0;
    memcpy(buffer + offset, packet->HEADER, sizeof(packet->HEADER));
    offset += sizeof(packet->HEADER);

    memcpy(buffer + offset, &packet->frame_size, sizeof(packet->frame_size));
    offset += sizeof(packet->frame_size);

    memcpy(buffer + offset, &packet->cmd, sizeof(packet->cmd));
    offset += sizeof(packet->cmd);

    if (additional_data_size > 0 && packet->frame_data != NULL)
    {
        memcpy(buffer + offset, packet->frame_data, additional_data_size);
        offset += additional_data_size;
    }

    memcpy(buffer + offset, packet->FOOTER, sizeof(packet->FOOTER));
    *out_size = packet_size;
    return buffer;
}

void ld2420_free_command_packet(ld2420_command_packet_t *packet)
{
    if (packet != NULL)
    {
        if (packet->frame_data != NULL)
        {
            free(packet->frame_data);
        }

        free(packet);
    }
}