#include "system_state.h"

volatile SystemState g_systemState = SystemState::ACTIVE;

DisplayMode getNextDisplayMode(DisplayMode current) {
    switch (current) {
        case DisplayMode::TEMPERATURE: return DisplayMode::HUMIDITY;
        case DisplayMode::HUMIDITY:    return DisplayMode::LIGHT;
        case DisplayMode::LIGHT:       return DisplayMode::MOTION;
        case DisplayMode::MOTION:      return DisplayMode::TEMPERATURE;
    }
    return DisplayMode::TEMPERATURE;
}

DisplayMode getPreviousDisplayMode(DisplayMode current) {
    switch (current) {
        case DisplayMode::TEMPERATURE: return DisplayMode::MOTION;
        case DisplayMode::HUMIDITY:    return DisplayMode::TEMPERATURE;
        case DisplayMode::LIGHT:       return DisplayMode::HUMIDITY;
        case DisplayMode::MOTION:      return DisplayMode::LIGHT;
    }
    return DisplayMode::TEMPERATURE;
}

SystemState evaluateSystemState(SystemState currentState, bool motionDetected, uint32_t elapsedTimeMs, uint32_t timeoutMs) {
    if (motionDetected) {
        return SystemState::ACTIVE;
    }
    if (currentState == SystemState::ACTIVE && elapsedTimeMs >= timeoutMs) {
        return SystemState::INACTIVE;
    }
    return currentState;
}