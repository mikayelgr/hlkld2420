#include "unity.h"
#include "ld2420/ld2420.h"

void setUp(void)
{
    // Set up code before each test
}

void tearDown(void)
{
    // Clean up code after each test
}

void test_example(void)
{
    // Example test case
    TEST_ASSERT_EQUAL(1, 1);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_example);
    return UNITY_END();
}
