#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#include <stdbool.h>

typedef enum {
    STATE_INACTIVE = 0,
    STATE_ACTIVE   = 1
} SystemState;

#ifdef __cplusplus
extern "C" {
#endif

SystemState evaluateSystemState(SystemState current, bool motionDetected, bool timeoutOccurred);

#ifdef __cplusplus
}
#endif

#endif 