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
 * @brief Validates the HEADER and FOOTER of the received packet.
 *
 * @param packet Pointer to the ld2420_command_packet_t to validate.
 * @return true if the packet is valid, false otherwise.
 */
static bool __validate_rx_packet__(ld2420_command_packet_t *packet)
{
    int header_cmp = memcmp(packet->HEADER, LD2420_BEG_COMMAND_PACKET, sizeof(packet->HEADER));
    int footer_cmp = memcmp(packet->FOOTER, LD2420_END_COMMAND_PACKET, sizeof(packet->FOOTER));
    return (header_cmp == 0 && footer_cmp == 0);
}

/**
 * Parses a received command packet from the given buffer. The function assumes that the pointer to
 * the provided buffer is a full packet, which has been obtained by reading the header and frame_size
 * data from a UART or other communication interface first.
 *
 * @param buffer Pointer to the buffer containing the received packet data.
 * @return Pointer to the parsed ld2420_command_packet_t structure, or NULL on failure.
 *
 * Note: This function now checks for NULL buffer and minimum buffer size.
 */
ld2420_command_packet_t *ld2420_parse_rx_command_packet(unsigned char *buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size < LD2420_MIN_RX_PACKET_SIZE)
    {
        return NULL;
    }

    ld2420_command_packet_t *packet = (ld2420_command_packet_t *)malloc(sizeof(ld2420_command_packet_t));
    if (packet == NULL)
    {
        return NULL;
    }

    size_t offset = 0;
    memcpy(packet->HEADER, buffer + offset, sizeof(packet->HEADER));
    offset += sizeof(packet->HEADER);

    memcpy(&packet->frame_size, buffer + offset, sizeof(packet->frame_size));
    offset += sizeof(packet->frame_size);

    memcpy(&packet->cmd, buffer + offset, sizeof(packet->cmd));
    offset += sizeof(packet->cmd);

    // Now that we know frame_size, check if buffer is large enough (optional, if buffer length is available)
    if (packet->frame_size > 0)
    {
        packet->frame_data = (unsigned char *)malloc(packet->frame_size);
        if (packet->frame_data == NULL)
        {
            free(packet);
            return NULL;
        }

        memcpy(packet->frame_data, buffer + offset, packet->frame_size);
        offset += packet->frame_size;
    }
    else
    {
        packet->frame_data = NULL;
    }

    memcpy(packet->FOOTER, buffer + offset, sizeof(packet->FOOTER));
    offset += sizeof(packet->FOOTER);

    if (!__validate_rx_packet__(packet))
    {
        ld2420_free_command_packet(packet);
        return NULL;
    }

    return packet;
}

ld2420_command_packet_t *ld2420_create_tx_command_packet(
    ld2420_command_t cmd,
    unsigned char *frame_data,
    unsigned short frame_size)
{
    if (frame_size > 0 && frame_data == NULL)
    {
        // Invalid input: nonzero frame_size but no data
        return NULL;
    }

    ld2420_command_packet_t *packet = (ld2420_command_packet_t *)malloc(sizeof(ld2420_command_packet_t));
    if (packet == NULL)
    {
        return NULL;
    }

    // Setting the default header and footer for the TX command packet.
    __initialize_packet_header_and_footer__(packet);

    packet->frame_size = frame_size;
    packet->cmd = cmd;

    if (frame_size > 0)
    {
        packet->frame_data = (unsigned char *)malloc(frame_size);
        if (packet->frame_data == NULL)
        {
            free(packet);
            return NULL;
        }
        memcpy(packet->frame_data, frame_data, frame_size);
    }
    else
    {
        packet->frame_data = NULL;
    }

    return packet;
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