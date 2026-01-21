/**
 * @file metrics.c
 * @brief Statistics and metrics implementation
 *
 * Implements SRS-002-METRICS requirements with integer-only arithmetic.
 *
 * @traceability SRS-002-METRICS, CB-MATH-001 §6
 *
 * Copyright © 2026 The Murray Family Innovation Trust. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cb_metrics.h"

#include <string.h>

/*─────────────────────────────────────────────────────────────────────────────
 * Constants
 *─────────────────────────────────────────────────────────────────────────────*/

/** Threshold for switching from insertion sort to heapsort */
#define SORT_THRESHOLD 64

/** Outlier threshold: 3.5 × 10000 = 35000 (scaled integer) */
#define OUTLIER_THRESH_SCALED 35000

/** Scaling factor for modified Z-score: 0.6745 × 10000 = 6745 */
#define MAD_SCALE_FACTOR 6745

/** WCET sigma multiplier (CB-MATH-001 §6.6) */
#define WCET_SIGMA 6

/*─────────────────────────────────────────────────────────────────────────────
 * Integer Square Root (CB-MATH-001 §6.3)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @satisfies METRICS-F-020 through METRICS-F-025
 */
uint64_t cb_isqrt64(uint64_t n)
{
    if (n == 0) {
        return 0;
    }

    uint64_t lo = 1;
    uint64_t hi = n;
    uint64_t mid;

    /* Binary search: O(32) iterations max for 64-bit input */
    while (lo < hi) {
        mid = lo + (hi - lo + 1) / 2;

        /* Overflow-safe: check mid <= n/mid instead of mid*mid <= n */
        if (mid <= n / mid) {
            lo = mid;
        } else {
            hi = mid - 1;
        }
    }

    return lo;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Deterministic Sorting
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Insertion sort for small arrays
 */
static void insertion_sort_u64(uint64_t *arr, uint32_t count)
{
    uint32_t i, j;
    uint64_t key;

    for (i = 1; i < count; i++) {
        key = arr[i];
        j = i;
        while (j > 0 && arr[j - 1] > key) {
            arr[j] = arr[j - 1];
            j--;
        }
        arr[j] = key;
    }
}

/**
 * @brief Heapify subtree rooted at index i
 */
static void heapify_u64(uint64_t *arr, uint32_t count, uint32_t i)
{
    uint32_t largest = i;
    uint32_t left = 2 * i + 1;
    uint32_t right = 2 * i + 2;
    uint64_t temp;

    if (left < count && arr[left] > arr[largest]) {
        largest = left;
    }

    if (right < count && arr[right] > arr[largest]) {
        largest = right;
    }

    if (largest != i) {
        temp = arr[i];
        arr[i] = arr[largest];
        arr[largest] = temp;
        heapify_u64(arr, count, largest);
    }
}

/**
 * @brief Heapsort for large arrays (deterministic, no pivot selection)
 */
static void heapsort_u64(uint64_t *arr, uint32_t count)
{
    uint32_t i;
    uint64_t temp;

    /* Build max heap */
    for (i = count / 2; i > 0; i--) {
        heapify_u64(arr, count, i - 1);
    }

    /* Extract elements from heap */
    for (i = count - 1; i > 0; i--) {
        temp = arr[0];
        arr[0] = arr[i];
        arr[i] = temp;
        heapify_u64(arr, i, 0);
    }
}

/**
 * @satisfies METRICS-NF-003
 */
void cb_sort_u64(uint64_t *arr, uint32_t count)
{
    if (arr == NULL || count <= 1) {
        return;
    }

    if (count <= SORT_THRESHOLD) {
        insertion_sort_u64(arr, count);
    } else {
        heapsort_u64(arr, count);
    }
}

/*─────────────────────────────────────────────────────────────────────────────
 * Percentile Computation (CB-MATH-001 §6.4)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @satisfies METRICS-F-030 through METRICS-F-036
 */
uint64_t cb_percentile(const uint64_t *sorted_samples,
                       uint32_t count,
                       uint32_t percentile)
{
    uint64_t rank_scaled;
    uint32_t rank;
    uint32_t frac;
    uint64_t lower, upper;
    uint64_t result;

    if (sorted_samples == NULL || count == 0) {
        return 0;
    }

    if (percentile > 100) {
        percentile = 100;
    }

    if (count == 1) {
        return sorted_samples[0];
    }

    /*
     * Per CB-MATH-001 §6.4:
     * rank_scaled = p × (n - 1)    (scaled by 100)
     * rank = rank_scaled / 100
     * frac = rank_scaled % 100
     */
    rank_scaled = (uint64_t)percentile * (count - 1);
    rank = (uint32_t)(rank_scaled / 100);
    frac = (uint32_t)(rank_scaled % 100);

    lower = sorted_samples[rank];

    /* Bounds check for upper */
    if (rank + 1 < count) {
        upper = sorted_samples[rank + 1];
    } else {
        upper = lower;
    }

    /* Linear interpolation: result = lower + ((upper - lower) × frac) / 100 */
    if (upper >= lower) {
        result = lower + ((upper - lower) * frac) / 100;
    } else {
        /* Shouldn't happen with sorted input, but handle gracefully */
        result = lower;
    }

    return result;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Basic Statistics (CB-MATH-001 §6.1, §6.2)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Check for addition overflow
 */
static bool would_overflow_add(uint64_t a, uint64_t b)
{
    return a > UINT64_MAX - b;
}

/**
 * @satisfies METRICS-F-001 through METRICS-F-010, METRICS-F-037,
 *            METRICS-F-057, METRICS-F-060 through METRICS-F-063
 */
cb_result_code_t cb_compute_stats(uint64_t *samples,
                                  uint32_t count,
                                  cb_latency_stats_t *stats,
                                  cb_fault_flags_t *faults)
{
    uint64_t sum = 0;
    uint64_t min_val, max_val;
    uint64_t mean, variance, stddev;
    uint32_t i;
    bool overflow_detected = false;

    /* Welford's algorithm state */
    int64_t M = 0;   /* Running mean (scaled) */
    int64_t S = 0;   /* Running sum of squared deviations */
    int64_t delta, delta2;

    if (samples == NULL || stats == NULL || faults == NULL) {
        return CB_ERR_NULL_PTR;
    }

    /* Clear output */
    memset(stats, 0, sizeof(*stats));

    if (count == 0) {
        faults->div_zero = 1;
        return CB_ERR_OVERFLOW;  /* Division by zero */
    }

    /* First pass: compute sum, min, max, and Welford's M/S */
    min_val = samples[0];
    max_val = samples[0];

    for (i = 0; i < count; i++) {
        uint64_t x = samples[i];

        /* Check for sum overflow */
        if (would_overflow_add(sum, x)) {
            overflow_detected = true;
            faults->overflow = 1;
        } else {
            sum += x;
        }

        /* Min/max */
        if (x < min_val) min_val = x;
        if (x > max_val) max_val = x;

        /* Welford's algorithm for variance (using signed for deltas) */
        delta = (int64_t)x - M;
        M = M + delta / (int64_t)(i + 1);
        delta2 = (int64_t)x - M;
        S = S + delta * delta2;
    }

    /* Compute mean */
    if (!overflow_detected) {
        mean = sum / count;
    } else {
        /* Fallback: use Welford's mean approximation */
        mean = (uint64_t)M;
    }

    /* Compute variance and stddev */
    if (count > 1) {
        variance = (uint64_t)(S / (int64_t)(count - 1));
        stddev = cb_isqrt64(variance);
    } else {
        variance = 0;
        stddev = 0;
    }

    /* Sort for percentiles (in-place) */
    cb_sort_u64(samples, count);

    /* Populate basic stats */
    stats->min_ns = min_val;
    stats->max_ns = max_val;
    stats->mean_ns = mean;
    stats->variance_ns2 = variance;
    stats->stddev_ns = stddev;
    stats->sample_count = count;

    /* Percentiles (on sorted array) */
    stats->median_ns = cb_percentile(samples, count, 50);
    stats->p95_ns = cb_percentile(samples, count, 95);
    stats->p99_ns = cb_percentile(samples, count, 99);

    /* WCET (CB-MATH-001 §6.6) */
    stats->wcet_observed_ns = max_val;

    /* WCET bound = max + 6 × stddev */
    if (stddev <= (UINT64_MAX - max_val) / WCET_SIGMA) {
        stats->wcet_bound_ns = max_val + WCET_SIGMA * stddev;
    } else {
        /* Overflow protection */
        stats->wcet_bound_ns = max_val;
        faults->overflow = 1;
    }

    /* Outlier count (simple threshold approach for stats) */
    /* Full outlier detection uses cb_detect_outliers() */
    stats->outlier_count = 0;
    if (stddev > 0) {
        uint64_t outlier_thresh = mean + 3 * stddev;
        for (i = 0; i < count; i++) {
            if (samples[i] > outlier_thresh) {
                stats->outlier_count++;
            }
        }
    }

    return overflow_detected ? CB_ERR_OVERFLOW : CB_OK;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Histogram Construction
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @traceability SRS-002-METRICS §4.4
 */
cb_result_code_t cb_histogram_init(cb_histogram_t *histogram,
                                   cb_histogram_bin_t *bins,
                                   uint32_t num_bins,
                                   uint64_t min_ns,
                                   uint64_t max_ns)
{
    uint32_t i;
    uint64_t bin_width;
    uint64_t current_min;

    if (histogram == NULL || bins == NULL) {
        return CB_ERR_NULL_PTR;
    }

    if (num_bins == 0 || min_ns >= max_ns) {
        return CB_ERR_INVALID_CONFIG;
    }

    /* Compute bin width */
    bin_width = (max_ns - min_ns) / num_bins;
    if (bin_width == 0) {
        bin_width = 1;  /* Minimum bin width */
    }

    /* Initialise histogram structure */
    histogram->range_min_ns = min_ns;
    histogram->range_max_ns = max_ns;
    histogram->bin_width_ns = bin_width;
    histogram->num_bins = num_bins;
    histogram->overflow_count = 0;
    histogram->underflow_count = 0;
    histogram->bins = bins;

    /* Initialise bins */
    current_min = min_ns;
    for (i = 0; i < num_bins; i++) {
        bins[i].min_ns = current_min;
        bins[i].max_ns = current_min + bin_width;
        bins[i].count = 0;
        bins[i]._padding = 0;
        current_min += bin_width;
    }

    /* Adjust last bin to exactly cover max_ns */
    bins[num_bins - 1].max_ns = max_ns;

    return CB_OK;
}

/**
 * @satisfies METRICS-F-040 through METRICS-F-047
 */
cb_result_code_t cb_histogram_build(const uint64_t *samples,
                                    uint32_t count,
                                    cb_histogram_t *histogram)
{
    uint32_t i;
    uint32_t bin_idx;

    if (samples == NULL || histogram == NULL || histogram->bins == NULL) {
        return CB_ERR_NULL_PTR;
    }

    /* Reset counts */
    histogram->overflow_count = 0;
    histogram->underflow_count = 0;
    for (i = 0; i < histogram->num_bins; i++) {
        histogram->bins[i].count = 0;
    }

    /* Bin each sample */
    for (i = 0; i < count; i++) {
        uint64_t sample = samples[i];

        if (sample < histogram->range_min_ns) {
            histogram->underflow_count++;
        } else if (sample >= histogram->range_max_ns) {
            histogram->overflow_count++;
        } else {
            /* bin = (sample - min_ns) / bin_width */
            bin_idx = (uint32_t)((sample - histogram->range_min_ns) / histogram->bin_width_ns);

            /* Bounds check (should not be needed but defensive) */
            if (bin_idx >= histogram->num_bins) {
                bin_idx = histogram->num_bins - 1;
            }

            histogram->bins[bin_idx].count++;
        }
    }

    return CB_OK;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Outlier Detection (CB-MATH-001 §6.5)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @satisfies METRICS-F-050 through METRICS-F-056
 */
cb_result_code_t cb_detect_outliers(const uint64_t *samples,
                                    uint32_t count,
                                    bool *outlier_flags,
                                    uint32_t *outlier_count)
{
    uint64_t *sorted_copy = NULL;
    uint64_t *deviations = NULL;
    uint64_t median, mad;
    uint64_t modified_z_scaled;
    uint32_t i;
    uint32_t num_outliers = 0;

    /* Static buffer for small arrays (avoid allocation) */
    static uint64_t static_buf1[CB_MAX_SAMPLES];
    static uint64_t static_buf2[CB_MAX_SAMPLES];

    if (samples == NULL || outlier_flags == NULL || outlier_count == NULL) {
        return CB_ERR_NULL_PTR;
    }

    if (count == 0) {
        *outlier_count = 0;
        return CB_OK;
    }

    if (count > CB_MAX_SAMPLES) {
        return CB_ERR_OUT_OF_MEMORY;
    }

    /* Use static buffers */
    sorted_copy = static_buf1;
    deviations = static_buf2;

    /* Copy and sort for median */
    memcpy(sorted_copy, samples, count * sizeof(uint64_t));
    cb_sort_u64(sorted_copy, count);

    /* Compute median */
    median = cb_percentile(sorted_copy, count, 50);

    /* Compute absolute deviations from median */
    for (i = 0; i < count; i++) {
        if (samples[i] >= median) {
            deviations[i] = samples[i] - median;
        } else {
            deviations[i] = median - samples[i];
        }
    }

    /* Sort deviations and compute MAD (median of deviations) */
    cb_sort_u64(deviations, count);
    mad = cb_percentile(deviations, count, 50);

    /* If MAD = 0, no outliers (all samples identical or nearly so) */
    if (mad == 0) {
        for (i = 0; i < count; i++) {
            outlier_flags[i] = false;
        }
        *outlier_count = 0;
        return CB_OK;
    }

    /* Flag outliers using modified Z-score */
    for (i = 0; i < count; i++) {
        uint64_t deviation;

        if (samples[i] >= median) {
            deviation = samples[i] - median;
        } else {
            deviation = median - samples[i];
        }

        /*
         * Modified Z-score (scaled by 10000):
         * modified_z_scaled = (6745 × deviation) / MAD
         *
         * Outlier if modified_z_scaled > 35000 (i.e., |z| > 3.5)
         */
        modified_z_scaled = (MAD_SCALE_FACTOR * deviation) / mad;

        if (modified_z_scaled > OUTLIER_THRESH_SCALED) {
            outlier_flags[i] = true;
            num_outliers++;
        } else {
            outlier_flags[i] = false;
        }
    }

    *outlier_count = num_outliers;
    return CB_OK;
}
