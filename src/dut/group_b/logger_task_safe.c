// ============================================================
// logger_task_safe.c — Logger task (Group B: protected)
//
// Priority 0 (idle) — reads all variables for logging.
// Each variable read is protected by its corresponding mutex.
// ============================================================
 
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "types.h"
#include "hal.h"
#include "shared_data_b.h"

void logger_task_safe(void) {
    log_t log;
    char log_buffer[128];
    int log_len;

    for (int i = 0; i < LOOPS_PER_TASK; i++) {
        log.tick = hal_get_tick_ms();

        // Protected read: sensor data
        pthread_mutex_lock(&mutex_sensor);
        log.temp     = sensor_data.temperature;
        log.humidity = sensor_data.humidity;
        pthread_mutex_unlock(&mutex_sensor);

        // Protected read: actuators
        pthread_mutex_lock(&mutex_actuators);
        log.pump_on  = actuators.pump_on;
        log.fan_duty = actuators.fan_duty;
        pthread_mutex_unlock(&mutex_actuators);

        // Protected read: alarm state
        pthread_mutex_lock(&mutex_alarm);
        log.alarm_level = (uint8_t)alarm_state;
        pthread_mutex_unlock(&mutex_alarm);

        // Reduce log printing to every 100 iterations to avoid flooding
        if (i % 100 == 0) {
            log_len = snprintf(
                log_buffer,
                sizeof(log_buffer),
                "[LOG] tick=%u T=%.1f H=%.1f pump=%d fan=%d alarm=%d\n",
                log.tick,
                log.temp,
                log.humidity,
                log.pump_on ? 1 : 0,
                log.fan_duty,
                log.alarm_level
            );
            hal_log_write(log_buffer, (uint16_t)log_len);
        }

        if (rand() % 5 == 0) hal_delay_ms(rand() % 4);
    }
}