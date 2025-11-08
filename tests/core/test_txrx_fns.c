#include "unity.h"
#include "ld2420/ld2420.h"
#include <stdlib.h>

void setUp(void)
{
    // Set up code before each test
}

void tearDown(void)
{
    // Clean up code after each test
}

void test__create_read_version_number_command(void)
{
    static unsigned char expected_frame_data[] = {
        0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0x00, 0x00, 0x04, 0x03, 0x02, 0x01};

    // Stack-allocated buffers - no malloc/free needed!
    ld2420_command_packet_t packet;
    unsigned char serialized[64];
    size_t serialized_out_size = 0;

    // Create TX command packet
    ld2420_status_t status = ld2420_create_tx_command_packet(
        LD2420_CMD_READ_VERSION_NUMBER, NULL, 0, &packet);
    TEST_ASSERT_EQUAL(LD2420_OK, status);

    // Serialize the packet
    status = ld2420_serialize_command_packet(
        &packet, NULL, 0, serialized, sizeof(serialized), &serialized_out_size);
    TEST_ASSERT_EQUAL(LD2420_OK, status);

    // Verify serialized output
    TEST_ASSERT_EQUAL(sizeof(expected_frame_data), serialized_out_size);
    TEST_ASSERT_EQUAL_CHAR_ARRAY(expected_frame_data, serialized, serialized_out_size);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test__create_read_version_number_command);
    return UNITY_END();
}
