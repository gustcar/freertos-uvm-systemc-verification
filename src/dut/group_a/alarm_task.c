#include <stdint.h>
#include "config.h"
#include "types.h"
#include "hal.h"
#include "shared_data_a.h"

void alarm_task(void) {
    float current_temp;
    float current_humidity;
    alarm_level_t new_alarm_state;
    bool alarm_led_active;

    for (int i = 0; i < LOOPS_PER_TASK; i++) {
        // VULNERABLE READ
        current_temp = sensor_data.temperature;
        current_humidity = sensor_data.humidity;

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

        // VULNERABLE WRITE
        // comm_task may be reading alarm_state simultaneously
        alarm_state = new_alarm_state;

        // Trigger alarm LED (also vulnerable if another task writes GPIO concurrently)
        hal_gpio_write(GPIO_PIN_LED_ALARM, alarm_led_active);

        // Short delay
        if (rand() % 5 == 0) {
            hal_delay_ms(rand() % 2);
        }
    }
}

