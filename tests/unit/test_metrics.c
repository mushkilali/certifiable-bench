/**
 * @file test_metrics.c
 * @brief Unit tests for cb_metrics API
 *
 * Tests all metrics functions against SRS-002-METRICS requirements.
 * Includes test vectors from CB-MATH-001 §10.
 *
 * @traceability SRS-002-METRICS §7, §8
 *
 * Copyright © 2026 The Murray Family Innovation Trust. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cb_metrics.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*─────────────────────────────────────────────────────────────────────────────
 * Test Infrastructure
 *─────────────────────────────────────────────────────────────────────────────*/

static int g_tests_run = 0;
static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define TEST_ASSERT(cond, msg) do { \
    g_tests_run++; \
    if (!(cond)) { \
        printf("    FAIL: %s\n", msg); \
        printf("          at %s:%d\n", __FILE__, __LINE__); \
        g_tests_failed++; \
        return -1; \
    } else { \
        g_tests_passed++; \
    } \
} while(0)

#define TEST_ASSERT_EQ(a, b, msg) TEST_ASSERT((a) == (b), msg)
#define TEST_ASSERT_NE(a, b, msg) TEST_ASSERT((a) != (b), msg)
#define TEST_ASSERT_GT(a, b, msg) TEST_ASSERT((a) > (b), msg)
#define TEST_ASSERT_GE(a, b, msg) TEST_ASSERT((a) >= (b), msg)
#define TEST_ASSERT_LT(a, b, msg) TEST_ASSERT((a) < (b), msg)
#define TEST_ASSERT_LE(a, b, msg) TEST_ASSERT((a) <= (b), msg)

#define RUN_TEST(fn) do { \
    printf("  %s\n", #fn); \
    int result = fn(); \
    if (result == 0) { \
        printf("    PASS\n"); \
    } \
} while(0)

/*─────────────────────────────────────────────────────────────────────────────
 * Test: Integer Square Root (SRS-002-METRICS §4.2)
 * Test Vectors: CB-MATH-001 §10.1
 * Traceability: METRICS-F-020 through METRICS-F-025
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Test cb_isqrt64 with test vectors from CB-MATH-001 §10.1
 * @satisfies METRICS-F-020, METRICS-F-021, METRICS-F-024, METRICS-F-025
 */
static int test_isqrt64_vectors(void)
{
    /* CB-MATH-001 §10.1 test vectors */
    TEST_ASSERT_EQ(cb_isqrt64(0), 0, "isqrt(0) = 0");
    TEST_ASSERT_EQ(cb_isqrt64(1), 1, "isqrt(1) = 1");
    TEST_ASSERT_EQ(cb_isqrt64(4), 2, "isqrt(4) = 2");
    TEST_ASSERT_EQ(cb_isqrt64(5), 2, "isqrt(5) = 2 (floor)");
    TEST_ASSERT_EQ(cb_isqrt64(100), 10, "isqrt(100) = 10");
    TEST_ASSERT_EQ(cb_isqrt64(101), 10, "isqrt(101) = 10 (floor)");
    TEST_ASSERT_EQ(cb_isqrt64(UINT64_MAX), 0xFFFFFFFFULL, "isqrt(UINT64_MAX) = 0xFFFFFFFF");

    return 0;
}

/**
 * @brief Test cb_isqrt64 with perfect squares
 * @satisfies METRICS-F-021
 */
static int test_isqrt64_perfect_squares(void)
{
    uint64_t i;

    for (i = 0; i <= 1000; i++) {
        uint64_t sq = i * i;
        uint64_t result = cb_isqrt64(sq);
        TEST_ASSERT_EQ(result, i, "isqrt of perfect square should be exact");
    }

    /* Larger perfect squares */
    TEST_ASSERT_EQ(cb_isqrt64(1000000ULL), 1000, "isqrt(1000000) = 1000");
    TEST_ASSERT_EQ(cb_isqrt64(1000000000000ULL), 1000000, "isqrt(10^12) = 10^6");

    return 0;
}

/**
 * @brief Test cb_isqrt64 returns floor for non-perfect squares
 * @satisfies METRICS-F-021
 */
static int test_isqrt64_floor(void)
{
    uint64_t i;

    for (i = 2; i <= 100; i++) {
        uint64_t sq = i * i;
        uint64_t result;

        /* Test values just above perfect square */
        result = cb_isqrt64(sq + 1);
        TEST_ASSERT_EQ(result, i, "floor property for sq+1");

        /* Test values just below next perfect square */
        result = cb_isqrt64((i + 1) * (i + 1) - 1);
        TEST_ASSERT_EQ(result, i, "floor property for (i+1)^2 - 1");
    }

    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Test: Sorting (SRS-002-METRICS §5.1)
 * Traceability: METRICS-NF-003
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Test cb_sort_u64 sorts correctly
 * @satisfies METRICS-NF-003
 */
static int test_sort_basic(void)
{
    uint64_t arr[] = {5, 2, 8, 1, 9, 3, 7, 4, 6, 0};
    uint32_t n = 10;
    uint32_t i;

    cb_sort_u64(arr, n);

    for (i = 0; i < n; i++) {
        TEST_ASSERT_EQ(arr[i], i, "sorted array should be 0..9");
    }

    return 0;
}

/**
 * @brief Test cb_sort_u64 with already sorted array
 */
static int test_sort_already_sorted(void)
{
    uint64_t arr[] = {1, 2, 3, 4, 5};
    uint32_t n = 5;

    cb_sort_u64(arr, n);

    TEST_ASSERT_EQ(arr[0], 1, "first element");
    TEST_ASSERT_EQ(arr[4], 5, "last element");

    return 0;
}

/**
 * @brief Test cb_sort_u64 with reverse sorted array
 */
static int test_sort_reverse(void)
{
    uint64_t arr[] = {5, 4, 3, 2, 1};
    uint32_t n = 5;

    cb_sort_u64(arr, n);

    TEST_ASSERT_EQ(arr[0], 1, "first element after sort");
    TEST_ASSERT_EQ(arr[4], 5, "last element after sort");

    return 0;
}

/**
 * @brief Test cb_sort_u64 with duplicates
 */
static int test_sort_duplicates(void)
{
    uint64_t arr[] = {3, 1, 4, 1, 5, 9, 2, 6, 5, 3};
    uint32_t n = 10;
    uint32_t i;

    cb_sort_u64(arr, n);

    /* Check sorted order */
    for (i = 1; i < n; i++) {
        TEST_ASSERT_LE(arr[i - 1], arr[i], "should be non-decreasing");
    }

    return 0;
}

/**
 * @brief Test cb_sort_u64 with large array (uses heapsort)
 */
static int test_sort_large(void)
{
    uint64_t arr[200];
    uint32_t i;

    /* Fill with reverse order */
    for (i = 0; i < 200; i++) {
        arr[i] = 199 - i;
    }

    cb_sort_u64(arr, 200);

    /* Verify sorted */
    for (i = 0; i < 200; i++) {
        TEST_ASSERT_EQ(arr[i], i, "large array should be sorted");
    }

    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Test: Percentile Computation (SRS-002-METRICS §4.3)
 * Test Vectors: CB-MATH-001 §10.2
 * Traceability: METRICS-F-030 through METRICS-F-036
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Test cb_percentile with test vectors from CB-MATH-001 §10.2
 * @satisfies METRICS-F-030 through METRICS-F-036
 */
static int test_percentile_vectors(void)
{
    /* CB-MATH-001 §10.2: samples [100, 200, 300, 400, 500] */
    uint64_t samples[] = {100, 200, 300, 400, 500};
    uint32_t n = 5;

    TEST_ASSERT_EQ(cb_percentile(samples, n, 0), 100, "p0 = 100");
    TEST_ASSERT_EQ(cb_percentile(samples, n, 25), 200, "p25 = 200");
    TEST_ASSERT_EQ(cb_percentile(samples, n, 50), 300, "p50 = 300 (median)");
    TEST_ASSERT_EQ(cb_percentile(samples, n, 75), 400, "p75 = 400");
    TEST_ASSERT_EQ(cb_percentile(samples, n, 100), 500, "p100 = 500");

    return 0;
}

/**
 * @brief Test cb_percentile boundary conditions
 * @satisfies METRICS-F-034, METRICS-F-035
 */
static int test_percentile_boundaries(void)
{
    uint64_t samples[] = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
    uint32_t n = 10;

    /* p0 should return first element */
    TEST_ASSERT_EQ(cb_percentile(samples, n, 0), 10, "p0 = first");

    /* p100 should return last element */
    TEST_ASSERT_EQ(cb_percentile(samples, n, 100), 100, "p100 = last");

    return 0;
}

/**
 * @brief Test cb_percentile with single element
 */
static int test_percentile_single(void)
{
    uint64_t samples[] = {42};

    TEST_ASSERT_EQ(cb_percentile(samples, 1, 0), 42, "single: p0");
    TEST_ASSERT_EQ(cb_percentile(samples, 1, 50), 42, "single: p50");
    TEST_ASSERT_EQ(cb_percentile(samples, 1, 100), 42, "single: p100");

    return 0;
}

/**
 * @brief Test cb_percentile with two elements
 */
static int test_percentile_two(void)
{
    uint64_t samples[] = {100, 200};

    TEST_ASSERT_EQ(cb_percentile(samples, 2, 0), 100, "two: p0 = 100");
    TEST_ASSERT_EQ(cb_percentile(samples, 2, 50), 150, "two: p50 = 150 (interpolated)");
    TEST_ASSERT_EQ(cb_percentile(samples, 2, 100), 200, "two: p100 = 200");

    return 0;
}

/**
 * @brief Test cb_percentile null handling
 */
static int test_percentile_null(void)
{
    TEST_ASSERT_EQ(cb_percentile(NULL, 5, 50), 0, "NULL samples returns 0");
    uint64_t samples[] = {1, 2, 3};
    TEST_ASSERT_EQ(cb_percentile(samples, 0, 50), 0, "count=0 returns 0");

    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Test: Basic Statistics (SRS-002-METRICS §4.1)
 * Traceability: METRICS-F-001 through METRICS-F-010
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Test cb_compute_stats basic functionality
 * @satisfies METRICS-F-001, METRICS-F-002, METRICS-F-003, METRICS-F-004
 */
static int test_stats_basic(void)
{
    uint64_t samples[] = {100, 200, 300, 400, 500};
    uint32_t n = 5;
    cb_latency_stats_t stats;
    cb_fault_flags_t faults;
    cb_result_code_t rc;

    cb_fault_clear(&faults);
    rc = cb_compute_stats(samples, n, &stats, &faults);

    TEST_ASSERT_EQ(rc, CB_OK, "compute_stats should succeed");
    TEST_ASSERT_EQ(cb_has_fault(&faults), false, "no faults");

    TEST_ASSERT_EQ(stats.min_ns, 100, "min = 100");
    TEST_ASSERT_EQ(stats.max_ns, 500, "max = 500");
    TEST_ASSERT_EQ(stats.mean_ns, 300, "mean = 300");
    TEST_ASSERT_EQ(stats.sample_count, 5, "count = 5");

    return 0;
}

/**
 * @brief Test cb_compute_stats percentiles
 * @satisfies METRICS-F-037
 */
static int test_stats_percentiles(void)
{
    uint64_t samples[] = {100, 200, 300, 400, 500};
    uint32_t n = 5;
    cb_latency_stats_t stats;
    cb_fault_flags_t faults;

    cb_fault_clear(&faults);
    cb_compute_stats(samples, n, &stats, &faults);

    TEST_ASSERT_EQ(stats.median_ns, 300, "median = 300");
    /* p95 and p99 will be interpolated towards 500 */
    TEST_ASSERT_GE(stats.p95_ns, 400, "p95 >= 400");
    TEST_ASSERT_GE(stats.p99_ns, 400, "p99 >= 400");

    return 0;
}

/**
 * @brief Test cb_compute_stats variance and stddev
 * @satisfies METRICS-F-005, METRICS-F-006
 */
static int test_stats_variance(void)
{
    /* Samples with known variance */
    uint64_t samples[] = {2, 4, 4, 4, 5, 5, 7, 9};
    uint32_t n = 8;
    cb_latency_stats_t stats;
    cb_fault_flags_t faults;

    cb_fault_clear(&faults);
    cb_compute_stats(samples, n, &stats, &faults);

    /* Mean = 40/8 = 5 */
    TEST_ASSERT_EQ(stats.mean_ns, 5, "mean = 5");

    /* Variance = 32/7 ≈ 4.57, stddev ≈ 2.13 → integer stddev = 2 */
    TEST_ASSERT_GT(stats.variance_ns2, 0, "variance > 0");
    TEST_ASSERT_GT(stats.stddev_ns, 0, "stddev > 0");
    TEST_ASSERT_LE(stats.stddev_ns, 3, "stddev should be ~2");

    return 0;
}

/**
 * @brief Test cb_compute_stats WCET computation
 * @satisfies METRICS-F-060, METRICS-F-061
 */
static int test_stats_wcet(void)
{
    uint64_t samples[] = {100, 100, 100, 100, 200};
    uint32_t n = 5;
    cb_latency_stats_t stats;
    cb_fault_flags_t faults;

    cb_fault_clear(&faults);
    cb_compute_stats(samples, n, &stats, &faults);

    TEST_ASSERT_EQ(stats.wcet_observed_ns, 200, "wcet_observed = max = 200");
    TEST_ASSERT_GE(stats.wcet_bound_ns, stats.wcet_observed_ns,
                   "wcet_bound >= wcet_observed");

    return 0;
}

/**
 * @brief Test cb_compute_stats null handling
 * @satisfies METRICS-F-009
 */
static int test_stats_null(void)
{
    uint64_t samples[] = {1, 2, 3};
    cb_latency_stats_t stats;
    cb_fault_flags_t faults;
    cb_result_code_t rc;

    rc = cb_compute_stats(NULL, 3, &stats, &faults);
    TEST_ASSERT_EQ(rc, CB_ERR_NULL_PTR, "NULL samples");

    rc = cb_compute_stats(samples, 3, NULL, &faults);
    TEST_ASSERT_EQ(rc, CB_ERR_NULL_PTR, "NULL stats");

    rc = cb_compute_stats(samples, 3, &stats, NULL);
    TEST_ASSERT_EQ(rc, CB_ERR_NULL_PTR, "NULL faults");

    return 0;
}

/**
 * @brief Test cb_compute_stats with count=0
 * @satisfies METRICS-F-010
 */
static int test_stats_empty(void)
{
    uint64_t samples[] = {1};
    cb_latency_stats_t stats;
    cb_fault_flags_t faults;

    cb_fault_clear(&faults);
    (void)cb_compute_stats(samples, 0, &stats, &faults);

    TEST_ASSERT_EQ(faults.div_zero, 1, "div_zero flag should be set");

    return 0;
}

/**
 * @brief Test cb_compute_stats with single sample
 */
static int test_stats_single(void)
{
    uint64_t samples[] = {42};
    cb_latency_stats_t stats;
    cb_fault_flags_t faults;

    cb_fault_clear(&faults);
    cb_compute_stats(samples, 1, &stats, &faults);

    TEST_ASSERT_EQ(stats.min_ns, 42, "single: min");
    TEST_ASSERT_EQ(stats.max_ns, 42, "single: max");
    TEST_ASSERT_EQ(stats.mean_ns, 42, "single: mean");
    TEST_ASSERT_EQ(stats.stddev_ns, 0, "single: stddev = 0");

    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Test: Histogram Construction (SRS-002-METRICS §4.4)
 * Traceability: METRICS-F-040 through METRICS-F-047
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Test cb_histogram_init and cb_histogram_build
 * @satisfies METRICS-F-040, METRICS-F-041, METRICS-F-046
 */
static int test_histogram_basic(void)
{
    cb_histogram_t hist;
    cb_histogram_bin_t bins[10];
    cb_result_code_t rc;

    rc = cb_histogram_init(&hist, bins, 10, 0, 1000);
    TEST_ASSERT_EQ(rc, CB_OK, "histogram_init should succeed");
    TEST_ASSERT_EQ(hist.num_bins, 10, "num_bins = 10");
    TEST_ASSERT_EQ(hist.bin_width_ns, 100, "bin_width = 100");

    return 0;
}

/**
 * @brief Test histogram bin assignment
 * @satisfies METRICS-F-045, METRICS-F-047
 */
static int test_histogram_binning(void)
{
    cb_histogram_t hist;
    cb_histogram_bin_t bins[5];
    uint64_t samples[] = {50, 150, 250, 350, 450};
    uint32_t n = 5;
    cb_result_code_t rc;
    uint32_t total;
    uint32_t i;

    cb_histogram_init(&hist, bins, 5, 0, 500);
    rc = cb_histogram_build(samples, n, &hist);

    TEST_ASSERT_EQ(rc, CB_OK, "histogram_build should succeed");

    /* Each sample should land in a different bin */
    total = hist.underflow_count + hist.overflow_count;
    for (i = 0; i < 5; i++) {
        total += bins[i].count;
    }
    TEST_ASSERT_EQ(total, n, "total count should equal sample count");

    return 0;
}

/**
 * @brief Test histogram underflow/overflow
 * @satisfies METRICS-F-043, METRICS-F-044
 */
static int test_histogram_overflow(void)
{
    cb_histogram_t hist;
    cb_histogram_bin_t bins[5];
    uint64_t samples[] = {50, 100, 200, 600, 700};  /* 600, 700 overflow */
    uint32_t n = 5;

    cb_histogram_init(&hist, bins, 5, 100, 500);
    cb_histogram_build(samples, n, &hist);

    TEST_ASSERT_EQ(hist.underflow_count, 1, "50 is underflow");
    TEST_ASSERT_EQ(hist.overflow_count, 2, "600, 700 are overflow");

    return 0;
}

/**
 * @brief Test histogram null handling
 */
static int test_histogram_null(void)
{
    cb_histogram_t hist;
    cb_histogram_bin_t bins[5];
    uint64_t samples[] = {1, 2, 3};
    cb_result_code_t rc;

    rc = cb_histogram_init(NULL, bins, 5, 0, 100);
    TEST_ASSERT_EQ(rc, CB_ERR_NULL_PTR, "NULL histogram");

    rc = cb_histogram_init(&hist, NULL, 5, 0, 100);
    TEST_ASSERT_EQ(rc, CB_ERR_NULL_PTR, "NULL bins");

    cb_histogram_init(&hist, bins, 5, 0, 100);
    rc = cb_histogram_build(NULL, 3, &hist);
    TEST_ASSERT_EQ(rc, CB_ERR_NULL_PTR, "NULL samples");

    rc = cb_histogram_build(samples, 3, NULL);
    TEST_ASSERT_EQ(rc, CB_ERR_NULL_PTR, "NULL histogram in build");

    return 0;
}

/**
 * @brief Test histogram invalid config
 */
static int test_histogram_invalid(void)
{
    cb_histogram_t hist;
    cb_histogram_bin_t bins[5];
    cb_result_code_t rc;

    rc = cb_histogram_init(&hist, bins, 0, 0, 100);
    TEST_ASSERT_EQ(rc, CB_ERR_INVALID_CONFIG, "num_bins = 0");

    rc = cb_histogram_init(&hist, bins, 5, 100, 100);
    TEST_ASSERT_EQ(rc, CB_ERR_INVALID_CONFIG, "min >= max");

    rc = cb_histogram_init(&hist, bins, 5, 200, 100);
    TEST_ASSERT_EQ(rc, CB_ERR_INVALID_CONFIG, "min > max");

    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Test: Outlier Detection (SRS-002-METRICS §4.5)
 * Test Vectors: CB-MATH-001 §10.3
 * Traceability: METRICS-F-050 through METRICS-F-056
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Test cb_detect_outliers with clear outlier
 * @satisfies METRICS-F-050, METRICS-F-052
 */
static int test_outliers_vectors(void)
{
    /*
     * For MAD-based outlier detection to work, we need MAD > 0.
     * With [100, 100, 100, 100, 1000], median=100, deviations=[0,0,0,0,900], MAD=0.
     * MAD=0 means "no outliers" per CB-MATH-001 §6.5.
     *
     * Use a sample set where MAD > 0:
     * [100, 110, 120, 130, 1000]
     * median = 120, deviations = [20, 10, 0, 10, 880], MAD = 10
     * For 1000: modified_z_scaled = 6745 * 880 / 10 = 593560 > 35000 → outlier
     */
    uint64_t samples[] = {100, 110, 120, 130, 1000};
    uint32_t n = 5;
    bool outlier_flags[5];
    uint32_t outlier_count;
    cb_result_code_t rc;

    rc = cb_detect_outliers(samples, n, outlier_flags, &outlier_count);

    TEST_ASSERT_EQ(rc, CB_OK, "detect_outliers should succeed");
    TEST_ASSERT_EQ(outlier_count, 1, "should find 1 outlier");
    TEST_ASSERT_EQ(outlier_flags[4], true, "1000 should be flagged");
    TEST_ASSERT_EQ(outlier_flags[0], false, "100 should not be flagged");
    TEST_ASSERT_EQ(outlier_flags[2], false, "120 should not be flagged");

    return 0;
}

/**
 * @brief Test cb_detect_outliers with no outliers
 * @satisfies METRICS-F-052
 */
static int test_outliers_none(void)
{
    uint64_t samples[] = {100, 101, 102, 103, 104};
    uint32_t n = 5;
    bool outlier_flags[5];
    uint32_t outlier_count;

    cb_detect_outliers(samples, n, outlier_flags, &outlier_count);

    TEST_ASSERT_EQ(outlier_count, 0, "should find no outliers");

    return 0;
}

/**
 * @brief Test cb_detect_outliers with MAD = 0 (all identical)
 * @satisfies METRICS-F-054
 */
static int test_outliers_mad_zero(void)
{
    uint64_t samples[] = {100, 100, 100, 100, 100};
    uint32_t n = 5;
    bool outlier_flags[5];
    uint32_t outlier_count;

    cb_detect_outliers(samples, n, outlier_flags, &outlier_count);

    TEST_ASSERT_EQ(outlier_count, 0, "MAD=0 means no outliers");

    return 0;
}

/**
 * @brief Test cb_detect_outliers null handling
 */
static int test_outliers_null(void)
{
    uint64_t samples[] = {1, 2, 3};
    bool outlier_flags[3];
    uint32_t outlier_count;
    cb_result_code_t rc;

    rc = cb_detect_outliers(NULL, 3, outlier_flags, &outlier_count);
    TEST_ASSERT_EQ(rc, CB_ERR_NULL_PTR, "NULL samples");

    rc = cb_detect_outliers(samples, 3, NULL, &outlier_count);
    TEST_ASSERT_EQ(rc, CB_ERR_NULL_PTR, "NULL outlier_flags");

    rc = cb_detect_outliers(samples, 3, outlier_flags, NULL);
    TEST_ASSERT_EQ(rc, CB_ERR_NULL_PTR, "NULL outlier_count");

    return 0;
}

/**
 * @brief Test cb_detect_outliers with empty array
 */
static int test_outliers_empty(void)
{
    uint64_t samples[] = {1};
    bool outlier_flags[1];
    uint32_t outlier_count;
    cb_result_code_t rc;

    rc = cb_detect_outliers(samples, 0, outlier_flags, &outlier_count);
    TEST_ASSERT_EQ(rc, CB_OK, "empty array should succeed");
    TEST_ASSERT_EQ(outlier_count, 0, "no outliers in empty array");

    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Test: Determinism (SRS-002-METRICS §5.1)
 * Traceability: METRICS-NF-001
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Test that statistics are deterministic
 * @satisfies METRICS-NF-001
 */
static int test_determinism(void)
{
    uint64_t samples1[] = {100, 200, 300, 400, 500};
    uint64_t samples2[] = {100, 200, 300, 400, 500};
    cb_latency_stats_t stats1, stats2;
    cb_fault_flags_t faults;

    cb_fault_clear(&faults);
    cb_compute_stats(samples1, 5, &stats1, &faults);

    cb_fault_clear(&faults);
    cb_compute_stats(samples2, 5, &stats2, &faults);

    TEST_ASSERT_EQ(stats1.min_ns, stats2.min_ns, "min deterministic");
    TEST_ASSERT_EQ(stats1.max_ns, stats2.max_ns, "max deterministic");
    TEST_ASSERT_EQ(stats1.mean_ns, stats2.mean_ns, "mean deterministic");
    TEST_ASSERT_EQ(stats1.stddev_ns, stats2.stddev_ns, "stddev deterministic");
    TEST_ASSERT_EQ(stats1.median_ns, stats2.median_ns, "median deterministic");
    TEST_ASSERT_EQ(stats1.p95_ns, stats2.p95_ns, "p95 deterministic");
    TEST_ASSERT_EQ(stats1.p99_ns, stats2.p99_ns, "p99 deterministic");

    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Main
 *─────────────────────────────────────────────────────────────────────────────*/

int main(void)
{
    printf("═══════════════════════════════════════════════════════════════════\n");
    printf(" certifiable-bench Metrics Tests\n");
    printf(" @traceability SRS-002-METRICS\n");
    printf("═══════════════════════════════════════════════════════════════════\n\n");

    printf("§4.2 Integer Square Root (METRICS-F-020..025)\n");
    RUN_TEST(test_isqrt64_vectors);
    RUN_TEST(test_isqrt64_perfect_squares);
    RUN_TEST(test_isqrt64_floor);

    printf("\n§5.1 Sorting (METRICS-NF-003)\n");
    RUN_TEST(test_sort_basic);
    RUN_TEST(test_sort_already_sorted);
    RUN_TEST(test_sort_reverse);
    RUN_TEST(test_sort_duplicates);
    RUN_TEST(test_sort_large);

    printf("\n§4.3 Percentile Computation (METRICS-F-030..036)\n");
    RUN_TEST(test_percentile_vectors);
    RUN_TEST(test_percentile_boundaries);
    RUN_TEST(test_percentile_single);
    RUN_TEST(test_percentile_two);
    RUN_TEST(test_percentile_null);

    printf("\n§4.1 Basic Statistics (METRICS-F-001..010)\n");
    RUN_TEST(test_stats_basic);
    RUN_TEST(test_stats_percentiles);
    RUN_TEST(test_stats_variance);
    RUN_TEST(test_stats_wcet);
    RUN_TEST(test_stats_null);
    RUN_TEST(test_stats_empty);
    RUN_TEST(test_stats_single);

    printf("\n§4.4 Histogram (METRICS-F-040..047)\n");
    RUN_TEST(test_histogram_basic);
    RUN_TEST(test_histogram_binning);
    RUN_TEST(test_histogram_overflow);
    RUN_TEST(test_histogram_null);
    RUN_TEST(test_histogram_invalid);

    printf("\n§4.5 Outlier Detection (METRICS-F-050..056)\n");
    RUN_TEST(test_outliers_vectors);
    RUN_TEST(test_outliers_none);
    RUN_TEST(test_outliers_mad_zero);
    RUN_TEST(test_outliers_null);
    RUN_TEST(test_outliers_empty);

    printf("\n§5.1 Determinism (METRICS-NF-001)\n");
    RUN_TEST(test_determinism);

    printf("\n═══════════════════════════════════════════════════════════════════\n");
    printf(" Results\n");
    printf("═══════════════════════════════════════════════════════════════════\n");
    printf(" Tests run:    %d\n", g_tests_run);
    printf(" Tests passed: %d\n", g_tests_passed);
    printf(" Tests failed: %d\n", g_tests_failed);
    printf("═══════════════════════════════════════════════════════════════════\n");

    if (g_tests_failed > 0) {
        printf("\n*** FAILURES DETECTED ***\n");
        return 1;
    }

    printf("\nAll tests passed.\n");
    return 0;
}
