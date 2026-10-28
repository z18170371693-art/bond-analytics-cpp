#include "bond/analytics.hpp"
#include <benchmark/benchmark.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
constexpr int periods[] = {2, 10, 40, 100, 1000};
constexpr int yield_basis_points[] = {200, 800};

void workloads(benchmark::internal::Benchmark* experiment) {
    for (int count : periods) {
        for (int yield_bp : yield_basis_points) {
            experiment->Args({count, yield_bp});
        }
    }
    experiment->ArgNames({"periods", "yield_bp"});
}

void BM_Direct(benchmark::State& state) {
    const bond::Bond contract{1000.0, 0.05,
                              static_cast<std::size_t>(state.range(0)), 2};
    const double yield = static_cast<double>(state.range(1)) / 10000.0;
    for (auto iteration : state) {
        (void)iteration;
        const double result = bond::price_direct(contract, yield);
        benchmark::DoNotOptimize(result);
    }
}

void BM_Horner(benchmark::State& state) {
    const bond::Bond contract{1000.0, 0.05,
                              static_cast<std::size_t>(state.range(0)), 2};
    const double yield = static_cast<double>(state.range(1)) / 10000.0;
    for (auto iteration : state) {
        (void)iteration;
        const double result = bond::price_horner(contract, yield);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_Direct)->Apply(workloads);
BENCHMARK(BM_Horner)->Apply(workloads);
} // namespace

int main(int argc, char** argv) {
    try {
        // Check comparable outputs before timing starts.
        for (int count : periods) {
            for (int yield_bp : yield_basis_points) {
                const bond::Bond contract{1000.0, 0.05,
                                          static_cast<std::size_t>(count), 2};
                const double yield = static_cast<double>(yield_bp) / 10000.0;
                const double a = bond::price_direct(contract, yield);
                const double b = bond::price_horner(contract, yield);
                if (std::abs(a - b) > 1e-10 * std::max(1.0, std::abs(a))) {
                    throw std::runtime_error("pricing methods disagree before timing");
                }
            }
        }
        benchmark::Initialize(&argc, argv);
        if (benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
        benchmark::RunSpecifiedBenchmarks();
        benchmark::Shutdown();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Benchmark failed: " << error.what() << '\n';
        return 1;
    }
}
