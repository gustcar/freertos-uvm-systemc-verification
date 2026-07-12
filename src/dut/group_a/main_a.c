#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "config.h"
#include "types.h"
#include "hal.h"
#include "shared_data_a.h"

static pthread_t tasks[NUM_TASKS];

typedef struct {
    int task_id;
    const char* name;
    int priority;
    void (*task_func)(void);
} task_arg_t;

static void* thread_wrapper(void* aux) {
    task_arg_t* task_arg = (task_arg_t*)aux;
    task_arg->task_func();
    return NULL;
}

// Task function declarations
extern void sensor_task(void);
extern void control_task(void);
extern void comm_task(void);
extern void alarm_task(void);
extern void logger_task(void);

int main(void) {
    task_arg_t task_args[NUM_TASKS];
    pthread_attr_t attr;
    int thread_create_result;

    printf("============================================\n"  );
    printf("  Environmental Multi-Sensor Controller\n"       );
    printf("  Group A — Vulnerable (no protection)\n"        );
    printf("============================================\n\n");

    hal_init(MODE_NORMAL);
    shared_data_reset();

    printf("[MAIN] Starting %d concurrent tasks...\n", NUM_TASKS);
    printf("[MAIN] Expected behavior: RACE CONDITIONS\n\n");

    // Task definitions - struct task_arg_t initialization
    // task_id, name, priority, function_pointer
    task_args[0] = (task_arg_t){0, "alarm_task", PRIORITY_ALARM, alarm_task};
    task_args[1] = (task_arg_t){1, "sensor_task", PRIORITY_SENSOR, sensor_task};
    task_args[2] = (task_arg_t){2, "control_task", PRIORITY_CONTROL, control_task};
    task_args[3] = (task_arg_t){3, "comm_task", PRIORITY_COMM, comm_task};
    task_args[4] = (task_arg_t){4, "logger_task", PRIORITY_LOGGER, logger_task};

    pthread_attr_init(&attr);
    
    for(int i = 0; i < NUM_TASKS; i++) {
        printf("[MAIN] Creating %s (priority=%d)\n", task_args[i].name, task_args[i].priority);
        
        thread_create_result = pthread_create(&tasks[i], &attr, thread_wrapper, &task_args[i]);
        if(thread_create_result != 0) {
            fprintf(stderr, "[ERROR] Failed to create %s: %s\n", task_args[i].name, strerror(thread_create_result));
            return EXIT_FAILURE;
        }
    }

    printf("\n[MAIN] All threads created. Running %d iterations...\n", LOOPS_PER_TASK);

    // Wait for all threads to complete
    for(int i = 0; i < NUM_TASKS; i++) {
        pthread_join(tasks[i], NULL);
        printf("[MAIN] %s completed\n", task_args[i].name);
    }

    // Print results.
    printf("\n[MAIN] Final shared state:\n");
    printf("\n  temperature=%.2f  humidity=%.2f\n", sensor_data.temperature, sensor_data.humidity);
    printf("\n  temperature target=%.2f  pump=%d  fan_duty=%u\n", target, actuators.pump_on, actuators.fan_duty);
    printf("\n  alarm_state=%d  system enabled=%d\n", alarm_state, system_enabled);

    printf("\n============================================\n");
    printf("  Group A completed\n");
    printf("============================================\n");

    pthread_attr_destroy(&attr);
    return EXIT_SUCCESS;
}