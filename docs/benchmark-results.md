# Pricing benchmark results

The table below records a local run on 26 October 2026 with Apple Clang 21.0.0,
a CMake Release build, and Google Benchmark 1.9.5. It is historical local
evidence, not a GitHub Actions result; the Actions log and downloadable JSON
artifact should be used to verify future CI runs.

Each entry is the median CPU time of three repetitions. The two price evaluators
receive the same synthetic contract: face value 1000, coupon rate 0.05, and two
payments per year. Parsing, duration calculations, yield inversion, and output
are outside the timed region. The code checks price agreement before timing.

| Periods | Annual yield | Direct (ns) | Horner (ns) | Direct / Horner |
|---:|---:|---:|---:|---:|
| 2 | 0.02 | 14.90 | 4.04 | 3.69x |
| 2 | 0.08 | 15.01 | 4.05 | 3.71x |
| 10 | 0.02 | 52.67 | 9.39 | 5.61x |
| 10 | 0.08 | 52.60 | 9.39 | 5.60x |
| 40 | 0.02 | 198.48 | 44.10 | 4.50x |
| 40 | 0.08 | 198.92 | 44.12 | 4.51x |
| 100 | 0.02 | 481.70 | 152.12 | 3.17x |
| 100 | 0.08 | 480.49 | 151.71 | 3.17x |
| 1000 | 0.02 | 4720.75 | 2532.15 | 1.86x |
| 1000 | 0.08 | 4726.08 | 2527.16 | 1.87x |

Horner was faster in this local run because it replaces repeated power calls
with additions and divisions. Both algorithms remain O(n) in payment count and
O(1) in extra space. A speed ratio is a measurement for this compiler and machine,
not a portable guarantee. The 1000-period input is a stress workload rather than
a claim about typical bond maturities.

The experiment has 20 workloads, each measured three times: 60 raw records plus
80 aggregate records (mean, median, standard deviation, coefficient of variation),
for 140 JSON entries. The result checker rejects missing or duplicated workloads,
bad repetition indices, failed runs, and nonfinite timing data.

Reproduce using `cmake --build build --config Release --target run_benchmarks`.
The console shows the timings and the validator result; `build/benchmark.json`
contains the current run. Keep each platform's results separately.
