#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "bond/analytics.hpp"

#include <cmath>
#include <limits>
#include <random>
#include <stdexcept>

namespace {
using bond::Bond;
using bond::Method;

void check_prices(const Bond& contract, double annual_yield, double expected) {
    CHECK(bond::price_direct(contract, annual_yield) ==
          doctest::Approx(expected).epsilon(2e-12));
    CHECK(bond::price_horner(contract, annual_yield) ==
          doctest::Approx(expected).epsilon(2e-12));
}

void check_bad_contract(const Bond& contract) {
    CHECK_THROWS_AS(bond::validate(contract), std::invalid_argument);
    CHECK_THROWS_AS(bond::price_direct(contract, 0.04), std::invalid_argument);
    CHECK_THROWS_AS(bond::price_horner(contract, 0.04), std::invalid_argument);
    CHECK_THROWS_AS(bond::analyze(contract, 0.04), std::invalid_argument);
}
} // namespace

TEST_CASE("one annual payment agrees with a hand calculation") {
    const Bond contract{100.0, 0.10, 1, 1};
    check_prices(contract, 0.10, 100.0); // 110 / 1.10.
    const auto result = bond::analyze(contract, 0.10);
    CHECK(result.macaulay_duration == doctest::Approx(1.0));
    CHECK(result.modified_duration == doctest::Approx(1.0 / 1.1));
    CHECK(result.convexity == doctest::Approx(2.0 / (1.1 * 1.1)));
}

TEST_CASE("two payments agree with rational hand values") {
    const Bond contract{100.0, 0.10, 2, 1};
    // At 25% yield the discounted payments are 8 and 70.4.
    check_prices(contract, 0.25, 78.4);
    const auto result = bond::analyze(contract, 0.25);
    CHECK(result.macaulay_duration == doctest::Approx(148.8 / 78.4));
    CHECK(result.modified_duration == doctest::Approx(148.8 / 98.0));
    CHECK(result.convexity == doctest::Approx(438.4 / 122.5));
}

TEST_CASE("zero coupon price and risk measures have closed forms") {
    const Bond contract{1000.0, 0.0, 12, 2};
    const double yield = 0.06;
    const double base = 1.03;
    check_prices(contract, yield, 1000.0 / std::pow(base, 12));
    for (const auto method : {Method::direct, Method::horner}) {
        const auto result = bond::analyze(contract, yield, method);
        CHECK(result.macaulay_duration == doctest::Approx(6.0));
        CHECK(result.modified_duration == doctest::Approx(6.0 / base));
        CHECK(result.convexity == doctest::Approx(12.0 * 13.0 / (4.0 * base * base)));
    }
}

TEST_CASE("coupon equal to yield prices a bond at par") {
    for (const unsigned frequency : {1U, 2U, 4U, 12U}) {
        const Bond contract{1000.0, 0.045, 7 * frequency, frequency};
        check_prices(contract, 0.045, 1000.0);
    }
}

TEST_CASE("zero yield is the undiscounted sum of payments") {
    const Bond contract{1000.0, 0.06, 8, 4};
    check_prices(contract, 0.0, 1120.0); // 8 * 15 coupon, plus principal.
    const auto result = bond::analyze(contract, 0.0);
    // Sum of payment times times cash flow: (15 * 36 + 8000) / 4.
    CHECK(result.macaulay_duration == doctest::Approx(2135.0 / 1120.0));
}

TEST_CASE("negative yields remain valid above the compounding boundary") {
    const Bond contract{100.0, 0.0, 2, 2};
    check_prices(contract, -0.2, 100.0 / 0.81);
    CHECK(bond::analyze(contract, -0.2).modified_duration > 1.0);
}

TEST_CASE("positive cash flows make price decrease with yield") {
    const Bond contract{1000.0, 0.07, 30, 2};
    double previous = bond::price_horner(contract, -0.10);
    for (const double yield : {-0.02, 0.0, 0.03, 0.07, 0.15, 0.50}) {
        const double price = bond::price_horner(contract, yield);
        CHECK(price < previous);
        previous = price;
    }
}

TEST_CASE("positive coupon brings Macaulay duration before maturity") {
    const Bond contract{1000.0, 0.05, 20, 2};
    const auto result = bond::analyze(contract, 0.04);
    CHECK(result.macaulay_duration > 0.0);
    CHECK(result.macaulay_duration < 10.0);
    CHECK(result.modified_duration > 0.0);
    CHECK(result.convexity > 0.0);
}

TEST_CASE("modified duration matches the first yield derivative") {
    const Bond contract{1000.0, 0.055, 24, 2};
    constexpr double yield = 0.04;
    constexpr double step = 1e-5;
    const auto metrics = bond::analyze(contract, yield);
    const double slope = (bond::price_direct(contract, yield + step) -
                          bond::price_direct(contract, yield - step)) / (2.0 * step);
    CHECK(-slope / metrics.price ==
          doctest::Approx(metrics.modified_duration).epsilon(2e-7));
}

TEST_CASE("convexity matches the second yield derivative") {
    const Bond contract{1000.0, 0.055, 24, 2};
    constexpr double yield = 0.04;
    constexpr double step = 1e-4;
    const auto metrics = bond::analyze(contract, yield);
    const double curvature = (bond::price_direct(contract, yield + step) -
                              2.0 * metrics.price +
                              bond::price_direct(contract, yield - step)) / (step * step);
    CHECK(curvature / metrics.price ==
          doctest::Approx(metrics.convexity).epsilon(1e-6));
}

TEST_CASE("face scaling changes price but leaves risk measures unchanged") {
    const Bond small{100.0, 0.03, 20, 2};
    const Bond large{10000.0, 0.03, 20, 2};
    const auto a = bond::analyze(small, 0.04);
    const auto b = bond::analyze(large, 0.04);
    CHECK(b.price == doctest::Approx(100.0 * a.price));
    CHECK(b.macaulay_duration == doctest::Approx(a.macaulay_duration));
    CHECK(b.modified_duration == doctest::Approx(a.modified_duration));
    CHECK(b.convexity == doctest::Approx(a.convexity));
}

TEST_CASE("payment frequency converts annual rates and time units") {
    const Bond annual{100.0, 0.04, 1, 1};
    const Bond half_year{100.0, 0.04, 1, 2};
    check_prices(annual, 0.08, 104.0 / 1.08);
    check_prices(half_year, 0.08, 102.0 / 1.04);
    CHECK(bond::analyze(half_year, 0.08).macaulay_duration == doctest::Approx(0.5));
}

TEST_CASE("direct and Horner agree across deterministic varied contracts") {
    std::mt19937 generator(76219U);
    std::uniform_int_distribution<int> periods(1, 400);
    std::uniform_int_distribution<int> frequency(1, 12);
    std::uniform_real_distribution<double> coupon(0.0, 0.12);
    std::uniform_real_distribution<double> yield(-0.02, 0.20);
    for (int i = 0; i < 150; ++i) {
        const Bond contract{1000.0, coupon(generator),
                            static_cast<std::size_t>(periods(generator)),
                            static_cast<unsigned>(frequency(generator))};
        const double annual_yield = yield(generator);
        const double direct = bond::price_direct(contract, annual_yield);
        const double horner = bond::price_horner(contract, annual_yield);
        INFO("case " << i);
        CHECK(horner == doctest::Approx(direct).epsilon(2e-11));
        const auto a = bond::analyze(contract, annual_yield, Method::direct);
        const auto b = bond::analyze(contract, annual_yield, Method::horner);
        CHECK(a.modified_duration == doctest::Approx(b.modified_duration).epsilon(2e-11));
        CHECK(a.convexity == doctest::Approx(b.convexity).epsilon(2e-11));
    }
}

TEST_CASE("contract range endpoints are usable") {
    CHECK_NOTHROW(bond::validate(Bond{1000.0, 0.03, 1, 1}));
    const Bond upper{1000.0, 0.03, 10000, 365};
    CHECK_NOTHROW(bond::validate(upper));
    CHECK(bond::price_horner(upper, 0.04) > 0.0);
}

TEST_CASE("yield inversion recovers positive zero and negative yields") {
    const Bond contract{1000.0, 0.05, 18, 2};
    for (const double yield : {-0.10, 0.0, 0.045, 0.25}) {
        const double target = bond::price_direct(contract, yield);
        const double recovered = bond::yield_to_maturity(contract, target);
        CHECK(recovered == doctest::Approx(yield).epsilon(1e-9).scale(1.0));
        CHECK(bond::price_horner(contract, recovered) ==
              doctest::Approx(target).epsilon(2e-12));
    }
}

TEST_CASE("yield inversion accepts target prices at bracket endpoints") {
    const Bond contract{1000.0, 0.05, 10, 2};
    for (const double yield : {-0.20, 0.30}) {
        const double target = bond::price_direct(contract, yield);
        CHECK(bond::yield_to_maturity(contract, target, -0.20, 0.30) ==
              doctest::Approx(yield).epsilon(1e-10));
    }
}

TEST_CASE("yield inversion rejects an unbracketed target") {
    const Bond contract{1000.0, 0.05, 10, 2};
    const double too_high = bond::price_horner(contract, -0.30);
    const double too_low = bond::price_horner(contract, 0.40);
    CHECK_THROWS_AS(bond::yield_to_maturity(contract, too_high, -0.20, 0.30),
                    std::invalid_argument);
    CHECK_THROWS_AS(bond::yield_to_maturity(contract, too_low, -0.20, 0.30),
                    std::invalid_argument);
}

TEST_CASE("yield inversion reports exhaustion instead of returning a guess") {
    const Bond contract{1000.0, 0.05, 10, 2};
    const double target = bond::price_horner(contract, 0.04321);
    CHECK_THROWS_AS(bond::yield_to_maturity(contract, target, -0.1, 0.5, 1e-14, 1),
                    std::runtime_error);
}

TEST_CASE("invalid principal values are rejected") {
    for (const double face : {0.0, -100.0, std::numeric_limits<double>::infinity(),
                              std::numeric_limits<double>::quiet_NaN()}) {
        check_bad_contract(Bond{face, 0.05, 10, 2});
    }
}

TEST_CASE("invalid coupon values are rejected") {
    for (const double coupon : {-0.01, std::numeric_limits<double>::infinity(),
                                std::numeric_limits<double>::quiet_NaN()}) {
        check_bad_contract(Bond{1000.0, coupon, 10, 2});
    }
}

TEST_CASE("invalid period and frequency bounds are rejected") {
    check_bad_contract(Bond{1000.0, 0.05, 0, 2});
    check_bad_contract(Bond{1000.0, 0.05, 10001, 2});
    check_bad_contract(Bond{1000.0, 0.05, 10, 0});
    check_bad_contract(Bond{1000.0, 0.05, 10, 366});
}

TEST_CASE("nonfinite yields and the compounding boundary are rejected") {
    const Bond contract{};
    for (const double yield : {-2.0, -3.0, std::numeric_limits<double>::infinity(),
                               std::numeric_limits<double>::quiet_NaN()}) {
        CHECK_THROWS_AS(bond::price_direct(contract, yield), std::invalid_argument);
        CHECK_THROWS_AS(bond::price_horner(contract, yield), std::invalid_argument);
        CHECK_THROWS_AS(bond::analyze(contract, yield), std::invalid_argument);
    }
}

TEST_CASE("unknown pricing method is rejected") {
    CHECK_THROWS_AS(bond::analyze(Bond{}, 0.04, static_cast<Method>(73)),
                    std::invalid_argument);
}

TEST_CASE("unrepresentable positive prices are reported") {
    const Bond overflow{std::numeric_limits<double>::max(), 1.0, 2, 1};
    CHECK_THROWS_AS(bond::price_direct(overflow, -0.5), std::overflow_error);
    CHECK_THROWS_AS(bond::price_horner(overflow, -0.5), std::overflow_error);
    const Bond underflow{1.0, 0.0, 10000, 1};
    CHECK_THROWS_AS(bond::price_direct(underflow, 1.0), std::overflow_error);
    CHECK_THROWS_AS(bond::price_horner(underflow, 1.0), std::overflow_error);
}

TEST_CASE("scaled cash flows survive discount-factor underflow") {
    const Bond contract{1e300, 0.0, 1100, 1};
    // ldexp scales the cash flow directly; it never materializes 2^-1100.
    const double expected = std::ldexp(1e300, -1100);
    REQUIRE(expected > 0.0);
    REQUIRE(std::isfinite(expected));
    CHECK(bond::price_direct(contract, 1.0) ==
          doctest::Approx(expected).epsilon(2e-12).scale(0.0));
    CHECK(bond::price_horner(contract, 1.0) ==
          doctest::Approx(expected).epsilon(2e-12).scale(0.0));
    for (const auto method : {Method::direct, Method::horner}) {
        const auto metrics = bond::analyze(contract, 1.0, method);
        CHECK(metrics.price == doctest::Approx(expected).epsilon(2e-12).scale(0.0));
        CHECK(metrics.macaulay_duration == doctest::Approx(1100.0).epsilon(2e-12));
        CHECK(metrics.modified_duration == doctest::Approx(550.0).epsilon(2e-12));
        CHECK(metrics.convexity == doctest::Approx(1100.0 * 1101.0 / 4.0).epsilon(2e-12));
    }
}

TEST_CASE("invalid yield inversion inputs are rejected") {
    const Bond contract{};
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double infinity = std::numeric_limits<double>::infinity();
    for (const double target : {0.0, -1.0, nan, infinity}) {
        CHECK_THROWS_AS(bond::yield_to_maturity(contract, target), std::invalid_argument);
    }
    CHECK_THROWS_AS(bond::yield_to_maturity(contract, 1000.0, 0.1, 0.1),
                    std::invalid_argument);
    CHECK_THROWS_AS(bond::yield_to_maturity(contract, 1000.0, 0.2, 0.1),
                    std::invalid_argument);
    CHECK_THROWS_AS(bond::yield_to_maturity(contract, 1000.0, -2.0, 0.1),
                    std::invalid_argument);
    CHECK_THROWS_AS(bond::yield_to_maturity(contract, 1000.0, nan, 0.1),
                    std::invalid_argument);
    CHECK_THROWS_AS(bond::yield_to_maturity(contract, 1000.0, -0.1, infinity),
                    std::invalid_argument);
    for (const double tolerance : {0.0, -1.0, nan, infinity}) {
        CHECK_THROWS_AS(bond::yield_to_maturity(contract, 1000.0, -0.1, 0.5, tolerance),
                        std::invalid_argument);
    }
    CHECK_THROWS_AS(bond::yield_to_maturity(contract, 1000.0, -0.1, 0.5, 1e-12, 0),
                    std::invalid_argument);
}
