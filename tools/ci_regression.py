#!/usr/bin/env python3
"""Check for performance regression in CI."""

import json
import sys

def check_regression(baseline_path, current_path, threshold_percent):
    with open(baseline_path) as f:
        baseline = json.load(f)
    with open(current_path) as f:
        current = json.load(f)

    baseline_p99 = baseline.get('latency', {}).get('p99_ns', 0)
    current_p99 = current.get('latency', {}).get('p99_ns', 0)

    if baseline_p99 == 0:
        print("No baseline latency data")
        return 0

    regression = ((current_p99 - baseline_p99) / baseline_p99) * 100

    print(f"Baseline p99: {baseline_p99:,} ns")
    print(f"Current p99:  {current_p99:,} ns")
    print(f"Change: {regression:+.1f}%")

    if regression > threshold_percent:
        print(f"FAIL: Regression exceeds {threshold_percent}% threshold")
        return 1

    print("PASS: Within threshold")
    return 0

if __name__ == "__main__":
    if len(sys.argv) < 4:
        print(f"Usage: {sys.argv[0]} <baseline.json> <current.json> <threshold%>")
        sys.exit(1)
    sys.exit(check_regression(sys.argv[1], sys.argv[2], float(sys.argv[3])))
