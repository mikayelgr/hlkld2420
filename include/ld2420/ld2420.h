#pragma once
#include <stddef.h>
#include <stdint.h>

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

    // /**
    //  * @brief Creates and initializes a transmit command packet for the LD2420 module.
    //  * @param cmd The command identifier
    //  * @param frame_data Pointer to the frame data payload (can be NULL if frame_size is 0)
    //  * @param frame_size Size of the additional frame data in bytes (not including the command itself)
    //  * @param out_packet Pointer to caller-provided packet structure to initialize
    //  * @return Status code indicating success or error type
    //  */
    // ld2420_status_t ld2420_create_tx_command_packet(
    //     const ld2420_command_t cmd,
    //     const u_int8_t *frame_data,
    //     const u_int16_t frame_size,
    //     u_int8_t *tx_buffer,
    //     u_int8_t *tx_buffer_size);

    // /**
    //  * @brief Serializes a command packet into a byte buffer suitable for transmission.
    //  * @param packet Pointer to the command packet to serialize
    //  * @param frame_data Pointer to the frame data payload (can be NULL if no additional frame data)
    //  * @param frame_data_size Size of the additional frame data in bytes (not including the command)
    //  * @param out_buffer Pointer to caller-provided buffer to store serialized packet
    //  * @param buffer_capacity Size of the output buffer in bytes
    //  * @param out_size Pointer to size_t variable to capture actual bytes written (cannot be NULL)
    //  * @return Status code indicating success or error type
    //  */
    // ld2420_status_t ld2420_serialize_command_packet(
    //     const u_int8_t *frame_data,
    //     const u_int16_t frame_data_size,
    //     u_int8_t *out_buffer,
    //     const size_t buffer_capacity,
    //     size_t *out_size);

    /**
     * @brief Parses a received command packet from the LD2420 module.
     *
     * This function validates and extracts the intra-frame data from a complete RX command packet
     * received from the LD2420 module. It verifies the packet structure including header, footer,
     * and frame size constraints before extracting the payload data.
     *
     * @param rx_buffer Pointer to the buffer containing the received command packet data.
     *                  Must be a valid buffer containing at least LD2420_MIN_RX_PACKET_SIZE bytes.
     *                  Cannot be NULL.
     * @param rx_buffer_size Size of the received buffer in bytes. Must be between
     *                       LD2420_MIN_RX_PACKET_SIZE and LD2420_MAX_RX_PACKET_SIZE inclusive.
     * @param out_intra_frame_data Pointer to caller-provided buffer to store the extracted
     *                             intra-frame data (the variable-length payload between the
     *                             command ID and footer). Can be NULL if only size is needed.
     * @param out_intra_frame_data_size Pointer to variable that will receive the size of the
     *                                  extracted intra-frame data in bytes. Cannot be NULL.
     *
     * @note The intra-frame data excludes the header, frame size field. However, it does include
     *      the command ID and any additional frame data, including the footer. For this reason,
     *      you must always use the out_intra_frame_data_size to determine how many bytes there
     *      are in the received data.
     * @note If out_intra_frame_data is NULL, the function will only validate the packet and
     *       return the required size in out_intra_frame_data_size.
     */
    ld2420_status_t ld2420_parse_raw_rx_command_packet(
        const uint8_t *rx_buffer,
        const uint8_t rx_buffer_size,
        uint8_t *out_intra_frame_data,
        uint16_t *out_intra_frame_data_size);

#ifdef __cplusplus
}
#endif