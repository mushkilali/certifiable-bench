#!/usr/bin/env python3
"""Compare benchmark results across platforms."""

import json
import sys

def load_result(path):
    with open(path) as f:
        return json.load(f)

def compare(ref_path, target_path):
    ref = load_result(ref_path)
    target = load_result(target_path)

    print("=" * 60)
    print("  Cross-Platform Performance Comparison")
    print(f"  Reference: {ref.get('platform', 'unknown')}")
    print(f"  Target: {target.get('platform', 'unknown')}")
    print("=" * 60)
    print()

    # Latency comparison
    ref_p99 = ref.get('latency', {}).get('p99_ns', 0)
    target_p99 = target.get('latency', {}).get('p99_ns', 0)

    if ref_p99 > 0:
        ratio = target_p99 / ref_p99
        print(f"Latency (p99):")
        print(f"  {ref.get('platform')}: {ref_p99:,} ns")
        print(f"  {target.get('platform')}: {target_p99:,} ns")
        print(f"  Ratio: {ratio:.2f}x")
    print()

    # Bit identity
    ref_hash = ref.get('output_hash', '')
    target_hash = target.get('output_hash', '')
    identical = ref_hash == target_hash and ref_hash != ''

    print(f"Bit Identity: {'✓ VERIFIED' if identical else '✗ MISMATCH'}")
    print()
    print("=" * 60)

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} <reference.json> <target.json>")
        sys.exit(1)
    compare(sys.argv[1], sys.argv[2])
