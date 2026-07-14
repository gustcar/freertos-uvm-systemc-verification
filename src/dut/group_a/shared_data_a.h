#ifndef SHARED_DATA_A_H
#define SHARED_DATA_A_H

#include "types.h"

// ============================================================
// shared_data_a.h — Global shared variables (Group A: vulnerable)
//
// No mutex, no semaphore, no atomic operations.
// All variables are volatile to prevent compiler optimization,
// but this does NOT guarantee atomicity or visibility across
// tasks. Race conditions are expected.
// ============================================================

// Written by sensor_task, read by all others
extern volatile sensor_data_t sensor_data;

// Written by comm_task, read by control_task
extern volatile float target_temp;

// Written by control_task
extern volatile actuator_state_t actuators;

// Written by alarm_task
extern volatile alarm_level_t alarm_state;

// Accessed by all tasks
extern volatile bool system_enabled;

// Reset all shared data to defaults
void shared_data_reset(void);

#endif