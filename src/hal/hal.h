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

#endif