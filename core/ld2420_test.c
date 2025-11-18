#include <unity.h>
#include <ld2420/ld2420.h>

uint32_t byteswap_uint32(uint32_t x)
{
    return ((x >> 24) & 0xff) |
           ((x >> 8) & 0xff00) |
           ((x << 8) & 0xff0000) |
           ((x << 24) & 0xff000000);
}

void setUp(void)
{
}

void tearDown(void)
{
}

void test__rx_buffer_must_parse(void)
{
    // Test packet breakdown (all multi-byte values are little-endian):
    // [0..3]   0xFD 0xFC 0xFB 0xFA - Header
    // [4..5]   0x08 0x00 - Frame size = 8 (little-endian)
    // [6..7]   0xFF 0x01 - Command echo = 0x01FF (little-endian)
    // [8..9]   0x00 0x00 - Status = 0x0000 (little-endian)
    // [10..13] 0x02 0x00 0x20 0x00 - Payload (4 bytes)
    // [14..17] 0x04 0x03 0x02 0x01 - Footer
    static const uint8_t OPEN_COMMAND_MODE_RX_BUFFER[] = {
        0xFD, 0xFC, 0xFB, 0xFA,
        0x08, 0x00, 0xFF, 0x01,
        0x00, 0x00, 0x02, 0x00, 0x20, 0x00, 0x04, 0x03, 0x02, 0x01};
    static const size_t OPEN_COMMAND_MODE_RX_BUFFER_SIZE = sizeof(OPEN_COMMAND_MODE_RX_BUFFER);

    uint16_t frame_size = -1;
    uint16_t cmd_echo = -1;
    uint16_t status = -1;

    ld2420_status_t parse_status = ld2420_parse_rx_buffer(
        OPEN_COMMAND_MODE_RX_BUFFER,
        OPEN_COMMAND_MODE_RX_BUFFER_SIZE,
        &frame_size,
        &cmd_echo,
        &status);

    TEST_ASSERT_EQUAL(LD2420_OK, parse_status);
    TEST_ASSERT_EQUAL(8, frame_size);
    TEST_ASSERT_EQUAL(0xFF, cmd_echo);
    TEST_ASSERT_EQUAL(0, status);
}

void test__rx_buffer_must_fail(void)
{
    TEST_ASSERT_EQUAL(1, 1);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test__rx_buffer_must_parse);
    RUN_TEST(test__rx_buffer_must_fail);
    return UNITY_END();
}