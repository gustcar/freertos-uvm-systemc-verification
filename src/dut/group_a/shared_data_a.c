#include "shared_data_a.h"
#include "config.h"

// ============================================================
// shared_data_a.c — Definitions (Group A: no protection)
//
// These globals are intentionally unprotected to expose:
//   - Race conditions (read-modify-write tearing)
//   - False sharing (sensor_data_t = 8 bytes, shares cache line)
//   - Priority inversion (no lock ordering)
//   - Deadlock (no locks to deadlock on — but Group B will have)
// ============================================================
 
volatile sensor_data_t     sensor_data;
volatile float             target_temp;
volatile actuator_state_t  actuators;
volatile alarm_level_t     alarm_state;
volatile bool              system_enabled;

void shared_data_reset(void) {
    sensor_data.temperature = 0.0f;
    sensor_data.humidity    = 0.0f;
    target                  = TEMP_TARGET_DEFAULT;
    actuators.pump_on       = false;
    actuators.fan_duty      = 0;
    alarm_state             = ALARM_NORMAL;
    system_enabled          = true;
}
