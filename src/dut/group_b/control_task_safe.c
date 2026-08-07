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

// Mutex indices — must match hal.c's HAL_MUTEX_COUNT ordering
#define MUTEX_ID_SENSOR       0
#define MUTEX_ID_TARGET_TEMP  1
#define MUTEX_ID_ACTUATORS    2
#define MUTEX_ID_ALARM        3
#define MUTEX_ID_SYSTEM       4

void control_task_safe(void) {
    float current_temp;
    float current_humidity;
    float target_temperature;
    float error;
    uint8_t calculated_fan_duty;
    bool should_pump_on;
    unsigned int holder_before;
    uint64_t lock_start;
    uint32_t wait_ms;

    for (int i = 0; i < LOOPS_PER_TASK; i++) {
        
        // NEW: priority inversion instrumentation
        holder_before = hal_mutex_current_holder(MUTEX_ID_SENSOR);
        lock_start = hal_get_tick_us();

        pthread_mutex_lock(&mutex_sensor);
        wait_ms = (uint32_t)(hal_get_tick_us() - lock_start);
        hal_mutex_mark_acquired(MUTEX_ID_SENSOR, PRIORITY_CONTROL);
        if (wait_ms > 0) hal_report_mutex_wait(PRIORITY_CONTROL, MUTEX_ID_SENSOR, wait_ms, holder_before);

        // Protected read: sensor data
        current_temp       = sensor_data.temperature;
        current_humidity   = sensor_data.humidity;

        hal_mutex_mark_released(MUTEX_ID_SENSOR);
        pthread_mutex_unlock(&mutex_sensor);

        holder_before = hal_mutex_current_holder(MUTEX_ID_TARGET_TEMP);
        lock_start = hal_get_tick_us();
        pthread_mutex_lock(&mutex_target_temp);
        wait_ms = (uint32_t)(hal_get_tick_us() - lock_start);
        hal_mutex_mark_acquired(MUTEX_ID_TARGET_TEMP, PRIORITY_CONTROL);
        if (wait_ms > 0) hal_report_mutex_wait(PRIORITY_CONTROL, MUTEX_ID_TARGET_TEMP, wait_ms, holder_before);
        // Protected read: setpoint
        target_temperature = target_temp;

        hal_mutex_mark_released(MUTEX_ID_TARGET_TEMP);
        pthread_mutex_unlock(&mutex_target_temp);

        // Report the exact triple this iteration consumed. Because the reads
        // above were mutex-protected, (temperature, humidity) always come from
        // a single coherent sensor reading — the testbench should see no
        // incoherent triples here (contrast with Group A).
        hal_report_control_inputs(current_temp, current_humidity, target_temperature);

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

        // priority inversion instrumentation
        holder_before = hal_mutex_current_holder(MUTEX_ID_ACTUATORS);
        lock_start = hal_get_tick_us();
        pthread_mutex_lock(&mutex_actuators);
        wait_ms = (uint32_t)(hal_get_tick_us() - lock_start);
        hal_mutex_mark_acquired(MUTEX_ID_ACTUATORS, PRIORITY_CONTROL);
        if (wait_ms > 0) hal_report_mutex_wait(PRIORITY_CONTROL, MUTEX_ID_ACTUATORS, wait_ms, holder_before);
        
        // Protected write: actuators
        actuators.pump_on  = should_pump_on;
        actuators.fan_duty = calculated_fan_duty;
        hal_mutex_mark_released(MUTEX_ID_ACTUATORS);
        pthread_mutex_unlock(&mutex_actuators);

        // Expose actuator state via HAL so the testbench can observe it
        // (mirrors control_task.c in Group A; kept outside the mutex since
        // these are already-computed local values, not shared state)
        hal_pwm_set(PWM_CH_FAN, calculated_fan_duty);
        hal_gpio_write(GPIO_PIN_PUMP, should_pump_on);

        if (rand() % 3 == 0) {
            hal_delay_ms(rand() % 2);
        }
    }
}