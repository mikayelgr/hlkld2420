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

    size_t serialized_out_size = 0;
    ld2420_command_packet_t *packet = ld2420_create_tx_command_packet(
        LD2420_CMD_READ_VERSION_NUMBER, NULL, 0, NULL);
    unsigned char *serialized = ld2420_serialize_command_packet(packet, &serialized_out_size, NULL);

    TEST_ASSERT_EQUAL(sizeof(expected_frame_data), serialized_out_size);
    TEST_ASSERT_EQUAL_CHAR_ARRAY(expected_frame_data, serialized, serialized_out_size);

    ld2420_free_command_packet(packet);
    free((void *)serialized);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test__create_read_version_number_command);
    return UNITY_END();
}
