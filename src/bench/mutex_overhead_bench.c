// ============================================================
// mutex_overhead_bench.c — Isolated mutex-overhead microbenchmark
//
// Measures, cleanly, the price the project's Group B pays for
// correctness: the cost of a pthread_mutex around every critical
// section. Reports two complementary numbers:
//
//   1) Uncontended cost — one thread, lock/increment/unlock vs a
//      bare increment. The delta is the pure lock+unlock cost of a
//      mutex with no contention (ns per critical section).
//
//   2) Contended cost — N threads hammering ONE shared counter,
//      mutex-protected vs unprotected. Shows the wall-time overhead
//      under contention AND the correctness cost of dropping the
//      lock: the unprotected version loses updates (lost_updates),
//      which is exactly the Group A race, quantified.
//
// The shared counter is volatile so the compiler cannot hoist the
// updates; the unprotected loop therefore performs real (racy)
// read-modify-write stores.
// ============================================================

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

#define BENCH_THREADS 4
static long g_uncontended_ops = 100000000L; // 1e8 lock/unlock pairs (single thread)
static long g_contended_ops   = 10000000L;  // 1e7 ops per thread (N threads)

static pthread_mutex_t g_mtx = PTHREAD_MUTEX_INITIALIZER;
static volatile long   g_shared;

static double secs_between(struct timespec a, struct timespec b) {
    return (double)(b.tv_sec - a.tv_sec) + (double)(b.tv_nsec - a.tv_nsec) / 1e9;
}

// ---- 1) Uncontended (single thread) ----
static double uncontended(int locked) {
    struct timespec a, b;
    g_shared = 0;
    clock_gettime(CLOCK_MONOTONIC, &a);
    if (locked) {
        for (long i = 0; i < g_uncontended_ops; i++) {
            pthread_mutex_lock(&g_mtx);
            g_shared++;
            pthread_mutex_unlock(&g_mtx);
        }
    } else {
        for (long i = 0; i < g_uncontended_ops; i++) {
            g_shared++;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &b);
    return secs_between(a, b);
}

// ---- 2) Contended (N threads, one shared counter) ----
static int  g_locked;
static long g_per_thread;

static void* cworker(void* arg) {
    (void)arg;
    if (g_locked) {
        for (long i = 0; i < g_per_thread; i++) {
            pthread_mutex_lock(&g_mtx);
            g_shared++;
            pthread_mutex_unlock(&g_mtx);
        }
    } else {
        for (long i = 0; i < g_per_thread; i++) {
            g_shared++;   // racy: read-modify-write without protection
        }
    }
    return NULL;
}

static double contended(int locked, long* final_value) {
    pthread_t th[BENCH_THREADS];
    struct timespec a, b;

    g_shared = 0;
    g_locked = locked;
    g_per_thread = g_contended_ops;

    clock_gettime(CLOCK_MONOTONIC, &a);
    for (int i = 0; i < BENCH_THREADS; i++) pthread_create(&th[i], NULL, cworker, NULL);
    for (int i = 0; i < BENCH_THREADS; i++) pthread_join(th[i], NULL);
    clock_gettime(CLOCK_MONOTONIC, &b);

    *final_value = g_shared;
    return secs_between(a, b);
}

int main(int argc, char** argv) {
    if (argc > 1) { long v = atol(argv[1]); if (v > 0) g_uncontended_ops = v; }
    if (argc > 2) { long v = atol(argv[2]); if (v > 0) g_contended_ops   = v; }

    printf("============================================================\n");
    printf(" Mutex-Overhead Microbenchmark  (threads=%d, cores=%ld)\n",
           BENCH_THREADS, sysconf(_SC_NPROCESSORS_ONLN));
    printf("============================================================\n");

    // 1) Uncontended
    double u_lock   = uncontended(1);
    double u_nolock = uncontended(0);
    double ns_per_cs = (u_lock - u_nolock) / (double)g_uncontended_ops * 1e9;
    printf("[1] Uncontended (single thread, %ld ops)\n", g_uncontended_ops);
    printf("    bare increment      : %.3f s\n", u_nolock);
    printf("    lock+incr+unlock    : %.3f s\n", u_lock);
    printf("    --> mutex cost      : %.2f ns per critical section\n", ns_per_cs);

    // 2) Contended
    long v_lock = 0, v_nolock = 0;
    double c_lock   = contended(1, &v_lock);
    double c_nolock = contended(0, &v_nolock);
    long   expected = (long)BENCH_THREADS * g_contended_ops;
    double overhead = (c_nolock > 0.0) ? c_lock / c_nolock : 0.0;
    long   lost     = expected - v_nolock;
    double lost_pct = (expected > 0) ? 100.0 * (double)lost / (double)expected : 0.0;
    printf("[2] Contended (%d threads x %ld ops on one shared counter)\n",
           BENCH_THREADS, g_contended_ops);
    printf("    unprotected (racy)  : %.3f s   final=%ld / %ld  (lost %ld updates, %.1f%%)\n",
           c_nolock, v_nolock, expected, lost, lost_pct);
    printf("    mutex-protected     : %.3f s   final=%ld / %ld  (correct)\n",
           c_lock, v_lock, expected);
    printf("    --> mutex overhead  : %.2fx wall time (buys correctness)\n", overhead);

    printf("CSV,mutex_uncontended,%ld,%.4f,%.4f,%.3f\n",
           g_uncontended_ops, u_nolock, u_lock, ns_per_cs);
    printf("CSV,mutex_contended,%d,%ld,%.4f,%.4f,%.3f,%ld,%.2f\n",
           BENCH_THREADS, g_contended_ops, c_nolock, c_lock, overhead, lost, lost_pct);

    return EXIT_SUCCESS;
}
