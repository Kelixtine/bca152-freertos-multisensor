#include <unity.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "alarm.h"
#include "system_state.h"

void setUp(void) {}
void tearDown(void) {}

void test_temp_below_lower_threshold(void) {
    TEST_ASSERT_EQUAL(AlarmState::LOW_TEMP, evaluateTemperature(15.0f));
}
void test_temp_exactly_lower_threshold(void) {
    TEST_ASSERT_EQUAL(AlarmState::NORMAL, evaluateTemperature(18.0f));
}
void test_temp_normal_value(void) {
    TEST_ASSERT_EQUAL(AlarmState::NORMAL, evaluateTemperature(24.0f));
}
void test_temp_exactly_upper_threshold(void) {
    TEST_ASSERT_EQUAL(AlarmState::NORMAL, evaluateTemperature(30.0f));
}
void test_temp_above_upper_threshold(void) {
    TEST_ASSERT_EQUAL(AlarmState::HIGH_TEMP, evaluateTemperature(35.0f));
}

void test_display_forward_transition(void) {
    TEST_ASSERT_EQUAL(DisplayMode::HUMIDITY, getNextDisplayMode(DisplayMode::TEMPERATURE));
}
void test_display_reverse_transition(void) {
    TEST_ASSERT_EQUAL(DisplayMode::TEMPERATURE, getPreviousDisplayMode(DisplayMode::HUMIDITY));
}
void test_display_forward_wraparound(void) {
    TEST_ASSERT_EQUAL(DisplayMode::TEMPERATURE, getNextDisplayMode(DisplayMode::MOTION));
}
void test_display_reverse_wraparound(void) {
    TEST_ASSERT_EQUAL(DisplayMode::MOTION, getPreviousDisplayMode(DisplayMode::TEMPERATURE));
}
void test_state_active_no_timeout(void) {
    TEST_ASSERT_EQUAL(SystemState::ACTIVE, evaluateSystemState(SystemState::ACTIVE, false, 5000, 15000));
}
void test_state_active_with_timeout(void) {
    TEST_ASSERT_EQUAL(SystemState::INACTIVE, evaluateSystemState(SystemState::ACTIVE, false, 15000, 15000));
}
void test_state_inactive_no_motion(void) {
    TEST_ASSERT_EQUAL(SystemState::INACTIVE, evaluateSystemState(SystemState::INACTIVE, false, 0, 15000));
}
void test_state_inactive_with_motion(void) {
    TEST_ASSERT_EQUAL(SystemState::ACTIVE, evaluateSystemState(SystemState::INACTIVE, true, 0, 15000));
}

extern "C" void app_main(void) {
    vTaskDelay(pdMS_TO_TICKS(2000)); 

    UNITY_BEGIN();
    
    RUN_TEST(test_temp_below_lower_threshold);
    RUN_TEST(test_temp_exactly_lower_threshold);
    RUN_TEST(test_temp_normal_value);
    RUN_TEST(test_temp_exactly_upper_threshold);
    RUN_TEST(test_temp_above_upper_threshold);

    RUN_TEST(test_display_forward_transition);
    RUN_TEST(test_display_reverse_transition);
    RUN_TEST(test_display_forward_wraparound);
    RUN_TEST(test_display_reverse_wraparound);

    RUN_TEST(test_state_active_no_timeout);
    RUN_TEST(test_state_active_with_timeout);
    RUN_TEST(test_state_inactive_no_motion);
    RUN_TEST(test_state_inactive_with_motion);

    UNITY_END();

    fflush(stdout);
}