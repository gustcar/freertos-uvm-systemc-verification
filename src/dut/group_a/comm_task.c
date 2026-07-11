// ============================================================
// comm_task.c — Communication Task
// Priority: 1 (low)
// Writes: target, system_enabled
// Reads: sensor_data.temperature, alarm_state
// Protection: NONE (Group A — vulnerable)
// ============================================================
#include <stdint.h>
#include "config.h"
#include "types.h"
#include "hal.h"
#include "shared_data_a.h"

void comm_task(void) {
    float current_temp;
    alarm_level_t current_alarm;
    uint8_t uart_buf[SERIAL_BUFFER_SIZE];
    command_t cmd;
    float new_target;

    for (int i = 0; i < LOOPS_PER_TASK; i++) {
        // VULNERABLE READ
        current_temp = sensor_data.temperature;
        current_alarm = alarm_state;

        // Simulate receiving commands (10% probability per iteration)
        if (rand() % 10 == 0) {
            hal_uart_rx(uart_buf, SERIAL_BUFFER_SIZE);
            
            int rand_cmd = rand() % 4;

            switch(rand_cmd) {
                case 0: // CMD_TARGET
                    cmd.type = CMD_TARGET;
                    cmd.value = (float)(20 + (rand() % 20)); // range: 20 - 40 Celsius
                    if(system_enabled) {
                        target = cmd.value;
                    }
                    break;
                case 1: // CMD_ENABLE
                    cmd.type = CMD_ENABLE;
                    system_enabled = true;
                    break;
                case 2: // CMD_DISABLE
                    cmd.type = CMD_DISABLE;
                    system_enabled = false;
                    break;
                case 3: // CMD_RESET
                    cmd.type = CMD_RESET;
                    target = TEMP_TARGET_DEFAULT;
                    system_enabled = true;
                    break;
            }
        }

        if(rand() % 4 == 0) { hal_delay_ms(rand() % 3); }
    }
}