#ifndef HAL_H
#define HAL_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"
#include "types.h"

// Initialization
void hal_init(sim_mode_t mode);

// ADC (Sensor Input)
float hal_adc_read(uint8_t channel);
// Used by UVM-SystemC
typedef float (*hal_adc_read_cb_t)(uint8_t channel);
void hal_register_adc(hal_adc_read_cb_t adc_read_callback);

// PWM (Fan Control Output)
void hal_pwm_set(uint8_t channel, uint8_t duty_percent);
// Used by UVM-SystemC
typedef void (*hal_pwm_set_cb_t)(uint8_t channel, uint8_t duty_percent);
void hal_register_pwm(hal_pwm_set_cb_t pwm_set_callback);

// GPIO (Digital I/O: Pump Relay, LEDs)
void hal_gpio_write(uint8_t pin, bool value);
bool hal_gpio_read(uint8_t pin);
// Used by UVM-SystemC
typedef void (*hal_gpio_write_cb_t)(uint8_t pin, bool value);
typedef bool (*hal_gpio_read_cb_t)(uint8_t pin);
void hal_register_gpio(hal_gpio_write_cb_t gpio_write_callback, hal_gpio_read_cb_t gpio_read_callback);

// UART (Command Reception)
int hal_uart_rx(uint8_t *buf, uint16_t len);
// Used by UVM-SystemC
typedef int (*hal_uart_rx_cb_t)(uint8_t *buf, uint16_t len);
void hal_register_uart(hal_uart_rx_cb_t uart_rx_callback);

// log (Persistent Logging)
int hal_log_write(const char *data, uint16_t len);
// Used by UVM-SystemC
typedef int (*hal_log_write_cb_t)(const char *data, uint16_t len);
void hal_register_log(hal_log_write_cb_t log_write_callback);

// Timing
uint32_t hal_get_tick_ms(void);
void hal_delay_ms(uint32_t ms);
uint64_t hal_get_tick_us(void);

// Mutex Wait Reporting (Group B priority inversion instrumentation)
// reports how long task_id waited to acquire mutex_id, and which
// task held it just before the lock attempt (best-effort snapshot —
// see hal.c for the race-window caveat).
void hal_report_mutex_wait(unsigned int task_id, unsigned int mutex_id, uint32_t wait_ms, unsigned int holder_task_id);
// Used by UVM-SystemC
typedef void (*hal_mutex_wait_cb_t)(unsigned int task_id, unsigned int mutex_id, uint32_t wait_ms, unsigned int holder_task_id);
void hal_register_mutex_wait(hal_mutex_wait_cb_t mutex_wait_callback);

// DUT-side holder tracking table (5 mutexes: sensor, target_temp,
// actuators, alarm, system — same indices as shared_data_b.h mutex order)
void hal_mutex_mark_acquired(unsigned int mutex_id, unsigned int task_id);
void hal_mutex_mark_released(unsigned int mutex_id);
unsigned int hal_mutex_current_holder(unsigned int mutex_id);

// Control input coherence instrumentation (torn-read exposure)
// control_task reports the (temperature, humidity, target) triple it
// actually consumed in one control iteration, so the testbench can verify
// the values were mutually coherent — i.e. that (temperature, humidity)
// came from a single sensor reading rather than a torn read across two.
// Group A (unprotected) can report incoherent triples; Group B (mutex) can't.
void hal_report_control_inputs(float temperature, float humidity, float target);
// Used by UVM-SystemC
typedef void (*hal_control_inputs_cb_t)(float temperature, float humidity, float target);
void hal_register_control_inputs(hal_control_inputs_cb_t control_inputs_callback);


#endif