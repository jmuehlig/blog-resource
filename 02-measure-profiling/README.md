# 02-measure-profiling

Benchmark measuring hardware performance counter overhead across working set sizes.

## Build

```bash
cmake -DCMAKE_BUILD_TYPE=Release .
make
```

## Run

The hardware `cycles` counter counts actual core cycles; rdtsc counts reference cycles at the
fixed base frequency. When turbo boost is active they diverge by the turbo multiplier and are
not directly comparable. Disable turbo before running:

```bash
# Intel
echo 1 | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo

# AMD
echo 0 | sudo tee /sys/devices/system/cpu/cpufreq/boost
```

Also pin to a single core to reduce scheduling noise:

```bash
taskset -c 2 ./random-access-bench
```

Restore turbo afterwards:

```bash
# Intel
echo 0 | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo
```

Writes results to `result.csv`.

### Convergence of cycle measurements

With turbo disabled, all three methods (EventCounter, LiveEventCounter, rdtsc) converge once
measurement overhead is amortized by sufficient work:

- **Sequential access**: converge at ~8 MB working set
- **Random access**: converge at ~32–64 MB (DRAM latency dominates, overhead negligible)

Below those thresholds the ioctl overhead of EventCounter and the cpuid/lfence/rdpmc overhead
of LiveEventCounter inflate their cycle counts relative to rdtsc.

## Plot

```bash
python3 scripts/plot.py
```

Produces `random_access.pdf` and `sequential_access.pdf`.
