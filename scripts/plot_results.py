#!/usr/bin/env python3
# ============================================================
# plot_results.py — Render stress-result charts as SVG (Step 6)
#
# Reads results/stress_results.csv (produced by collect_results.py)
# and writes vector charts to results/plots/:
#   - torn_reads_per_run.svg : per-run torn reads, Group A vs B
#   - failure_rate.svg       : failure rate (%), Group A vs B
#   - means.svg              : mean torn reads & mean mismatches, A vs B
#
# Pure standard library (csv + hand-written SVG) — no matplotlib,
# no numpy, no network. SVG is vector, so it scales cleanly in the
# final report.
# ============================================================
import csv
import html
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CSV_PATH = os.path.join(ROOT, "results", "stress_results.csv")
PLOTDIR = os.path.join(ROOT, "results", "plots")

# Colour-blind-safe pair (Group A = vulnerable/red, Group B = protected/teal)
COL_A = "#d1495b"
COL_B = "#00798c"
COL_AXIS = "#333333"
COL_GRID = "#dddddd"
COL_TEXT = "#222222"


def esc(s):
    return html.escape(str(s), quote=True)


def svg_header(w, h, title):
    return (
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" '
        f'viewBox="0 0 {w} {h}" font-family="sans-serif">\n'
        f'<rect width="{w}" height="{h}" fill="white"/>\n'
        f'<text x="{w/2:.0f}" y="26" text-anchor="middle" font-size="18" '
        f'font-weight="bold" fill="{COL_TEXT}">{esc(title)}</text>\n'
    )


def axes(x0, y0, x1, y1):
    """x-axis and y-axis lines (plot area corners)."""
    return (
        f'<line x1="{x0}" y1="{y1}" x2="{x1}" y2="{y1}" stroke="{COL_AXIS}" stroke-width="1.5"/>\n'
        f'<line x1="{x0}" y1="{y0}" x2="{x0}" y2="{y1}" stroke="{COL_AXIS}" stroke-width="1.5"/>\n'
    )


def y_gridlines(x0, x1, y0, y1, ymax, steps=5):
    out = ""
    for i in range(steps + 1):
        frac = i / steps
        y = y1 - frac * (y1 - y0)
        val = ymax * frac
        out += f'<line x1="{x0}" y1="{y:.1f}" x2="{x1}" y2="{y:.1f}" stroke="{COL_GRID}" stroke-width="1"/>\n'
        label = f"{val:.0f}" if ymax >= 5 else f"{val:.1f}"
        out += (f'<text x="{x0-8}" y="{y+4:.1f}" text-anchor="end" font-size="11" '
                f'fill="{COL_TEXT}">{esc(label)}</text>\n')
    return out


def legend(x, y, items):
    out = ""
    for i, (label, colour) in enumerate(items):
        yy = y + i * 20
        out += f'<rect x="{x}" y="{yy}" width="14" height="14" fill="{colour}"/>\n'
        out += (f'<text x="{x+20}" y="{yy+12}" font-size="12" fill="{COL_TEXT}">'
                f'{esc(label)}</text>\n')
    return out


def write_svg(path, body):
    with open(path, "w") as fh:
        fh.write(body + "</svg>\n")
    print(f"[ok] {os.path.relpath(path, ROOT)}")


# ---- Chart 1: torn reads per run (grouped bars A vs B) ----
def chart_torn_per_run(a, b):
    n = max(len(a), len(b))
    W, H = max(720, 60 + n * 22), 380
    x0, y0, x1, y1 = 60, 50, W - 30, H - 60
    ymax = max([r["torn_reads"] for r in a + b] + [1])
    ymax = ymax + (1 if ymax % 2 else 0)

    s = svg_header(W, H, "Torn reads per run — Group A (vulnerable) vs B (protected)")
    s += y_gridlines(x0, x1, y0, y1, ymax)
    s += axes(x0, y0, x1, y1)

    slot = (x1 - x0) / n
    bw = min(9, slot / 2 - 1)

    def bars(rows, colour, offset):
        out = ""
        for i, r in enumerate(rows):
            cx = x0 + i * slot + slot / 2 + offset
            hgt = (r["torn_reads"] / ymax) * (y1 - y0)
            out += (f'<rect x="{cx-bw/2:.1f}" y="{y1-hgt:.1f}" width="{bw:.1f}" '
                    f'height="{hgt:.1f}" fill="{colour}"/>\n')
        return out

    s += bars(a, COL_A, -bw / 2 - 0.5)
    s += bars(b, COL_B, bw / 2 + 0.5)
    s += (f'<text x="{(x0+x1)/2:.0f}" y="{H-18}" text-anchor="middle" font-size="12" '
          f'fill="{COL_TEXT}">run index (1..{n})</text>\n')
    s += (f'<text x="18" y="{(y0+y1)/2:.0f}" text-anchor="middle" font-size="12" '
          f'fill="{COL_TEXT}" transform="rotate(-90 18 {(y0+y1)/2:.0f})">torn reads</text>\n')
    s += legend(x1 - 150, y0, [("Group A", COL_A), ("Group B", COL_B)])
    return s


# ---- generic grouped-bar summary (few categories) ----
def chart_summary(title, ylabel, categories, series_a, series_b, ymax, fmt="{:.0f}"):
    W, H = 560, 380
    x0, y0, x1, y1 = 70, 50, W - 30, H - 70
    s = svg_header(W, H, title)
    s += y_gridlines(x0, x1, y0, y1, ymax)
    s += axes(x0, y0, x1, y1)

    ncat = len(categories)
    slot = (x1 - x0) / ncat
    bw = min(60, slot / 3)

    for i, cat in enumerate(categories):
        base = x0 + i * slot + slot / 2
        for val, colour, off in ((series_a[i], COL_A, -bw / 2 - 2),
                                 (series_b[i], COL_B, bw / 2 + 2)):
            hgt = (val / ymax) * (y1 - y0) if ymax else 0
            bx = base + off - bw / 2
            s += (f'<rect x="{bx:.1f}" y="{y1-hgt:.1f}" width="{bw:.1f}" '
                  f'height="{hgt:.1f}" fill="{colour}"/>\n')
            s += (f'<text x="{bx+bw/2:.1f}" y="{y1-hgt-6:.1f}" text-anchor="middle" '
                  f'font-size="11" fill="{COL_TEXT}">{esc(fmt.format(val))}</text>\n')
        s += (f'<text x="{base:.1f}" y="{y1+18}" text-anchor="middle" font-size="12" '
              f'fill="{COL_TEXT}">{esc(cat)}</text>\n')

    s += (f'<text x="20" y="{(y0+y1)/2:.0f}" text-anchor="middle" font-size="12" '
          f'fill="{COL_TEXT}" transform="rotate(-90 20 {(y0+y1)/2:.0f})">{esc(ylabel)}</text>\n')
    s += legend(x1 - 150, y0, [("Group A", COL_A), ("Group B", COL_B)])
    return s


def load():
    if not os.path.isfile(CSV_PATH):
        print(f"[error] {os.path.relpath(CSV_PATH, ROOT)} not found. "
              f"Run: make stress && make collect", file=sys.stderr)
        sys.exit(1)
    a, b = [], []
    with open(CSV_PATH) as fh:
        for row in csv.DictReader(fh):
            rec = {k: int(row[k]) for k in
                   ("run", "mismatches", "torn_reads", "deadlock", "pass")}
            (a if row["group"] == "a" else b).append(rec)
    a.sort(key=lambda r: r["run"])
    b.sort(key=lambda r: r["run"])
    return a, b


def mean(xs):
    return sum(xs) / len(xs) if xs else 0.0


def main():
    a, b = load()
    if not a and not b:
        print("[error] no rows in CSV", file=sys.stderr)
        return 1
    os.makedirs(PLOTDIR, exist_ok=True)

    write_svg(os.path.join(PLOTDIR, "torn_reads_per_run.svg"),
              chart_torn_per_run(a, b))

    # Authoritative race metric: fraction of runs with >=1 torn read.
    fail_a = 100.0 * sum(1 for r in a if r["torn_reads"] > 0) / len(a) if a else 0
    fail_b = 100.0 * sum(1 for r in b if r["torn_reads"] > 0) / len(b) if b else 0
    write_svg(os.path.join(PLOTDIR, "failure_rate.svg"),
              chart_summary("Torn-read failure rate — Group A vs B", "runs with torn read (%)",
                            ["torn-read rate"], [fail_a], [fail_b], 100.0, "{:.0f}%"))

    means_a = [mean([r["torn_reads"] for r in a]), mean([r["mismatches"] for r in a])]
    means_b = [mean([r["torn_reads"] for r in b]), mean([r["mismatches"] for r in b])]
    ymax = max(means_a + means_b + [1])
    ymax += ymax * 0.15
    write_svg(os.path.join(PLOTDIR, "means.svg"),
              chart_summary("Mean defects per run — Group A vs B", "mean per run",
                            ["torn reads", "mismatches"], means_a, means_b, ymax, "{:.2f}"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
