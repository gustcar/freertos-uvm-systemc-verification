// ============================================================
// main_b.c — Entry point for Group B (Protected)
//
// Creates 5 pthreads with SCHED_FIFO priorities (same as Group A),
// but initializes mutexes before starting threads.
//
// Comparison with Group A:
//   - Group A: no synchronization → race conditions expected
//   - Group B: mutex-protected → consistent results expected
// ============================================================

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include "config.h"
#include "types.h"
#include "hal.h"
#include "shared_data_b.h"

extern void sensor_task_safe(void);
extern void control_task_safe(void);
extern void comm_task_safe(void);
extern void alarm_task_safe(void);
extern void logger_task_safe(void);

// Thread wrappers (pthread requires void* return)
static void* sensor_thread(void* arg)  { (void)arg; sensor_task_safe();  return NULL; }
static void* control_thread(void* arg) { (void)arg; control_task_safe(); return NULL; }
static void* comm_thread(void* arg)   { (void)arg; comm_task_safe();    return NULL; }
static void* alarm_thread(void* arg)  { (void)arg; alarm_task_safe();   return NULL; }
static void* logger_thread(void* arg) { (void)arg; logger_task_safe();  return NULL; }

// Set SCHED_FIFO priority on a thread
static void set_fifo_priority(pthread_t thread, int priority) {
    struct sched_param sp;
    sp.sched_priority = priority;
    pthread_setschedparam(thread, SCHED_FIFO, &sp);
}

int main(void) {
    printf("============================================\n");
    printf("  Environmental Multi-Sensor Controller\n");
    printf("  Group B — Protected (mutex)\n");
    printf("============================================\n\n");

    // Initialize HAL
    hal_init(MODE_NORMAL);

    srand(42);

    // Initialize mutexes BEFORE creating threads
    shared_data_init_mutexes();

    // Reset shared data to defaults
    shared_data_reset();

    printf("[INIT] Mutexes initialized, shared data reset.\n");
    printf("[INIT] Spawning %d threads with SCHED_FIFO...\n\n", NUM_TASKS);

    // Create threads (order: highest priority first is conventional)
    pthread_t t_alarm, t_sensor, t_control, t_comm, t_logger;

    pthread_create(&t_alarm,   NULL, alarm_thread,   NULL);
    set_fifo_priority(t_alarm,   PRIORITY_ALARM);

    pthread_create(&t_sensor,  NULL, sensor_thread,  NULL);
    set_fifo_priority(t_sensor,  PRIORITY_SENSOR);

    pthread_create(&t_control, NULL, control_thread, NULL);
    set_fifo_priority(t_control, PRIORITY_CONTROL);

    pthread_create(&t_comm,    NULL, comm_thread,    NULL);
    set_fifo_priority(t_comm,    PRIORITY_COMM);

    pthread_create(&t_logger,  NULL, logger_thread,  NULL);
    set_fifo_priority(t_logger,  PRIORITY_LOGGER);

    // Wait for all threads to finish
    pthread_join(t_alarm,   NULL);
    pthread_join(t_sensor,  NULL);
    pthread_join(t_control, NULL);
    pthread_join(t_comm,    NULL);
    pthread_join(t_logger,  NULL);

    // Print final state
    printf("\n[FINAL] All threads completed.\n");
    printf("[FINAL] temp=%.2f  humidity=%.2f\n",
           sensor_data.temperature, sensor_data.humidity);
    printf("[FINAL] setpoint=%.2f  pump=%d  fan=%d  alarm=%d  enabled=%d\n",
           target_temp, actuators.pump_on, actuators.fan_duty,
           alarm_state, system_enabled);

    // Cleanup
    shared_data_destroy_mutexes();

    printf("\n============================================\n");
    printf("  Group B execution complete.\n");
    printf("============================================\n");

    return 0;
}