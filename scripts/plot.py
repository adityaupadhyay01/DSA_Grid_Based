"""Reads results.csv and draws the comparison charts.

Python is used only for plotting. It never touches the measured path, so the
timings stay a property of the C++ code rather than of the interpreter.

    python3 scripts/plot.py
"""
import csv
import statistics as st
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

STRUCTS = ["unsorted_array", "binary_heap", "bucket_queue"]
LABELS = {"unsorted_array": "Unsorted array  O(n²)",
          "binary_heap": "Binary heap  O(n log n)",
          "bucket_queue": "Bucket queue  O(n)"}


def load(path="results.csv"):
    rows = list(csv.DictReader(open(path)))
    sizes = sorted({int(r["grid"]) for r in rows})
    data = {}
    for s in STRUCTS:
        data[s] = {}
        for sz in sizes:
            vals = [float(r["runtime_ms"]) for r in rows
                    if r["structure"] == s and int(r["grid"]) == sz]
            if vals:
                data[s][sz] = (st.median(vals),
                               min(vals), max(vals))
    return sizes, data, rows


def chart_runtime(sizes, data):
    fig, ax = plt.subplots(figsize=(7, 4.5))
    for s in STRUCTS:
        xs = sorted(data[s])
        if not xs:
            continue
        ys = [data[s][x][0] for x in xs]
        lo = [data[s][x][0] - data[s][x][1] for x in xs]
        hi = [data[s][x][2] - data[s][x][0] for x in xs]
        ax.errorbar(xs, ys, yerr=[lo, hi], marker="o",
                    capsize=3, label=LABELS[s])
    ax.set_xscale("log", base=2)
    ax.set_yscale("log")
    ax.set_xticks(sizes)
    ax.set_xticklabels([f"{s}x{s}" for s in sizes])
    ax.set_xlabel("Grid size")
    ax.set_ylabel("Planning time (ms, median, log scale)")
    ax.set_title("OPEN-list structure vs planning time")
    ax.grid(True, which="both", alpha=0.3)
    ax.legend()
    fig.tight_layout()
    fig.savefig("runtime.png", dpi=150)
    print("wrote runtime.png")


def chart_expansions(rows):
    sizes = sorted({int(r["grid"]) for r in rows})
    fig, ax = plt.subplots(figsize=(7, 4.5))
    width = 0.27
    for i, s in enumerate(STRUCTS):
        xs, ys = [], []
        for j, sz in enumerate(sizes):
            vals = [int(r["expansions"]) for r in rows
                    if r["structure"] == s and int(r["grid"]) == sz]
            if vals:
                xs.append(j + (i - 1) * width)
                ys.append(st.median(vals))
        ax.bar(xs, ys, width, label=LABELS[s])
    ax.set_xticks(range(len(sizes)))
    ax.set_xticklabels([f"{s}x{s}" for s in sizes])
    ax.set_ylabel("Node expansions (median)")
    ax.set_title("Expansions are algorithmic, not machine-dependent")
    ax.grid(True, axis="y", alpha=0.3)
    ax.legend()
    fig.tight_layout()
    fig.savefig("expansions.png", dpi=150)
    print("wrote expansions.png")


if __name__ == "__main__":
    sizes, data, rows = load()
    chart_runtime(sizes, data)
    chart_expansions(rows)
