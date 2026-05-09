import sys
import re
import argparse
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

parser = argparse.ArgumentParser()
parser.add_argument("csv", nargs="?", default="result.csv")
parser.add_argument("--all", action="store_true", dest="all_data")
args = parser.parse_args()

csv_path = args.csv

MACHINE_NAMES = {
    "skylake": "Intel Skylake",
    "zen4": "AMD Zen4",
    "sapphire-rapids": "Intel Sapphire Rapids",
}

# Extract prefix from filenames like "result-skylake.csv" -> "skylake-"
match = re.match(r"result-(.+)\.csv", csv_path.split("/")[-1])
prefix = f"{match.group(1)}-" if match else ""
if match:
    key = match.group(1)
    machine_name = MACHINE_NAMES.get(key, key)
else:
    machine_name = csv_path.split("/")[-1].replace("result-", "").replace(".csv", "")

df = pd.read_csv(csv_path)

# Average over runs for each (is_random, measure_type, kb) group,
# excluding run 0 (cold start).
df = df[df["run"] > 0]
df = df.groupby(["is_random", "measure_type", "kb"], as_index=False).mean(numeric_only=True)

if not args.all_data:
    df = df[df["is_random"] == 1]       # random access only
    df = df[df["kb"] <= 32 * 1024]      # up to 32 MB

ASSEMBLY_BASELINE = 6  # instructions per iteration from disassembly
L1D_BASELINE = 2       # L1-dcache-loads per iteration
BRANCHES_BASELINE = 1  # branches per iteration (one loop back-edge)

MEASURE_TYPES_CYCLES = ["EventCounter", "LiveEventCounter", "cycles"]
MEASURE_TYPES_INSTRUCTIONS = ["EventCounter", "LiveEventCounter"]
COLORS = {"EventCounter": "#4C72B0", "LiveEventCounter": "#DD8452", "cycles": "#55A868"}
LABELS = {"EventCounter": r"EventCounter ($\mathbf{ioctl}$)", "LiveEventCounter": r"LiveEventCounter ($\mathbf{rdpmc}$)", "cycles": r"$\mathbf{rdtsc}$"}

KB_TO_CACHE_LINES = (
    df[["kb", "cache_lines"]].drop_duplicates().set_index("kb")["cache_lines"].astype(int).to_dict()
)


def kb_label(kb, count_baseline=ASSEMBLY_BASELINE, count_unit="instr."):
    if kb >= 1024 * 1024:
        size = f"{kb // (1024 * 1024)}GB"
    elif kb >= 1024:
        size = f"{kb // 1024}MB"
    else:
        size = f"{kb}KB"

    total = KB_TO_CACHE_LINES[kb] * count_baseline
    if total >= 1_000_000:
        count = f"{total / 1_000_000:.0f}M"
    elif total >= 1_000:
        count = f"{total / 1_000:.0f}k"
    else:
        count = str(total)

    return f"{size} / {count} {count_unit}"


def make_plot(is_random, metric, ylabel, measure_types, filename, baseline=None, baseline_label=None, x_label_fn=None, title=None):
    if metric not in df.columns:
        print(f"Skipping {filename}: column '{metric}' not in data")
        return
    subset = df[df["is_random"] == is_random]
    kb_values = sorted(subset["kb"].unique())
    x = np.arange(len(kb_values))
    n = len(measure_types)
    width = 0.32
    label_fn = x_label_fn if x_label_fn is not None else kb_label
    x_labels = [label_fn(kb) for kb in kb_values]

    fig, ax = plt.subplots(figsize=(16, 7))

    for i, mtype in enumerate(measure_types):
        rows = subset[subset["measure_type"] == mtype]
        values = []
        for kb in kb_values:
            row = rows[rows["kb"] == kb]
            values.append(row[metric].values[0] if not row.empty and not row[metric].isna().all() else 0)

        offset = (i - n / 2 + 0.5) * width
        ax.bar(x + offset, values, width, label=LABELS[mtype], color=COLORS[mtype], alpha=0.85)

    if baseline is not None:
        ax.axhline(baseline, color="crimson", linewidth=1.5, linestyle="--", label=baseline_label)

    ax.set_ylabel(ylabel)
    ax.set_xlabel("Working set size")
    if title:
        ax.set_title(title, fontsize=19, fontweight="bold")
    ax.set_xticks(x)
    ax.set_xticklabels(x_labels, rotation=45, ha="right", fontsize=15)
    ax.set_ylim(bottom=0)
    ax.tick_params(axis="y", labelsize=17)
    ax.xaxis.label.set_size(16)
    ax.yaxis.label.set_size(18)
    ax.legend(fontsize=17, framealpha=0)
    ax.grid(axis="y", linestyle="--", alpha=0.4)
    ax.set_axisbelow(True)

    fig.tight_layout()
    fig.savefig(filename, bbox_inches="tight", transparent=True)
    print(f"Written {filename}")


print(df.to_string())

access_types = [(1, "random_access"), (0, "sequential_access")] if args.all_data else [(1, "random_access")]

for is_random, name in access_types:
    make_plot(
        is_random=is_random,
        metric="cycles_per_cache_line",
        ylabel="measured cycles / iteration",
        measure_types=MEASURE_TYPES_CYCLES,
        filename=f"plots/{prefix}{name}_cycles.svg",
        title=f"cycles – {machine_name}",
    )
    make_plot(
        is_random=is_random,
        metric="instructions_per_cache_line",
        ylabel="measured instructions / iteration",
        measure_types=MEASURE_TYPES_INSTRUCTIONS,
        filename=f"plots/{prefix}{name}_instructions.svg",
        baseline=ASSEMBLY_BASELINE,
        baseline_label=f"Baseline ({ASSEMBLY_BASELINE} instructions/iteration)",
        title=f"instructions – {machine_name}",
    )
    make_plot(
        is_random=is_random,
        metric="L1-dcache-loads_per_cache_line",
        ylabel="measured L1-dcache-loads / iteration",
        measure_types=MEASURE_TYPES_INSTRUCTIONS,
        filename=f"plots/{prefix}{name}_l1_dcache_loads.svg",
        baseline=L1D_BASELINE,
        baseline_label=f"Baseline ({L1D_BASELINE} L1d loads/iteration)",
        x_label_fn=lambda kb: kb_label(kb, count_baseline=L1D_BASELINE, count_unit="loads"),
        title=f"L1-dcache-loads – {machine_name}",
    )
    make_plot(
        is_random=is_random,
        metric="branches_per_cache_line",
        ylabel="measured branches / iteration",
        measure_types=MEASURE_TYPES_INSTRUCTIONS,
        filename=f"plots/{prefix}{name}_branches.svg",
        baseline=BRANCHES_BASELINE,
        baseline_label=f"Baseline ({BRANCHES_BASELINE} branch/iteration)",
        x_label_fn=lambda kb: kb_label(kb, count_baseline=BRANCHES_BASELINE, count_unit="branches"),
        title=f"branches – {machine_name}",
    )
