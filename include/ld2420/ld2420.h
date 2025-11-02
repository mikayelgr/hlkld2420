#pragma once
#include <stddef.h>

/**
 * The baud rate for HLK-LD2420 communication needs to be set to 115,200 from the official
 * documentation at https://hlktech.net/index.php?id=1291.
 */
#define LD2420_BAUD_RATE 115200

/**
 * Minimum size of a valid RX command packet (header + frame_size + cmd + footer). This
 * doesn't take into consideration the variable-length frame_data.
 */
#define MIN_RX_PACKET_SIZE (sizeof(((ld2420_command_packet_t *)0)->HEADER) +    \
                            sizeof(unsigned short) + sizeof(ld2420_command_t) + \
                            sizeof(((ld2420_command_packet_t *)0)->FOOTER))

/**
 * The header bytes for sending a command packet to the LD2420 module. This is documented
 * in the official protocol for HLK-LD2420 at https://hlktech.net/index.php?id=1291.
 */
static const unsigned char LD2420_BEG_COMMAND_PACKET[4] = {0xFD, 0xFC, 0xFB, 0xFA};

/**
 * The end bytes for sending a command packet to the LD2420 module. This is documented
 * in the official protocol for HLK-LD2420 at https://hlktech.net/index.php?id=1291.
 */
static const unsigned char LD2420_END_COMMAND_PACKET[4] = {04, 03, 02, 01};

#ifdef __cplusplus
extern "C"
{
#endif
    typedef enum
    {
        LD2420_CMD_OPEN_MODE = (unsigned short)0xFF,
        LD2420_CMD_CLOSE_MODE = (unsigned short)0xFE,
        LD2420_CMD_READ_VERSION = (unsigned short)0x00,
        LD2420_CMD_REBOOT = (unsigned short)0x68,
        LD2420_CMD_READ_CONFIG = (unsigned short)0x08,
        LD2420_CMD_SET_CONFIG = (unsigned short)0x07,
    } ld2420_command_t;

    typedef enum
    {
        LD2420_PARAM_MIN_DISTANCE = (unsigned short)0x00,
        LD2420_PARAM_MAX_DISTANCE = (unsigned short)0x01,
        LD2420_PARAM_DELAY_TIME = (unsigned short)0x04,
        LD2420_PARAM_TRIGGER_BASE = (unsigned short)0x10,
        LD2420_PARAM_MAINTAIN_BASE = (unsigned short)0x20,
    } ld2420_command_parameter_t;

    /**
     * @brief Structure representing a transmit command packet for the LD2420 module.
     */
    typedef struct
    {
        /**
         * Default packet header for all command packets. This will always need to be pre-populated
         * with the LD2420_BEG_COMMAND_PACKET value.
         */
        unsigned char HEADER[4];

        /**
         * The size of the frame data in bytes.
         */
        unsigned short frame_size;

        /**
         * Command identifier (2 bytes).
         */
        unsigned short cmd;

        /**
         * The frame data payload. The length of this data is variable and its full size
         * is indicated by the size field.
         */
        unsigned char *frame_data;

        /**
         * Default packet footer for all command packets. This will always need to be pre-populated
         * with the LD2420_END_COMMAND_PACKET value.
         */
        unsigned char FOOTER[4];
    } ld2420_command_packet_t;

    /**
     * @brief Creates and initializes a transmit command packet for the LD2420 module.
     */
    ld2420_command_packet_t *ld2420_create_tx_command_packet(
        ld2420_command_t cmd,
        unsigned char *frame_data,
        unsigned short frame_size);

    /**
     * @brief Given a buffer, parses the RX packet sent from the LD2420 module and parses it
     *        as an rx command packet.
     */
    ld2420_command_packet_t *ld2420_parse_rx_command_packet(
        unsigned char *buffer, size_t buffer_size);

    /**
     * @brief Frees the memory allocated for a command packet.
     */
    void ld2420_free_command_packet(ld2420_command_packet_t *packet);
#ifdef __cplusplus
}
#endif