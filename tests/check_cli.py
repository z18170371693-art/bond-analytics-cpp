#!/usr/bin/env python3
"""Check the CLI against an independent high-precision cash-flow oracle."""

import argparse
import csv
from decimal import Decimal, localcontext
import io
import math
from pathlib import Path
import subprocess
import sys

METRICS = ("price", "macaulay_duration", "modified_duration", "convexity")


def oracle(face, coupon, periods, frequency, annual_yield):
    """Use 60-digit Decimal discounted cash flows, independent of C++ arithmetic."""
    with localcontext() as context:
        context.prec = 60
        face = Decimal(face)
        coupon = Decimal(coupon)
        annual_yield = Decimal(annual_yield)
        frequency = Decimal(frequency)
        base = 1 + annual_yield / frequency
        payment = face * coupon / frequency
        discounted = [
            (payment + (face if i == periods else 0)) / base**i
            for i in range(1, periods + 1)
        ]
        price = sum(discounted)
        weighted_time = sum(
            Decimal(i) / frequency * value
            for i, value in enumerate(discounted, start=1)
        )
        curvature = sum(
            Decimal(i * (i + 1)) * value
            for i, value in enumerate(discounted, start=1)
        )
        macaulay = weighted_time / price
        return {
            "price": float(price),
            "macaulay_duration": float(macaulay),
            "modified_duration": float(macaulay / base),
            "convexity": float(curvature / (frequency**2 * base**2 * price)),
        }


def run(app, arguments, success=True):
    result = subprocess.run(
        [str(app), *arguments], capture_output=True, text=True, timeout=15
    )
    if success and result.returncode != 0:
        raise AssertionError(f"CLI failed for {arguments}: {result.stderr}")
    if not success:
        if result.returncode == 0:
            raise AssertionError(f"CLI accepted invalid arguments: {arguments}")
        if not result.stderr.strip():
            raise AssertionError(f"CLI returned no diagnostic: {arguments}")
    return result


def read_metrics(result):
    rows = list(csv.reader(io.StringIO(result.stdout)))
    if not rows or rows[0] != ["metric", "value"]:
        raise AssertionError(f"Expected CSV header metric,value: {result.stdout!r}")
    if any(len(row) != 2 for row in rows):
        raise AssertionError("Malformed CSV row")
    values = dict(rows[1:])
    if len(values) != len(rows) - 1:
        raise AssertionError("Duplicate CSV metric")
    for metric in METRICS:
        if metric not in values or not math.isfinite(float(values[metric])):
            raise AssertionError(f"Missing or nonfinite metric: {metric}")
    return values


def close(actual, expected, label):
    if not math.isclose(actual, expected, rel_tol=3e-11, abs_tol=1e-11):
        raise AssertionError(f"{label}: got {actual:.17g}, expected {expected:.17g}")


def arguments_for(case, method):
    face, coupon, periods, frequency, annual_yield = case
    return [
        "--face", face, "--coupon", coupon,
        "--periods", str(periods), "--frequency", str(frequency),
        "--yield", annual_yield, "--method", method,
    ]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--app", type=Path, required=True)
    app = parser.parse_args().app.resolve()
    if not app.is_file():
        raise AssertionError(f"Application does not exist: {app}")

    cases = [
        ("100", "0.10", 1, 1, "0.10"),
        ("100", "0.10", 2, 1, "0.25"),
        ("1000", "0.05", 10, 2, "0.04"),
        ("1000", "0", 12, 2, "0.06"),
        ("1000", "0.06", 8, 4, "0"),
        ("1000", "0.03", 20, 2, "-0.02"),
        ("100", "0.04", 360, 12, "0.035"),
    ]
    numeric_checks = 0
    yield_checks = 0
    for case in cases:
        expected = oracle(*case)
        for method in ("direct", "horner"):
            values = read_metrics(run(app, arguments_for(case, method)))
            if values.get("method") != method:
                raise AssertionError("The reported pricing method does not match the request")
            for metric in METRICS:
                close(float(values[metric]), expected[metric], f"{case}, {method}, {metric}")
                numeric_checks += 1
            target = format(expected["price"], ".17g")
            solved = read_metrics(run(app, arguments_for(case, method) + [
                "--target-price", target, "--lower", "-0.1", "--upper", "0.6"
            ]))
            implied = float(solved["implied_yield"])
            if not math.isfinite(implied):
                raise AssertionError("Nonfinite implied yield")
            if not math.isclose(implied, float(case[4]), rel_tol=1e-8, abs_tol=2e-10):
                raise AssertionError(f"YTM did not recover the input yield: {case}, {implied}")
            repriced = oracle(case[0], case[1], case[2], case[3], str(implied))["price"]
            if not math.isclose(repriced, expected["price"], rel_tol=3e-11, abs_tol=1e-10):
                raise AssertionError("YTM does not reprice the target")
            yield_checks += 1

    invalid_cases = [
        ["--unknown", "1"], ["--face"], ["--face", "100", "--face", "200"],
        ["--method", "horner", "--method", "direct"], ["--face", "100abc"],
        ["--yield", "0.04oops"], ["--face", "nan"], ["--coupon", "inf"],
        ["--yield", "nan"], ["--face", "0"], ["--coupon", "-0.05"],
        ["--periods", "-1"], ["--periods", "1.5"], ["--periods", "10001"],
        ["--frequency", "-2"], ["--frequency", "0"], ["--frequency", "366"],
        ["--yield", "-2"], ["--method", "other"], ["--target-price", "-100"],
        ["--target-price", "1e100", "--lower", "0", "--upper", "0.1"],
        ["--target-price", "1000", "--lower", "0.2", "--upper", "0.1"],
    ]
    for arguments in invalid_cases:
        run(app, arguments, success=False)
    help_result = run(app, ["--help"])
    if "--yield" not in help_result.stdout or "--coupon" not in help_result.stdout:
        raise AssertionError("Help does not explain the main options")
    read_metrics(run(app, []))
    print(
        f"CLI validation passed: {numeric_checks} Decimal metric comparisons, "
        f"{yield_checks} yield round trips, {len(invalid_cases)} invalid inputs, help and defaults."
    )


if __name__ == "__main__":
    try:
        main()
    except (AssertionError, KeyError, ValueError, OSError, subprocess.TimeoutExpired) as error:
        print(f"CLI validation failed: {error}", file=sys.stderr)
        sys.exit(1)
