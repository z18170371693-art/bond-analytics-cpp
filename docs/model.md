# Financial model

## Contract and cash flows

Let `F` be face value, `c` the annual coupon rate, `m` the number of payments per
year, `n` the remaining payment count, and `y` the nominal annual yield compounded
`m` times per year. The per-period coupon is `C = F*c/m`, and `q = 1+y/m` must
be positive. Rates are decimals; time is measured in years.

The first payment is one complete period in the future. Payments 1 through
`n-1` are `C`; payment `n` is `C+F`. With `CF_i` denoting payment `i`:

```text
PV_i = CF_i / q^i
P    = sum(PV_i, i = 1..n)
```

The principal is repaid once, at maturity. This is the standard coupon-plus-
principal valuation described in [OpenStax, Bond Valuation](https://openstax.org/books/principles-finance/pages/10-2-bond-valuation).
Here the settlement convention is explicitly restricted to a coupon date,
immediately after that day's coupon, so clean and dirty prices agree.

## Duration and convexity

Macaulay duration is the present-value-weighted payment time. Differentiating the
above price with respect to the annual yield gives modified duration and convexity:

```text
D_Mac = sum((i/m)*PV_i) / P
D_Mod = -P'(y)/P = D_Mac/q
K     = P''(y)/P = sum(i*(i+1)*PV_i) / (m*m*q*q*P)
```

Thus a small absolute yield change `dy` has the approximation
`delta_price / price = -D_Mod*dy + 0.5*K*dy*dy`. A 1 percentage point increase
means `dy=0.01`. Macaulay duration is in years and convexity in years squared;
modified duration measures sensitivity to the annual yield, not the period yield.
The [CFA Institute overview of yield duration](https://www.cfainstitute.org/insights/professional-learning/refresher-readings/2026/yield-based-bond-duration-measures-and-properties)
explains this interpretation. The implemented formulas above follow by direct
differentiation; tests additionally check first and second finite differences.

## A two-payment example

For `F=100`, `c=0.10`, `m=1`, `n=2`, `y=0.10`, the cash flows are `10,110`.
Price is `10/1.1 + 110/1.1^2 = 100`. The earlier coupon makes the Macaulay duration
less than the two-year maturity. With a zero coupon the single payment is at
maturity, so Macaulay duration is exactly `n/m`.

The default contract has `F=1000`, `c=0.05`, `m=2`, `n=10`, `y=0.04`.
It pays 25 every six months and 1025 at the last payment. The price is
approximately 1044.912925, above face value because the coupon exceeds yield.
The Python oracle independently evaluates cash flows with 60-digit Decimal
arithmetic rather than calling either C++ price method.

## Yield inversion and supported domain

Positive principal and nonnegative coupons make price continuous and strictly
decreasing for `y > -m`. A target between the prices at an explicit lower and
upper yield therefore has one solution in that bracket. Bisection halves the
interval each iteration; the returned answer must meet the requested repricing
residual. An endpoint within that residual tolerance is accepted, including a
target slightly outside the mathematical bracket. Failure to meet the residual
within the iteration limit is an error.

Contracts accept 1 to 10000 periods, 1 to 365 payments per year, positive finite
principal, and a nonnegative finite coupon rate. Outputs must be positive/finite
where required and representable as doubles. A mathematical price that overflows
or rounds to zero is rejected. Intermediate arithmetic also has finite range;
extreme contracts can be rejected if that range is exceeded. Negative yields
are permitted above `-m`.

This model has equal periods, one flat yield, no credit risk, and no embedded
options. It does not construct a day-count schedule or price between coupon
dates. The API and CLI describe these assumptions rather than interpreting
the input as a live tradeable instrument.
