/*
 * LD2420 streaming parser implementation
 *
 * Design Principles
 * -----------------
 * 1. Feed one byte at a time via ld2420_stream_feed()
 * 2. On each byte, validate the buffer state
 * 3. If a complete valid frame is assembled, invoke the callback
 * 4. If a frame is corrupted, discard it and return error status
 *
 * State Machine
 * ------------- 
 * - NOT_SYNCED: Scanning for the 4-byte header
 * - SYNCED: Header found, accumulating frame bytes
 * - FRAME_READY: Complete frame assembled, footer validated, ready to parse
 *
 * Memory & Threading
 * ------------------
 * - Single linear buffer sized to LD2420_MAX_RX_PACKET_SIZE
 * - Not thread-safe; use one context per stream
 * - No dynamic allocation
 */

#include <string.h>

#include <ld2420/ld2420.h>
#include <ld2420/ld2420_stream.h>

/**
 * When compiling for a big-endian platform, we need to use helper routines
 * to read/write little-endian multi-byte values.
 */
#ifdef LD2420_PLATFORM_BE
/**
 * Read a 16-bit little-endian value from a buffer regardless of host endianness.
 * Example: bytes [0xFF, 0x01] represent 0x01FF in little-endian.
 */
static inline uint16_t read_le16_inline(const uint8_t *b)
{
    return (uint16_t)b[0] | ((uint16_t)b[1] << 8);
}
#endif

void ld2420_stream_init(ld2420_stream_t *s)
{
    if (!s)
        return;
    s->index = 0;
    s->expected_total_size = 0;
    s->synced = false;
}

/**
 * Scan backwards through the current buffer to find the last occurrence of the
 * 4-byte header. If found, move it to the front and return true. Otherwise
 * keep at most 3 trailing bytes (potential partial header) and return false.
 */
static bool resync_to_next_header(ld2420_stream_t *s)
{
    const uint16_t header_size = sizeof(LD2420_BEG_COMMAND_PACKET);

    // Scan backwards for the last occurrence of a valid header
    for (int16_t i = (int16_t)(s->index - header_size); i >= 0; --i)
    {
        if (memcmp(&s->buffer[i], LD2420_BEG_COMMAND_PACKET, header_size) == 0)
        {
            // Found header at position i
            uint16_t remaining = s->index - i;
            memmove(s->buffer, &s->buffer[i], remaining);
            s->index = remaining;
            s->synced = true;
            s->expected_total_size = 0;
            return true;
        }
    }

    // No full header found; preserve up to 3 trailing bytes (max header overlap)
    uint16_t keep = (s->index < header_size - 1) ? s->index : (header_size - 1);
    if (keep > 0 && keep < s->index)
        memmove(s->buffer, &s->buffer[s->index - keep], keep);
    s->index = keep;
    s->synced = false;
    s->expected_total_size = 0;
    return false;
}

ld2420_status_t ld2420_stream_feed(
    ld2420_stream_t *s,
    const uint8_t *data,
    size_t len,
    ld2420_stream_on_frame_fn on_frame)
{
    // Validate arguments
    if (!s || !on_frame)
        return LD2420_STATUS_ERROR_INVALID_ARGUMENTS;

    // Allow empty feed (data==NULL && len==0) as a valid no-op
    if (!data || len == 0)
        return LD2420_STATUS_OK;

    // For now, feed only one byte at a time per specification
    if (len != 1)
        return LD2420_STATUS_ERROR_INVALID_ARGUMENTS;

    uint8_t byte = data[0];

    // Buffer overflow check
    if (s->index >= sizeof(s->buffer))
    {
        // Try to resync; if still can't sync, discard and start fresh
        if (!resync_to_next_header(s))
        {
            s->index = 0;
            s->synced = false;
            s->expected_total_size = 0;
            return LD2420_STATUS_ERROR_BUFFER_TOO_SMALL;
        }
    }

    // Add the byte to the buffer
    s->buffer[s->index++] = byte;

    // If not synced yet, check if we now have a header
    if (!s->synced)
    {
        const uint16_t header_size = sizeof(LD2420_BEG_COMMAND_PACKET);
        if (s->index >= header_size)
        {
            if (memcmp(&s->buffer[s->index - header_size], LD2420_BEG_COMMAND_PACKET, header_size) == 0)
            {
                // Align header to front
                uint16_t remaining = s->index - (s->index - header_size);
                memmove(s->buffer, &s->buffer[s->index - header_size], remaining);
                s->index = remaining;
                s->synced = true;
                s->expected_total_size = 0;
            }
            else if (s->index >= header_size)
            {
                // Shift buffer left by 1 to continue searching for header
                memmove(s->buffer, &s->buffer[1], s->index - 1);
                s->index--;
            }
        }
        return LD2420_STATUS_OK;
    }

    // We are synced (buffer starts with header). Check if we can determine expected size.
    if (s->expected_total_size == 0 && s->index >= sizeof(LD2420_BEG_COMMAND_PACKET) + 2)
    {
        // We have header + 2-byte length field; compute expected total size
#ifdef LD2420_PLATFORM_BE
        uint16_t frame_len = read_le16_inline(&s->buffer[sizeof(LD2420_BEG_COMMAND_PACKET)]);
#else
        uint16_t frame_len = *(uint16_t *)(&s->buffer[sizeof(LD2420_BEG_COMMAND_PACKET)]);
#endif
        // total = header(4) + len(2) + frame_len + footer(4)
        uint32_t total = (uint32_t)sizeof(LD2420_BEG_COMMAND_PACKET) + 2u + (uint32_t)frame_len + (uint32_t)sizeof(LD2420_END_COMMAND_PACKET);

        if (total > sizeof(s->buffer) || total > LD2420_MAX_RX_PACKET_SIZE)
        {
            // Invalid length; resync to next header or discard
            if (!resync_to_next_header(s))
            {
                s->index = 0;
                s->synced = false;
            }
            return LD2420_STATUS_ERROR_BUFFER_TOO_SMALL;
        }
        s->expected_total_size = (uint16_t)total;
    }

    // If we know the expected size and buffer is complete, validate and emit
    if (s->expected_total_size > 0 && s->index == s->expected_total_size)
    {
        // Validate footer
        const uint16_t footer_offset = s->expected_total_size - sizeof(LD2420_END_COMMAND_PACKET);
        if (memcmp(&s->buffer[footer_offset], LD2420_END_COMMAND_PACKET, sizeof(LD2420_END_COMMAND_PACKET)) != 0)
        {
            // Footer mismatch; resync or discard
            if (!resync_to_next_header(s))
            {
                s->index = 0;
                s->synced = false;
            }
            return LD2420_STATUS_ERROR_INVALID_FOOTER;
        }

        // Frame is complete and footer is valid; parse metadata
        uint16_t out_frame_size = 0, out_cmd_echo = 0, out_status = 0;
        ld2420_status_t parse_status = ld2420_parse_rx_buffer(
            s->buffer,
            (uint8_t)s->expected_total_size,
            &out_frame_size,
            &out_cmd_echo,
            &out_status);

        if (parse_status == LD2420_STATUS_OK)
        {
            // Valid frame; invoke callback
            on_frame(s->buffer, s->expected_total_size, out_cmd_echo, out_status);
        }
        else
        {
            // Parse failed; treat as corrupted frame
            parse_status = LD2420_STATUS_ERROR_INVALID_PACKET;
        }

        // Reset for next frame
        s->index = 0;
        s->expected_total_size = 0;
        s->synced = false;

        return parse_status;
    }

    return LD2420_STATUS_OK;
}
