#include <unity.h>

#include "alarm.h"
#include "input.h"

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

void test_temp_normal(void)
{
    TEST_ASSERT_EQUAL(
        ALARM_NORMAL,
        evaluateTemperature(24.0f)
    );
}

void test_temp_exactly_upper(void)
{
    TEST_ASSERT_EQUAL(
        ALARM_HIGH_TEMPERATURE,
        evaluateTemperature(30.0f)
    );
}

void test_temp_above_upper(void)
{
    TEST_ASSERT_EQUAL(
        ALARM_HIGH_TEMPERATURE,
        evaluateTemperature(35.0f)
    );
}

void test_next_display_mode(void)
{
    TEST_ASSERT_EQUAL(
        MODE_HUMIDITY,
        nextDisplayMode(MODE_TEMPERATURE)
    );
}

void test_next_display_mode_wrap(void)
{
    TEST_ASSERT_EQUAL(
        MODE_TEMPERATURE,
        nextDisplayMode(MODE_MOTION)
    );
}

void test_previous_display_mode(void)
{
    TEST_ASSERT_EQUAL(
        MODE_MOTION,
        previousDisplayMode(MODE_TEMPERATURE)
    );
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_temp_below_lower);
    RUN_TEST(test_temp_exactly_lower);
    RUN_TEST(test_temp_normal);
    RUN_TEST(test_temp_exactly_upper);
    RUN_TEST(test_temp_above_upper);

    RUN_TEST(test_next_display_mode);
    RUN_TEST(test_next_display_mode_wrap);
    RUN_TEST(test_previous_display_mode);

    return UNITY_END();
}