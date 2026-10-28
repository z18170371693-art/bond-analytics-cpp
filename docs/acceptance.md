# Six-item acceptance checklist

This checklist distinguishes project implementation from verified CI/CD results.
The GitHub repository is public; the workflow must still complete successfully
before criteria 2-5 can be counted at full value.

| Assignment criterion | Points | Implementation and evidence | Public verification |
|---|---:|---|---|
| Public repository, structure, README | 2 | [Public source](https://github.com/z18170371693-art/bond-analytics-cpp), separated source, tests, benchmarks, and build instructions | Check the visible tree and README |
| CMake and automatic build | 4 | CMake library/CLI targets; Ubuntu, macOS, and Windows Actions workflow | Successful build jobs on GitHub |
| C++ unit tests in CI | 4 | doctest cases plus an independent Decimal CLI check registered with CTest | Successful CTest step on GitHub |
| Automatic benchmark and log results | 4 | Google Benchmark compares two price evaluators; timings print to CI logs and JSON is checked | Successful benchmark step and downloadable artifacts |
| Doxygen publication and README link | 4 | Doxygen target, local link checker, Pages workflow, and actual repository links in README | Enable Pages, deploy successfully, then open the published docs |
| Root agent configuration | 2 | `AGENTS.md` gives purpose, structure, commands, style, and constraints | Confirm the file is in the default branch |

The teacher awards full points for criteria 2-5 only after successful CI/CD
execution. A workflow file or a local build is not proof of a successful public
run. The deadline in the supplied assignment is 28 October 2026, 23:59 Moscow time.

For a fresh local check, run `bash scripts/verify.sh`, or use
`./scripts/verify.ps1` in Windows PowerShell with the required C++ toolchain.
The benchmark uses synthetic contracts with 2, 10, 40, 100, and 1000 periods,
two annual yields, and two algorithms. Measurements are identified by their
own machine context; do not compare different CI machines as if they were one.

Useful links: [repository](https://github.com/z18170371693-art/bond-analytics-cpp),
[Actions runs](https://github.com/z18170371693-art/bond-analytics-cpp/actions),
[expected Pages URL](https://z18170371693-art.github.io/bond-analytics-cpp/).
The Pages URL is not confirmed until the deployment succeeds.
