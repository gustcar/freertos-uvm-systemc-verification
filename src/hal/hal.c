#define _POSIX_C_SOURCE 200809L
#include "hal.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>

// Internal callback pointers
static hal_adc_read_cb_t     adc_cb         = NULL;
static hal_pwm_set_cb_t      pwm_cb         = NULL;
static hal_gpio_write_cb_t   gpio_write_cb  = NULL;
static hal_gpio_read_cb_t    gpio_read_cb   = NULL;
static hal_uart_rx_cb_t      uart_cb        = NULL;
static hal_log_write_cb_t    log_write_cb   = NULL;
static hal_mutex_wait_cb_t   mutex_wait_cb  = NULL;
static hal_control_inputs_cb_t control_inputs_cb = NULL;

// Simulated-time bridge: published by the SystemC thread, read by DUT threads.
// Lock-free single 64-bit atomic — no blocking, so no deadlock/starvation risk.
static atomic_uint_least64_t sim_time_ns_atomic = 0;

static sim_mode_t current_mode = MODE_NORMAL;

// NEW: DUT-side mutex holder tracking (5 mutexes: sensor, target_temp,
// actuators, alarm, system — same indices as shared_data_b.h mutex order)
#define HAL_MUTEX_COUNT 5
static unsigned int mutex_holder[HAL_MUTEX_COUNT] = {
    (unsigned int)-1, (unsigned int)-1, (unsigned int)-1, (unsigned int)-1, (unsigned int)-1
};
static pthread_mutex_t holder_table_lock = PTHREAD_MUTEX_INITIALIZER;

// Init
void hal_init(sim_mode_t mode) {
    current_mode    = mode;
    adc_cb          = NULL;
    pwm_cb          = NULL;
    gpio_write_cb   = NULL;
    gpio_read_cb    = NULL;
    uart_cb         = NULL;
    log_write_cb    = NULL;
    mutex_wait_cb   = NULL;
    control_inputs_cb = NULL;
    atomic_store(&sim_time_ns_atomic, 0);

    pthread_mutex_lock(&holder_table_lock);
    for (int i = 0; i < HAL_MUTEX_COUNT; i++) mutex_holder[i] = (unsigned int)-1;
    pthread_mutex_unlock(&holder_table_lock);
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
void hal_register_mutex_wait(hal_mutex_wait_cb_t callback) {
    mutex_wait_cb = callback;
}
void hal_register_control_inputs(hal_control_inputs_cb_t callback) {
    control_inputs_cb = callback;
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

uint64_t hal_get_tick_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)(ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000);
}

uint64_t hal_get_sim_time_ns(void) {
    return (uint64_t)atomic_load(&sim_time_ns_atomic);
}

void hal_set_sim_time_ns(uint64_t sim_time_ns) {
    atomic_store(&sim_time_ns_atomic, (uint_least64_t)sim_time_ns);
}

// ============================================================
// Mutex wait reporting (Group B priority inversion instrumentation)
//
// LIMITATION: hal_mutex_current_holder() is read BEFORE the caller
// attempts pthread_mutex_lock(). There is a race window between that
// read and the actual lock attempt where the holder may change —
// this is a best-effort snapshot, not an atomic guarantee. Documented
// as a known limitation of this caixa-preta instrumentation approach.
// ============================================================

void hal_report_mutex_wait(unsigned int task_id, unsigned int mutex_id, uint32_t wait_ms, unsigned int holder_task_id) {
    if (mutex_wait_cb) {
        mutex_wait_cb(task_id, mutex_id, wait_ms, holder_task_id);
        return;
    }
    /* Default POSIX stub: no-op — standalone Group B build ignores this */
}

void hal_report_control_inputs(float temperature, float humidity, float target) {
    if (control_inputs_cb) {
        control_inputs_cb(temperature, humidity, target);
        return;
    }
    /* Default POSIX stub: no-op — standalone builds ignore this */
}

void hal_mutex_mark_acquired(unsigned int mutex_id, unsigned int task_id) {
    if (mutex_id >= HAL_MUTEX_COUNT) return;
    pthread_mutex_lock(&holder_table_lock);
    mutex_holder[mutex_id] = task_id;
    pthread_mutex_unlock(&holder_table_lock);
}

void hal_mutex_mark_released(unsigned int mutex_id) {
    if (mutex_id >= HAL_MUTEX_COUNT) return;
    pthread_mutex_lock(&holder_table_lock);
    mutex_holder[mutex_id] = (unsigned int)-1;
    pthread_mutex_unlock(&holder_table_lock);
}

unsigned int hal_mutex_current_holder(unsigned int mutex_id) {
    if (mutex_id >= HAL_MUTEX_COUNT) return (unsigned int)-1;
    pthread_mutex_lock(&holder_table_lock);
    unsigned int h = mutex_holder[mutex_id];
    pthread_mutex_unlock(&holder_table_lock);
    return h;
}