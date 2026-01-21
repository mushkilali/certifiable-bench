/**
 * @file cb_types.h
 * @brief Core types for certifiable-bench
 *
 * All structures derived from CB-STRUCT-001.
 * Design Rules:
 * - All structures use fixed-size types (uint32_t, int64_t, etc.)
 * - No pointers in serialised structures (except caller-provided buffers)
 * - All multi-byte integers are little-endian when serialised
 * - Padding is explicit, never implicit
 *
 * @traceability CB-STRUCT-001
 *
 * Copyright © 2026 The Murray Family Innovation Trust. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CB_TYPES_H
#define CB_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*─────────────────────────────────────────────────────────────────────────────
 * Constants (CB-MATH-001 §3.1)
 *─────────────────────────────────────────────────────────────────────────────*/

#define CB_NS_PER_SEC      1000000000ULL
#define CB_NS_PER_MS       1000000ULL
#define CB_NS_PER_US       1000ULL

#define CB_Q16_SHIFT       16
#define CB_Q16_ONE         (1 << CB_Q16_SHIFT)        /* 65536 */
#define CB_Q16_HALF        (1 << (CB_Q16_SHIFT - 1))  /* 32768 */

#define CB_Q32_SHIFT       32
#define CB_Q32_ONE         (1ULL << CB_Q32_SHIFT)

#define CB_WCET_SIGMA      6      /* σ multiplier for WCET bound */
#define CB_OUTLIER_THRESH  35000  /* 3.5 × 10000 for integer MAD */

#define CB_MAX_SAMPLES     1000000  /* Maximum benchmark iterations */
#define CB_MAX_HISTOGRAM   256      /* Maximum histogram bins */
#define CB_MAX_OUTLIERS    1000     /* Maximum outliers to track */
#define CB_MAX_PLATFORM    32       /* Platform string length */
#define CB_MAX_CPU_MODEL   128      /* CPU model string length */
#define CB_HASH_SIZE       32       /* SHA-256 digest size */

/*─────────────────────────────────────────────────────────────────────────────
 * Result Codes (CB-STRUCT-001 §12)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Function result codes
 *
 * @traceability CB-STRUCT-001 §12
 */
typedef enum {
    CB_OK = 0,                 /**< Success */
    CB_ERR_NULL_PTR,           /**< NULL pointer argument */
    CB_ERR_INVALID_CONFIG,     /**< Invalid configuration */
    CB_ERR_TIMER_INIT,         /**< Timer initialisation failed */
    CB_ERR_TIMER_READ,         /**< Timer read failed */
    CB_ERR_MODEL_LOAD,         /**< Model loading failed */
    CB_ERR_DATA_LOAD,          /**< Data loading failed */
    CB_ERR_GOLDEN_LOAD,        /**< Golden reference loading failed */
    CB_ERR_VERIFICATION,       /**< Output verification failed */
    CB_ERR_OVERFLOW,           /**< Arithmetic overflow */
    CB_ERR_IO,                 /**< File I/O error */
    CB_ERR_HWCOUNTERS,         /**< Hardware counter access failed */
    CB_ERR_ENV_READ,           /**< Environmental sensor read failed */
    CB_ERR_OUT_OF_MEMORY       /**< Insufficient buffer space */
} cb_result_code_t;

/*─────────────────────────────────────────────────────────────────────────────
 * Fault Flags (CB-MATH-001 §5.1)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Fault flags for benchmark operations
 *
 * Faults are sticky — once set, they persist until explicitly cleared.
 * A result with cb_has_fault() == true SHALL NOT be used for certification.
 *
 * @traceability CB-STRUCT-001 §3, CB-MATH-001 §5
 */
typedef struct {
    uint32_t overflow      : 1;  /**< Accumulator saturated (CB-MATH-001 §5.2) */
    uint32_t underflow     : 1;  /**< Unexpected negative value */
    uint32_t div_zero      : 1;  /**< Division by zero attempted */
    uint32_t timer_error   : 1;  /**< Timer read failed or wrapped */
    uint32_t verify_fail   : 1;  /**< Output hash mismatch (determinism broken) */
    uint32_t thermal_drift : 1;  /**< Frequency dropped > 5% (warning) */
    uint32_t _reserved     : 26;
} cb_fault_flags_t;

/**
 * @brief Check if any hard fault is set
 * @param f Fault flags
 * @return true if result is invalid
 */
static inline bool cb_has_fault(const cb_fault_flags_t *f) {
    return f->overflow || f->underflow || f->div_zero ||
           f->timer_error || f->verify_fail;
}

/**
 * @brief Check if any warning is set
 * @param f Fault flags
 * @return true if result has warnings (may still be valid)
 */
static inline bool cb_has_warning(const cb_fault_flags_t *f) {
    return f->thermal_drift;
}

/**
 * @brief Clear all fault flags
 * @param f Fault flags to clear
 */
static inline void cb_fault_clear(cb_fault_flags_t *f) {
    f->overflow = 0;
    f->underflow = 0;
    f->div_zero = 0;
    f->timer_error = 0;
    f->verify_fail = 0;
    f->thermal_drift = 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Timer Structures (CB-STRUCT-001 §8)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Timer source selection
 *
 * @traceability CB-STRUCT-001 §8
 */
typedef enum {
    CB_TIMER_AUTO = 0,         /**< Auto-detect best available */
    CB_TIMER_POSIX,            /**< clock_gettime(CLOCK_MONOTONIC) */
    CB_TIMER_RDTSC,            /**< x86 RDTSC instruction */
    CB_TIMER_CNTVCT,           /**< ARM64 CNTVCT_EL0 register */
    CB_TIMER_RISCV_CYCLE       /**< RISC-V cycle CSR */
} cb_timer_source_t;

/**
 * @brief Timer state
 *
 * @traceability CB-STRUCT-001 §8
 */
typedef struct {
    cb_timer_source_t source;  /**< Active timer source */
    uint32_t _padding;
    uint64_t resolution_ns;    /**< Timer resolution in nanoseconds */
    uint64_t freq_hz;          /**< Timer frequency (for cycle counters) */
    uint64_t calibration_ns;   /**< Calibration overhead */
} cb_timer_state_t;

/*─────────────────────────────────────────────────────────────────────────────
 * Latency Statistics (CB-MATH-001 §6)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Latency statistics in nanoseconds
 *
 * All fields are integers per CB-MATH-001 §7.3 (Integer Statistical Invariant).
 *
 * @traceability CB-STRUCT-001 §4, CB-MATH-001 §6
 */
typedef struct {
    uint64_t min_ns;           /**< Minimum observed latency */
    uint64_t max_ns;           /**< Maximum observed latency */
    uint64_t mean_ns;          /**< Arithmetic mean (CB-MATH-001 §6.1) */
    uint64_t median_ns;        /**< 50th percentile */
    uint64_t p95_ns;           /**< 95th percentile */
    uint64_t p99_ns;           /**< 99th percentile */
    uint64_t stddev_ns;        /**< Standard deviation (CB-MATH-001 §6.2) */
    uint64_t variance_ns2;     /**< Variance in ns² */
    uint32_t sample_count;     /**< Number of samples */
    uint32_t outlier_count;    /**< Samples with |z| > 3.5 (CB-MATH-001 §6.5) */
    uint64_t wcet_observed_ns; /**< Maximum observed (= max_ns) */
    uint64_t wcet_bound_ns;    /**< Statistical bound (CB-MATH-001 §6.6) */
} cb_latency_stats_t;

/**
 * @brief Throughput metrics
 *
 * @traceability CB-STRUCT-001 §4
 */
typedef struct {
    uint64_t inferences_per_sec;  /**< Complete inferences per second */
    uint64_t samples_per_sec;     /**< inferences × batch_size */
    uint64_t bytes_per_sec;       /**< Estimated memory bandwidth */
    uint32_t batch_size;          /**< Batch size used */
    uint32_t _padding;
} cb_throughput_t;

/**
 * @brief Histogram bin structure
 *
 * @traceability CB-STRUCT-001 §4
 */
typedef struct {
    uint64_t min_ns;           /**< Bin lower bound (inclusive) */
    uint64_t max_ns;           /**< Bin upper bound (exclusive) */
    uint32_t count;            /**< Samples in this bin */
    uint32_t _padding;
} cb_histogram_bin_t;

/**
 * @brief Latency histogram
 *
 * @traceability CB-STRUCT-001 §4
 */
typedef struct {
    uint64_t range_min_ns;     /**< Histogram minimum */
    uint64_t range_max_ns;     /**< Histogram maximum */
    uint64_t bin_width_ns;     /**< Width of each bin */
    uint32_t num_bins;         /**< Number of bins */
    uint32_t overflow_count;   /**< Samples above range_max */
    uint32_t underflow_count;  /**< Samples below range_min */
    uint32_t _padding;
    cb_histogram_bin_t *bins;  /**< Caller-provided bin array */
} cb_histogram_t;

/*─────────────────────────────────────────────────────────────────────────────
 * Cryptographic Structures (CB-MATH-001 §8)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief SHA-256 hash digest
 *
 * @traceability CB-STRUCT-001 §5, CB-MATH-001 §8.1
 */
typedef struct {
    uint8_t bytes[CB_HASH_SIZE];
} cb_hash_t;

/**
 * @brief Verification context for output hashing
 *
 * @traceability CB-STRUCT-001 §5, CB-MATH-001 §8.1
 */
typedef struct {
    uint8_t state[128];        /**< SHA-256 streaming state */
    uint64_t bytes_hashed;     /**< Total bytes processed */
    bool finalised;            /**< Hash has been finalised */
    uint8_t _padding[7];
} cb_verify_ctx_t;

/**
 * @brief Golden reference for verification
 *
 * @traceability CB-STRUCT-001 §5, CB-MATH-001 §8.3
 */
typedef struct {
    cb_hash_t output_hash;     /**< Expected H(outputs) */
    uint32_t sample_count;     /**< Expected number of outputs */
    uint32_t output_size;      /**< Size of each output in bytes */
    char platform[CB_MAX_PLATFORM];  /**< Platform that generated golden */
} cb_golden_ref_t;

/*─────────────────────────────────────────────────────────────────────────────
 * Environmental Structures (CB-MATH-001 §9)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Point-in-time environmental snapshot
 *
 * @traceability CB-STRUCT-001 §6, CB-MATH-001 §9.2
 */
typedef struct {
    uint64_t timestamp_ns;     /**< Monotonic timestamp */
    uint64_t cpu_freq_hz;      /**< Current CPU frequency */
    int32_t  cpu_temp_mC;      /**< CPU temperature in millidegrees Celsius */
    uint32_t throttle_count;   /**< Cumulative throttle events */
} cb_env_snapshot_t;

/**
 * @brief Environmental statistics over benchmark duration
 *
 * @traceability CB-STRUCT-001 §6, CB-MATH-001 §9.2
 */
typedef struct {
    cb_env_snapshot_t start;   /**< Snapshot at benchmark start */
    cb_env_snapshot_t end;     /**< Snapshot at benchmark end */
    uint64_t min_freq_hz;      /**< Minimum frequency observed */
    uint64_t max_freq_hz;      /**< Maximum frequency observed */
    int32_t  min_temp_mC;      /**< Minimum temperature */
    int32_t  max_temp_mC;      /**< Maximum temperature */
    uint32_t total_throttle_events;  /**< Throttle events during benchmark */
    uint32_t _padding;
} cb_env_stats_t;

/**
 * @brief Check environmental stability (CB-MATH-001 §9.3)
 * @param env Environmental statistics
 * @return true if hardware state was stable
 */
static inline bool cb_env_is_stable(const cb_env_stats_t *env) {
    /* Frequency drop > 5% indicates thermal throttling */
    uint64_t threshold = (env->start.cpu_freq_hz * 95) / 100;

    if (env->end.cpu_freq_hz < threshold) {
        return false;
    }

    if (env->total_throttle_events > 0) {
        return false;
    }

    return true;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Hardware Performance Counters (CB-STRUCT-001 §7)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Hardware performance counter readings
 *
 * Note: IPC is stored as Q16.16 fixed-point for determinism.
 * Display code may convert to floating-point for presentation only.
 *
 * @traceability CB-STRUCT-001 §7
 */
typedef struct {
    bool     available;        /**< Counters were successfully read */
    uint8_t  _padding[7];
    uint64_t cycles;           /**< CPU cycles */
    uint64_t instructions;     /**< Instructions retired */
    uint64_t cache_refs;       /**< Cache references */
    uint64_t cache_misses;     /**< Cache misses */
    uint64_t branch_refs;      /**< Branch instructions */
    uint64_t branch_misses;    /**< Branch mispredictions */
    uint32_t ipc_q16;          /**< Instructions per cycle (Q16.16) */
    uint32_t cache_miss_rate_q16;  /**< Miss rate (Q16.16, 0-65536 = 0-100%) */
} cb_hwcounters_t;

/*─────────────────────────────────────────────────────────────────────────────
 * Benchmark Configuration (CB-STRUCT-001 §9)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Benchmark configuration
 *
 * @traceability CB-STRUCT-001 §9, SRS-003-RUNNER §4.1
 */
typedef struct {
    /* Iteration control */
    uint32_t warmup_iterations;     /**< Iterations before measurement (default: 100) */
    uint32_t measure_iterations;    /**< Iterations to measure (default: 1000) */
    uint32_t batch_size;            /**< Inference batch size (default: 1) */
    uint32_t _padding1;

    /* Timer configuration */
    cb_timer_source_t timer_source; /**< Preferred timer (default: CB_TIMER_AUTO) */
    uint32_t _padding2;

    /* Verification */
    bool     verify_outputs;        /**< Check bit-identity during bench (default: true) */
    uint8_t  _padding3[7];

    /* Histogram configuration */
    bool     collect_histogram;     /**< Collect latency distribution (default: false) */
    uint8_t  _padding4[3];
    uint32_t histogram_bins;        /**< Number of histogram bins (default: 100) */
    uint64_t histogram_min_ns;      /**< Histogram lower bound (default: 0) */
    uint64_t histogram_max_ns;      /**< Histogram upper bound (default: 10ms) */

    /* Environmental monitoring */
    bool     monitor_environment;   /**< Collect thermal/frequency data (default: true) */
    uint8_t  _padding5[7];

    /* Paths (NULL = not used) */
    const char *model_path;         /**< Path to model bundle (.cbf) */
    const char *data_path;          /**< Path to test data */
    const char *golden_path;        /**< Path to golden reference */
    const char *output_path;        /**< Path for result JSON */
} cb_config_t;

/*─────────────────────────────────────────────────────────────────────────────
 * Benchmark Result (CB-STRUCT-001 §10)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Complete benchmark result
 *
 * This structure binds performance metrics to correctness verification
 * per CB-MATH-001 §7.1 (Determinism Preservation Invariant).
 *
 * @traceability CB-STRUCT-001 §10, CB-MATH-001 §7, §8
 */
typedef struct {
    /*─────────────────────────────────────────────────────────────────────────
     * Platform Identification
     *─────────────────────────────────────────────────────────────────────────*/
    char platform[CB_MAX_PLATFORM];      /**< "x86_64", "aarch64", "riscv64" */
    char cpu_model[CB_MAX_CPU_MODEL];    /**< CPU identification string */
    uint32_t cpu_freq_mhz;               /**< Nominal CPU frequency */
    uint32_t _padding1;

    /*─────────────────────────────────────────────────────────────────────────
     * Configuration Echo
     *─────────────────────────────────────────────────────────────────────────*/
    uint32_t warmup_iterations;
    uint32_t measure_iterations;
    uint32_t batch_size;
    uint32_t _padding2;

    /*─────────────────────────────────────────────────────────────────────────
     * Timing Results (CB-MATH-001 §6)
     *─────────────────────────────────────────────────────────────────────────*/
    cb_latency_stats_t latency;
    cb_throughput_t throughput;

    /*─────────────────────────────────────────────────────────────────────────
     * Hardware Counters (optional)
     *─────────────────────────────────────────────────────────────────────────*/
    cb_hwcounters_t hwcounters;

    /*─────────────────────────────────────────────────────────────────────────
     * Environmental Data (CB-MATH-001 §9)
     *─────────────────────────────────────────────────────────────────────────*/
    cb_env_stats_t environment;
    bool env_stable;                     /**< cb_env_is_stable() result */
    uint8_t _padding3[7];

    /*─────────────────────────────────────────────────────────────────────────
     * Histogram (optional, caller-provided buffer)
     *─────────────────────────────────────────────────────────────────────────*/
    cb_histogram_t histogram;
    bool histogram_valid;
    uint8_t _padding4[7];

    /*─────────────────────────────────────────────────────────────────────────
     * Verification (CB-MATH-001 §8)
     *─────────────────────────────────────────────────────────────────────────*/
    bool determinism_verified;           /**< All outputs matched golden */
    uint8_t _padding5[3];
    uint32_t verification_failures;      /**< Count of mismatches */
    cb_hash_t output_hash;               /**< H(all outputs) */
    cb_hash_t result_hash;               /**< H(result binding) */

    /*─────────────────────────────────────────────────────────────────────────
     * Metadata
     *─────────────────────────────────────────────────────────────────────────*/
    uint64_t benchmark_start_ns;         /**< Start timestamp */
    uint64_t benchmark_end_ns;           /**< End timestamp */
    uint64_t benchmark_duration_ns;      /**< Total duration */
    uint64_t timestamp_unix;             /**< Unix timestamp for reporting */

    /*─────────────────────────────────────────────────────────────────────────
     * Fault State (CB-MATH-001 §5)
     *─────────────────────────────────────────────────────────────────────────*/
    cb_fault_flags_t faults;
    uint32_t _padding6;
} cb_result_t;

/**
 * @brief Check if result is valid for certification evidence
 * @param result Benchmark result
 * @return true if result can be used as certification evidence
 *
 * @traceability CB-STRUCT-001 §10, CB-MATH-001 §5.3
 */
static inline bool cb_result_is_valid(const cb_result_t *result) {
    if (cb_has_fault(&result->faults)) {
        return false;
    }
    if (result->verification_failures > 0) {
        return false;
    }
    return true;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Cross-Platform Comparison (CB-STRUCT-001 §11)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Result of comparing two benchmark results
 *
 * Per CB-MATH-001 §8.3: Comparable(A, B) ⟺ H_outputs(A) = H_outputs(B)
 *
 * @traceability CB-STRUCT-001 §11, CB-MATH-001 §8.3
 */
typedef struct {
    /* Platform identification */
    char platform_a[CB_MAX_PLATFORM];
    char platform_b[CB_MAX_PLATFORM];

    /* Comparability gate */
    bool outputs_identical;              /**< H_outputs match */
    bool comparable;                     /**< Safe to compare performance */
    uint8_t _padding1[6];

    /* Latency comparison (only valid if comparable == true) */
    int64_t  latency_diff_ns;            /**< B.p99 - A.p99 (+ = B slower) */
    uint32_t latency_ratio_q16;          /**< B.p99 / A.p99 in Q16.16 */
    uint32_t _padding2;

    /* Throughput comparison */
    int64_t  throughput_diff;            /**< B - A inferences/sec */
    uint32_t throughput_ratio_q16;       /**< B / A in Q16.16 */
    uint32_t _padding3;

    /* WCET comparison */
    int64_t  wcet_diff_ns;               /**< B.wcet_bound - A.wcet_bound */
    uint32_t wcet_ratio_q16;             /**< B / A in Q16.16 */
    uint32_t _padding4;
} cb_comparison_t;

/*─────────────────────────────────────────────────────────────────────────────
 * Inference Function Pointer (SRS-003-RUNNER §4.2)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Inference function signature for benchmark runner
 *
 * The runner calls this function for each iteration.
 *
 * @param ctx     User context (model, input data, etc.)
 * @param input   Input buffer
 * @param output  Output buffer
 * @return CB_OK on success, error code otherwise
 */
typedef cb_result_code_t (*cb_inference_fn)(void *ctx,
                                            const void *input,
                                            void *output);

/*─────────────────────────────────────────────────────────────────────────────
 * Runner State (SRS-003-RUNNER §4.2)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Runner internal state
 *
 * Holds caller-provided sample buffer and configuration for benchmark execution.
 * No dynamic allocation — caller provides all buffers.
 *
 * @traceability SRS-003-RUNNER §4.2
 */
typedef struct {
    cb_config_t config;                  /**< Copy of configuration */
    uint64_t *samples;                   /**< Caller-provided sample buffer */
    uint32_t sample_capacity;            /**< Buffer capacity (caller-provided) */
    uint32_t samples_collected;          /**< Samples collected so far */
    cb_verify_ctx_t verify_ctx;          /**< Output verification context */
    cb_env_snapshot_t env_start;         /**< Environment at start */
    uint64_t benchmark_start_ns;         /**< Benchmark start time */
    bool initialised;                    /**< Runner has been initialised */
    bool warmup_complete;                /**< Warmup phase done */
    uint8_t _padding[6];
    cb_fault_flags_t faults;             /**< Accumulated faults */
} cb_runner_t;

#ifdef __cplusplus
}
#endif

#endif /* CB_TYPES_H */
