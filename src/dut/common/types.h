#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"

// Alarm Severity
typedef enum {
    ALARM_NORMAL   = 0,
    ALARM_WARNING   = 1,
    ALARM_CRITICAL  = 2
} alarm_level_t;

// Sensor Data: Group A (vulnerable)
typedef struct {
    volatile float temperature;
    volatile float humidity;
} sensor_data_t;

// Sensor Data: Group B (protected)
typedef struct {
    volatile float temperature;
    volatile float humidity;
    char cache_line_padding[CACHE_LINE_SIZE - (2 * sizeof(float))];
} sensor_data_aligned_t;

// Actuator Outputs
typedef struct {
    volatile bool    pump_on;
    volatile uint8_t fan_duty;
} actuator_state_t;

// Communication Commands
typedef enum {
    CMD_NONE      = 0,
    CMD_TARGET    = 1,
    CMD_ENABLE    = 2,
    CMD_DISABLE   = 3,
    CMD_RESET     = 4
} command_type_t;

typedef struct {
    command_type_t type;
    float value;
} command_t;

// Log data
typedef struct {
    uint32_t tick;
    float    temp;
    float    humidity;
    bool     pump_on;
    uint8_t  fan_duty;
    uint8_t  alarm_level;
} log_t;

#endif /* TYPES_H */