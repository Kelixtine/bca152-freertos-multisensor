# Fault Experiments

Three controlled fault experiments were performed to observe the effects of changes to the FreeRTOS system. After each experiment, the original implementation was restored and the project was verified to be working correctly.

## Fault Experiment 1 — Removing the MotionTask Delay

### Change Made

The periodic delay in `MotionTask` was temporarily removed.

### Observation

The Wokwi simulation produced abnormal behavior. The task executed continuously, producing excessive task activity and eventually causing the simulation session to terminate.

### Cause

Without the periodic delay, `MotionTask` continuously consumed CPU time instead of yielding at the intended interval. This affected task scheduling and the overall simulation behavior.

### Restoration

The task delay was restored to the original implementation.

### Result

The project was rebuilt and the normal Wokwi behavior returned successfully.

---

## Fault Experiment 2 — Changing DisplayTask Priority

### Change Made

The priority of `DisplayTask` was temporarily changed from priority `1` to priority `4`.

### Observation

The project continued to build successfully, and the Wokwi simulation remained functional during the test.

### Purpose

This experiment demonstrated the effect of changing a FreeRTOS task priority and allowed the scheduling behavior of the system to be observed.

### Restoration

The `DisplayTask` priority was restored from `4` back to `1`.

### Result

The original working configuration was restored successfully.

---

## Fault Experiment 3 — Removing the Serial Mutex

### Change Made

The `xSemaphoreTake()` and `xSemaphoreGive()` protection in `safe_log()` was temporarily removed.

### Observation

The system continued operating during the short Wokwi test. The serial output remained readable, and the embedded unit tests continued to pass.

### Cause / Potential Effect

Without the mutex, concurrent tasks no longer have synchronization protection when accessing the shared serial output. Under different task scheduling conditions, messages from different tasks could interfere with each other.

### Restoration

The mutex protection was restored in `safe_log()`.

### Result

The original synchronized logging implementation was restored and the project continued to operate normally.

---

## Overall Fault-Experiment Result

The three experiments demonstrated the importance of task timing, task priority, and synchronization in the FreeRTOS-based system. Each temporary modification was reverted after observation, leaving the final project in its verified working configuration.