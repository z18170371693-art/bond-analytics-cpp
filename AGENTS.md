# Repository instructions

## Purpose and layout
This C++17 project values fixed-rate bullet bonds on a coupon payment date.
`include/bond/` contains the public API, `src/` its implementation and CLI,
`tests/` automated tests, `benchmarks/` timing experiments, `docs/` explanations,
and `scripts/` reproducible checks. Third-party sources are fetched from pinned
upstream tags and checked by SHA-256 in `CMakeLists.txt`; network access is
required on first configure.

## Commands
- Configure: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`
- Build: `cmake --build build --config Release --parallel 2`
- Test: `ctest --test-dir build -C Release --output-on-failure`
- Benchmark: `cmake --build build --config Release --target run_benchmarks`
- Documentation: `cmake --build build --config Release --target docs`
- Complete Linux/macOS check: `bash scripts/verify.sh`
- Example: `./build/bin/bond_analytics --face 1000 --coupon 0.05 --periods 10 --frequency 2 --yield 0.04`

## Conventions and constraints
Use C++17, explicit units, small functions, and Doxygen comments on public APIs.
Express annual rates as decimals; coupon frequency converts them to period rates.
Use actual executed checks as evidence. Do not claim public deployment or CI
success based on a local build. Keep timing measurements identified by platform.
Build and run tests before completing a code change; use numerical oracle checks
when pricing logic changes. Preserve third-party licenses. Do not alter workflow
permissions, publish to a remote account, or change release tags without a request.
Do not edit or copy the unrelated Market Data Toolkit implementation.
