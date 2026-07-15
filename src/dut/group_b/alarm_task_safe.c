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

void alarm_task_safe(void) {
    float current_temp;
    float current_humidity;
    alarm_level_t new_alarm_state;
    bool alarm_led_active;

    for (int i = 0; i < LOOPS_PER_TASK; i++) {
        // PROTECTED READ: sensor data
        pthread_mutex_lock(&mutex_sensor);
        current_temp = sensor_data.temperature;
        current_humidity = sensor_data.humidity;
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

        // PROTECTED WRITE: alarm state
        pthread_mutex_lock(&mutex_alarm);
        alarm_state = new_alarm_state;
        pthread_mutex_unlock(&mutex_alarm);

        // GPIO write (not shared — no mutex needed)
        hal_gpio_write(GPIO_PIN_LED_ALARM, alarm_led_active);

        if (rand() % 5 == 0) {
            hal_delay_ms(rand() % 2);
        }
    }
}