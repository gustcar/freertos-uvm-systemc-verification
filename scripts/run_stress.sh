#!/usr/bin/env bash
# ============================================================
# run_stress.sh — Statistical stress harness (Step 6)
#
# Runs the UVM-SystemC testbench for Group A (race_condition_test)
# and Group B (protected_test) N times each, saving one log per run.
#
# Failure detection lives in the TESTBENCH scoreboard (torn reads,
# mismatches, deadlock) — not in the standalone DUT, which only
# prints final state. Because the tasks run as real pthreads, the
# interleaving is driven by OS scheduling, so repeated runs of the
# same binary already yield a genuine statistical distribution.
#
# This script only RUNS and stores logs; parsing/aggregation is done
# by collect_results.py (single source of truth for the metrics).
#
# Usage:  scripts/run_stress.sh [N]        (default N=20)
# ============================================================
set -uo pipefail

N="${1:-20}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/build"
LOGDIR="$ROOT/results/logs"

mkdir -p "$LOGDIR"

echo "[stress] Building testbenches..."
make -C "$ROOT" build-tb-a build-tb-b >/dev/null

# group label | binary | uvm test name
run_group() {
    local group="$1" bin="$2" test="$3"
    echo "[stress] Group ${group^^}: $N runs of '$test'"
    for ((r = 1; r <= N; r++)); do
        local log
        log="$LOGDIR/stress_${group}_run$(printf '%03d' "$r").log"
        # The TB exits non-zero when the scoreboard raises UVM_ERRORs
        # (expected for Group A) — do not let that abort the sweep.
        "$bin" "$test" > "$log" 2>&1 || true
        printf '\r[stress]   %s run %d/%d' "$group" "$r" "$N"
    done
    printf '\n'
}

# Fresh logs for this sweep
rm -f "$LOGDIR"/stress_a_run*.log "$LOGDIR"/stress_b_run*.log

start=$(date +%s)
run_group a "$BUILD/tb_a.elf" race_condition_test
run_group b "$BUILD/tb_b.elf" protected_test
end=$(date +%s)

echo "[stress] Done: $((2 * N)) runs in $((end - start))s. Logs in results/logs/"
echo "[stress] Next: python3 scripts/collect_results.py"
