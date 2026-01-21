# SRS-005-REPORT: Report Generation Requirements

**Component:** certifiable-bench  
**Module:** Report (`cb_report.h`, `src/report/`)  
**Version:** 1.0  
**Status:** Draft  
**Date:** January 2026  
**Traceability:** CB-STRUCT-001 §14

---

## 1. Purpose

This document specifies requirements for the report generation module, which produces JSON and CSV output files and supports cross-platform result comparison.

---

## 2. Scope

The report module provides:
- JSON result serialisation
- CSV result serialisation
- Result loading from JSON
- Cross-platform comparison
- Human-readable summary output

---

## 3. Definitions

| Term | Definition |
|------|------------|
| Serialisation | Converting in-memory structures to file format |
| Deserialisation | Converting file format to in-memory structures |
| Comparison | Evaluating performance differences between two benchmark results |

---

## 4. Functional Requirements

### 4.1 JSON Output

| ID | Requirement | Verification |
|----|-------------|--------------|
| REPORT-F-001 | The module SHALL provide a function `cb_write_json()` to write results to JSON file. | Test |
| REPORT-F-002 | JSON output SHALL include version field ("1.0"). | Test |
| REPORT-F-003 | JSON output SHALL include platform identifier. | Test |
| REPORT-F-004 | JSON output SHALL include CPU model string. | Test |
| REPORT-F-005 | JSON output SHALL include configuration parameters. | Test |
| REPORT-F-006 | JSON output SHALL include all latency statistics. | Test |
| REPORT-F-007 | JSON output SHALL include throughput metrics. | Test |
| REPORT-F-008 | JSON output SHALL include verification status and output hash. | Test |
| REPORT-F-009 | JSON output SHALL include environment data if collected. | Test |
| REPORT-F-010 | JSON output SHALL include fault flags. | Test |
| REPORT-F-011 | JSON output SHALL include Unix timestamp. | Test |
| REPORT-F-012 | Hash values SHALL be serialised as 64-character lowercase hex strings. | Test |
| REPORT-F-013 | `cb_write_json()` SHALL return `CB_ERR_IO` on file write failure. | Test |
| REPORT-F-014 | JSON output SHALL be valid JSON (parseable by standard tools). | Test |
| REPORT-F-015 | JSON output SHALL use 2-space indentation for readability. | Inspection |

### 4.2 CSV Output

| ID | Requirement | Verification |
|----|-------------|--------------|
| REPORT-F-020 | The module SHALL provide a function `cb_write_csv()` to write results to CSV file. | Test |
| REPORT-F-021 | CSV output SHALL include header row with column names. | Test |
| REPORT-F-022 | CSV output SHALL include single data row with result values. | Test |
| REPORT-F-023 | CSV columns SHALL include: platform, min_ns, max_ns, mean_ns, median_ns, p95_ns, p99_ns, stddev_ns, throughput, determinism_verified, output_hash, timestamp. | Test |
| REPORT-F-024 | CSV values SHALL be comma-separated with no spaces. | Test |
| REPORT-F-025 | String values containing commas SHALL be quoted. | Test |
| REPORT-F-026 | `cb_write_csv()` SHALL return `CB_ERR_IO` on file write failure. | Test |

### 4.3 JSON Loading

| ID | Requirement | Verification |
|----|-------------|--------------|
| REPORT-F-030 | The module SHALL provide a function `cb_load_json()` to load results from JSON file. | Test |
| REPORT-F-031 | `cb_load_json()` SHALL populate `cb_result_t` structure from JSON. | Test |
| REPORT-F-032 | `cb_load_json()` SHALL return `CB_ERR_IO` on file read failure. | Test |
| REPORT-F-033 | `cb_load_json()` SHALL validate JSON structure before populating result. | Test |
| REPORT-F-034 | Missing optional fields SHALL be set to zero/null without error. | Test |
| REPORT-F-035 | Missing required fields (platform, latency) SHALL return error. | Test |

### 4.4 Cross-Platform Comparison

| ID | Requirement | Verification |
|----|-------------|--------------|
| REPORT-F-040 | The module SHALL provide a function `cb_compare_results()` for comparing two results. | Test |
| REPORT-F-041 | Comparison SHALL check output hash equality first. | Test |
| REPORT-F-042 | `cb_comparison_t.outputs_identical` SHALL be true only if output hashes match. | Test |
| REPORT-F-043 | `cb_comparison_t.comparable` SHALL be true only if outputs are identical. | Test |
| REPORT-F-044 | If outputs differ, performance comparison fields SHALL be zeroed. | Test |
| REPORT-F-045 | `latency_diff_ns` SHALL be computed as: result_b.p99 - result_a.p99. | Test |
| REPORT-F-046 | `latency_ratio_q16` SHALL be computed as: (result_b.p99 << 16) / result_a.p99. | Test |
| REPORT-F-047 | `throughput_diff` SHALL be computed as: result_b.inferences_per_sec - result_a.inferences_per_sec. | Test |
| REPORT-F-048 | `throughput_ratio_q16` SHALL be computed as: (result_b.throughput << 16) / result_a.throughput. | Test |
| REPORT-F-049 | Division by zero in ratio computation SHALL result in ratio = 0. | Test |

### 4.5 Human-Readable Output

| ID | Requirement | Verification |
|----|-------------|--------------|
| REPORT-F-050 | The module SHALL provide a function `cb_print_summary()` to print result summary. | Test |
| REPORT-F-051 | Summary SHALL include platform and CPU model. | Test |
| REPORT-F-052 | Summary SHALL include key latency metrics (min, max, mean, p99). | Test |
| REPORT-F-053 | Summary SHALL include throughput. | Test |
| REPORT-F-054 | Summary SHALL include determinism verification status. | Test |
| REPORT-F-055 | Summary SHALL indicate any fault flags set. | Test |
| REPORT-F-056 | The module SHALL provide a function `cb_print_comparison()` to print comparison summary. | Test |
| REPORT-F-057 | Comparison summary SHALL indicate whether outputs are identical. | Test |
| REPORT-F-058 | Comparison summary SHALL show latency and throughput ratios. | Test |

### 4.6 Histogram Output

| ID | Requirement | Verification |
|----|-------------|--------------|
| REPORT-F-060 | JSON output SHALL include histogram data if collected. | Test |
| REPORT-F-061 | Histogram SHALL be serialised as array of bin counts. | Test |
| REPORT-F-062 | Histogram metadata (min, max, bin_width) SHALL be included. | Test |
| REPORT-F-063 | Overflow and underflow counts SHALL be included. | Test |

---

## 5. Non-Functional Requirements

### 5.1 Determinism

| ID | Requirement | Verification |
|----|-------------|--------------|
| REPORT-NF-001 | JSON serialisation SHALL produce identical output for identical input across platforms. | Test |
| REPORT-NF-002 | Floating-point values SHALL NOT be used in JSON output (integers only). | Inspection |
| REPORT-NF-003 | JSON key order SHALL be deterministic (alphabetical or fixed). | Test |

### 5.2 Memory

| ID | Requirement | Verification |
|----|-------------|--------------|
| REPORT-NF-010 | `cb_write_json()` SHALL use streaming output (no large intermediate buffers). | Inspection |
| REPORT-NF-011 | `cb_load_json()` MAY allocate memory for parsing (freed before return). | Inspection |

### 5.3 Compatibility

| ID | Requirement | Verification |
|----|-------------|--------------|
| REPORT-NF-020 | JSON output SHALL be compatible with standard JSON parsers (Python json, jq). | Test |
| REPORT-NF-021 | CSV output SHALL be compatible with standard spreadsheet software. | Test |

---

## 6. Interface Definition

```c
/**
 * @brief Write results to JSON file
 * @param result Benchmark result
 * @param path Output file path
 * @return CB_OK on success
 *
 * @satisfies REPORT-F-001 through REPORT-F-015
 */
cb_result_code_t cb_write_json(const cb_result_t *result, const char *path);

/**
 * @brief Write results to CSV file
 * @param result Benchmark result
 * @param path Output file path
 * @return CB_OK on success
 *
 * @satisfies REPORT-F-020 through REPORT-F-026
 */
cb_result_code_t cb_write_csv(const cb_result_t *result, const char *path);

/**
 * @brief Load results from JSON file
 * @param path Input file path
 * @param result Output result structure
 * @return CB_OK on success
 *
 * @satisfies REPORT-F-030 through REPORT-F-035
 */
cb_result_code_t cb_load_json(const char *path, cb_result_t *result);

/**
 * @brief Compare two benchmark results
 * @param result_a Reference result (typically baseline)
 * @param result_b Target result (typically new/comparison)
 * @param comparison Output comparison structure
 * @return CB_OK on success
 *
 * @satisfies REPORT-F-040 through REPORT-F-049
 */
cb_result_code_t cb_compare_results(const cb_result_t *result_a,
                                     const cb_result_t *result_b,
                                     cb_comparison_t *comparison);

/**
 * @brief Print result summary to stdout
 * @param result Benchmark result
 *
 * @satisfies REPORT-F-050 through REPORT-F-055
 */
void cb_print_summary(const cb_result_t *result);

/**
 * @brief Print comparison summary to stdout
 * @param comparison Comparison result
 *
 * @satisfies REPORT-F-056 through REPORT-F-058
 */
void cb_print_comparison(const cb_comparison_t *comparison);
```

---

## 7. JSON Output Schema

```json
{
  "version": "1.0",
  "platform": "x86_64",
  "cpu_model": "Intel Core i7-10700K @ 3.80GHz",
  "cpu_freq_mhz": 3800,
  "config": {
    "warmup_iterations": 100,
    "measure_iterations": 1000,
    "batch_size": 1
  },
  "latency": {
    "min_ns": 1234567,
    "max_ns": 2345678,
    "mean_ns": 1500000,
    "median_ns": 1480000,
    "p95_ns": 1890000,
    "p99_ns": 2100000,
    "stddev_ns": 150000,
    "variance_ns2": 22500000000,
    "sample_count": 1000,
    "outlier_count": 3,
    "wcet_observed_ns": 2345678,
    "wcet_bound_ns": 3245678
  },
  "throughput": {
    "inferences_per_sec": 666,
    "samples_per_sec": 666,
    "bytes_per_sec": 2664000,
    "batch_size": 1
  },
  "verification": {
    "determinism_verified": true,
    "verification_failures": 0,
    "output_hash": "48f4ecebc0eec79ab15fe694717a831e7218993502f84c66f0ef0f3d178c0138",
    "result_hash": "abc123..."
  },
  "environment": {
    "stable": true,
    "start_freq_hz": 3800000000,
    "end_freq_hz": 3750000000,
    "min_freq_hz": 3700000000,
    "max_freq_hz": 3800000000,
    "start_temp_mC": 45000,
    "end_temp_mC": 52000,
    "max_temp_mC": 54000,
    "throttle_events": 0
  },
  "histogram": {
    "range_min_ns": 0,
    "range_max_ns": 10000000,
    "bin_width_ns": 100000,
    "num_bins": 100,
    "overflow_count": 0,
    "underflow_count": 0,
    "bins": [0, 0, 5, 23, 89, 234, ...]
  },
  "faults": {
    "overflow": false,
    "underflow": false,
    "div_zero": false,
    "timer_error": false,
    "verify_fail": false,
    "thermal_drift": false
  },
  "benchmark_start_ns": 1234567890123456789,
  "benchmark_end_ns": 1234567891623456789,
  "benchmark_duration_ns": 1500000000,
  "timestamp_unix": 1737500000
}
```

---

## 8. CSV Output Format

Header:
```
platform,cpu_model,min_ns,max_ns,mean_ns,median_ns,p95_ns,p99_ns,stddev_ns,inferences_per_sec,determinism_verified,output_hash,timestamp_unix
```

Data row:
```
x86_64,"Intel Core i7-10700K @ 3.80GHz",1234567,2345678,1500000,1480000,1890000,2100000,150000,666,true,48f4ecebc0eec79ab15fe694717a831e7218993502f84c66f0ef0f3d178c0138,1737500000
```

---

## 9. Comparison Output Format

```
══════════════════════════════════════════════════════════════════════════════
  Cross-Platform Performance Comparison
  Reference: x86_64    Target: riscv64
══════════════════════════════════════════════════════════════════════════════

Bit Identity: ✓ VERIFIED (outputs identical)

Latency (p99):
  x86_64:   1,234,567 ns
  riscv64:  2,567,890 ns
  Diff:     +1,333,323 ns
  Ratio:    2.08x slower

Throughput:
  x86_64:   8,100 inferences/sec
  riscv64:  3,890 inferences/sec
  Diff:     -4,210 inferences/sec
  Ratio:    0.48x

══════════════════════════════════════════════════════════════════════════════
```

---

## 10. Traceability Matrix

| Requirement | CB-MATH-001 | CB-STRUCT-001 | Test |
|-------------|-------------|---------------|------|
| REPORT-F-001 | — | §14 | test_write_json |
| REPORT-F-020 | — | — | test_write_csv |
| REPORT-F-030 | — | §14 | test_load_json |
| REPORT-F-040 | §8.3 | §11 | test_compare |
| REPORT-F-042 | §8.3 | — | test_compare_gate |
| REPORT-NF-001 | — | — | test_json_determinism |

---

## 11. Document Control

| Version | Date | Author | Change |
|---------|------|--------|--------|
| 1.0 | January 2026 | William Murray | Initial release |

---

*Built for safety. Designed for certification. Proven through testing.*

Copyright © 2026 The Murray Family Innovation Trust. All rights reserved.
