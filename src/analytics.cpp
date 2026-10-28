#include "bond/analytics.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>

namespace bond {
namespace {

long double discount_base(const Bond& contract, double annual_yield) {
    if (!std::isfinite(annual_yield) ||
        annual_yield <= -static_cast<double>(contract.payments_per_year)) {
        throw std::invalid_argument("annual yield must be finite and greater than -frequency");
    }
    const long double base = 1.0L + static_cast<long double>(annual_yield) /
                                       contract.payments_per_year;
    if (!(base > 0.0L) || !std::isfinite(base)) {
        throw std::invalid_argument("yield produces an invalid discount base");
    }
    return base;
}

long double coupon_payment(const Bond& contract) {
    return static_cast<long double>(contract.face_value) *
           (static_cast<long double>(contract.coupon_rate) /
            contract.payments_per_year);
}

// Keep the direct power path for ordinary cash flows. When the discount
// factor alone leaves the floating-point range, combine the logarithms before
// exponentiating; the discounted cash flow can still be representable.
long double discounted_cash_flow(long double cash_flow, long double base,
                                 std::size_t period) {
    if (cash_flow == 0.0L) {
        return 0.0L;
    }
    const long double exponent = static_cast<long double>(period);
    const long double factor = std::pow(base, -exponent);
    const long double present_value = cash_flow * factor;
    if (present_value > 0.0L && std::isfinite(present_value)) {
        return present_value;
    }
    return std::exp(std::log(cash_flow) - exponent * std::log(base));
}

double positive_output(long double value, const char* name) {
    if (!(value > 0.0L) || !std::isfinite(value) ||
        value > std::numeric_limits<double>::max()) {
        throw std::overflow_error(std::string(name) + " is not representable as a positive finite double");
    }
    const double result = static_cast<double>(value);
    if (!(result > 0.0) || !std::isfinite(result)) {
        throw std::overflow_error(std::string(name) + " underflows double precision");
    }
    return result;
}

// A zero or infinite bound price is useful to the bisection comparison even
// when that bound price cannot itself be returned through the public price API.
long double horner_value(const Bond& contract, long double base) {
    const long double coupon = coupon_payment(contract);
    long double value = 0.0L;
    for (std::size_t period = contract.periods; period > 0; --period) {
        const long double cash_flow = coupon +
            (period == contract.periods ? contract.face_value : 0.0L);
        value = (value + cash_flow) / base;
        if (!std::isfinite(value)) {
            return std::numeric_limits<long double>::infinity();
        }
    }
    return value;
}

} // namespace

void validate(const Bond& contract) {
    if (!std::isfinite(contract.face_value) || contract.face_value <= 0.0) {
        throw std::invalid_argument("face value must be positive and finite");
    }
    if (!std::isfinite(contract.coupon_rate) || contract.coupon_rate < 0.0) {
        throw std::invalid_argument("coupon rate must be nonnegative and finite");
    }
    if (contract.periods == 0 || contract.periods > 10000) {
        throw std::invalid_argument("remaining coupon periods must be between 1 and 10000");
    }
    if (contract.payments_per_year == 0 || contract.payments_per_year > 365) {
        throw std::invalid_argument("payment frequency must be between 1 and 365");
    }
}

double price_direct(const Bond& contract, double annual_yield) {
    validate(contract);
    const long double base = discount_base(contract, annual_yield);
    const long double coupon = coupon_payment(contract);
    long double value = 0.0L;
    for (std::size_t period = 1; period <= contract.periods; ++period) {
        const long double cash_flow = coupon +
            (period == contract.periods ? contract.face_value : 0.0L);
        const long double discounted = discounted_cash_flow(cash_flow, base, period);
        value += discounted;
        if (!std::isfinite(value)) {
            throw std::overflow_error("discounted cash flows exceed numeric range");
        }
    }
    return positive_output(value, "price");
}

double price_horner(const Bond& contract, double annual_yield) {
    validate(contract);
    return positive_output(horner_value(contract, discount_base(contract, annual_yield)), "price");
}

Metrics analyze(const Bond& contract, double annual_yield, Method method) {
    double price = 0.0;
    switch (method) {
    case Method::direct:
        price = price_direct(contract, annual_yield);
        break;
    case Method::horner:
        price = price_horner(contract, annual_yield);
        break;
    default:
        throw std::invalid_argument("unknown pricing method");
    }

    const long double base = discount_base(contract, annual_yield);
    const long double frequency = contract.payments_per_year;
    const long double coupon = coupon_payment(contract);
    long double duration = 0.0L;
    long double convexity = 0.0L;
    for (std::size_t period = 1; period <= contract.periods; ++period) {
        const long double number = static_cast<long double>(period);
        const long double cash_flow = coupon +
            (period == contract.periods ? contract.face_value : 0.0L);
        const long double present_value = discounted_cash_flow(cash_flow, base, period);
        const long double weight = present_value / price;
        duration += weight * (number / frequency);
        // Divide factors separately rather than forming base^2, which can
        // overflow even when the final scaled contribution is finite.
        convexity += (weight * (number / frequency) / base) *
                     ((number + 1.0L) / frequency / base);
    }
    return {price,
            positive_output(duration, "Macaulay duration"),
            positive_output(duration / base, "modified duration"),
            positive_output(convexity, "convexity")};
}

double yield_to_maturity(const Bond& contract, double target_price,
                         double lower, double upper,
                         double relative_price_tolerance,
                         std::size_t max_iterations) {
    validate(contract);
    if (!std::isfinite(target_price) || target_price <= 0.0) {
        throw std::invalid_argument("target price must be positive and finite");
    }
    const long double lower_base = discount_base(contract, lower);
    const long double upper_base = discount_base(contract, upper);
    if (!(lower < upper)) {
        throw std::invalid_argument("lower yield must be strictly less than upper yield");
    }
    if (!std::isfinite(relative_price_tolerance) || relative_price_tolerance <= 0.0 ||
        max_iterations == 0) {
        throw std::invalid_argument("solver tolerance and iteration limit must be positive");
    }
    const long double target = target_price;
    const long double bound_price_lower = horner_value(contract, lower_base);
    const long double bound_price_upper = horner_value(contract, upper_base);
    const long double tolerance = relative_price_tolerance * std::max(1.0L, target);
    if (std::abs(bound_price_lower - target) <= tolerance) {
        return lower;
    }
    if (std::abs(bound_price_upper - target) <= tolerance) {
        return upper;
    }
    if (target > bound_price_lower || target < bound_price_upper) {
        throw std::invalid_argument("target price is outside the specified yield bracket");
    }

    for (std::size_t iteration = 0; iteration < max_iterations; ++iteration) {
        const double midpoint = lower * 0.5 + upper * 0.5;
        const long double value = horner_value(contract, discount_base(contract, midpoint));
        if (std::abs(value - target) <= tolerance) {
            return midpoint;
        }
        if (midpoint == lower || midpoint == upper) {
            break;
        }
        if (value > target) {
            lower = midpoint;
        } else {
            upper = midpoint;
        }
    }
    throw std::runtime_error("yield solver did not reach the requested price residual");
}

} // namespace bond
