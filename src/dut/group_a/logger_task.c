// ============================================================
// logger_task.c — Logger task (Group A: vulnerable)
//
// Priority 0 (idle+) — reads all variables for logging.
// NO protection — race conditions expected.
// ============================================================

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "types.h"
#include "hal.h"
#include "shared_data_a.h"


void logger_task(void) {
    log_t log;
    char log_buffer[128];
    int log_len;

    for (int i = 0; i < LOOPS_PER_TASK; i++) {
        log.tick = hal_get_tick_ms();
        // VULNERABLE READ OF ALL VARIABLES
        log.temp = sensor_data.temperature;
        log.humidity = sensor_data.humidity;
        log.pump_on = actuators.pump_on;
        log.fan_duty = actuators.fan_duty;
        log.alarm_level = (uint8_t)alarm_state;

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