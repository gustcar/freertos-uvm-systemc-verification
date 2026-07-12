#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "types.h"
#include "hal.h"
#include "shared_data_a.h"

/**
 * @brief Logger Task (Priority 0 - Idle+)
 * 
 * Responsibilities:
 * 1. Periodically reads ALL shared variables to create consistency snapshot
 * 2. Writes log entry to persistent storage via HAL flash
 * 3. Reads without any synchronization (maximum vulnerability exposure)
 * 
 * Note: Lowest priority. Runs during idle times, maximizing chance to catch 
 *       inconsistent states created by concurrent writes from other tasks.
 */
void logger_task(void) {
    uint32_t tick_count;
    log_t log_entry;
    char log_buffer[64];
    int log_len;

    for (int i = 0; i < LOOPS_PER_TASK; i++) {
        // VULNERABLE READ OF ALL VARIABLES
        // This is a consistency checker: if race conditions exist,
        // the snapshot may capture impossible states (e.g., temp updated but humidity old)

        tick_count = hal_get_tick_ms();
        log_entry.tick = tick_count;
        
        // Read all shared data (no lock!)
        log_entry.temp = sensor_data.temperature;
        log_entry.humidity = sensor_data.humidity;
        log_entry.pump_on = actuators.pump_on;
        log_entry.fan_duty = actuators.fan_duty;
        log_entry.alarm_level = alarm_state;

        // Format log entry as ASCII (for visibility)
        log_len = snprintf(
            log_buffer, sizeof(log_buffer),
            "[LOG] tick=%u temp=%.1f hum=%.1f pump=%d fan=%u alarm=%d\n",
            (unsigned)log_entry.tick,
            log_entry.temp,
            log_entry.humidity,
            log_entry.pump_on ? 1 : 0,
            log_entry.fan_duty,
            log_entry.alarm_level
        );

        // Write to flash (vulnerable write path)
        hal_log_write(log_buffer, (uint16_t)log_len);

        // Infrequent logging (every ~50 iterations to reduce I/O overhead)
        if (rand() % 50 == 0) {
            hal_delay_ms(1);  // Minimal delay
        }
    }
}