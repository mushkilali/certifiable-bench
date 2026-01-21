# CB-STRUCT-001: Certifiable Bench — Data Structure Specification

**Component:** certifiable-bench  
**Version:** 1.0  
**Status:** ✅ APPROVED  
**Date:** January 2026  
**Derived From:** CB-MATH-001

---

## 1. Overview

This document specifies all data structures used in certifiable-bench. Every structure is derived from the mathematical foundations in CB-MATH-001.

**Canonical Location:** `include/cb_types.h`

**Design Rules:**
- All structures use fixed-size types (`uint32_t`, `int64_t`, etc.)
- No pointers in serialised structures (except caller-provided buffers)
- All multi-byte integers are little-endian when serialised
- Padding is explicit, never implicit

---

## 2. Fixed-Point and Integer Types

Derived from CB-MATH-001 §3.

```c
/**
 * @file cb_types.h
 * @brief Core types for certifiable-bench
 *
 * @traceability CB-STRUCT-001 §2
 */

#include <stdint.h>
#include <stdbool.h>

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
```

---

## 3. Fault Flags

Derived from CB-MATH-001 §5.

```c
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
```

---

## 4. Statistical Structures

Derived from CB-MATH-001 §6.

```c
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
```

---

## 5. Cryptographic Structures

Derived from CB-MATH-001 §8.

```c
/*─────────────────────────────────────────────────────────────────────────────
 * Cryptographic Binding (CB-MATH-001 §8)
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
```

---

## 6. Environmental Structures

Derived from CB-MATH-001 §9.

```c
/*─────────────────────────────────────────────────────────────────────────────
 * Environmental Characterisation (CB-MATH-001 §9)
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
```

---

## 7. Hardware Counter Structures

```c
/*─────────────────────────────────────────────────────────────────────────────
 * Hardware Performance Counters
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
```

---

## 8. Timer Structures

```c
/*─────────────────────────────────────────────────────────────────────────────
 * Timer Configuration
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
```

---

## 9. Configuration Structure

```c
/*─────────────────────────────────────────────────────────────────────────────
 * Benchmark Configuration
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Benchmark configuration
 *
 * @traceability CB-STRUCT-001 §9
 */
typedef struct {
    /* Iteration control */
    uint32_t warmup_iterations;    /**< Iterations before measurement */
    uint32_t measure_iterations;   /**< Iterations to measure */
    uint32_t batch_size;           /**< Inference batch size */
    uint32_t _padding1;
    
    /* Timer configuration */
    cb_timer_source_t timer_source;  /**< Preferred timer */
    uint32_t _padding2;
    
    /* Verification */
    bool     verify_outputs;       /**< Check bit-identity during bench */
    uint8_t  _padding3[7];
    
    /* Histogram configuration */
    bool     collect_histogram;    /**< Collect latency distribution */
    uint8_t  _padding4[3];
    uint32_t histogram_bins;       /**< Number of histogram bins */
    uint64_t histogram_min_ns;     /**< Histogram lower bound */
    uint64_t histogram_max_ns;     /**< Histogram upper bound */
    
    /* Environmental monitoring */
    bool     monitor_environment;  /**< Collect thermal/frequency data */
    uint8_t  _padding5[7];
    
    /* Paths (NULL = not used) */
    const char *model_path;        /**< Path to model bundle (.cbf) */
    const char *data_path;         /**< Path to test data */
    const char *golden_path;       /**< Path to golden reference */
    const char *output_path;       /**< Path for result JSON */
} cb_config_t;

/**
 * @brief Initialise configuration with defaults
 * @param config Configuration to initialise
 *
 * @traceability CB-STRUCT-001 §9
 */
static inline void cb_config_init(cb_config_t *config) {
    config->warmup_iterations = 100;
    config->measure_iterations = 1000;
    config->batch_size = 1;
    config->timer_source = CB_TIMER_AUTO;
    config->verify_outputs = true;
    config->collect_histogram = false;
    config->histogram_bins = 100;
    config->histogram_min_ns = 0;
    config->histogram_max_ns = 10 * CB_NS_PER_MS;  /* 10ms */
    config->monitor_environment = true;
    config->model_path = NULL;
    config->data_path = NULL;
    config->golden_path = NULL;
    config->output_path = NULL;
}
```

---

## 10. Result Structure

```c
/*─────────────────────────────────────────────────────────────────────────────
 * Benchmark Result
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
    /* Must have no hard faults */
    if (cb_has_fault(&result->faults)) {
        return false;
    }
    
    /* Determinism must be verified (if verification was enabled) */
    if (result->verification_failures > 0) {
        return false;
    }
    
    return true;
}
```

---

## 11. Comparison Structure

Derived from CB-MATH-001 §8.3.

```c
/*─────────────────────────────────────────────────────────────────────────────
 * Cross-Platform Comparison (CB-MATH-001 §8.3)
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
```

---

## 12. Result Codes

```c
/*─────────────────────────────────────────────────────────────────────────────
 * Result Codes
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
```

---

## 13. Document Alignment Matrix

| Structure | CB-MATH-001 | SRS | Notes |
|-----------|-------------|-----|-------|
| `cb_fault_flags_t` | §5 | SRS-003, SRS-004 | Fault model |
| `cb_latency_stats_t` | §6 | SRS-002 | Statistical methods |
| `cb_throughput_t` | — | SRS-002 | Derived metric |
| `cb_histogram_t` | §6.4 | SRS-002 | Distribution capture |
| `cb_hash_t` | §8 | SRS-004 | Cryptographic binding |
| `cb_verify_ctx_t` | §8.1 | SRS-004 | Streaming verification |
| `cb_golden_ref_t` | §8.3 | SRS-004 | Reference for comparison |
| `cb_env_snapshot_t` | §9.2 | SRS-006 | Environmental data |
| `cb_env_stats_t` | §9.2 | SRS-006 | Thermal characterisation |
| `cb_hwcounters_t` | — | SRS-006 | Hardware counters |
| `cb_timer_source_t` | — | SRS-001 | Timer selection |
| `cb_timer_state_t` | — | SRS-001 | Timer state |
| `cb_config_t` | — | SRS-003 | Configuration |
| `cb_result_t` | §7, §8 | SRS-003, SRS-005 | Complete result |
| `cb_comparison_t` | §8.3 | SRS-005 | Cross-platform comparison |

---

## 14. Serialisation Format

For JSON output (SRS-005), structures are serialised as follows:

```json
{
  "version": "1.0",
  "platform": "x86_64",
  "cpu_model": "Intel Core i7-10700K",
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
    "outlier_count": 3,
    "wcet_observed_ns": 2345678,
    "wcet_bound_ns": 3245678
  },
  "throughput": {
    "inferences_per_sec": 666,
    "samples_per_sec": 666,
    "bytes_per_sec": 2664000
  },
  "verification": {
    "determinism_verified": true,
    "verification_failures": 0,
    "output_hash": "48f4ecebc0eec79ab15fe694717a831e7218993502f84c66f0ef0f3d178c0138"
  },
  "environment": {
    "stable": true,
    "start_freq_hz": 3800000000,
    "end_freq_hz": 3750000000,
    "max_temp_mC": 72000,
    "throttle_events": 0
  },
  "faults": {
    "overflow": false,
    "underflow": false,
    "div_zero": false,
    "timer_error": false,
    "verify_fail": false,
    "thermal_drift": false
  },
  "timestamp_unix": 1737500000
}
```

---

## 15. Document Control

| Version | Date | Author | Change |
|---------|------|--------|--------|
| 1.0 | January 2026 | William Murray | Initial release |

---

*Built for safety. Designed for certification. Proven through testing.*

Copyright © 2026 The Murray Family Innovation Trust. All rights reserved.
