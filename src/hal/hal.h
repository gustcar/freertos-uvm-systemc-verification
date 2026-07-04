#ifndef HAL_H
#define HAL_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"
#include "types.h"

// TODO: Implement HAL functions in hal_*.c
// TODO: Add callback registration mechanism
// TODO: add timing functions???

// Initialization
void hal_init(sim_mode_t mode);

// ADC (Sensor Input)
float hal_adc_read(uint8_t channel);

// PWM (Fan Control Output)
void hal_pwm_set(uint8_t channel, uint8_t duty_percent);

// GPIO (Digital I/O: Pump Relay, LEDs)
void hal_gpio_write(uint8_t pin, bool value);
bool hal_gpio_read(uint8_t pin);

// UART (Command Reception)
int hal_uart_rx(uint8_t *buf, uint16_t len);

// log (Persistent Logging)
int hal_log_write(const char *data, uint16_t len);

#endif