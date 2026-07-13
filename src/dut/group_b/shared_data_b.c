#include "shared_data_b.h"
#include "config.h"

// ============================================================
// shared_data_b.c — Definitions (Group B: mutex-protected)
//
// Same globals as Group A, but each access must be guarded
// by the corresponding mutex. Variables are NOT volatile
// because pthread_mutex_lock/unlock provide memory barriers.
// Uses sensor_data_aligned_t (64 bytes) to prevent false sharing.
// ============================================================

sensor_data_aligned_t  sensor_data;
float                  target_temp;
actuator_state_t       actuators;
alarm_level_t          alarm_state;
bool                   system_enabled;

pthread_mutex_t mutex_sensor;
pthread_mutex_t mutex_target_temp;
pthread_mutex_t mutex_actuators;
pthread_mutex_t mutex_alarm;
pthread_mutex_t mutex_system;

// Initialize all mutexes
void shared_data_init_mutexes(void) {
    pthread_mutex_init(&mutex_sensor,       NULL);
    pthread_mutex_init(&mutex_target_temp,  NULL);
    pthread_mutex_init(&mutex_actuators,    NULL);
    pthread_mutex_init(&mutex_alarm,        NULL);
    pthread_mutex_init(&mutex_system,       NULL);
}

// Destroy all mutexes
void shared_data_destroy_mutexes(void) {
    pthread_mutex_destroy(&mutex_sensor);
    pthread_mutex_destroy(&mutex_target_temp);
    pthread_mutex_destroy(&mutex_actuators);
    pthread_mutex_destroy(&mutex_alarm);
    pthread_mutex_destroy(&mutex_system);
}

// Reset all shared data to defaults (thread-safe)
void shared_data_reset(void) {
    pthread_mutex_lock(&mutex_sensor);
    sensor_data.temperature = 0.0f;
    sensor_data.humidity    = 0.0f;
    pthread_mutex_unlock(&mutex_sensor);

    pthread_mutex_lock(&mutex_target_temp);
    target_temp = TEMP_TARGET_DEFAULT;
    pthread_mutex_unlock(&mutex_target_temp);

    pthread_mutex_lock(&mutex_actuators);
    actuators.pump_on  = false;
    actuators.fan_duty = 0;
    pthread_mutex_unlock(&mutex_actuators);

    pthread_mutex_lock(&mutex_alarm);
    alarm_state = ALARM_NORMAL;
    pthread_mutex_unlock(&mutex_alarm);

    pthread_mutex_lock(&mutex_system);
    system_enabled = true;
    pthread_mutex_unlock(&mutex_system);
}