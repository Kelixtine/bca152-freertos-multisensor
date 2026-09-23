\# BCA152 FreeRTOS Multisensor Room Monitor



An ESP32-based room monitoring system developed for BCA152 Microcontrollers. The project uses \*\*ESP-IDF\*\*, \*\*FreeRTOS\*\*, and \*\*Wokwi\*\* to demonstrate multitasking, inter-task communication, synchronization, sensor monitoring, display control, alarm handling, and an ACTIVE/INACTIVE system state.



\## Features



\- Temperature monitoring

\- Humidity monitoring

\- Light-level monitoring

\- PIR motion detection

\- OLED display

\- Rotary encoder navigation

\- High- and low-temperature alarm

\- ACTIVE/INACTIVE system state

\- Automatic inactivity timeout

\- Motion-based system wake-up

\- FreeRTOS task-based architecture

\- Queue-based sensor communication

\- Mutex-protected serial logging

\- Event-group system-state signaling

\- Periodic task scheduling using `vTaskDelayUntil()`

\- Hardware-independent unit testing

\- Wokwi functional verification



\## Hardware



\- ESP32 DevKit

\- SSD1306 128x64 I2C OLED

\- PIR motion sensor

\- Rotary encoder

\- Passive buzzer

\- Temperature/humidity sensor simulation

\- Light sensor simulation



\## Software



\- PlatformIO

\- ESP-IDF

\- FreeRTOS

\- C/C++

\- Unity testing framework

\- Wokwi

\- Cppcheck



Arduino is not used. The project uses the ESP-IDF framework and native FreeRTOS APIs.



\## FreeRTOS Tasks



The system is divided into multiple tasks:



| Task | Purpose | Priority |

|---|---|---:|

| MotionTask | PIR monitoring and ACTIVE/INACTIVE state | 3 |

| InputTask | Rotary encoder input and display navigation | 3 |

| SensorTask | Sensor data acquisition | 2 |

| AlarmTask | Temperature alarm handling | 2 |

| DisplayTask | OLED display updates | 1 |



The project uses:



\- `sensorQueue` for sensor data communication

\- `modeQueue` for display-mode communication

\- `serialMutex` for protected serial logging

\- `systemEvents` for system-state and motion events

\- `vTaskDelayUntil()` for periodic task timing



\## Display Modes



The rotary encoder cycles through four display modes:



1\. Temperature

2\. Humidity

3\. Light

4\. Motion



Clockwise rotation moves to the next mode, while counter-clockwise rotation moves to the previous mode.



\## Temperature Alarm



The temperature alarm uses the following thresholds:



| Temperature | Alarm State |

|---|---|

| ≤ 18 °C | Low-temperature alarm |

| 18–30 °C | Normal |

| ≥ 30 °C | High-temperature alarm |



\## System State



The room monitor uses two system states:



\- \*\*ACTIVE\*\* — the OLED and monitoring functions remain active.

\- \*\*INACTIVE\*\* — entered after 15 seconds without motion.



Motion detected while inactive returns the system to ACTIVE.



\## Unit Testing



Hardware-independent logic is tested using the Unity framework.



The project contains tests for:



\- Temperature alarm evaluation

\- Display-mode navigation

\- Display-mode wraparound

\- ACTIVE/INACTIVE state transitions



A total of \*\*14 native unit tests\*\* pass successfully.



The embedded Wokwi build also executes the required system tests successfully.



\## Functional Verification



Ten required functional tests were performed in Wokwi:



\- FT-01 — Temperature display

\- FT-02 — Humidity display

\- FT-03 — Light display

\- FT-04 — Clockwise encoder

\- FT-05 — Counter-clockwise encoder

\- FT-06 — High-temperature alarm

\- FT-07 — Alarm recovery

\- FT-08 — Motion detection

\- FT-09 — Inactivity timeout

\- FT-10 — Motion wake-up



\*\*Result: 10/10 functional tests passed.\*\*



Detailed results are available in:



`docs/functional-verification.md`



\## Fault Experiments



Three controlled FreeRTOS fault experiments were performed:



1\. Removing the MotionTask periodic delay

2\. Changing DisplayTask priority

3\. Removing serial mutex protection



Each experiment was restored to the original working implementation afterward.



Detailed observations are available in:



`docs/fault-experiments.md`



\## Static Analysis



Cppcheck is configured through PlatformIO.



The current analysis completed successfully. One low-severity portability finding related to `memset()` and a structure containing floating-point data remains documented for review.



\## Building the Project



Build the ESP32 project with:



```powershell

pio run

