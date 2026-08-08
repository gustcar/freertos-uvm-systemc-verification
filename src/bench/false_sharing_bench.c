// ============================================================
// false_sharing_bench.c — Isolated false-sharing microbenchmark
//
// Demonstrates, with a clean and reproducible number, the cost of
// false sharing — the effect the project's Group A shared data is
// exposed to (several independent hot variables packed onto one
// 64-byte cache line, written by different threads/cores).
//
// Design: N threads, each incrementing its OWN counter M times.
// There is NO logical sharing between threads — each touches only
// its own counter. The ONLY variable is the memory layout:
//
//   packed[] : counters adjacent, many per cache line  -> false sharing
//   padded[] : each counter isolated on its own line   -> no false sharing
//
// Threads are pinned to distinct cores so the coherence traffic is
// real. Counters are volatile so every increment is a genuine store
// (otherwise -O2 would collapse the loop into a single add and no
// cache line would ever move). Same work, same result — only the
// layout differs, so the time difference IS the false-sharing cost.
// ============================================================

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <unistd.h>

#include "config.h"   // CACHE_LINE_SIZE (64B — matches this host)

#define BENCH_THREADS 4          // independent writers (one per core)
#define BENCH_REPS    3          // repetitions; we keep the best (min) time
static long g_iters = 100000000L; // increments per thread (1e8 default)

// Packed layout: BENCH_THREADS counters adjacent in memory. At 8 bytes
// each, 8 counters fit in one 64-byte line, so all writers hammer the
// same cache line(s) — textbook false sharing.
static volatile long packed_counters[BENCH_THREADS];

// Padded layout: each counter owns a full cache line, so no two writers
// ever contend for the same line.
typedef struct {
    volatile long v;
    char pad[CACHE_LINE_SIZE - sizeof(long)];
} padded_counter_t;
static padded_counter_t padded_counters[BENCH_THREADS]
    __attribute__((aligned(CACHE_LINE_SIZE)));

typedef struct { int idx; int padded; } targ_t;

static void pin_to_core(int core) {
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(core, &set);
    pthread_setaffinity_np(pthread_self(), sizeof(set), &set);
}

static void* worker(void* arg) {
    targ_t* t = (targ_t*)arg;
    int ncores = (int)sysconf(_SC_NPROCESSORS_ONLN);
    pin_to_core(t->idx % (ncores > 0 ? ncores : 1));

    if (t->padded) {
        for (long i = 0; i < g_iters; i++) padded_counters[t->idx].v++;
    } else {
        for (long i = 0; i < g_iters; i++) packed_counters[t->idx]++;
    }
    return NULL;
}

// One parallel run; returns wall-clock seconds for the create..join region.
static double run_once(int padded) {
    pthread_t th[BENCH_THREADS];
    targ_t    ta[BENCH_THREADS];
    struct timespec a, b;

    for (int i = 0; i < BENCH_THREADS; i++) { packed_counters[i] = 0; padded_counters[i].v = 0; }

    clock_gettime(CLOCK_MONOTONIC, &a);
    for (int i = 0; i < BENCH_THREADS; i++) {
        ta[i].idx = i; ta[i].padded = padded;
        pthread_create(&th[i], NULL, worker, &ta[i]);
    }
    for (int i = 0; i < BENCH_THREADS; i++) pthread_join(th[i], NULL);
    clock_gettime(CLOCK_MONOTONIC, &b);

    return (double)(b.tv_sec - a.tv_sec) + (double)(b.tv_nsec - a.tv_nsec) / 1e9;
}

static double best_of(int padded) {
    double best = 1e18;
    for (int r = 0; r < BENCH_REPS; r++) {
        double t = run_once(padded);
        if (t < best) best = t;
    }
    return best;
}

int main(int argc, char** argv) {
    if (argc > 1) {
        long v = atol(argv[1]);
        if (v > 0) g_iters = v;
    }

    // Fail loudly if the padded struct is not exactly one cache line — the
    // whole experiment depends on that invariant.
    if (sizeof(padded_counter_t) != (size_t)CACHE_LINE_SIZE) {
        fprintf(stderr, "[BENCH] padded_counter_t is %zuB, expected %dB\n",
                sizeof(padded_counter_t), CACHE_LINE_SIZE);
        return EXIT_FAILURE;
    }

    printf("============================================================\n");
    printf(" False-Sharing Microbenchmark\n");
    printf("   threads=%d  iters/thread=%ld  cache_line=%dB  cores=%ld\n",
           BENCH_THREADS, g_iters, CACHE_LINE_SIZE, sysconf(_SC_NPROCESSORS_ONLN));
    printf("   (each thread writes ONLY its own counter — no logical sharing)\n");
    printf("============================================================\n");

    double packed_s = best_of(0);   // adjacent counters -> false sharing
    double padded_s = best_of(1);   // isolated counters -> no false sharing
    double slowdown = (padded_s > 0.0) ? packed_s / padded_s : 0.0;

    printf("  packed (shared cache line): %.3f s  (best of %d)\n", packed_s, BENCH_REPS);
    printf("  padded (own cache line)   : %.3f s  (best of %d)\n", padded_s, BENCH_REPS);
    printf("  --> false sharing slowdown: %.2fx\n", slowdown);
    printf("CSV,false_sharing,%d,%ld,%.4f,%.4f,%.3f\n",
           BENCH_THREADS, g_iters, packed_s, padded_s, slowdown);

    return EXIT_SUCCESS;
}
