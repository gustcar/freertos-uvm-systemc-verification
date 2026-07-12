// ============================================================
// control_task.c — Climate Control Task
// Priority: 2 (medium)
// Writes: actuators.pump_on, actuators.fan_duty
// Reads: sensor_data.temperature, sensor_data.humidity, target
// Protection: NONE (Group A — vulnerable)
// ============================================================
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include "config.h"
#include "types.h"
#include "hal.h"
#include "shared_data_a.h"

void control_task(void) {
    float current_temperature;
    float current_humidity;
    float target_setpoint;
    float error;
    uint8_t calculated_fan_duty;
    bool should_pump_on;

    for (int i = 0; i < LOOPS_PER_TASK; i++) {
        // unprotected read (vulnerable)
        // if sensor_task is writing simultaneously, partial or stale values can be fetched
        current_temperature = sensor_data.temperature;
        current_humidity = sensor_data.humidity;
        
        // Can be concurrently modified by comm_task
        target_setpoint = target;

        // Control logic
        error = current_temperature - target_setpoint;

        // Fan control
        if (error > FAN_TEMP_TOLERANCE) {
            // Temperature too high -> Max Fan
            calculated_fan_duty = FAN_SPEED_MAX;
        } else if (error < -FAN_TEMP_TOLERANCE) {
            // Temperature too low -> Min Fan
            calculated_fan_duty = FAN_SPEED_MIN;
        } else {
            // Applies simple proportional control based on error
            calculated_fan_duty = (uint8_t)(fabsf(error) * 5.0f); 
            if (calculated_fan_duty > FAN_SPEED_MAX) calculated_fan_duty = FAN_SPEED_MAX;
        }

        // Pump control - low humidity turns on the pump
        should_pump_on = (current_humidity < HUMIDITY_MIN_LIMIT);

        // unprotected write (vulnerable)
        // if another task reads actuators during this write, it may see an inconsistent state
        actuators.pump_on = should_pump_on;
        actuators.fan_duty = calculated_fan_duty;

        // Small random delay to increase the chance of thread interleaving
        if (rand() % 3 == 0) {
            hal_delay_ms(rand() % 2);
        }
    }
}