#pragma once
#include <stddef.h>
#include <stdint.h>

/**
 * Protocol endianness
 * -------------------
 * The LD2420 protocol uses LITTLE-ENDIAN byte order for all multi-byte values.
 * This library uses explicit little-endian helpers to ensure correct operation
 * on both little-endian and big-endian host architectures.
 *
 * All multi-byte fields (frame_size, cmd_echo, status, payload words) are read
 * and written using helper routines that interpret bytes as little-endian.
 */

/**
 * Default serial baud rate for HLK-LD2420 modules as per vendor docs.
 * Reference: https://hlktech.net/index.php?id=1291
 */
#define LD2420_BAUD_RATE 115200

/**
 * Minimum size of a valid RX command packet (bytes):
 * header(4) + frame_size(2) + minimum payload(4 for cmd_echo+status) + footer(4) = 14.
 * Does not include any transport framing or escaping.
 */
#define LD2420_MIN_RX_PACKET_SIZE (unsigned char)14

/**
 * Maximum size of a valid RX command packet (bytes). See README for derivation.
 */
#define LD2420_MAX_RX_PACKET_SIZE (unsigned char)154

/**
 * Minimum size of a valid TX command packet (bytes). See README for framing details.
 */
#define LD2420_MIN_TX_PACKET_SIZE (unsigned char)12

/**
 * Maximum size of a valid TX command packet (bytes). See README for derivation.
 */
#define LD2420_MAX_TX_PACKET_SIZE (unsigned char)222

/**
 * Fixed 4-byte header marking the start of an LD2420 packet.
 */
static const uint8_t LD2420_BEG_COMMAND_PACKET[] = {0xFD, 0xFC, 0xFB, 0xFA};

/**
 * Fixed 4-byte footer marking the end of an LD2420 packet.
 */
static const uint8_t LD2420_END_COMMAND_PACKET[] = {0x04, 0x03, 0x02, 0x01};

#ifdef __cplusplus
extern "C"
{
#endif
    typedef enum
    {
        LD2420_OK = 0,                    /** Success */
        LD2420_ERROR_UNKNOWN,             /** Unspecified failure */
        LD2420_ERROR_INVALID_PACKET,      /** Packet content invalid */
        LD2420_ERROR_INVALID_BUFFER,      /** Null or malformed buffer pointer */
        LD2420_ERROR_INVALID_BUFFER_SIZE, /** Buffer size out of range or unexpected */
        LD2420_ERROR_INVALID_FRAME,       /** Frame content invalid */
        LD2420_ERROR_INVALID_FRAME_SIZE,  /** Intra-frame length field invalid (0 or too small) */
        LD2420_ERROR_BUFFER_TOO_SMALL,    /** Computed frame exceeds internal buffer limits */
        LD2420_ERROR_INVALID_HEADER,      /** Header bytes mismatch */
        LD2420_ERROR_INVALID_FOOTER,      /** Footer bytes mismatch */
        LD2420_ERROR_INVALID_ARGUMENTS,   /** One or more arguments invalid */
        LD2420_ERROR_ALREADY_INITIALIZED, /** Re-initialization not allowed in specific contexts */
    } ld2420_status_t;

    /** Enumeration of command IDs for the LD2420 module. */
    typedef enum
    {
        LD2420_CMD_OPEN_CONFIG_MODE = (unsigned short)0xFF,    /** Enter configuration mode */
        LD2420_CMD_CLOSE_CONFIG_MODE = (unsigned short)0xFE,   /** Exit configuration mode */
        LD2420_CMD_READ_VERSION_NUMBER = (unsigned short)0x00, /** Read firmware version */
        LD2420_CMD_REBOOT = (unsigned short)0x68,              /** Reboot the device */
        LD2420_CMD_READ_CONFIG = (unsigned short)0x08,         /** Read current configuration */
        LD2420_CMD_SET_CONFIG = (unsigned short)0x07,          /** Set configuration parameters */
    } ld2420_command_t;

    /** Enumeration of command parameter IDs for the LD2420 module. */
    typedef enum
    {
        LD2420_PARAM_MIN_DISTANCE = (unsigned short)0x00,  /** Minimum detection distance */
        LD2420_PARAM_MAX_DISTANCE = (unsigned short)0x01,  /** Maximum detection distance */
        LD2420_PARAM_DELAY_TIME = (unsigned short)0x04,    /** Hold/trigger delay */
        LD2420_PARAM_TRIGGER_BASE = (unsigned short)0x10,  /** Trigger threshold base */
        LD2420_PARAM_MAINTAIN_BASE = (unsigned short)0x20, /** Maintain threshold base */
    } ld2420_command_parameter_t;

    /**
     * Parse a single complete LD2420 RX buffer (one-shot parsing).
     *
     * Input:
     * - in_raw_rx_buffer: Pointer to contiguous bytes beginning at header.
     * - in_raw_rx_buffer_size: Total number of bytes in the packet (header..footer).
     *
     * Output:
     * - out_frame_size: Intra-frame data size extracted from the 2-byte length field.
     * - out_cmd_echo: Command echo field (LSB significant). The library normalizes
     *   this to fit the 16-bit echo without the "+1 trailing 1-bit" noted in protocol.
     * - out_status: Status word emitted by the device.
     *
     * Return:
     * - LD2420_OK on success.
     * - Error codes on invalid header/footer, buffer size mismatch, or bad length.
     */
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