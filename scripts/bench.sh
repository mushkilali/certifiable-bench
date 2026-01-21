#!/bin/bash
set -euo pipefail

mkdir -p results
./build/certifiable-bench \
    --warmup 100 \
    --iterations 1000 \
    --verify \
    --output "results/bench_$(uname -m).json"

# Verify determinism
if ! grep -q '"determinism_verified": true' results/bench_*.json 2>/dev/null; then
    echo "WARNING: No determinism verification in results"
fi

echo "Results saved to results/bench_$(uname -m).json"
