#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#include <stdint.h>

enum class SystemState {
    ACTIVE,
    INACTIVE
};

enum class DisplayMode {
    TEMPERATURE,
    HUMIDITY,
    LIGHT,
    MOTION
};

extern volatile SystemState g_systemState;

DisplayMode getNextDisplayMode(DisplayMode current);
DisplayMode getPreviousDisplayMode(DisplayMode current);
SystemState evaluateSystemState(SystemState currentState, bool motionDetected, uint32_t elapsedTimeMs, uint32_t timeoutMs);

#endif