#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <unity.h>

#include "rtos_objects.h"
#include "sensors.h"
#include "display.h"
#include "input.h"
#include "alarm.h"
#include "motion.h"
#include "system_state.h"


static void test_alarm_below_lower_threshold(void) {
    TEST_ASSERT_EQUAL(ALARM_LOW_TEMPERATURE, evaluateTemperature(15.0f));
}

static void test_alarm_exact_lower_threshold(void) {
    TEST_ASSERT_EQUAL(ALARM_LOW_TEMPERATURE, evaluateTemperature(18.0f));
}

static void test_alarm_normal_temperature(void) {
    TEST_ASSERT_EQUAL(ALARM_NORMAL, evaluateTemperature(24.5f));
}

static void test_alarm_exact_upper_threshold(void) {
    TEST_ASSERT_EQUAL(ALARM_HIGH_TEMPERATURE, evaluateTemperature(30.0f));
}

static void test_alarm_above_upper_threshold(void) {
    TEST_ASSERT_EQUAL(ALARM_HIGH_TEMPERATURE, evaluateTemperature(35.5f));
}


static void test_display_navigation_forward(void) {
    TEST_ASSERT_EQUAL(MODE_HUMIDITY, nextDisplayMode(MODE_TEMPERATURE));
}

static void test_display_navigation_reverse(void) {
    TEST_ASSERT_EQUAL(MODE_TEMPERATURE, previousDisplayMode(MODE_HUMIDITY));
}

static void test_display_navigation_wrap_forward(void) {
    TEST_ASSERT_EQUAL(MODE_TEMPERATURE, nextDisplayMode(MODE_MOTION));
}

static void test_display_navigation_wrap_reverse(void) {
    TEST_ASSERT_EQUAL(MODE_MOTION, previousDisplayMode(MODE_TEMPERATURE));
}


static void test_state_active_no_timeout(void) {
    TEST_ASSERT_EQUAL(
        STATE_ACTIVE,
        evaluateSystemState(STATE_ACTIVE, false, false)
    );
}

static void test_state_active_timeout_to_inactive(void) {
    TEST_ASSERT_EQUAL(
        STATE_INACTIVE,
        evaluateSystemState(STATE_ACTIVE, false, true)
    );
}

static void test_state_inactive_no_motion(void) {
    TEST_ASSERT_EQUAL(
        STATE_INACTIVE,
        evaluateSystemState(STATE_INACTIVE, false, false)
    );
}

static void test_state_inactive_motion_to_active(void) {
    TEST_ASSERT_EQUAL(
        STATE_ACTIVE,
        evaluateSystemState(STATE_INACTIVE, true, false)
    );
}


// Function to run all unit tests
void run_system_unit_tests(void) {
    UNITY_BEGIN();

    RUN_TEST(test_alarm_below_lower_threshold);
    RUN_TEST(test_alarm_exact_lower_threshold);
    RUN_TEST(test_alarm_normal_temperature);
    RUN_TEST(test_alarm_exact_upper_threshold);
    RUN_TEST(test_alarm_above_upper_threshold);

    RUN_TEST(test_display_navigation_forward);
    RUN_TEST(test_display_navigation_reverse);
    RUN_TEST(test_display_navigation_wrap_forward);
    RUN_TEST(test_display_navigation_wrap_reverse);

    RUN_TEST(test_state_active_no_timeout);
    RUN_TEST(test_state_active_timeout_to_inactive);
    RUN_TEST(test_state_inactive_no_motion);
    RUN_TEST(test_state_inactive_motion_to_active);

    UNITY_END();
}


extern "C" void app_main(void) {
    rtos_objects_init();
    sensors_init();

    run_system_unit_tests();

    safe_log("\n--- Starting FreeRTOS Modular Multisensor Room Monitor ---");

    xTaskCreate(
        motion_task,
        "MotionTask",
        3072,
        NULL,
        3,
        NULL
    );

    xTaskCreate(
        input_task,
        "InputTask",
        3072,
        NULL,
        3,
        NULL
    );

    xTaskCreate(
        sensor_task,
        "SensorTask",
        3072,
        NULL,
        2,
        NULL
    );

    xTaskCreate(
        alarm_task,
        "AlarmTask",
        3072,
        NULL,
        2,
        NULL
    );

    xTaskCreate(
        display_task,
        "DisplayTask",
        3072,
        NULL,
        1,
        NULL
    );
}