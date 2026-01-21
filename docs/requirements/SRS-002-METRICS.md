# SRS-002-METRICS: Statistics Module Requirements

**Component:** certifiable-bench  
**Module:** Metrics (`cb_metrics.h`, `src/metrics/`)  
**Version:** 1.0  
**Status:** Draft  
**Date:** January 2026  
**Traceability:** CB-MATH-001 §6

---

## 1. Purpose

This document specifies requirements for the statistics module, which computes latency metrics, histograms, and outlier detection using integer arithmetic only.

---

## 2. Scope

The metrics module provides:
- Basic statistics (min, max, mean, variance, standard deviation)
- Percentile computation (median, p95, p99)
- Histogram construction
- Outlier detection using modified Z-score
- WCET bound estimation

---

## 3. Definitions

| Term | Definition |
|------|------------|
| MAD | Median Absolute Deviation |
| Modified Z-score | Outlier detection metric: 0.6745 × (x - median) / MAD |
| WCET | Worst-Case Execution Time |
| Q32.32 | 64-bit fixed-point format with 32 integer and 32 fractional bits |

---

## 4. Functional Requirements

### 4.1 Basic Statistics

| ID | Requirement | Verification |
|----|-------------|--------------|
| METRICS-F-001 | The module SHALL provide a function `cb_compute_stats()` to compute statistics from latency samples. | Test |
| METRICS-F-002 | `cb_compute_stats()` SHALL compute minimum latency. | Test |
| METRICS-F-003 | `cb_compute_stats()` SHALL compute maximum latency. | Test |
| METRICS-F-004 | `cb_compute_stats()` SHALL compute arithmetic mean using integer division. | Test |
| METRICS-F-005 | `cb_compute_stats()` SHALL compute variance using Welford's algorithm. | Test |
| METRICS-F-006 | `cb_compute_stats()` SHALL compute standard deviation using integer square root. | Test |
| METRICS-F-007 | Mean computation SHALL use 64-bit accumulator to prevent overflow. | Inspection |
| METRICS-F-008 | If accumulator overflow is detected, `overflow` fault flag SHALL be set. | Test |
| METRICS-F-009 | `cb_compute_stats()` SHALL return `CB_ERR_NULL_PTR` if samples or stats is NULL. | Test |
| METRICS-F-010 | `cb_compute_stats()` SHALL return `CB_ERR_DIV_ZERO` if count is 0. | Test |

### 4.2 Integer Square Root

| ID | Requirement | Verification |
|----|-------------|--------------|
| METRICS-F-020 | The module SHALL provide a function `cb_isqrt64()` for integer square root. | Test |
| METRICS-F-021 | `cb_isqrt64()` SHALL return floor(√n) for all inputs. | Test |
| METRICS-F-022 | `cb_isqrt64()` SHALL use binary search (no floating-point). | Inspection |
| METRICS-F-023 | `cb_isqrt64()` SHALL complete in O(32) iterations maximum. | Analysis |
| METRICS-F-024 | `cb_isqrt64(0)` SHALL return 0. | Test |
| METRICS-F-025 | `cb_isqrt64(UINT64_MAX)` SHALL return 0xFFFFFFFF. | Test |

### 4.3 Percentile Computation

| ID | Requirement | Verification |
|----|-------------|--------------|
| METRICS-F-030 | The module SHALL provide a function `cb_percentile()` for percentile computation. | Test |
| METRICS-F-031 | `cb_percentile()` SHALL accept sorted samples and percentile (0-100). | Test |
| METRICS-F-032 | `cb_percentile()` SHALL use linear interpolation between adjacent samples. | Test |
| METRICS-F-033 | Interpolation SHALL use integer arithmetic per CB-MATH-001 §6.4. | Inspection |
| METRICS-F-034 | `cb_percentile(samples, n, 0)` SHALL return samples[0]. | Test |
| METRICS-F-035 | `cb_percentile(samples, n, 100)` SHALL return samples[n-1]. | Test |
| METRICS-F-036 | `cb_percentile(samples, n, 50)` SHALL return the median. | Test |
| METRICS-F-037 | `cb_compute_stats()` SHALL compute p50, p95, and p99 percentiles. | Test |

### 4.4 Histogram Construction

| ID | Requirement | Verification |
|----|-------------|--------------|
| METRICS-F-040 | The module SHALL provide a function `cb_build_histogram()` for histogram construction. | Test |
| METRICS-F-041 | `cb_build_histogram()` SHALL accept range bounds (min_ns, max_ns) and bin count. | Test |
| METRICS-F-042 | Bin width SHALL be computed as (max_ns - min_ns) / num_bins using integer division. | Inspection |
| METRICS-F-043 | Samples below min_ns SHALL increment `underflow_count`. | Test |
| METRICS-F-044 | Samples at or above max_ns SHALL increment `overflow_count`. | Test |
| METRICS-F-045 | Bin assignment SHALL use: bin = (sample - min_ns) / bin_width. | Inspection |
| METRICS-F-046 | `cb_build_histogram()` SHALL NOT allocate memory (caller provides bin array). | Inspection |
| METRICS-F-047 | Sum of all bin counts plus overflow and underflow SHALL equal sample count. | Test |

### 4.5 Outlier Detection

| ID | Requirement | Verification |
|----|-------------|--------------|
| METRICS-F-050 | The module SHALL provide a function `cb_detect_outliers()` for outlier detection. | Test |
| METRICS-F-051 | Outlier detection SHALL use modified Z-score per CB-MATH-001 §6.5. | Inspection |
| METRICS-F-052 | A sample SHALL be flagged as outlier if |modified_z| > 3.5. | Test |
| METRICS-F-053 | Modified Z-score SHALL be computed using integer arithmetic (scaled by 10000). | Inspection |
| METRICS-F-054 | If MAD = 0, no outliers SHALL be flagged. | Test |
| METRICS-F-055 | `cb_detect_outliers()` SHALL return outlier indices in caller-provided array. | Test |
| METRICS-F-056 | `cb_detect_outliers()` SHALL respect max_outliers limit. | Test |
| METRICS-F-057 | `cb_compute_stats()` SHALL populate `outlier_count` field. | Test |

### 4.6 WCET Bound Estimation

| ID | Requirement | Verification |
|----|-------------|--------------|
| METRICS-F-060 | `cb_compute_stats()` SHALL compute `wcet_observed_ns` as maximum sample value. | Test |
| METRICS-F-061 | `cb_compute_stats()` SHALL compute `wcet_bound_ns` as max + 6×stddev. | Test |
| METRICS-F-062 | WCET bound computation SHALL use integer arithmetic. | Inspection |
| METRICS-F-063 | If stddev computation overflows, `wcet_bound_ns` SHALL equal `wcet_observed_ns`. | Test |

---

## 5. Non-Functional Requirements

### 5.1 Determinism

| ID | Requirement | Verification |
|----|-------------|--------------|
| METRICS-NF-001 | All statistical computations SHALL produce bit-identical results across platforms. | Test |
| METRICS-NF-002 | No floating-point operations SHALL be used in any computation. | Inspection |
| METRICS-NF-003 | Sorting for percentile computation SHALL use a deterministic algorithm. | Inspection |

### 5.2 Memory

| ID | Requirement | Verification |
|----|-------------|--------------|
| METRICS-NF-010 | No function in the metrics module SHALL allocate memory. | Inspection |
| METRICS-NF-011 | All output buffers SHALL be caller-provided. | Inspection |
| METRICS-NF-012 | Working memory for sorting SHALL be caller-provided or in-place. | Inspection |

### 5.3 Performance

| ID | Requirement | Verification |
|----|-------------|--------------|
| METRICS-NF-020 | `cb_compute_stats()` SHALL complete in O(n log n) time (dominated by sort). | Analysis |
| METRICS-NF-021 | `cb_build_histogram()` SHALL complete in O(n) time. | Analysis |
| METRICS-NF-022 | `cb_detect_outliers()` SHALL complete in O(n log n) time. | Analysis |

---

## 6. Interface Definition

```c
/**
 * @brief Compute statistics from latency samples
 * @param samples Array of latency measurements (nanoseconds)
 * @param count Number of samples
 * @param stats Output statistics
 * @return CB_OK on success
 *
 * @satisfies METRICS-F-001 through METRICS-F-010
 */
cb_result_code_t cb_compute_stats(const uint64_t *samples,
                                   uint32_t count,
                                   cb_latency_stats_t *stats);

/**
 * @brief Integer square root
 * @param n Input value
 * @return floor(sqrt(n))
 *
 * @satisfies METRICS-F-020 through METRICS-F-025
 */
uint64_t cb_isqrt64(uint64_t n);

/**
 * @brief Compute percentile from sorted samples
 * @param sorted_samples Sorted array of measurements
 * @param count Number of samples
 * @param percentile Percentile (0-100)
 * @return Percentile value
 *
 * @satisfies METRICS-F-030 through METRICS-F-036
 */
uint64_t cb_percentile(const uint64_t *sorted_samples,
                        uint32_t count,
                        uint32_t percentile);

/**
 * @brief Build histogram from samples
 * @param samples Array of measurements
 * @param count Number of samples
 * @param histogram Output histogram structure (bins caller-provided)
 * @return CB_OK on success
 *
 * @satisfies METRICS-F-040 through METRICS-F-047
 */
cb_result_code_t cb_build_histogram(const uint64_t *samples,
                                     uint32_t count,
                                     cb_histogram_t *histogram);

/**
 * @brief Detect outliers using modified Z-score
 * @param samples Array of measurements
 * @param count Number of samples
 * @param outlier_indices Output array of outlier indices
 * @param max_outliers Maximum outliers to report
 * @param outlier_count Actual count found
 * @return CB_OK on success
 *
 * @satisfies METRICS-F-050 through METRICS-F-056
 */
cb_result_code_t cb_detect_outliers(const uint64_t *samples,
                                     uint32_t count,
                                     uint32_t *outlier_indices,
                                     uint32_t max_outliers,
                                     uint32_t *outlier_count);
```

---

## 7. Test Vectors

### 7.1 Integer Square Root

| Input | Expected |
|-------|----------|
| 0 | 0 |
| 1 | 1 |
| 4 | 2 |
| 5 | 2 |
| 100 | 10 |
| 101 | 10 |
| 0xFFFFFFFFFFFFFFFF | 0xFFFFFFFF |

### 7.2 Percentile Computation

For samples [100, 200, 300, 400, 500] (n=5):

| Percentile | Expected |
|------------|----------|
| 0 | 100 |
| 25 | 200 |
| 50 | 300 |
| 75 | 400 |
| 100 | 500 |

### 7.3 Outlier Detection

For samples [100, 100, 100, 100, 1000]:

| Sample | Outlier? |
|--------|----------|
| 100 | No |
| 100 | No |
| 100 | No |
| 100 | No |
| 1000 | Yes |

---

## 8. Traceability Matrix

| Requirement | CB-MATH-001 | CB-STRUCT-001 | Test |
|-------------|-------------|---------------|------|
| METRICS-F-001 | §6 | §4 | test_compute_stats |
| METRICS-F-004 | §6.1 | — | test_mean |
| METRICS-F-005 | §6.2 | — | test_variance |
| METRICS-F-006 | §6.2, §6.3 | — | test_stddev |
| METRICS-F-020 | §6.3 | — | test_isqrt64 |
| METRICS-F-030 | §6.4 | — | test_percentile |
| METRICS-F-040 | — | §4 | test_histogram |
| METRICS-F-050 | §6.5 | — | test_outliers |
| METRICS-F-060 | §6.6 | §4 | test_wcet |

---

## 9. Document Control

| Version | Date | Author | Change |
|---------|------|--------|--------|
| 1.0 | January 2026 | William Murray | Initial release |

---

*Built for safety. Designed for certification. Proven through testing.*

Copyright © 2026 The Murray Family Innovation Trust. All rights reserved.
