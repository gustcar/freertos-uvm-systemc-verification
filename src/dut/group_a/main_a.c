#include <stdio.h>
#include "config.h"
#include "types.h"
#include "hal.h"

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

    /* Test HAL initialization (stub for now) */
    printf("[HAL] hal_init(MODE_STANDALONE) → TODO (Step 2)\n");
    /* hal_init(MODE_STANDALONE); */  /* TODO: uncomment after Step 2 */

    printf("\n============================================\n");
    printf("  Headers validated.\n");
    printf("  Next: Step 2 — HAL implementation\n");
    printf("============================================\n");

    return 0;
}