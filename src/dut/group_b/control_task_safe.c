// ============================================================
// control_task_safe.c — Control task (Group B: protected)
//
// Priority 2 — reads temp/humidity/target_temp, writes pump_on/fan_duty.
// Reads protected by mutex_sensor and mutex_target_temp.
// Writes protected by mutex_actuators.
// ============================================================

#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include "config.h"
#include "types.h"
#include "hal.h"
#include "shared_data_b.h"

void control_task_safe(void) {
    float current_temp;
    float current_humidity;
    float target_temperature;
    float error;
    uint8_t calculated_fan_duty;
    bool should_pump_on;

    for (int i = 0; i < LOOPS_PER_TASK; i++) {
        // Protected read: sensor data
        pthread_mutex_lock(&mutex_sensor);
        current_temp       = sensor_data.temperature;
        current_humidity   = sensor_data.humidity;
        pthread_mutex_unlock(&mutex_sensor);

        // Protected read: setpoint
        pthread_mutex_lock(&mutex_target_temp);
        target_temperature = target_temp;
        pthread_mutex_unlock(&mutex_target_temp);

        // Control logic
        error = current_temp - target_temperature;

        if (error > FAN_TEMP_TOLERANCE) {
            calculated_fan_duty = FAN_SPEED_MAX;
        } else if (error < -FAN_TEMP_TOLERANCE) {
            calculated_fan_duty = FAN_SPEED_MIN;
        } else {
            calculated_fan_duty = (uint8_t)(fabsf(error) * 5.0f);
            if (calculated_fan_duty > FAN_SPEED_MAX) calculated_fan_duty = FAN_SPEED_MAX;
        }

        should_pump_on = (current_humidity < HUMIDITY_MIN_LIMIT);

        /* Protected write: actuators */
        pthread_mutex_lock(&mutex_actuators);
        actuators.pump_on  = should_pump_on;
        actuators.fan_duty = calculated_fan_duty;
        pthread_mutex_unlock(&mutex_actuators);

        if (rand() % 3 == 0) {
            hal_delay_ms(rand() % 2);
        }
    }
}