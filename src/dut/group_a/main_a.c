#include <stdio.h>
#include "config.h"
#include "types.h"
#include "hal.h"
#include "shared_data_a.h"

int main(void) {
    printf("============================================\n"  );
    printf("  Environmental Multi-Sensor Controller\n"       );
    printf("  Group A — Vulnerable (no protection)\n"        );
    printf("============================================\n\n");

    /* Print configuration */
    printf(
        "[CONFIG] NUM_TASKS=%d  LOOPS=%d  STRESS_RUNS=%d\n",
        NUM_TASKS, LOOPS_PER_TASK, STRESS_RUNS
    );
    printf(
        "[CONFIG] Stack=%d words  Cache line=%d bytes\n\n",
        STACK_SIZE_WORDS, CACHE_LINE_SIZE
    );

    /* Validate type sizes — critical for false-sharing analysis */
    printf("[TYPES] sizeof(sensor_data_t)         = %zu bytes\n", sizeof(sensor_data_t));
    printf("[TYPES] sizeof(sensor_data_aligned_t) = %zu bytes\n", sizeof(sensor_data_aligned_t));
    printf("[TYPES] sizeof(actuator_state_t)      = %zu bytes\n", sizeof(actuator_state_t));
    printf("[TYPES] sizeof(command_t)             = %zu bytes\n", sizeof(command_t));
    printf("[TYPES] sizeof(log_t)                 = %zu bytes\n\n", sizeof(log_t));

    /* Verify false-sharing hypothesis */
    if (sizeof(sensor_data_t) < CACHE_LINE_SIZE) {
        printf(
            "[CHECK] sensor_data_t (%zuB) < cache line (%dB) → FALSE SHARING POSSIBLE\n",
            sizeof(sensor_data_t), CACHE_LINE_SIZE
        );
    }
    if (sizeof(sensor_data_aligned_t) == CACHE_LINE_SIZE) {
        printf(
            "[CHECK] sensor_data_aligned_t (%zuB) = cache line (%dB) → FALSE SHARING MITIGATED\n\n",
            sizeof(sensor_data_aligned_t), CACHE_LINE_SIZE
        );
    }

    // Test HAL initialization
    printf("[HAL] [HAL] Initializing in MODE_NORMAL...\n");
    hal_init(MODE_NORMAL);

    float temp = hal_adc_read(ADC_CH_TEMP);
    float hum  = hal_adc_read(ADC_CH_HUMIDITY);
    printf("[HAL] ADC temp=%.1f°C  humidity=%.1f%%\n", temp, hum);

    hal_pwm_set(PWM_CH_FAN, 50);
    hal_gpio_write(GPIO_PIN_PUMP, true);
    printf(
        "[HAL] GPIO pump=%d  led_status=%d\n",
        hal_gpio_read(GPIO_PIN_PUMP),
        hal_gpio_read(GPIO_PIN_LED_STATUS)
    );

    hal_log_write("test log entry\n", 15);

    // Test shared data
    printf("\n[SHARED] Reseting shared data...\n");
    shared_data_reset();

    printf(
        "[SHARED] temperature=%.1f humidity=%.1f\n",
        sensor_data.temperature,
        sensor_data.humidity
    );
    printf(
        "[SHARED] target=%.1f pump=%d fan=%d alarm=%d enabled=%d\n",
        target,
        actuators.pump_on,
        actuators.fan_duty,
        alarm_state,
        system_enabled
    );

    printf("\n============================================\n");
    printf("  HAL validated.\n");
    printf("============================================\n");

    return 0;
}