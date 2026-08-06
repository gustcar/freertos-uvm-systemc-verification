// ============================================================
// alarm_task_safe.c — Alarm task (Group B: protected)
//
// Priority 4 (highest) — reads temp/humidity, writes alarm_state.
// Reads protected by mutex_sensor.
// Writes protected by mutex_alarm.
// ============================================================

#include <stdint.h>
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

void alarm_task_safe(void) {
    float current_temp;
    float current_humidity;
    alarm_level_t new_alarm_state;
    bool alarm_led_active;
    unsigned int holder_before;
    uint64_t lock_start;
    uint32_t wait_ms;

    for (int i = 0; i < LOOPS_PER_TASK; i++) {
        // PROTECTED READ: sensor data
        // priority inversion instrumentation
        holder_before = hal_mutex_current_holder(MUTEX_ID_SENSOR);
        lock_start = hal_get_tick_us();

        pthread_mutex_lock(&mutex_sensor);
        wait_ms = (uint32_t)(hal_get_tick_us() - lock_start);
        hal_mutex_mark_acquired(MUTEX_ID_SENSOR, PRIORITY_ALARM);
        if (wait_ms > 0) hal_report_mutex_wait(PRIORITY_ALARM, MUTEX_ID_SENSOR, wait_ms, holder_before);

        current_temp = sensor_data.temperature;
        current_humidity = sensor_data.humidity;
        
        hal_mutex_mark_released(MUTEX_ID_SENSOR);
        pthread_mutex_unlock(&mutex_sensor);

        // Evaluate alarm conditions (priority logic)
        new_alarm_state = ALARM_NORMAL;
        alarm_led_active = false;

        if (current_temp >= TEMP_CRITICAL_LIMIT) {
            new_alarm_state = ALARM_CRITICAL;
            alarm_led_active = true;
        } else if (
            current_temp >= TEMP_ALARM_LIMIT ||
            current_humidity >= HUMIDITY_MAX_LIMIT ||
            current_humidity <= HUMIDITY_MIN_LIMIT
          ) {
            new_alarm_state = ALARM_WARNING;
            alarm_led_active = true;
        }
        
        holder_before = hal_mutex_current_holder(MUTEX_ID_ALARM);
        lock_start = hal_get_tick_us();

        pthread_mutex_lock(&mutex_alarm);

        wait_ms = (uint32_t)(hal_get_tick_us() - lock_start);
        hal_mutex_mark_acquired(MUTEX_ID_ALARM, PRIORITY_ALARM);
        if (wait_ms > 0) hal_report_mutex_wait(PRIORITY_ALARM, MUTEX_ID_ALARM, wait_ms, holder_before);
        
        // PROTECTED WRITE: alarm state
        alarm_state = new_alarm_state;
        
        hal_mutex_mark_released(MUTEX_ID_ALARM);
        
        pthread_mutex_unlock(&mutex_alarm);

        // GPIO write (not shared — no mutex needed)
        hal_gpio_write(GPIO_PIN_LED_ALARM, alarm_led_active);

        if (rand() % 5 == 0) {
            hal_delay_ms(rand() % 2);
        }
    }
}