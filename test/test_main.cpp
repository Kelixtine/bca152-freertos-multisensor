#include <unity.h>

#include "alarm.h"
#include "input.h"
#include "system_state.h"

// --- Temperature alarm logic (5 required: below, exact-lower, normal, exact-upper, above) ---
void test_temp_below_lower(void) { TEST_ASSERT_EQUAL(ALARM_LOW_TEMPERATURE, evaluateTemperature(15.0f)); }
void test_temp_exact_lower(void) { TEST_ASSERT_EQUAL(ALARM_NORMAL, evaluateTemperature(18.0f)); }
void test_temp_normal(void)      { TEST_ASSERT_EQUAL(ALARM_NORMAL, evaluateTemperature(24.5f)); }
void test_temp_exact_upper(void) { TEST_ASSERT_EQUAL(ALARM_NORMAL, evaluateTemperature(30.0f)); }
void test_temp_above_upper(void) { TEST_ASSERT_EQUAL(ALARM_HIGH_TEMPERATURE, evaluateTemperature(35.5f)); }

// --- Display navigation (4 required: forward, reverse, wrap forward, wrap reverse) ---
void test_disp_forward(void)  { TEST_ASSERT_EQUAL(MODE_HUMIDITY, nextDisplayMode(MODE_TEMPERATURE)); }
void test_disp_reverse(void)  { TEST_ASSERT_EQUAL(MODE_TEMPERATURE, previousDisplayMode(MODE_HUMIDITY)); }
void test_disp_wrap_fwd(void) { TEST_ASSERT_EQUAL(MODE_TEMPERATURE, nextDisplayMode(MODE_MOTION)); }
void test_disp_wrap_rev(void) { TEST_ASSERT_EQUAL(MODE_MOTION, previousDisplayMode(MODE_TEMPERATURE)); }

// --- System state machine (4 required: active/no-timeout, active/timeout, inactive/no-motion, inactive/motion) ---
void test_st_active_no_timeout(void)  { TEST_ASSERT_EQUAL(STATE_ACTIVE, evaluateSystemState(STATE_ACTIVE, false, false)); }
void test_st_active_timeout(void)     { TEST_ASSERT_EQUAL(STATE_INACTIVE, evaluateSystemState(STATE_ACTIVE, false, true)); }
void test_st_inactive_no_motion(void) { TEST_ASSERT_EQUAL(STATE_INACTIVE, evaluateSystemState(STATE_INACTIVE, false, false)); }
void test_st_inactive_motion(void)    { TEST_ASSERT_EQUAL(STATE_ACTIVE, evaluateSystemState(STATE_INACTIVE, true, false)); }

void setUp(void) {}
void tearDown(void) {}

extern "C" void app_main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_temp_below_lower);
    RUN_TEST(test_temp_exact_lower);
    RUN_TEST(test_temp_normal);
    RUN_TEST(test_temp_exact_upper);
    RUN_TEST(test_temp_above_upper);

    RUN_TEST(test_disp_forward);
    RUN_TEST(test_disp_reverse);
    RUN_TEST(test_disp_wrap_fwd);
    RUN_TEST(test_disp_wrap_rev);

    RUN_TEST(test_st_active_no_timeout);
    RUN_TEST(test_st_active_timeout);
    RUN_TEST(test_st_inactive_no_motion);
    RUN_TEST(test_st_inactive_motion);

    UNITY_END();
}