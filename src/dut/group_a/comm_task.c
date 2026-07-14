// ============================================================
// comm_task.c — Communication Task
// Priority: 1 (low)
// Writes: target_temp, system_enabled
// Protection: NONE (Group A — vulnerable)
// ============================================================
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "types.h"
#include "hal.h"
#include "shared_data_a.h"

void comm_task(void) {
    uint8_t uart_buffer[SERIAL_BUFFER_SIZE];
    command_t command;
    int bytes;
    
    for (int i = 0; i < LOOPS_PER_TASK; i++) {
        bytes = hal_uart_rx(uart_buffer, SERIAL_BUFFER_SIZE);

        if(bytes > 0) {
            memcpy(&command, uart_buffer, sizeof(command_t));

            switch (command.type) {
                case CMD_TARGET:
                    target_temp = command.value;
                    break;
                case CMD_ENABLE:
                    system_enabled = true;
                    break;
                case CMD_DISABLE:
                    system_enabled = false;
                    break;
                case CMD_RESET:
                    target_temp = TEMP_TARGET_DEFAULT;
                    system_enabled = true;
                default:
                    break;
            }
        }
        // Random delay
        if(rand() % 4 == 0) { hal_delay_ms(rand() % 3); }
    }
}