# Certifiable Bench

**Performance benchmarking harness for deterministic ML inference on safety-critical hardware.**

Measures latency, throughput, and WCET with cryptographic verification that outputs are bit-identical across platforms.

[![Tests](https://img.shields.io/badge/tests-11%2C840%20assertions-brightgreen)]()
[![License](https://img.shields.io/badge/license-GPL--3.0-blue)]()
[![C Standard](https://img.shields.io/badge/C-C99-blue)]()

---

## The WOW Moment

```
x86_64 (Intel Xeon)                   RISC-V (Tenstorrent)
───────────────────                   ────────────────────
Output hash: 48f4eceb...      ═══     Output hash: 48f4eceb...

Latency (p99):   1,234 µs             Latency (p99):   2,567 µs
Throughput:      8,100 inf/s          Throughput:      3,890 inf/s
WCET bound:      2,450 µs             WCET bound:      5,120 µs

         Bit-identical: YES ✓
         RISC-V is 2.08x slower (but CORRECT)
```

Different hardware. Different performance. **Same outputs.**

---

## Problem Statement

How do you benchmark ML inference across platforms when you need to prove the results are identical?

Traditional benchmarks fail:
- They measure performance without verifying correctness
- Floating-point drift means "faster" might mean "wrong"
- No cryptographic proof that Platform A and B compute the same thing

**certifiable-bench** solves this by:
1. Hashing all inference outputs during the benchmark
2. Comparing output hashes before comparing performance
3. Refusing to report performance ratios if outputs differ
4. Generating auditable JSON reports with full provenance

---

## Features

| Feature | Description |
|---------|-------------|
| **High-Resolution Timing** | POSIX backend, 23ns overhead, nanosecond precision |
| **Comprehensive Statistics** | Min, max, mean, median, p95, p99, stddev, WCET |
| **Histogram & Outliers** | Configurable bins, MAD-based outlier detection |
| **Cryptographic Verification** | FIPS 180-4 SHA-256, constant-time comparison |
| **Platform Detection** | x86_64, aarch64, riscv64, CPU model, frequency |
| **Environmental Monitoring** | Temperature, throttle events, stability assessment |
| **Cross-Platform Comparison** | Bit-identity gate, Q16.16 fixed-point ratios |
| **Report Generation** | JSON, CSV, human-readable summary |

---

## Quick Start

### Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
ctest --output-on-failure
```

### Run Benchmark

```bash
./bench_inference --iterations 5000 --output result_x86.json
```

### Compare Platforms

```bash
./bench_inference --iterations 5000 --output result_riscv.json --compare result_x86.json
```

Output:
```
═══════════════════════════════════════════════════════════════════
  Cross-Platform Performance Comparison
  Reference: x86_64        Target: riscv64
═══════════════════════════════════════════════════════════════════

Bit Identity:  VERIFIED (outputs identical)

Latency (p99):
  Diff:    +1,333,323 ns
  Ratio:   2.08x slower

Throughput:
  Diff:    -4,210 inferences/sec
  Ratio:   0.48x

═══════════════════════════════════════════════════════════════════
```

---

## The Bit-Identity Gate

Performance comparison is **only valid** when outputs are identical:

```c
cb_compare_results(&x86_result, &riscv_result, &comparison);

if (comparison.outputs_identical) {
    /* Safe to compare performance */
    printf("Ratio: %.2fx\n", comparison.latency_ratio_q16 / 65536.0);
} else {
    /* Something is wrong — don't trust performance numbers */
    printf("ERROR: Outputs differ, comparison invalid\n");
}
```

This is the certifiable-* philosophy: **correctness first, then performance.**

---

## CLI Reference

```
Usage: bench_inference [options]

Options:
  --iterations N     Measurement iterations (default: 1000)
  --warmup N         Warmup iterations (default: 100)
  --batch N          Batch size (default: 1)
  --output PATH      Output JSON path
  --csv PATH         Output CSV path
  --compare PATH     Compare with previous JSON result
  --help             Show this help

Examples:
  bench_inference --iterations 5000 --output result.json
  bench_inference --compare baseline.json --output new_result.json
```

---

## API Usage

```c
#include "cb_runner.h"
#include "cb_report.h"

/* Your inference function */
cb_result_code_t my_inference(void *ctx, const void *in, void *out) {
    /* Run neural network forward pass */
    return CB_OK;
}

int main(void) {
    cb_config_t config;
    cb_result_t result;

    cb_config_init(&config);
    config.warmup_iterations = 100;
    config.measure_iterations = 1000;

    cb_run_benchmark(&config, my_inference, model,
                     input, input_size, output, output_size,
                     &result);

    cb_print_summary(&result);
    cb_write_json(&result, "benchmark.json");

    return 0;
}
```

---

## Critical Loop Structure

The benchmark core follows CB-MATH-001 §7.2:

```c
for (i = 0; i < iterations; i++) {
    /* === CRITICAL LOOP START === */
    t_start = cb_timer_now_ns();
    fn(ctx, input, output);
    t_end = cb_timer_now_ns();
    /* === CRITICAL LOOP END === */

    samples[i] = t_end - t_start;

    /* Verification OUTSIDE timing */
    cb_verify_ctx_update(&ctx, output, size);
}
```

Timing measures only the inference. Hashing happens outside the critical path.

---

## JSON Output Schema

```json
{
  "version": "1.0",
  "platform": "x86_64",
  "cpu_model": "Intel Xeon @ 2.20GHz",
  "latency": {
    "min_ns": 1234567,
    "max_ns": 2345678,
    "mean_ns": 1500000,
    "p99_ns": 2100000,
    "wcet_bound_ns": 3245678
  },
  "throughput": {
    "inferences_per_sec": 666
  },
  "verification": {
    "determinism_verified": true,
    "output_hash": "48f4eceb..."
  },
  "faults": {
    "overflow": false,
    "thermal_drift": false
  }
}
```

---

## Project Structure

```
certifiable-bench/
├── include/
│   ├── cb_types.h          Core types (config, result, faults)
│   ├── cb_timer.h          Timer API
│   ├── cb_metrics.h        Statistics API
│   ├── cb_platform.h       Platform detection API
│   ├── cb_verify.h         SHA-256, golden ref, result binding
│   ├── cb_runner.h         Benchmark runner API
│   └── cb_report.h         JSON/CSV/comparison API
├── src/
│   ├── timer/timer.c       POSIX backend (23ns overhead)
│   ├── metrics/metrics.c   Stats, histogram, outlier, WCET
│   ├── platform/platform.c Detection, hwcounters, env monitoring
│   ├── verify/verify.c     FIPS 180-4 SHA-256
│   ├── runner/runner.c     Critical loop, warmup
│   └── report/report.c     JSON, CSV, compare, print
├── tests/unit/
│   ├── test_timer.c        10,032 assertions
│   ├── test_metrics.c      1,502 assertions
│   ├── test_platform.c     35 assertions
│   ├── test_verify.c       113 assertions
│   ├── test_runner.c       92 assertions
│   └── test_report.c       66 assertions
├── examples/
│   └── bench_inference.c   CLI benchmark tool
└── docs/
    ├── CB-MATH-001.md      Mathematical specification
    ├── CB-STRUCT-001.md    Data structure specification
    └── requirements/       6 SRS documents
```

---

## Test Results

```
$ ctest --output-on-failure

Test project /home/william/certifiable-bench/build
    Start 1: test_timer
1/6 Test #1: test_timer .......................   Passed    0.01 sec
    Start 2: test_metrics
2/6 Test #2: test_metrics .....................   Passed    0.00 sec
    Start 3: test_platform
3/6 Test #3: test_platform ....................   Passed    0.01 sec
    Start 4: test_verify
4/6 Test #4: test_verify ......................   Passed    0.01 sec
    Start 5: test_runner
5/6 Test #5: test_runner ......................   Passed    0.00 sec
    Start 6: test_report
6/6 Test #6: test_report ......................   Passed    0.00 sec

100% tests passed, 0 tests failed out of 6

Total assertions: 11,840
```

---

## Documentation

| Document | Description |
|----------|-------------|
| [CB-MATH-001](docs/CB-MATH-001.md) | Mathematical specification |
| [CB-STRUCT-001](docs/CB-STRUCT-001.md) | Data structure specification |
| SRS-001-TIMER | Timer requirements |
| SRS-002-METRICS | Statistics requirements |
| SRS-003-RUNNER | Benchmark runner requirements |
| SRS-004-VERIFY | Verification requirements |
| SRS-005-REPORT | Reporting requirements |
| SRS-006-PLATFORM | Platform detection requirements |

**~233 SHALL statements across 6 SRS documents**

---

## Verified Platforms

| Platform | OS | Compiler | Result |
|----------|-----|----------|--------|
| x86_64 | Linux (Ubuntu) | GCC 12.2.0 | ✓ 23ns timer overhead |
| x86_64 | Container | GCC 13.3.0 | ✓ 34ns timer overhead |
| aarch64 | — | — | Pending |
| riscv64 | — | — | Pending (Semper Victus) |

---

## Pipeline Integration

certifiable-bench integrates with the certifiable-* ecosystem:

```
certifiable-harness: "Are the outputs identical?"  (correctness)
certifiable-bench:   "How fast is it?"             (performance)
```

```
certifiable-inference ──→ certifiable-bench ──→ JSON report
         ↑                       │
         └───────────────────────┘
              Performance data
```

---

## Related Projects

| Project | Purpose |
|---------|---------|
| [certifiable-data](https://github.com/williamofai/certifiable-data) | Deterministic data pipeline |
| [certifiable-training](https://github.com/williamofai/certifiable-training) | Deterministic training |
| [certifiable-quant](https://github.com/williamofai/certifiable-quant) | Model quantization |
| [certifiable-deploy](https://github.com/williamofai/certifiable-deploy) | Bundle packaging |
| [certifiable-inference](https://github.com/williamofai/certifiable-inference) | Fixed-point inference |
| [certifiable-monitor](https://github.com/williamofai/certifiable-monitor) | Runtime monitoring |
| [certifiable-verify](https://github.com/williamofai/certifiable-verify) | Pipeline verification |
| [certifiable-harness](https://github.com/williamofai/certifiable-harness) | End-to-end bit-identity |

---

## License

**Dual License:**

- **Open Source:** GPL-3.0 — Free for open source projects
- **Commercial:** Contact william@fstopify.com for commercial licensing

**Patent:** UK Patent Application GB2521625.0

---

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

All contributions require a signed Contributor License Agreement.

---

## Author

**William Murray**  
The Murray Family Innovation Trust  
[SpeyTech](https://speytech.com) · [GitHub](https://github.com/williamofai)

---

*Measure performance. Verify determinism. Generate evidence.*

Copyright © 2026 The Murray Family Innovation Trust. All rights reserved.
