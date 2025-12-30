#include <unity.h>
#include <stdlib.h>
#include <ld2420/ld2420.h>
#include <ld2420/ld2420_stream.h>

static int stream_frames;
static uint16_t stream_cmd;
static uint16_t stream_status;
static uint16_t stream_packet_len;

void setUp(void)
{
    stream_frames = 0;
    stream_cmd = 0;
    stream_status = 0;
    stream_packet_len = 0;
}

void tearDown(void)
{
}

static bool on_stream_frame(
    const uint8_t *frame,
    uint16_t frame_size_bytes,
    uint16_t cmd_echo,
    uint16_t status)
{
    (void)frame; // unused in test
    stream_frames++;
    stream_cmd = cmd_echo;
    stream_status = status;
    stream_packet_len = frame_size_bytes;
    return true; // keep accepting frames
}

void test__streaming_parser_handles_chunking(void)
{
    // Single valid frame fed one byte at a time.
    static const uint8_t FRAME[] = {
        0xFD, 0xFC, 0xFB, 0xFA, // header
        0x08, 0x00,             // frame size (8)
        0xFF, 0x01,             // cmd echo
        0x00, 0x00,             // status
        0x02, 0x00, 0x20, 0x00, // payload (4 bytes)
        0x04, 0x03, 0x02, 0x01  // footer
    };
    const size_t TOTAL = sizeof(FRAME);

    ld2420_stream_t s;
    ld2420_stream_init(&s);
    stream_frames = 0;
    stream_cmd = stream_status = stream_packet_len = 0;

    // Feed each byte individually
    for (size_t i = 0; i < TOTAL; i++)
    {
        ld2420_status_t status = ld2420_stream_feed(&s, &FRAME[i], 1, on_stream_frame);
        // Frame should be complete only on the last byte
        if (i < TOTAL - 1)
        {
            TEST_ASSERT_EQUAL(LD2420_STATUS_OK, status);
            TEST_ASSERT_EQUAL(0, stream_frames); // No frame yet
        }
    }

    // After final byte, frame should be parsed and callback invoked
    TEST_ASSERT_EQUAL(1, stream_frames);
    TEST_ASSERT_EQUAL_UINT16(0xFF, stream_cmd);
    TEST_ASSERT_EQUAL_UINT16(LD2420_STATUS_OK, stream_status);
    TEST_ASSERT_EQUAL_UINT16(TOTAL, stream_packet_len);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test__streaming_parser_handles_chunking);
    return UNITY_END();
}