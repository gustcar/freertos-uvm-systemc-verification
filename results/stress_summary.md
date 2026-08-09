# Concurrency Stress Summary

| Metric | Group A (vulnerable) | Group B (protected) |
|---|---|---|
| Runs | 30 | 30 |
| **Torn-read failure rate** (authoritative) | 100.0% | 0.0% |
| Runs with >=1 torn read | 30/30 | 0/30 |
| Mismatch rate (weak reconstruction check) | 100.0% | 3.3% |
| Torn reads (mean / max) | 8.87 / 22 | 0.00 / 0 |
| Mismatches (mean / max) | 11.53 / 34 | 0.03 / 1 |
| Deadlocks flagged | 0 | 0 |

> The **torn-read** check is the authoritative race detector (direct input coherence). Group B eliminates it entirely (0%). Group B's residual mismatches come from the *weak* output-reconstruction check (finite snapshot window), not real races — the same runs show zero torn reads.
