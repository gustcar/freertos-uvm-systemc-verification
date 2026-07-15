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

void comm_task_safe(void) {
    uint8_t uart_buffer[SERIAL_BUFFER_SIZE];
    command_t command;
    int bytes;

    for (int i = 0; i < LOOPS_PER_TASK; i++) {
        // Simulate UART command reception
        bytes = hal_uart_rx(uart_buffer, SERIAL_BUFFER_SIZE);

        if(bytes > 0) {
            // Process command if received
            memcpy(&command, uart_buffer, sizeof(command_t));

            switch (command.type) {
                case CMD_TARGET:
                    pthread_mutex_lock(&mutex_target_temp);
                    target_temp = command.value;
                    pthread_mutex_unlock(&mutex_target_temp);
                    break;
                case CMD_ENABLE:
                    pthread_mutex_lock(&mutex_system);
                    system_enabled = true;
                    pthread_mutex_unlock(&mutex_system);
                    break;
                case CMD_DISABLE:
                    pthread_mutex_lock(&mutex_system);
                    system_enabled = false;
                    pthread_mutex_unlock(&mutex_system);
                    break;
                case CMD_RESET:
                    pthread_mutex_lock(&mutex_target_temp);
                    target_temp = command.value;
                    pthread_mutex_unlock(&mutex_target_temp);
                    
                    pthread_mutex_lock(&mutex_system);
                    system_enabled = true;
                    pthread_mutex_unlock(&mutex_system);
                    break;
                default:
                    break;
            }
        }

        if (rand() % 4 == 0) hal_delay_ms(rand() % 3);
    }
}