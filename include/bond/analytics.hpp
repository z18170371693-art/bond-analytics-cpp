/** @file analytics.hpp
 * @brief Fixed-rate bullet-bond valuation at a coupon payment date.
 */
#pragma once

#include <cstddef>

namespace bond {

/** Contract with equal coupon periods. Rates are annual decimals. */
struct Bond {
    double face_value{1000.0}; ///< Principal repaid at maturity, positive and finite.
    double coupon_rate{0.05}; ///< Annual coupon rate, nonnegative and finite.
    std::size_t periods{10}; ///< Remaining coupon periods, from 1 to 10000.
    unsigned payments_per_year{2}; ///< Equal payments per year, from 1 to 365.
};

/** Available price evaluation methods. Both use O(n) work and O(1) space. */
enum class Method { direct, horner };

/** Price and annual-yield risk measures. */
struct Metrics {
    double price; ///< Currency units; dirty and clean price agree on coupon dates.
    double macaulay_duration; ///< Present-value-weighted payment time, in years.
    double modified_duration; ///< Fractional price sensitivity to annual yield.
    double convexity; ///< Second price derivative divided by price, in years squared.
};

/** Check the contract; throws std::invalid_argument for invalid input.
 * @param contract Contract with positive principal and equal coupon periods.
 */
void validate(const Bond& contract);

/** Sum each discounted cash flow with a separate power operation.
 * @param contract Contract to value at a coupon payment date.
 * @param annual_yield Nominal annual yield compounded at the coupon frequency.
 * @throws std::invalid_argument If input is nonfinite or yield <= -frequency.
 * @throws std::overflow_error If a positive finite price cannot be represented.
 */
double price_direct(const Bond& contract, double annual_yield);

/** Evaluate the same cash-flow polynomial using backward Horner evaluation.
 * @copydetails price_direct
 */
double price_horner(const Bond& contract, double annual_yield);

/** Calculate price, Macaulay duration, modified duration and convexity.
 * @param contract Contract to value at a coupon payment date.
 * @param annual_yield Nominal annual yield compounded at the coupon frequency.
 * @param method Method for computing price; risk moments use discounted cash flows.
 * @throws std::invalid_argument For invalid contracts, yields or method values.
 * @throws std::overflow_error For unrepresentable output.
 */
Metrics analyze(const Bond& contract, double annual_yield,
                Method method = Method::horner);

/** Invert the monotone price function within an explicit yield bracket.
 * @param contract Contract with positive principal and nonnegative coupons.
 * @param target_price Positive finite observed bond price on a coupon date.
 * @param lower Lower annual yield, strictly greater than -frequency.
 * @param upper Upper annual yield, strictly greater than lower.
 * @param relative_price_tolerance Residual tolerance relative to max(1,target).
 * @param max_iterations Maximum number of bisection iterations.
 * @return Yield whose repriced residual satisfies the requested tolerance.
 * @throws std::invalid_argument For bad inputs or a target outside the bracket.
 * @throws std::runtime_error If the requested residual is not reached.
 */
double yield_to_maturity(const Bond& contract, double target_price,
                         double lower = -0.5, double upper = 1.0,
                         double relative_price_tolerance = 1e-12,
                         std::size_t max_iterations = 200);

} // namespace bond
