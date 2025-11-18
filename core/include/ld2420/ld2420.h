#pragma once
#include <stddef.h>
#include <stdint.h>

/**
 * IMPORTANT: The LD2420 protocol uses LITTLE-ENDIAN byte order for all multi-byte values.
 * This library uses explicit little-endian byte-order conversion functions to ensure
 * correct operation on both little-endian and big-endian host architectures.
 * All multi-byte fields (frame_size, cmd_echo, status, etc.) are read/written using
 * read_le16()/write_le16() helpers that handle byte-order conversion transparently.
 */

/**
 * The baud rate for HLK-LD2420 communication needs to be set to 115,200 from the official
 * documentation at https://hlktech.net/index.php?id=1291.
 */
#define LD2420_BAUD_RATE 115200

/**
 * Minimum size of a valid RX command packet (header + frame_size + cmd + footer). This
 * doesn't take into consideration the variable-length frame_data. The specifics of
 * this number will be reflected in the README.
 */
#define LD2420_MIN_RX_PACKET_SIZE (unsigned char)14

/**
 * Maximum size of a valid RX command packet. Refer to the "Maximum Receive (RX) Packet Size"
 * section in the README for details on how to compute this value.
 */
#define LD2420_MAX_RX_PACKET_SIZE (unsigned char)154

/**
 * Minimum size of a valid TX command packet. Refer to the "Maximum Transmit (TX) Packet Size"
 * section in the README for details on how to compute this value.
 */
#define LD2420_MIN_TX_PACKET_SIZE (unsigned char)12

/**
 * Maximum size of a valid TX command packet. Refer to the "Maximum Transmit (TX) Packet Size"
 * section in the README for details on how to compute this value.
 */
#define LD2420_MAX_TX_PACKET_SIZE (unsigned char)222

/**
 * The header bytes for sending a command packet to the LD2420 module. This is documented
 * in the official protocol for HLK-LD2420 at https://hlktech.net/index.php?id=1291.
 */
static const uint8_t LD2420_BEG_COMMAND_PACKET[] = {0xFD, 0xFC, 0xFB, 0xFA};

/**
 * The end bytes for sending a command packet to the LD2420 module. This is documented
 * in the official protocol for HLK-LD2420 at https://hlktech.net/index.php?id=1291.
 */
static const uint8_t LD2420_END_COMMAND_PACKET[] = {0x04, 0x03, 0x02, 0x01};

#ifdef __cplusplus
extern "C"
{
#endif
    typedef enum
    {
        LD2420_OK = 0,
        LD2420_ERROR_UNKNOWN,
        LD2420_ERROR_INVALID_PACKET,
        LD2420_ERROR_INVALID_BUFFER,
        LD2420_ERROR_INVALID_BUFFER_SIZE,
        LD2420_ERROR_INVALID_FRAME,
        LD2420_ERROR_INVALID_FRAME_SIZE,
        LD2420_ERROR_BUFFER_TOO_SMALL,
        LD2420_ERROR_INVALID_HEADER,
        LD2420_ERROR_INVALID_FOOTER,
        LD2420_ERROR_INVALID_ARGUMENTS,
        LD2420_ERROR_ALREADY_INITIALIZED,
    } ld2420_status_t;

    /**
     * @brief Enumeration of command IDs for the LD2420 module.
     */
    typedef enum
    {
        LD2420_CMD_OPEN_CONFIG_MODE = (unsigned short)0xFF,
        LD2420_CMD_CLOSE_CONFIG_MODE = (unsigned short)0xFE,
        LD2420_CMD_READ_VERSION_NUMBER = (unsigned short)0x00,
        LD2420_CMD_REBOOT = (unsigned short)0x68,
        LD2420_CMD_READ_CONFIG = (unsigned short)0x08,
        LD2420_CMD_SET_CONFIG = (unsigned short)0x07,
    } ld2420_command_t;

    /**
     * @brief Enumeration of command parameter IDs for the LD2420 module.
     */
    typedef enum
    {
        LD2420_PARAM_MIN_DISTANCE = (unsigned short)0x00,
        LD2420_PARAM_MAX_DISTANCE = (unsigned short)0x01,
        LD2420_PARAM_DELAY_TIME = (unsigned short)0x04,
        LD2420_PARAM_TRIGGER_BASE = (unsigned short)0x10,
        LD2420_PARAM_MAINTAIN_BASE = (unsigned short)0x20,
    } ld2420_command_parameter_t;

    ld2420_status_t ld2420_parse_rx_buffer(
        // Input variables
        const uint8_t *in_raw_rx_buffer,
        const uint8_t in_raw_rx_buffer_size,

        // Pointers to output variables
        uint16_t *out_frame_size,
        uint16_t *out_cmd_echo,
        uint16_t *out_status);

#ifdef __cplusplus
}
#endif