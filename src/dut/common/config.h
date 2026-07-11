#ifndef CONFIG_H
#define CONFIG_H

// Verification parameters
#define NUM_TASKS           5
#define LOOPS_PER_TASK      10000
#define STRESS_RUNS         100
#define STACK_SIZE_WORDS    512

// Task priorities (higher = more urgent)
#define PRIORITY_ALARM      4
#define PRIORITY_SENSOR     3
#define PRIORITY_CONTROL    2
#define PRIORITY_COMM       1
#define PRIORITY_LOGGER     0

// Targets and limits (Temperature and humidity)
#define TEMP_TARGET_DEFAULT     25.0f // Celsius
#define TEMP_ALARM_LIMIT        35.0f
#define TEMP_CRITICAL_LIMIT     40.0f
#define HUMIDITY_TARGET_DEFAULT 50.0f // %
#define HUMIDITY_MIN_LIMIT      30.0f
#define HUMIDITY_MAX_LIMIT      70.0f

// Actuator controls
#define FAN_SPEED_MIN           0
#define FAN_SPEED_MAX           100
#define FAN_TEMP_TOLERANCE      2.0f // Celsius

// Hardware pins
#define ADC_CH_TEMP             0
#define ADC_CH_HUMIDITY         1
#define PWM_CH_FAN              0
#define GPIO_PIN_PUMP           0
#define GPIO_PIN_LED_STATUS     1
#define GPIO_PIN_LED_ALARM      2

// Communication
#define SERIAL_BUFFER_SIZE      128 // UART

// Cache line size for false-sharing analysis
#define CACHE_LINE_SIZE     64 // 64 bytes - ARM

// Simulation mode
typedef enum {
    MODE_NORMAL = 0, // HAL with POSIX defaults
    MODE_UVM_SYSTEMC // HAL with UVM-SystemC testbench
} sim_mode_t;

#endif