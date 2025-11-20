#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "ld2420.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * Streaming (incremental) parser for LD2420 frames.
     *
     * Motivation:
     * - UART/serial transports can split frames arbitrarily and may include noise.
     * - The streaming parser incrementally finds headers, reads the 16-bit little-endian
     *   intra-frame length, accumulates the frame, validates the footer, and then invokes
     *   the existing one-shot parser (ld2420_parse_rx_buffer) to decode metadata.
     *
     * Design highlights:
     * - Uses a single linear buffer sized to LD2420_MAX_RX_PACKET_SIZE.
     * - Resynchronizes on malformed data by scanning for the next header; preserves up to
     *   3 trailing bytes to match headers split across chunks.
     * - Can emit zero or more frames per feed() call (handles back-to-back frames).
     * - Remains agnostic of the transport; thread-unsafe by design (one context per stream).
     */
    typedef struct
    {
        /** Internal linear buffer accumulating the current frame under construction. */
        uint8_t buffer[LD2420_MAX_RX_PACKET_SIZE];
        /** Number of bytes currently in buffer (0..LD2420_MAX_RX_PACKET_SIZE). */
        uint16_t index;
        /**
         * Expected total frame size in bytes (header+len+payload+footer) once known.
         * 0 means the 2-byte length has not been fully observed yet.
         */
        uint16_t expected_total_size;
        /** True after a valid header was recognized at buffer[0]. */
        bool synced;
    } ld2420_stream_t;

    /**
     * Signature for the streaming frame callback.
     *
     * Parameters:
     * - frame: Pointer to the contiguous raw frame (starts at header).
     * - frame_size_bytes: Total size of frame in bytes.
     * - cmd_echo: Parsed command echo value (16-bit; library-normalized to 0x00FF for open config example).
     * - status: Parsed status field (16-bit).
     *
     * Return:
     * - true to continue processing more bytes/frames in this call.
     * - false to stop early (useful when pushing frames into a bounded queue).
     */
    typedef bool (*ld2420_stream_on_frame_fn)(
        const uint8_t *frame,
        uint16_t frame_size_bytes,
        uint16_t cmd_echo,
        uint16_t status);

    /**
     * Initialize/reset a stream parser context.
     *
     * Notes:
     * - Must be called before the first use.
     * - Safe to call to discard any in-progress partial frame and resync.
     */
    void ld2420_stream_init(ld2420_stream_t *s);

    /**
     * Feed a single byte to the streaming parser.
     *
     * Parameters:
     * - s: Parser context (must be initialized).
     * - data: Pointer to exactly one byte (may be NULL if len==0).
     * - len: Must be exactly 1 (or 0 for no-op).
     * - on_frame: Callback invoked when a valid complete frame is assembled.
     *
     * Return:
     * - LD2420_STATUS_OK on successful byte processing (frame may or may not be complete).
     * - LD2420_STATUS_ERROR_INVALID_ARGUMENTS if len != 0 and len != 1, or callback is NULL.
     * - LD2420_STATUS_ERROR_BUFFER_TOO_SMALL if computed frame size exceeds limits.
     * - LD2420_STATUS_ERROR_INVALID_FOOTER if a complete frame has invalid footer.
     * - LD2420_STATUS_ERROR_INVALID_PACKET if frame parsing fails.
     *
     * Behavior:
     * - On each feed, the buffer is validated.
     * - If a complete valid frame is assembled, it is validated and the callback is invoked.
     * - If a frame is detected as corrupted, it is discarded and an error status is returned.
     * - Handles resynchronization to the next valid header on corruption.
     */
    ld2420_status_t ld2420_stream_feed(
        ld2420_stream_t *s,
        const uint8_t *data,
        size_t len,
        ld2420_stream_on_frame_fn on_frame);

#ifdef __cplusplus
}
#endif
