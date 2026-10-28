# Algorithms and checks

## Direct discounting

Compute each cash flow and discount it with an independent `pow(q,-i)` call,
then sum. This mirrors the financial definition and serves as a readable
reference evaluator. For `n` payments it uses `n` loop iterations and `n` power
evaluations, with constant extra storage. The benchmark treats the library power
operation as a scalar operation; no claim about its internal implementation is
needed to compare measured time.

## Horner evaluation

Write `v=1/q`. Price is the polynomial
`CF_1*v + CF_2*v^2 + ... + CF_n*v^n`. Starting with zero and visiting payments
backwards, repeatedly set `result=(result+CF_i)*v`.

For the two-payment cash flows `10,110`:

| Step | Accumulator |
|---|---|
| Start | 0 |
| Process the last payment | `110*v` |
| Process the first payment | `(110*v + 10)*v` |
| Expanded expression | `110*v*v + 10*v` |

Both implementations use linear work and constant extra space. Horner removes
the repeated power calls; it does not change the asymptotic complexity. Its
measured advantage depends on the compiler, processor, and number of payments.
The benchmark checks matching prices before timing both functions on the same
contracts. It excludes argument parsing, file output, duration calculations,
and yield inversion.

Intermediate arithmetic uses `long double`, and public results use `double`.
On some compilers these types have the same precision. Method agreement is
checked with a tolerance rather than bitwise equality. The independent Decimal
oracle and closed-form cases provide checks beyond comparing the two methods.
An extreme discount factor can underflow even when its product with a large
cash flow is representable. The direct evaluator and risk calculations use
`exp(log(cash_flow)-period*log(q))` as a fallback for that intermediate-range
case; ordinary benchmark inputs use the direct power calculation.

## Yield to maturity

The initial bracket is explicit. For each midpoint, compare its price with the
target and keep the half that contains the solution. Since price decreases with
yield, an excessive price means that the yield must increase. Endpoints are
valid answers when they meet the residual tolerance. A target outside the
bracket beyond the endpoint residual tolerance is rejected, and exhausting the
iteration limit is reported.

## Questions for code review

1. Why is the coupon `face*annual_coupon/frequency`? The annual coupon is split
   across equal payments.
2. Why does the final payment include principal? A bullet bond repays principal
   at maturity, once.
3. Why does price fall when yield rises? Every positive cash flow has a smaller
   discount factor.
4. Why divide Macaulay duration by `q`? The derivative is taken with respect to
   annual nominal yield; the chain rule introduces the period-frequency factor.
5. Is Horner an `O(1)` algorithm? No. It visits every payment, so time is `O(n)`.
6. Why use bisection? Monotonicity and an explicit bracket give a simple,
   verifiable solver with a bounded number of iterations.
7. Why can a default bracket reject a price? The supplied yield interval may
   not contain the solution; `--lower` and `--upper` make that choice visible.
8. Why are local checks insufficient for all 20 points? The assignment requires
   the public repository and successful CI execution, including publication.
