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

// Mutex indices — must match hal.c's HAL_MUTEX_COUNT ordering
#define MUTEX_ID_SENSOR       0
#define MUTEX_ID_TARGET_TEMP  1
#define MUTEX_ID_ACTUATORS    2
#define MUTEX_ID_ALARM        3
#define MUTEX_ID_SYSTEM       4

void logger_task_safe(void) {
    log_t log;
    char log_buffer[128];
    int log_len;
    unsigned int holder_before;
    uint32_t lock_start, wait_ms;

    for (int i = 0; i < LOOPS_PER_TASK; i++) {
        log.tick = hal_get_tick_ms();

        // NEW: priority inversion instrumentation
        holder_before = hal_mutex_current_holder(MUTEX_ID_SENSOR);
        lock_start = hal_get_tick_ms();
        pthread_mutex_lock(&mutex_sensor);
        wait_ms = hal_get_tick_ms() - lock_start;
        hal_mutex_mark_acquired(MUTEX_ID_SENSOR, PRIORITY_LOGGER);
        /*if (wait_ms >= 0)*/ hal_report_mutex_wait(PRIORITY_LOGGER, MUTEX_ID_SENSOR, wait_ms, holder_before);
        // Protected read: sensor data
        log.temp     = sensor_data.temperature;
        log.humidity = sensor_data.humidity;
        hal_mutex_mark_released(MUTEX_ID_SENSOR);
        pthread_mutex_unlock(&mutex_sensor);

        // priority inversion instrumentation
        holder_before = hal_mutex_current_holder(MUTEX_ID_ACTUATORS);
        lock_start = hal_get_tick_ms();
        pthread_mutex_lock(&mutex_actuators);
        wait_ms = hal_get_tick_ms() - lock_start;
        hal_mutex_mark_acquired(MUTEX_ID_ACTUATORS, PRIORITY_LOGGER);
        /*if (wait_ms >= 0)*/ hal_report_mutex_wait(PRIORITY_LOGGER, MUTEX_ID_ACTUATORS, wait_ms, holder_before);
        // Protected read: actuators
        log.pump_on  = actuators.pump_on;
        log.fan_duty = actuators.fan_duty;
        hal_mutex_mark_released(MUTEX_ID_ACTUATORS);
        pthread_mutex_unlock(&mutex_actuators);

        // priority inversion instrumentation
        holder_before = hal_mutex_current_holder(MUTEX_ID_ALARM);
        lock_start = hal_get_tick_ms();
        pthread_mutex_lock(&mutex_alarm);
        wait_ms = hal_get_tick_ms() - lock_start;
        hal_mutex_mark_acquired(MUTEX_ID_ALARM, PRIORITY_LOGGER);
        /*if (wait_ms >= 0)*/ hal_report_mutex_wait(PRIORITY_LOGGER, MUTEX_ID_ALARM, wait_ms, holder_before);
        // Protected read: alarm state
        log.alarm_level = (uint8_t)alarm_state;
        hal_mutex_mark_released(MUTEX_ID_ALARM);
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