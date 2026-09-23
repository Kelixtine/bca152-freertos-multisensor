# Functional Verification

The ESP32 Room Multisensor was tested in Wokwi to verify the required sensor, display, input, alarm, and system-state functions. Ten functional tests were performed based on the required functional verification tests.

| Test ID | Function Tested | Test Procedure | Expected Result | Actual Result | Status |
|---|---|---|---|---|---|
| FT-01 | Temperature display | Changed the simulated temperature several times. | OLED displays the current temperature. | Temperature changed from 28.8 °C to 37.3 °C and 33.1 °C. | PASS |
| FT-02 | Humidity display | Changed the simulated humidity values. | OLED displays the current humidity. | Humidity changed from 46.0% to 88.5%, 39.5%, and 0.0%. | PASS |
| FT-03 | Light display | Changed the simulated light level. | OLED displays the current light level. | Light values changed through 92%, 3%, 8%, 30%, 85%, and 98%. | PASS |
| FT-04 | Clockwise encoder | Rotated the encoder clockwise. | Display mode advances to the next mode. | Mode changed from 1 → 2 → 3 → 0. | PASS |
| FT-05 | Counter-clockwise encoder | Rotated the encoder counter-clockwise. | Display mode changes to the previous mode. | Mode changed from 3 → 2 → 1 → 0. | PASS |
| FT-06 | High-temperature alarm | Set the simulated temperature to 34.1 °C. | Alarm activates when the temperature reaches the high threshold. | `[AlarmTask] Activated` was displayed. | PASS |
| FT-07 | Alarm recovery | Returned the simulated temperature to 22.5 °C. | Alarm deactivates when the temperature returns to the normal range. | `[AlarmTask] Deactivated` was displayed. | PASS |
| FT-08 | Motion detection | Activated the simulated PIR sensor. | System detects motion. | `Motion YES` was detected. | PASS |
| FT-09 | Inactivity timeout | Left the PIR inactive for 15 seconds. | System changes to INACTIVE and turns the OLED off. | `Inactivity timeout -> INACTIVE` and `OLED OFF` were observed. | PASS |
| FT-10 | Motion wake-up | Activated the PIR while the system was inactive. | Motion reactivates the system. | `Motion detected -> ACTIVE` followed by `Motion: YES`. | PASS |

## Summary

**10 out of 10 functional tests passed successfully.**

The results confirm that the implemented sensor monitoring, OLED display, rotary encoder navigation, temperature alarm, motion detection, inactivity timeout, and motion-based system reactivation functions operated as expected in the Wokwi simulation.