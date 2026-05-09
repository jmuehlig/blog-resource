import re
import argparse
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

MACHINE_NAMES = {
    "skylake": "Intel Skylake",
    "zen4": "AMD Zen4",
    "sapphire-rapids": "Intel Sapphire Rapids",
}

METRIC_MAP = {
    "cycles":        ("cycles_per_cache_line",           ["EventCounter", "LiveEventCounter", "cycles"]),
    "instructions":  ("instructions_per_cache_line",     ["EventCounter", "LiveEventCounter"]),
    "L1-dcache-loads": ("L1-dcache-loads_per_cache_line", ["EventCounter", "LiveEventCounter"]),
    "branches":      ("branches_per_cache_line",         ["EventCounter", "LiveEventCounter"]),
}

COLORS = {"EventCounter": "#4C72B0", "LiveEventCounter": "#DD8452", "cycles": "#55A868"}
LABELS = {
    "EventCounter":    r"EventCounter ($\mathbf{ioctl}$)",
    "LiveEventCounter": r"LiveEventCounter ($\mathbf{rdpmc}$)",
    "cycles":          r"$\mathbf{rdtsc}$",
}

parser = argparse.ArgumentParser()
parser.add_argument("csvs", nargs="+")
parser.add_argument("--metric", required=True, choices=METRIC_MAP.keys())
parser.add_argument("--kb", type=int, required=True)
parser.add_argument("--sequential", action="store_true")
args = parser.parse_args()

is_random = 0 if args.sequential else 1
access_label = "sequential_access" if args.sequential else "random_access"

metric_col, measure_types = METRIC_MAP[args.metric]


def kb_label(kb):
    if kb >= 1024 * 1024:
        return f"{kb // (1024 * 1024)}GB"
    if kb >= 1024:
        return f"{kb // 1024}MB"
    return f"{kb}KB"


def machine_info(csv_path):
    basename = csv_path.split("/")[-1]
    match = re.match(r"result-(.+)\.csv", basename)
    key = match.group(1) if match else basename.replace(".csv", "")
    name = MACHINE_NAMES.get(key, key)
    return key, name


machines = [machine_info(p) for p in args.csvs]
machine_keys = [k for k, _ in machines]
machine_names = [n for _, n in machines]

# Load and aggregate each CSV
data = {}
for csv_path, (key, name) in zip(args.csvs, machines):
    df = pd.read_csv(csv_path)
    df = df[df["run"] > 0]  # exclude run 0 (cold start)
    df = df.groupby(["is_random", "measure_type", "kb"], as_index=False).mean(numeric_only=True)
    df = df[(df["is_random"] == is_random) & (df["kb"] == args.kb)]
    data[key] = df

n_groups = len(machines)
n_bars = len(measure_types)
width = 0.32
group_spacing = n_bars * width + 0.3
x = np.arange(n_groups) * group_spacing

fig, ax = plt.subplots(figsize=(16, 7))

for i, mtype in enumerate(measure_types):
    values = []
    for key in machine_keys:
        df = data[key]
        row = df[df["measure_type"] == mtype]
        values.append(row[metric_col].values[0] if not row.empty and not row[metric_col].isna().all() else 0)
    offset = (i - n_bars / 2 + 0.5) * width
    ax.bar(x + offset, values, width, label=LABELS[mtype], color=COLORS[mtype], alpha=0.85)

title = f"{args.metric} @ {kb_label(args.kb)} – {' vs '.join(machine_names)}"
ax.set_title(title, fontsize=19, fontweight="bold")
ax.set_ylabel(f"measured {args.metric} / iteration", fontsize=18)
ax.set_xlabel("Architecture", fontsize=16)
ax.set_xticks(x)
ax.set_xticklabels(machine_names, fontsize=16)
ax.set_ylim(bottom=0)
ax.tick_params(axis="y", labelsize=17)
ax.legend(fontsize=17, framealpha=0)
ax.grid(axis="y", linestyle="--", alpha=0.4)
ax.set_axisbelow(True)

fig.tight_layout()

slug = "-".join(machine_keys)
outfile = f"plots/{slug}-{access_label}_{args.metric}_{kb_label(args.kb)}.svg"
fig.savefig(outfile, bbox_inches="tight", transparent=True)
print(f"Written {outfile}")
