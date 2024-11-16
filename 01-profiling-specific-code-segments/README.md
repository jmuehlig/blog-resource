# Profiling Specific Code Segments of Applications
## Build
```bash
cmake . 
make
```

## Execute
Run 
```bash
./bin/random-access-bench --size 16777216
```
to execute the benchmark with `1GB` data (`16777216` is the number of accessed cache lines).

Run 
```bash
./bin/random-access-bench --size 16777216 --perf
```
to utilize `perf-cpp` for counting hardware performance counters.
