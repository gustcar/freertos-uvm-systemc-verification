// ============================================================
// sensor_task_safe.c — Sensor task (Group B: protected)
//
// Priority 3 — reads HAL ADC, writes sensor_data.
// Writes protected by mutex_sensor.
// ============================================================


#include <stdint.h>
#include <stdlib.h>
#include "config.h"
#include "types.h"
#include "hal.h"
#include "shared_data_b.h"

void sensor_task_safe(void) {
    for (int i = 0; i < LOOPS_PER_TASK; i++) {
        float temperature_read = hal_adc_read(ADC_CH_TEMP);
        float humidity_read    = hal_adc_read(ADC_CH_HUMIDITY);
        float temp_noise       = ((float)(rand() % 100) / 100.0f) - 0.5f;
        float hum_noise        = ((float)(rand() % 100) / 100.0f) - 0.5f;

        // Protected write
        pthread_mutex_lock(&mutex_sensor);
        sensor_data.temperature = temperature_read + temp_noise;
        sensor_data.humidity    = humidity_read + hum_noise;
        pthread_mutex_unlock(&mutex_sensor);

        if (rand() % 2 == 0) hal_delay_ms(rand() % 5);
    }
}