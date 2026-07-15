#define _POSIX_C_SOURCE 200809L
#include "hal.h"
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

// Internal callback pointers
static hal_adc_read_cb_t     adc_cb         = NULL;
static hal_pwm_set_cb_t      pwm_cb         = NULL;
static hal_gpio_write_cb_t   gpio_write_cb  = NULL;
static hal_gpio_read_cb_t    gpio_read_cb   = NULL;
static hal_uart_rx_cb_t      uart_cb        = NULL;
static hal_log_write_cb_t    log_write_cb   = NULL;

static sim_mode_t current_mode = MODE_NORMAL;

// Init
void hal_init(sim_mode_t mode) {
    current_mode    = mode;
    adc_cb          = NULL;
    pwm_cb          = NULL;
    gpio_write_cb   = NULL;
    gpio_read_cb    = NULL;
    uart_cb         = NULL;
    log_write_cb    = NULL;
}

// Callback registration
void hal_register_adc(hal_adc_read_cb_t callback) {
    adc_cb = callback;
}
void hal_register_pwm(hal_pwm_set_cb_t callback) {
    pwm_cb = callback;
}
void hal_register_gpio(hal_gpio_write_cb_t write_cb, hal_gpio_read_cb_t read_cb) {
    gpio_write_cb = write_cb;
    gpio_read_cb = read_cb;
}
void hal_register_uart(hal_uart_rx_cb_t callback) {
    uart_cb = callback;
}
void hal_register_log(hal_log_write_cb_t callback) {
    log_write_cb = callback;
}

// ADC: returns simulated sensor values
float hal_adc_read(uint8_t channel) {
    if (adc_cb) {
        return adc_cb(channel);
    }

    /* Default POSIX stub */
    switch (channel) {
        case ADC_CH_TEMP:      return TEMP_TARGET_DEFAULT;
        case ADC_CH_HUMIDITY:  return HUMIDITY_TARGET_DEFAULT;
        default:               return 0.0f;
    }
}

// PWM: prints duty cycle (normal mode)
void hal_pwm_set(uint8_t channel, uint8_t duty_percent) {
    if (pwm_cb) {
        pwm_cb(channel, duty_percent);
        return;
    }

    /* Default POSIX stub */
    if (current_mode == MODE_NORMAL) {
        printf("[HAL PWM] ch=%d duty=%d%%\n", channel, duty_percent);
    }
}

// GPIO: stores state in local array (normal mode)
static bool gpio_state[8] = {false};

void hal_gpio_write(uint8_t pin, bool value) {
    if (gpio_write_cb) {
        gpio_write_cb(pin, value);
        return;
    }

    /* Default POSIX stub */
    if (pin < 8) gpio_state[pin] = value;
}

bool hal_gpio_read(uint8_t pin) {
    if (gpio_read_cb) {
        return gpio_read_cb(pin);
    }

    /* Default POSIX stub */
    return (pin < 8) ? gpio_state[pin] : false;
}

// UART: no-op in normal mode (returns 0 bytes)
int hal_uart_rx(uint8_t *buf, uint16_t len) {
    command_t command;
    if (uart_cb) {
        return uart_cb(buf, len);
    }
    
    // Default POSIX stub: 10% chance to inject a command */
    if (rand() % 10 == 0 && len >= sizeof(command_t)) {
        switch (rand() % 4) {
            case 0: command.type = CMD_TARGET;
                    command.value = TEMP_TARGET_DEFAULT + ((float)(rand() % 100) / 10.0f) - 5.0f;
                    break;
            case 1: command.type = CMD_ENABLE;   command.value = 0; break;
            case 2: command.type = CMD_DISABLE;  command.value = 0; break;
            case 3: command.type = CMD_RESET;    command.value = 0; break;
        }
        memcpy(buf, &command, sizeof(command_t));
        return sizeof(command_t);
    }
    return 0;
}

// Log: writes to stdout in normal mode
int hal_log_write(const char *data, uint16_t len) {
    if (log_write_cb) {
        return log_write_cb(data, len);
    }

    /* Default POSIX stub: print to stdout */
    if (current_mode == MODE_NORMAL) {
        printf("[HAL LOG] %.*s", (int)len, data);
    }
    return (int)len;
}

// Timing: uses POSIX clock
uint32_t hal_get_tick_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

void hal_delay_ms(uint32_t ms) {
    struct timespec ts = {
        .tv_sec  = ms / 1000,
        .tv_nsec = (ms % 1000) * 1000000L
    };
    nanosleep(&ts, NULL);
}