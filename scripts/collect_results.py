#!/usr/bin/env python3
# ============================================================
# collect_results.py — Aggregate stress-run results (Step 6)
#
# Parses every results/logs/stress_{a,b}_runNNN.log produced by
# run_stress.sh, extracts the scoreboard metrics, writes a per-run
# CSV (results/stress_results.csv) and a human-readable aggregate
# summary (printed + results/stress_summary.md) for the report.
#
# Metrics per run (from the UVM scoreboard report):
#   - mismatches       : reference-model disagreements
#   - torn_reads       : incoherent (temp,humidity) pairs (Group A race)
#   - deadlock         : 1 if the detector flagged a stall, else 0
#   - pass             : 1 iff mismatches==0 and torn_reads==0 and no deadlock
#
# Stdlib only (re, csv, glob, statistics) — no external dependencies.
# ============================================================
import csv
import glob
import os
import re
import statistics as stats
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LOGDIR = os.path.join(ROOT, "results", "logs")
CSV_PATH = os.path.join(ROOT, "results", "stress_results.csv")
MD_PATH = os.path.join(ROOT, "results", "stress_summary.md")

RE_NAME = re.compile(r"stress_([ab])_run(\d+)\.log$")
RE_MISMATCH = re.compile(r"Final mismatches:\s*(\d+)\s*\(out of\s*(\d+)\s*actuator checks\)")
RE_TORN = re.compile(r"Torn reads:\s*(\d+)\s*\(out of\s*(\d+)\s*control-input coherence checks")
RE_DEADLOCK_FAIL = re.compile(r"Deadlock/stall detected")


def parse_log(path):
    """Return a metrics dict for one run, or None if it lacks a scoreboard report."""
    with open(path, "r", errors="replace") as fh:
        text = fh.read()

    m_mis = RE_MISMATCH.search(text)
    m_torn = RE_TORN.search(text)
    if not m_mis or not m_torn:
        return None  # incomplete/crashed run — skip

    mismatches = int(m_mis.group(1))
    actuator_checks = int(m_mis.group(2))
    torn_reads = int(m_torn.group(1))
    coherence_checks = int(m_torn.group(2))
    deadlock = 1 if RE_DEADLOCK_FAIL.search(text) else 0
    passed = 1 if (mismatches == 0 and torn_reads == 0 and deadlock == 0) else 0

    return {
        "mismatches": mismatches,
        "actuator_checks": actuator_checks,
        "torn_reads": torn_reads,
        "coherence_checks": coherence_checks,
        "deadlock": deadlock,
        "pass": passed,
    }


def collect():
    rows = []
    for path in sorted(glob.glob(os.path.join(LOGDIR, "stress_*_run*.log"))):
        name = RE_NAME.search(os.path.basename(path))
        if not name:
            continue
        metrics = parse_log(path)
        if metrics is None:
            print(f"[warn] skipping incomplete log: {os.path.basename(path)}", file=sys.stderr)
            continue
        rows.append({"group": name.group(1), "run": int(name.group(2)), **metrics})
    return rows


def write_csv(rows):
    fields = ["group", "run", "mismatches", "actuator_checks",
              "torn_reads", "coherence_checks", "deadlock", "pass"]
    with open(CSV_PATH, "w", newline="") as fh:
        w = csv.DictWriter(fh, fieldnames=fields)
        w.writeheader()
        w.writerows(rows)


def summarize_group(rows, group):
    g = [r for r in rows if r["group"] == group]
    n = len(g)
    if n == 0:
        return None
    torn = [r["torn_reads"] for r in g]
    mism = [r["mismatches"] for r in g]
    fails = sum(1 for r in g if r["pass"] == 0)
    deadlocks = sum(r["deadlock"] for r in g)
    runs_with_torn = sum(1 for t in torn if t > 0)
    return {
        "n": n,
        "fail_rate": 100.0 * fails / n,
        "runs_with_torn": runs_with_torn,
        "torn_mean": stats.mean(torn),
        "torn_max": max(torn),
        "mism_mean": stats.mean(mism),
        "mism_max": max(mism),
        "deadlocks": deadlocks,
    }


def fmt_group(label, s):
    if s is None:
        return f"{label}: (no runs)\n"
    return (
        f"{label}  (n={s['n']} runs)\n"
        f"  failure rate     : {s['fail_rate']:.1f}%  "
        f"({s['runs_with_torn']}/{s['n']} runs with >=1 torn read)\n"
        f"  torn reads       : mean {s['torn_mean']:.2f}, max {s['torn_max']}\n"
        f"  mismatches       : mean {s['mism_mean']:.2f}, max {s['mism_max']}\n"
        f"  deadlocks flagged: {s['deadlocks']}\n"
    )


def main():
    if not os.path.isdir(LOGDIR):
        print(f"[error] no log dir at {LOGDIR}. Run scripts/run_stress.sh first.", file=sys.stderr)
        return 1
    rows = collect()
    if not rows:
        print("[error] no parseable stress logs found. Run scripts/run_stress.sh first.", file=sys.stderr)
        return 1

    write_csv(rows)
    a = summarize_group(rows, "a")
    b = summarize_group(rows, "b")

    report = (
        "=== Concurrency Stress Summary ===\n\n"
        + fmt_group("Group A (vulnerable, race_condition_test)", a)
        + "\n"
        + fmt_group("Group B (protected, protected_test)", b)
    )
    print(report)
    print(f"[ok] per-run CSV : {os.path.relpath(CSV_PATH, ROOT)}")

    # Markdown for the final report
    with open(MD_PATH, "w") as fh:
        fh.write("# Concurrency Stress Summary\n\n")
        fh.write("| Metric | Group A (vulnerable) | Group B (protected) |\n")
        fh.write("|---|---|---|\n")
        if a and b:
            fh.write(f"| Runs | {a['n']} | {b['n']} |\n")
            fh.write(f"| Failure rate | {a['fail_rate']:.1f}% | {b['fail_rate']:.1f}% |\n")
            fh.write(f"| Runs with >=1 torn read | {a['runs_with_torn']}/{a['n']} | {b['runs_with_torn']}/{b['n']} |\n")
            fh.write(f"| Torn reads (mean / max) | {a['torn_mean']:.2f} / {a['torn_max']} | {b['torn_mean']:.2f} / {b['torn_max']} |\n")
            fh.write(f"| Mismatches (mean / max) | {a['mism_mean']:.2f} / {a['mism_max']} | {b['mism_mean']:.2f} / {b['mism_max']} |\n")
            fh.write(f"| Deadlocks flagged | {a['deadlocks']} | {b['deadlocks']} |\n")
    print(f"[ok] summary md  : {os.path.relpath(MD_PATH, ROOT)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
