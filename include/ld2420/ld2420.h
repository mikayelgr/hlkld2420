#pragma once
#include <stddef.h>

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
 * The header bytes for sending a command packet to the LD2420 module. This is documented
 * in the official protocol for HLK-LD2420 at https://hlktech.net/index.php?id=1291.
 */
#define LD2420_BEG_COMMAND_PACKET ((const unsigned char[4]){0xFD, 0xFC, 0xFB, 0xFA})

/**
 * The end bytes for sending a command packet to the LD2420 module. This is documented
 * in the official protocol for HLK-LD2420 at https://hlktech.net/index.php?id=1291.
 */
#define LD2420_END_COMMAND_PACKET ((const unsigned char[4]){0x04, 0x03, 0x02, 0x01})

#ifdef __cplusplus
extern "C"
{
#endif
    typedef enum
    {
        LD2420_OK = 0,
        LD2420_ERROR_INVALID_PACKET,
        LD2420_ERROR_INVALID_BUFFER,
        LD2420_ERROR_INVALID_BUFFER_SIZE,
        LD2420_ERROR_INVALID_FRAME,
        LD2420_ERROR_INVALID_FRAME_SIZE,
        LD2420_ERROR_BUFFER_TOO_SMALL,
        LD2420_ERROR_INVALID_HEADER_OR_FOOTER,
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

    /**
     * @brief Structure representing a command packet for the LD2420 module.
     * @note UPPERCASE field names are used to indicate that these fields are constant,
     *       are instantiated automatically, and should not be modified after initialization.
     * @note Frame data is passed separately to functions, not stored in this struct.
     *       This design eliminates dynamic memory allocation for better embedded performance.
     */
    typedef struct
    {
        /**
         * Default packet header for all command packets. This will always need to be pre-populated
         * with the LD2420_BEG_COMMAND_PACKET value.
         */
        unsigned char HEADER[4];

        /**
         * The size of the frame data in bytes. This includes the command size (2 bytes)
         * plus any additional frame data.
         */
        unsigned short frame_size;

        /**
         * Command identifier (2 bytes).
         */
        unsigned short cmd;

        /**
         * Default packet footer for all command packets. This will always need to be pre-populated
         * with the LD2420_END_COMMAND_PACKET value.
         */
        unsigned char FOOTER[4];
    } ld2420_command_packet_t;

    /**
     * @brief A single parameter block for a command passable to the LD2420 module.
     */
    typedef struct
    {
        /**
         * The parameter ID (2 bytes). Usually, this represents the band/channel number
         * for which the parameter is being set or read.
         */
        unsigned short param_id;

        /**
         * The parameter value (4 bytes). This will typically be used in contexts when we are
         * writing configuration parameters to the LD2420 module, specifically, for the
         * "Set-up module configuration parameters" command (0x07).
         */
        unsigned int value;
    } ld2420_command_param_block_t;

    /**
     * @brief Creates and initializes a transmit command packet for the LD2420 module.
     * @param cmd The command identifier
     * @param frame_data Pointer to the frame data payload (can be NULL if frame_size is 0)
     * @param frame_size Size of the additional frame data in bytes (not including the command itself)
     * @param out_packet Pointer to caller-provided packet structure to initialize
     * @return Status code indicating success or error type
     */
    ld2420_status_t ld2420_create_tx_command_packet(
        const ld2420_command_t cmd,
        const unsigned char *frame_data,
        const unsigned short frame_size,
        ld2420_command_packet_t *out_packet);

    /**
     * @brief Serializes a command packet into a byte buffer suitable for transmission.
     * @param packet Pointer to the command packet to serialize
     * @param frame_data Pointer to the frame data payload (can be NULL if no additional frame data)
     * @param frame_data_size Size of the additional frame data in bytes (not including the command)
     * @param out_buffer Pointer to caller-provided buffer to store serialized packet
     * @param buffer_capacity Size of the output buffer in bytes
     * @param out_size Pointer to size_t variable to capture actual bytes written (cannot be NULL)
     * @return Status code indicating success or error type
     */
    ld2420_status_t ld2420_serialize_command_packet(
        const ld2420_command_packet_t *packet,
        const unsigned char *frame_data,
        const unsigned short frame_data_size,
        unsigned char *out_buffer,
        const size_t buffer_capacity,
        size_t *out_size);

    /**
     * @brief Parses a received buffer into an RX command packet from the LD2420 module.
     * @param buffer Pointer to the buffer containing the received data
     * @param buffer_size Size of the input buffer in bytes
     * @param out_packet Pointer to caller-provided packet structure to populate
     * @param out_frame_data Pointer to caller-provided buffer for frame data (can be NULL if no frame data expected)
     * @param frame_data_capacity Size of the frame data buffer in bytes
     * @param out_frame_data_size Pointer to size_t to capture actual frame data bytes read (cannot be NULL)
     * @return Status code indicating success or error type
     * @note RX packets will always contain 2 additional bytes as response padding after the
     *       echo of the command.
     */
    ld2420_status_t ld2420_parse_rx_command_packet(
        const unsigned char *buffer,
        const size_t buffer_size,
        ld2420_command_packet_t *out_packet,
        unsigned char *out_frame_data,
        const size_t frame_data_capacity,
        size_t *out_frame_data_size);

#ifdef __cplusplus
}
#endif