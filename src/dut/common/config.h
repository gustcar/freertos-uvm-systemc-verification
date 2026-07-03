#ifndef CONFIG_H
#define CONFIG_H

// Verification parameters
#define NUM_TASKS           5
#define ITERATIONS_PER_TASK 10000
#define STRESS_RUNS         100

// Task priorities
#define PRIORITY_ALARM      4
#define PRIORITY_SENSOR     3
#define PRIORITY_CONTROL    2
#define PRIORITY_COMM       1
#define PRIORITY_LOGGER     0

#endif