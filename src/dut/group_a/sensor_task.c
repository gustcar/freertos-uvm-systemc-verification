// ============================================================
// sensor_task.c — Simulates sensor readings via HAL
// Priority: 3 (high)
// Writes: sensor_data.temperature, sensor_data.humidity
// Reads: —
// Protection: NONE (Grup A — vulnerable)
// ============================================================

#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include "config.h"
#include "types.h"
#include "hal.h"
#include "shared_data_a.h"

void sensor_task(void) {
    for (int i = 0; i < LOOPS_PER_TASK; i++) {
        // Read sensors via HAL
        // Normal mode: fixed values from HAL (no noise)
        // UVM mode: activate below to inject noise via callback
        sensor_data.temperature = hal_adc_read(ADC_CH_TEMP);
        sensor_data.humidity = hal_adc_read(ADC_CH_HUMIDITY);

#if 0   // UVM-SystemC noise injection mode
        float temperature_read = hal_adc_read(ADC_CH_TEMP);
        float humidity_read    = hal_adc_read(ADC_CH_HUMIDITY);

        float temperature_noise = ((float)(rand() % 100) / 100.0f) - 0.5f;
        float humidity_noise    = ((float)(rand() % 100) / 100.0f) - 0.5f;

        sensor_data.temperature = temperature_read + temperature_noise;
        sensor_data.humidity    = humidity_read + humidity_noise;
#endif        

        // Small delay to allow preemption (increase race probability)
        if (rand() % 2 == 0) {
            hal_delay_ms(rand() % 5);
        }
    }
}