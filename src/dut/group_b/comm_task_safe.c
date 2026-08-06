// ============================================================
// comm_task_safe.c — Communication task (Group B: protected)
// Priority 1 — (low)
// Writes: target_temp, system_enabled
// Reads: sensor_data.temperature, alarm_state
// Protection:
//  - Reads protected by mutex_sensor and mutex_alarm.
//  - Writes protected by mutex_target_temp and mutex_system.
// ============================================================

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
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

void comm_task_safe(void) {
    uint8_t uart_buffer[SERIAL_BUFFER_SIZE];
    command_t command;
    int bytes;
    unsigned int holder_before;
    uint64_t lock_start;
    uint32_t wait_ms;

    for (int i = 0; i < LOOPS_PER_TASK; i++) {
        // Simulate UART command reception
        bytes = hal_uart_rx(uart_buffer, SERIAL_BUFFER_SIZE);

        if(bytes > 0) {
            // Process command if received
            memcpy(&command, uart_buffer, sizeof(command_t));

            switch (command.type) {
                case CMD_TARGET:
                    // priority inversion instrumentation
                    holder_before = hal_mutex_current_holder(MUTEX_ID_TARGET_TEMP);
                    lock_start = hal_get_tick_us();
                    pthread_mutex_lock(&mutex_target_temp);
                    wait_ms = (uint32_t)(hal_get_tick_us() - lock_start);
                    hal_mutex_mark_acquired(MUTEX_ID_TARGET_TEMP, PRIORITY_COMM);
                    if (wait_ms > 0) hal_report_mutex_wait(PRIORITY_COMM, MUTEX_ID_TARGET_TEMP, wait_ms, holder_before);
                    target_temp = command.value;
                    hal_mutex_mark_released(MUTEX_ID_TARGET_TEMP);
                    pthread_mutex_unlock(&mutex_target_temp);
                    break;
                case CMD_ENABLE:
                    holder_before = hal_mutex_current_holder(MUTEX_ID_SYSTEM);
                    lock_start = hal_get_tick_us();
                    pthread_mutex_lock(&mutex_system);
                    wait_ms = (uint32_t)(hal_get_tick_us() - lock_start);
                    hal_mutex_mark_acquired(MUTEX_ID_SYSTEM, PRIORITY_COMM);
                    if (wait_ms > 0) hal_report_mutex_wait(PRIORITY_COMM, MUTEX_ID_SYSTEM, wait_ms, holder_before);
                    system_enabled = true;
                    hal_mutex_mark_released(MUTEX_ID_SYSTEM);
                    pthread_mutex_unlock(&mutex_system);
                    break;
                case CMD_DISABLE:
                    holder_before = hal_mutex_current_holder(MUTEX_ID_SYSTEM);
                    lock_start = hal_get_tick_us();
                    pthread_mutex_lock(&mutex_system);
                    wait_ms = (uint32_t)(hal_get_tick_us() - lock_start);
                    hal_mutex_mark_acquired(MUTEX_ID_SYSTEM, PRIORITY_COMM);
                    if (wait_ms > 0) hal_report_mutex_wait(PRIORITY_COMM, MUTEX_ID_SYSTEM, wait_ms, holder_before);
                    system_enabled = false;
                    hal_mutex_mark_released(MUTEX_ID_SYSTEM);
                    pthread_mutex_unlock(&mutex_system);
                    break;
                case CMD_RESET:
                    holder_before = hal_mutex_current_holder(MUTEX_ID_TARGET_TEMP);
                    lock_start = hal_get_tick_us();
                    pthread_mutex_lock(&mutex_target_temp);
                    wait_ms = (uint32_t)(hal_get_tick_us() - lock_start);
                    hal_mutex_mark_acquired(MUTEX_ID_TARGET_TEMP, PRIORITY_COMM);
                    if (wait_ms > 0) hal_report_mutex_wait(PRIORITY_COMM, MUTEX_ID_TARGET_TEMP, wait_ms, holder_before);

                    target_temp = command.value;

                    hal_mutex_mark_released(MUTEX_ID_TARGET_TEMP);
                    pthread_mutex_unlock(&mutex_target_temp);
                    
                    holder_before = hal_mutex_current_holder(MUTEX_ID_SYSTEM);
                    lock_start = hal_get_tick_us();
                    pthread_mutex_lock(&mutex_system);
                    wait_ms = (uint32_t)(hal_get_tick_us() - lock_start);
                    hal_mutex_mark_acquired(MUTEX_ID_SYSTEM, PRIORITY_COMM);
                    if (wait_ms > 0) hal_report_mutex_wait(PRIORITY_COMM, MUTEX_ID_SYSTEM, wait_ms, holder_before);

                    system_enabled = true;

                    hal_mutex_mark_released(MUTEX_ID_SYSTEM);
                    pthread_mutex_unlock(&mutex_system);
                    break;
                default:
                    break;
            }
        }

        if (rand() % 4 == 0) hal_delay_ms(rand() % 3);
    }
}