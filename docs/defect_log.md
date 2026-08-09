# Defect Log

Concurrency defects exposed in the vulnerable firmware (Group A) and their status
in the protected firmware (Group B). Severity follows the usual scale
(Critical = silent data corruption / safety impact; Major = incorrect behavior;
Minor = tooling/observability).

Evidence is the N = 30 stress campaign (`results/stress_results.csv`) and the
isolated microbenchmarks unless noted.

---

## D1 — Torn read of shared `sensor_data` (race condition)

| | |
|---|---|
| **Severity** | Critical |
| **Group A** | Present — 100% of runs (30/30), mean 8.87 torn reads/run, max 22 |
| **Group B** | Fixed — 0 torn reads in 30/30 runs |

**Root cause.** `sensor_task` writes `sensor_data.temperature` then
`sensor_data.humidity` as two separate stores; `control_task` reads both without
atomicity. With no lock, `control_task` can observe the temperature from one
sensor reading and the humidity from the next — a pair that never physically
co-occurred. The control law then acts on an inconsistent snapshot.

**Detection.** Direct input-coherence check: the pair consumed by `control_task`
is absent from the set of pairs ever produced together in one real sensor reading.

**Fix (Group B).** `sensor_data` accesses are wrapped in `mutex_sensor`, giving
atomic multi-field read/write and a memory barrier for visibility.

**Note.** The reference-model output-reconstruction check also flags this, but is
a *weak* detector (the control law saturates, masking most torn reads) — the
direct coherence check is authoritative.

---

## D2 — False sharing on packed hot globals

| | |
|---|---|
| **Severity** | Major (performance) |
| **Group A** | Present by layout — the ~19 B of hot globals (`sensor_data`, `target_temp`, `actuators`, `alarm_state`, `system_enabled`) fit in one 64 B cache line, written by four different tasks |
| **Group B** | Mitigated on the hottest struct — `sensor_data` is aligned to its own 64 B cache line |

**Root cause.** Independent variables written by different threads share a cache
line, so each write invalidates the others' copy, forcing needless coherence
traffic between cores.

**Evidence.** Isolated microbenchmark (`make bench-falsesharing`): four threads
each writing only their own counter, packed vs padded to a cache line —
**0.536 s → 0.075 s, a 7.2× slowdown** purely from layout.

**Fix status.** Group B aligns `sensor_data` (the highest-frequency contended
struct). The remaining scalars are lower-frequency; padding them all was judged
premature optimization with no measurable benefit at the real workload scale,
where sleep/I/O time dominates (see D-note below). Documented as an accepted,
bounded limitation rather than a defect left open.

---

## D3 — Mutex overhead (cost of the fix)

| | |
|---|---|
| **Severity** | Informational (accepted trade-off) |
| **Group A** | No lock cost, but incorrect (see D1) |
| **Group B** | Correct, at a measured lock cost |

**Evidence.** `make bench-mutex`:
- Uncontended: **7.0 ns per critical section** (lock + op + unlock).
- Contended (4 threads, one shared counter): mutex-protected is **23× the wall
  time** of the unprotected path — but the unprotected path **loses 27.5M of
  40M updates (68.8%)**. Speed without correctness is worthless; this is the
  Group A race quantified.

**Conclusion.** The overhead buys correctness and is negligible against the real
firmware's runtime (see D-note).

---

## D4 — Deadlock / stall

| | |
|---|---|
| **Severity** | Critical (if present) |
| **Group A** | Not observed (no locks to deadlock on) |
| **Group B** | Not observed — `No deadlock detected` in all runs; detector tracks all 5 tasks |

**Coverage.** The watchdog keys on simulated time, covers all five tasks,
distinguishes normal completion from a stall, keeps sparse producers alive with a
liveness heartbeat, and runs a periodic silence-probe tick. No false positives on
task completion; a genuine mid-run stall is flagged (verified with a hang proxy).

---

## D5 — Priority inversion

| | |
|---|---|
| **Severity** | Major |
| **Status** | Instrumented and checked via mutex-wait times (`priority_inversion_test`) |

Low-priority tasks holding a mutex needed by a high-priority task are detected
from the recorded wait times; `priority_inversion_test` pins tasks to one core to
reproduce the contention scenario.

---

## D-note — Why end-to-end DUT timing does not show D2/D3

The standalone firmware runs in ≈10.2 s (A) vs ≈10.25 s (B) — indistinguishable —
because wall time is dominated by the tasks' random `hal_delay_ms` sleeps. This is
expected and is *why* false sharing (D2) and mutex cost (D3) are quantified with
isolated microbenchmarks rather than read off the full run.
