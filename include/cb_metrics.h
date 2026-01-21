/**
 * @file cb_metrics.h
 * @brief Statistics and metrics API for certifiable-bench
 *
 * Provides latency statistics, percentile computation, histogram construction,
 * and outlier detection using integer arithmetic only.
 *
 * @traceability SRS-002-METRICS
 *
 * Copyright © 2026 The Murray Family Innovation Trust. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CB_METRICS_H
#define CB_METRICS_H

#include "cb_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*─────────────────────────────────────────────────────────────────────────────
 * Integer Square Root (CB-MATH-001 §6.3)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Integer square root using binary search
 *
 * Returns floor(√n) for all inputs. Uses no floating-point operations.
 * Completes in O(32) iterations maximum.
 *
 * @param n Input value
 * @return floor(sqrt(n))
 *
 * @satisfies METRICS-F-020, METRICS-F-021, METRICS-F-022, METRICS-F-023,
 *            METRICS-F-024, METRICS-F-025
 * @traceability SRS-002-METRICS §4.2, CB-MATH-001 §6.3
 */
uint64_t cb_isqrt64(uint64_t n);

/*─────────────────────────────────────────────────────────────────────────────
 * Percentile Computation (CB-MATH-001 §6.4)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Compute percentile from sorted samples
 *
 * Uses linear interpolation between adjacent samples.
 * All arithmetic is integer-only per CB-MATH-001 §6.4.
 *
 * @param sorted_samples Sorted array of measurements (ascending)
 * @param count          Number of samples (must be > 0)
 * @param percentile     Percentile to compute (0-100)
 * @return Percentile value, or 0 if inputs invalid
 *
 * @satisfies METRICS-F-030, METRICS-F-031, METRICS-F-032, METRICS-F-033,
 *            METRICS-F-034, METRICS-F-035, METRICS-F-036
 * @traceability SRS-002-METRICS §4.3, CB-MATH-001 §6.4
 */
uint64_t cb_percentile(const uint64_t *sorted_samples,
                       uint32_t count,
                       uint32_t percentile);

/*─────────────────────────────────────────────────────────────────────────────
 * Basic Statistics (CB-MATH-001 §6.1, §6.2)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Compute comprehensive statistics from latency samples
 *
 * Computes min, max, mean, variance, stddev, percentiles (p50, p95, p99),
 * outlier count, and WCET bounds. Sorts the input array in-place.
 *
 * Uses Welford's algorithm for numerically stable variance computation.
 * All arithmetic is integer-only.
 *
 * @param samples Array of latency measurements in nanoseconds (will be sorted)
 * @param count   Number of samples
 * @param stats   Output statistics structure
 * @param faults  Fault flags (overflow, div_zero set on error)
 * @return CB_OK on success
 * @return CB_ERR_NULL_PTR if samples, stats, or faults is NULL
 * @return CB_ERR_OVERFLOW if accumulator overflow detected
 *
 * @note The samples array is sorted in-place for percentile computation.
 *       Caller must provide a copy if original order must be preserved.
 *
 * @satisfies METRICS-F-001 through METRICS-F-010, METRICS-F-037,
 *            METRICS-F-057, METRICS-F-060 through METRICS-F-063
 * @traceability SRS-002-METRICS §4.1, CB-MATH-001 §6.1, §6.2
 */
cb_result_code_t cb_compute_stats(uint64_t *samples,
                                  uint32_t count,
                                  cb_latency_stats_t *stats,
                                  cb_fault_flags_t *faults);

/*─────────────────────────────────────────────────────────────────────────────
 * Histogram Construction (CB-MATH-001 §6.4)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Initialise histogram structure
 *
 * Sets up histogram bounds and bin width. Does not allocate memory —
 * caller must provide the bins array.
 *
 * @param histogram Output histogram structure
 * @param bins      Caller-provided bin array
 * @param num_bins  Number of bins
 * @param min_ns    Histogram lower bound (inclusive)
 * @param max_ns    Histogram upper bound (exclusive)
 * @return CB_OK on success
 * @return CB_ERR_NULL_PTR if histogram or bins is NULL
 * @return CB_ERR_INVALID_CONFIG if num_bins is 0 or min_ns >= max_ns
 *
 * @traceability SRS-002-METRICS §4.4
 */
cb_result_code_t cb_histogram_init(cb_histogram_t *histogram,
                                   cb_histogram_bin_t *bins,
                                   uint32_t num_bins,
                                   uint64_t min_ns,
                                   uint64_t max_ns);

/**
 * @brief Build histogram from samples
 *
 * Populates histogram bins from sample array. Tracks underflow and overflow.
 * Does not allocate memory — histogram must be initialised with cb_histogram_init().
 *
 * @param samples   Array of latency measurements in nanoseconds
 * @param count     Number of samples
 * @param histogram Initialised histogram structure
 * @return CB_OK on success
 * @return CB_ERR_NULL_PTR if samples or histogram is NULL
 *
 * @satisfies METRICS-F-040 through METRICS-F-047
 * @traceability SRS-002-METRICS §4.4
 */
cb_result_code_t cb_histogram_build(const uint64_t *samples,
                                    uint32_t count,
                                    cb_histogram_t *histogram);

/*─────────────────────────────────────────────────────────────────────────────
 * Outlier Detection (CB-MATH-001 §6.5)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Detect outliers using modified Z-score (MAD-based)
 *
 * Identifies samples with |modified Z-score| > 3.5.
 * Uses integer arithmetic scaled by 10000 to avoid floating-point.
 *
 * If MAD = 0 (all samples identical), no outliers are flagged.
 *
 * @param samples         Array of latency measurements (will be copied internally for MAD)
 * @param count           Number of samples
 * @param outlier_flags   Output boolean array (true = outlier), caller-provided, size = count
 * @param outlier_count   Output: number of outliers detected
 * @return CB_OK on success
 * @return CB_ERR_NULL_PTR if any pointer is NULL
 *
 * @satisfies METRICS-F-050 through METRICS-F-056
 * @traceability SRS-002-METRICS §4.5, CB-MATH-001 §6.5
 */
cb_result_code_t cb_detect_outliers(const uint64_t *samples,
                                    uint32_t count,
                                    bool *outlier_flags,
                                    uint32_t *outlier_count);

/*─────────────────────────────────────────────────────────────────────────────
 * Sorting (Deterministic)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Sort array of uint64_t values in ascending order
 *
 * Uses a deterministic sorting algorithm (insertion sort for small arrays,
 * heapsort for large arrays — no quicksort due to non-deterministic pivots).
 *
 * @param arr   Array to sort
 * @param count Number of elements
 *
 * @satisfies METRICS-NF-003
 * @traceability SRS-002-METRICS §5.1
 */
void cb_sort_u64(uint64_t *arr, uint32_t count);

#ifdef __cplusplus
}
#endif

#endif /* CB_METRICS_H */
