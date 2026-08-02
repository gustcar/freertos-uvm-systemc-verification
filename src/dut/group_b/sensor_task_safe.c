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

// Mutex indices — must match hal.c's HAL_MUTEX_COUNT ordering
#define MUTEX_ID_SENSOR       0
#define MUTEX_ID_TARGET_TEMP  1
#define MUTEX_ID_ACTUATORS    2
#define MUTEX_ID_ALARM        3
#define MUTEX_ID_SYSTEM       4

void sensor_task_safe(void) {
    unsigned int holder_before;
    uint32_t lock_start, wait_ms;

    for (int i = 0; i < LOOPS_PER_TASK; i++) {
        // priority inversion instrumentation
        holder_before = hal_mutex_current_holder(MUTEX_ID_SENSOR);
        lock_start = hal_get_tick_ms();
        pthread_mutex_lock(&mutex_sensor);
        wait_ms = hal_get_tick_ms() - lock_start;
        hal_mutex_mark_acquired(MUTEX_ID_SENSOR, PRIORITY_SENSOR);
        /*if (wait_ms >= 0)*/ hal_report_mutex_wait(PRIORITY_SENSOR, MUTEX_ID_SENSOR, wait_ms, holder_before);
        
        sensor_data.temperature = hal_adc_read(ADC_CH_TEMP);
        sensor_data.humidity = hal_adc_read(ADC_CH_HUMIDITY);

        hal_mutex_mark_released(MUTEX_ID_SENSOR);
        pthread_mutex_unlock(&mutex_sensor);

#if 0  // UVM-SystemC noise injection mode
       float temperature_read = hal_adc_read(ADC_CH_TEMP);
       float humidity_read    = hal_adc_read(ADC_CH_HUMIDITY);

       float temp_noise       = ((float)(rand() % 100) / 100.0f) - 0.5f;
       float hum_noise        = ((float)(rand() % 100) / 100.0f) - 0.5f;

       // PROTECTED WRITE — mutex ensures atomic access
       pthread_mutex_lock(&mutex_sensor);
       sensor_data.temperature = temperature_read + temp_noise;
       sensor_data.humidity    = humidity_read + hum_noise;  
       pthread_mutex_unlock(&mutex_sensor);
#endif
        if (rand() % 2 == 0) hal_delay_ms(rand() % 5);
    }
}