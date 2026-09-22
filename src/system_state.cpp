#include "system_state.h"

SystemState evaluateSystemState(SystemState current, bool motionDetected, bool timeoutOccurred) {
    switch (current) {
        case STATE_INACTIVE:
            if (motionDetected) {
                return STATE_ACTIVE;
            }
            return STATE_INACTIVE;

        case STATE_ACTIVE:
            if (timeoutOccurred && !motionDetected) {
                return STATE_INACTIVE;
            }
            return STATE_ACTIVE;

        default:
            return STATE_INACTIVE;
    }
}