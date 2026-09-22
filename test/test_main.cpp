#include <unity.h>

#include "alarm.h"

void setUp(void)
{
}

void tearDown(void)
{
}

void test_temp_below_lower(void)
{
    TEST_ASSERT_EQUAL(
        ALARM_LOW_TEMPERATURE,
        evaluateTemperature(15.0f)
    );
}

void test_temp_exactly_lower(void)
{
    TEST_ASSERT_EQUAL(
        ALARM_LOW_TEMPERATURE,
        evaluateTemperature(18.0f)
    );
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_temp_below_lower);
    RUN_TEST(test_temp_exactly_lower);

    return UNITY_END();
}