#ifndef SHARED_DATA_B_H
#define SHARED_DATA_B_H

#include <pthread.h>
#include "types.h"

// ============================================================
// shared_data_b.h — Global shared variables (Group B: protected)
//
// Same variables as Group A, but protected with mutexes.
// Variables are NOT volatile: mutex provides memory barriers
// that guarantee visibility and atomicity across threads.
// Uses sensor_data_aligned_t to eliminate false sharing.
// ============================================================

// Shared variables (protected by mutexes)
extern sensor_data_aligned_t sensor_data;
extern float                 target_temp;
extern actuator_state_t      actuators;
extern alarm_level_t         alarm_state;
extern bool                  system_enabled;

// Mutexes (one per logical variable group)
extern pthread_mutex_t mutex_sensor;
extern pthread_mutex_t mutex_target_temp;
extern pthread_mutex_t mutex_actuators;
extern pthread_mutex_t mutex_alarm;
extern pthread_mutex_t mutex_system;

// Lifecycle
void shared_data_init_mutexes(void);
void shared_data_destroy_mutexes(void);
void shared_data_reset(void);

#endif