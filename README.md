# Bond Analytics in C++

A C++17 command-line calculator for fixed-rate bullet bonds. It computes price,
Macaulay duration, modified duration, convexity, and yield to maturity. Two
independent price evaluators use direct discounting and Horner evaluation.
The project includes CMake, doctest, Google Benchmark, Doxygen, and GitHub Actions.

Clone this public repository and follow the build commands below. To publish a
separate copy under another GitHub account, see [Publishing](docs/publishing.md);
the helper script refuses to change private visibility or force-push history.

<!-- repository-links:start -->
**Repository:** [source](https://github.com/z18170371693-art/bond-analytics-cpp) | [Actions](https://github.com/z18170371693-art/bond-analytics-cpp/actions)

**Documentation:** [Doxygen](https://z18170371693-art.github.io/bond-analytics-cpp/). This address requires a successful Pages deployment; check the actual workflow before submitting.
<!-- repository-links:end -->

## Build and run

Requirements: a C++17 compiler, CMake 3.24 or later, a build tool, Python 3.9 or
later, and Doxygen. The pinned doctest and Google Benchmark release archives are
downloaded from upstream tags and verified by SHA-256 during CMake configuration.

Linux/macOS:

```sh
bash scripts/verify.sh
./build/bin/bond_analytics --face 1000 --coupon 0.05 --periods 10 --frequency 2 --yield 0.04
```

Windows, from a PowerShell environment with the Visual Studio C++ build tools:

```powershell
./scripts/verify.ps1
./build/bin/bond_analytics.exe --face 1000 --coupon 0.05 --periods 10 --frequency 2 --yield 0.04
```

The scripts configure a Release build, compile, run CTest, benchmark both price
methods, validate the measurement JSON, generate Doxygen, and check its local
links. Any failing command stops the script. An optional first argument selects
the build directory.

The example represents a five-year bond paying twice a year, with face value
1000, coupon rate 5%, and nominal annual yield 4%. Approximate output values:

| Metric | Value |
|---|---:|
| Price | 1044.912925031211 |
| Macaulay duration (years) | 4.498903644526 |
| Modified duration | 4.410689847575 |
| Convexity | 22.923145625342 |

Rates are decimals: `0.04` means 4%. `--periods 10` means ten remaining coupon
payments, not ten years. The CLI prints a `metric,value` CSV to standard output.
Use `--help` for all options, or `--method direct` to select the other evaluator.

To recover yield from an observed coupon-date price:

```sh
./build/bin/bond_analytics --target-price 1044.912925031211 --lower -0.5 --upper 1.0
```

The implied annual yield should be approximately `0.04`. A price outside the
explicit bracket, beyond the endpoint residual tolerance, causes a reported
error rather than an invented solution.

## Separate checks and outputs

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
cmake --build build --config Release --target run_benchmarks
cmake --build build --config Release --target docs
```

The executables are in `build/bin/`; benchmark measurements are in
`build/benchmark.json`. Open `build/docs/html/index.html` for the generated API
documentation. The benchmark workflow prints the measurement summary in its
CI log and preserves `benchmark.json` as an artifact.

## Structure and reading order

| Path | Purpose |
|---|---|
| `include/bond/analytics.hpp` | Public contracts and units |
| `src/analytics.cpp` | Pricing, risk measures, and yield solver |
| `src/main.cpp` | Validated arguments and CSV output |
| `tests/` | C++ cases and an independent Python Decimal oracle |
| `benchmarks/` | Direct discounting versus Horner evaluation |
| `scripts/` | Full verification and result validators |
| `docs/` | Financial model, algorithm explanation, and publishing |
| `.github/workflows/` | Platform matrix and Pages deployment |
| `third_party/` | Pinned dependencies and their licenses |

Read the [model](docs/model.md), the [algorithm walkthrough](docs/algorithms.md),
the [measured benchmark report](docs/benchmark-results.md), and the
[six-item acceptance checklist](docs/acceptance.md).

Valuation is immediately after a coupon payment, with the next payment one full
period away. Cash flows are certain and discounted at one nominal annual yield.
Irregular dates, accrued interest, default, callable bonds, and a yield curve are
outside this model. All examples and workloads are synthetic, not market quotes.
