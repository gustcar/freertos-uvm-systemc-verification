# Methodology

How the FreeRTOS firmware is verified with UVM-SystemC over a simulated HAL, and
how the statistical and isolated-performance evidence is produced.

---

## 1. Why this architecture

The hardware is not yet available, so the firmware runs on a **simulated HAL**
(the hardware/software boundary) inside a SystemC virtual platform. The **real
firmware** executes unchanged on top of that HAL, and a **UVM-SystemC** testbench
acts as quality control — the coverage-driven verification (CDV) methodology
applied to concurrent embedded software.

```
        UVM-SystemC Testbench (C++)
   Sequences → HAL callbacks → Monitors → Scoreboard / Checkers / Coverage
                        │  (HAL = boundary)
        DUT (C, FreeRTOS-style on POSIX threads)
   sensor · control · comm · alarm · logger   +   shared state
```

## 2. The HAL bridge

The firmware calls a small HAL (`hal_adc_read`, `hal_pwm_set`, `hal_gpio_write`,
`hal_uart_rx`, `hal_log_write`, plus instrumentation hooks
`hal_report_mutex_wait`, `hal_report_control_inputs`). The HAL is **dual-mode**:

- **Standalone (POSIX):** default stubs, for running the firmware natively.
- **UVM-SystemC:** the testbench registers callbacks (`hal_register_*`). Each
  firmware HAL call is turned into a `hal_event` and pushed onto a **thread-safe
  queue** by the DUT pthreads; the single SystemC thread drains the queue and
  forwards events to the UVM monitors. DUT threads never touch UVM/SystemC
  directly — the queue is the only crossing point.

Simulated time is published to the firmware (`hal_set/get_sim_time_ns`) so every
event carries a consistent timestamp, which the deadlock detector keys on.

## 3. Testbench components

- **Agents (passive monitors):** `sensor`, `actuator`, `comm`, plus three
  instrumentation agents — `control_input` (the exact temp/humidity pair
  `control_task` consumed), `logger` (liveness heartbeat), `mutex_wait`
  (lock waiting times).
- **Scoreboard (`concurrency_sb`):** a **reference model** recomputes the
  expected fan/pump/alarm outputs from recent sensor readings and flags
  disagreements. It also runs a **direct torn-read detector**: it remembers every
  (temp, humidity) pair that co-occurred in one real sensor reading; a control
  input whose pair was never produced together was torn across two readings.
- **Checkers:** `data_integrity` (value ranges), `timing` (actuator reaction to
  threshold crossings), `priority_inversion` (from mutex-wait data),
  `deadlock_detector` (heartbeat watchdog).
- **Coverage:** functional bins over temperature range × priority × task, plus
  critical-corner counts.

### 3.1 Two torn-read detectors, one authoritative

The reference-model reconstruction (`check_actuator_pair`) is a **weak** torn-read
detector: the control law saturates (fan pins to 0/100, pump is binary), so most
torn reads produce the same output as a coherent read and slip through. It also
has a finite 5-sample snapshot window, which can raise the occasional false
positive. The **direct coherence check** on the control inputs is therefore the
**authoritative** race signal, and all headline correctness claims use it.

### 3.2 Deadlock detection

A watchdog tracks a per-task heartbeat keyed on simulated time. It covers all five
tasks, **distinguishes normal completion from a stall** (a finished task is
excluded so its trailing silence is not mistaken for a hang), keeps sparse
producers (comm) visibly alive with a throttled liveness heartbeat, and runs a
periodic silence-probe tick so total silence is caught even without events.

## 4. Exposing the race (stimulus design)

A race only appears if the inputs actually vary. The sensor driver enqueues a
stream of distinct (temp, humidity) pairs into a bounded FIFO; the DUT ADC
callback dequeues one coherent pair per read. The pair is coherent *at the
source*, so any torn read observed later is genuinely the DUT's own unprotected
read of `sensor_data` — not an artifact of the stimulus. Random `hal_delay_ms`
delays widen the interleaving window.

## 5. Statistical method

The DUT tasks are **real pthreads**, so the interleaving is driven by OS
scheduling, not by the RNG seed. Repeated runs of the same binary are therefore
independent samples of a real distribution — no seed plumbing is required.

- `scripts/run_stress.sh [N]` runs `race_condition_test` (A) and `protected_test`
  (B) N times each, one log per run.
- `scripts/collect_results.py` parses every log, writes a per-run CSV and an
  aggregate summary, reporting the **torn-read failure rate** (authoritative)
  separately from the weak-checker mismatch rate.
- `scripts/plot_results.py` renders the charts as **SVG** (pure standard library —
  no matplotlib/numpy/network; vector output for the report).

## 6. Isolated performance method

Two hazards are not well measured inside the full DUT because its wall time is
dominated by the tasks' random sleeps. They are measured in isolation, each
varying exactly one factor (wall-clock timing, threads pinned to distinct cores,
no profiler dependency):

- **False sharing** (`false_sharing_bench`): N threads each write only their own
  counter; the sole variable is memory layout (adjacent = shared cache line vs
  padded to 64 B).
- **Mutex overhead** (`mutex_overhead_bench`): uncontended lock/unlock cost, and
  a contended shared counter (mutex vs racy) that also quantifies the lost
  updates the unprotected path drops.

## 7. Reproducing everything

```bash
make bench                     # false sharing + mutex overhead + DUT end-to-end
make stress STRESS_N=30        # 30 runs per group
make collect                   # aggregate -> results/stress_{results.csv,summary.md}
make plots                     # results/plots/*.svg
```
