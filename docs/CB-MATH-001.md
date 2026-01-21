# CB-MATH-001: Certifiable Bench — Mathematical Foundations

**Component:** certifiable-bench  
**Version:** 1.0  
**Status:** ✅ APPROVED  
**Date:** January 2026

---

## 1. Overview

This document specifies the mathematical foundations for certifiable-bench, the performance benchmarking component of the certifiable-* family.

**Core Principle:** Performance measurements are valid **if and only if** deterministic correctness is cryptographically proven during the benchmark run.

**Relationship to Other Documents:**
- **CB-STRUCT-001** — Data structures (derived from this document)
- **SRS-001 through SRS-006** — Module requirements

---

## 2. The Three Theorems

certifiable-bench satisfies the three theorems required of all certifiable-* projects:

| Theorem | Statement | Implementation |
|---------|-----------|----------------|
| **Bit Identity** | B_A(f, x) = B_B(f, x) for DVM-compliant platforms A, B | Output hash verified during benchmark |
| **Bounded Error** | Measurement overhead ε < 1% | No allocation/instrumentation in critical loop |
| **Auditability** | Any result verifiable in O(1) | Cryptographic binding of outputs to metrics |

---

## 3. Fixed-Point Formats

All statistical computations use integer or fixed-point arithmetic.

| Format | Total Bits | Integer | Fractional | Use Case |
|--------|------------|---------|------------|----------|
| uint64_t | 64 | 64 | 0 | Timestamps, latencies (nanoseconds) |
| Q16.16 | 32 | 16 | 16 | Ratios, normalised metrics |
| Q32.32 | 64 | 32 | 32 | Variance accumulators |

### 3.1 Constants

```c
#define CB_NS_PER_SEC     1000000000ULL
#define CB_NS_PER_MS      1000000ULL
#define CB_NS_PER_US      1000ULL

#define CB_Q16_ONE        (1 << 16)           /* 65536 */
#define CB_Q16_HALF       (1 << 15)           /* 32768 */

#define CB_WCET_SIGMA     6                   /* σ multiplier for WCET bound */
#define CB_OUTLIER_THRESH 35                  /* 3.5 × 10 for integer comparison */
```

---

## 4. DVM Primitive Mapping

All certifiable-bench computations map to DVM primitives:

| Operation | DVM Primitive | Use Case |
|-----------|---------------|----------|
| Timestamp delta | `dvm_sub64` | Latency calculation |
| Sum accumulation | `dvm_add64` | Mean computation |
| Square accumulation | `dvm_mul64` + `dvm_add64` | Variance computation |
| Division | `dvm_div_q` | Mean, ratios |
| Square root | `cb_isqrt64` (§6.3) | Standard deviation |
| Percentile interpolation | `dvm_mul64` + `dvm_round_shift_rne` | p50, p95, p99 |
| Hash computation | SHA-256 (from certifiable-deploy) | Output binding |

### 4.1 Forbidden Operations

| Forbidden | Reason | Alternative |
|-----------|--------|-------------|
| `double` in logic | Non-deterministic | Integer or Q16.16 |
| `sqrt()` | Platform-dependent | `cb_isqrt64` (§6.3) |
| `qsort()` with floating comparator | Non-deterministic | Integer comparison |
| Allocation during measurement | Timing interference | Pre-allocated buffers |

---

## 5. Fault Model

### 5.1 Fault Flags

```c
typedef struct {
    uint32_t overflow      : 1;  /* Accumulator saturated */
    uint32_t underflow     : 1;  /* Unexpected negative value */
    uint32_t div_zero      : 1;  /* Division by zero attempted */
    uint32_t timer_error   : 1;  /* Timer read failed or wrapped */
    uint32_t verify_fail   : 1;  /* Output hash mismatch */
    uint32_t thermal_drift : 1;  /* Frequency dropped > 5% */
    uint32_t _reserved     : 26;
} cb_fault_flags_t;
```

### 5.2 Fault Semantics

| Fault | Trigger | Result Validity |
|-------|---------|-----------------|
| `overflow` | Accumulator exceeds INT64_MAX | Invalid — metrics unreliable |
| `underflow` | Negative latency computed | Invalid — timer error |
| `div_zero` | n = 0 in mean/variance | Invalid — no samples |
| `timer_error` | clock_gettime failure | Invalid — no timing data |
| `verify_fail` | H(outputs) ≠ H_golden | Invalid — determinism broken |
| `thermal_drift` | freq_end < 0.95 × freq_start | Flagged — WCET unreliable |

### 5.3 Fault Propagation

Faults are **sticky** — once set, they persist until explicitly cleared.

```c
static inline bool cb_has_fault(const cb_fault_flags_t *f) {
    return f->overflow || f->underflow || f->div_zero || 
           f->timer_error || f->verify_fail;
}

static inline bool cb_has_warning(const cb_fault_flags_t *f) {
    return f->thermal_drift;
}
```

A benchmark result with `cb_has_fault() == true` SHALL NOT be used for certification evidence.

---

## 6. Statistical Methods

### 6.1 Mean (Integer)

For n samples with latencies L₀, L₁, ..., L_{n-1}:

```
sum = Σᵢ Lᵢ                     (using dvm_add64, saturating)
mean = sum / n                   (using dvm_div_q, integer division)
```

**Overflow protection:** If `sum` would exceed INT64_MAX, set `overflow` fault.

### 6.2 Variance and Standard Deviation (Integer)

Using Welford's online algorithm with Q32.32 accumulator:

```
M₀ = 0
S₀ = 0

For each sample xₖ:
    Mₖ = M_{k-1} + (xₖ - M_{k-1}) / k
    Sₖ = S_{k-1} + (xₖ - M_{k-1}) × (xₖ - Mₖ)

variance = Sₙ / (n - 1)
stddev = isqrt(variance)
```

All operations use 64-bit integers with explicit overflow checking.

### 6.3 Integer Square Root

```c
/**
 * @brief Integer square root using binary search
 * @param n Input value
 * @return floor(sqrt(n))
 *
 * @traceability CB-MATH-001 §6.3
 */
uint64_t cb_isqrt64(uint64_t n) {
    if (n == 0) return 0;
    
    uint64_t lo = 1;
    uint64_t hi = n;
    uint64_t mid, sq;
    
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
```

**Properties:**
- Deterministic across all platforms
- O(32) iterations maximum for 64-bit input
- No floating-point operations

### 6.4 Percentile Computation

For percentile p (0–100) on n sorted samples:

```
rank_scaled = p × (n - 1)              /* Scaled by 100 */
rank = rank_scaled / 100
frac = rank_scaled % 100

lower = samples[rank]
upper = samples[rank + 1]              /* Bounds-checked */

result = lower + ((upper - lower) × frac) / 100
```

All operations use integer arithmetic with `dvm_mul64` and `dvm_round_shift_rne`.

### 6.5 Outlier Detection

Modified Z-score using Median Absolute Deviation (MAD):

```
median = percentile(samples, 50)
deviations[i] = |samples[i] - median|
MAD = percentile(deviations, 50)

For each sample xᵢ:
    /* Scaled by 10000 to avoid floating-point */
    modified_z_scaled = (6745 × |xᵢ - median|) / MAD
    
    if modified_z_scaled > 35000:    /* 3.5 × 10000 */
        mark as outlier
```

**Edge case:** If MAD = 0 (all samples identical), no outliers are flagged.

### 6.6 WCET Bound Estimation

**Empirical bound** (not a formal WCET proof):

```
wcet_observed = max(samples)
wcet_bound = wcet_observed + CB_WCET_SIGMA × stddev
```

Where `CB_WCET_SIGMA = 6` provides 99.9999% confidence under Gaussian assumption.

**Disclaimer:** For DO-178C Level A certification, static analysis tools (e.g., aiT, RapiTime) are required. This provides empirical supporting evidence only.

---

## 7. Core Invariants

### 7.1 Determinism Preservation Invariant

Let f(x) be certified inference and B the benchmark harness:

```
∀ configurations B, ∀ platforms P₁, P₂:
    H(B_{P₁}(f)(x₁…xₙ)) = H(B_{P₂}(f)(x₁…xₙ))
```

If this invariant is violated, `verify_fail` is set and the result is invalid.

**Enforced by:**
- `cb_verify` module
- Mandatory output hashing during benchmark
- `determinism_verified` gate in result structure

### 7.2 Measurement Non-Interference Invariant

Let T(f) be true execution time and T_B(f) be benchmarked time:

```
|T_B(f) − T(f)| / T(f) < 0.01    (1% overhead maximum)
```

**Guaranteed by:**
- No allocation during measurement phase
- Pre-flight setup of all buffers
- No instrumentation inside critical loop
- Timer reads only at loop boundaries

### 7.3 Integer Statistical Invariant

All metrics used for gating, regression, or certification evidence are computed in integer space:

```
∀ M ∈ {min, max, mean, median, p95, p99, stddev, wcet_bound}:
    M ∈ ℤ
```

**Floating-point restriction:**
- `double` fields (e.g., `ipc`, display ratios) are for **presentation only**
- SHALL NOT participate in pass/fail logic, CI regression, or certification gating

---

## 8. Cryptographic Binding

### 8.1 Output Hash

All inference outputs during benchmark are hashed incrementally:

```
H_outputs = SHA256(output₀ ‖ output₁ ‖ … ‖ output_{n-1})
```

Using streaming SHA-256 to avoid memory allocation.

### 8.2 Result Binding

The benchmark result cryptographically binds performance to correctness:

```
H_result = SHA256(
    "CB:RESULT:v1" ‖
    H_outputs ‖
    platform ‖
    LE64(config_hash) ‖
    LE64(min_ns) ‖
    LE64(max_ns) ‖
    LE64(mean_ns) ‖
    LE64(p99_ns) ‖
    LE64(timestamp)
)
```

### 8.3 Cross-Platform Comparability

```
Comparable(A, B) ⟺ H_outputs(A) = H_outputs(B)
```

Performance comparisons are **meaningful only when outputs are bit-identical**. Results with different output hashes SHALL NOT be compared for performance regression.

---

## 9. Environmental Characterisation (CB-SPEC-THERMAL)

### 9.1 Thermal & Frequency Drift

A benchmark result is conditionally valid based on hardware stability:

```
Valid(result) ⇒ hardware_state is stable
```

Where "stable" is defined as:

```
freq_end ≥ 0.95 × freq_start
temp_max < thermal_throttle_threshold
throttle_events = 0
```

### 9.2 Environmental Snapshot

```c
typedef struct {
    uint64_t timestamp_ns;
    uint64_t cpu_freq_hz;
    int32_t  cpu_temp_mC;     /* Millidegrees Celsius */
    uint32_t throttle_count;
} cb_env_snapshot_t;

typedef struct {
    cb_env_snapshot_t start;
    cb_env_snapshot_t end;
    uint64_t min_freq_hz;
    uint64_t max_freq_hz;
    int32_t  max_temp_mC;
    uint32_t total_throttle_events;
} cb_env_stats_t;
```

### 9.3 Stability Check

```c
bool cb_env_is_stable(const cb_env_stats_t *env) {
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

If `cb_env_is_stable() == false`, set `thermal_drift` warning flag.

---

## 10. Test Vectors

### 10.1 Integer Square Root

| Input | Expected | Notes |
|-------|----------|-------|
| 0 | 0 | Edge case |
| 1 | 1 | Minimum positive |
| 4 | 2 | Perfect square |
| 5 | 2 | floor(sqrt(5)) |
| 100 | 10 | Perfect square |
| 101 | 10 | floor(sqrt(101)) |
| 0xFFFFFFFFFFFFFFFF | 0xFFFFFFFF | Maximum input |

### 10.2 Percentile Computation

For samples [100, 200, 300, 400, 500] (n=5):

| Percentile | Expected | Calculation |
|------------|----------|-------------|
| 0 | 100 | First element |
| 25 | 200 | Interpolated |
| 50 | 300 | Median |
| 75 | 400 | Interpolated |
| 100 | 500 | Last element |

### 10.3 Outlier Detection

For samples [100, 100, 100, 100, 1000]:

| Sample | Modified Z (scaled) | Outlier? |
|--------|---------------------|----------|
| 100 | 0 | No |
| 1000 | > 35000 | Yes |

---

## 11. Document Alignment Matrix

| Topic | CB-MATH-001 | CB-STRUCT-001 | SRS |
|-------|-------------|---------------|-----|
| Fixed-point formats | §3 | §2 | SRS-002 |
| Fault flags | §5 | §3 | SRS-003, SRS-004 |
| Statistical methods | §6 | §4 | SRS-002 |
| Cryptographic binding | §8 | §5 | SRS-004 |
| Environmental data | §9 | §6 | SRS-006 |

*All documents MUST remain synchronised.*

---

## 12. References

**Standards:**
- DO-178C §6.3.4: Software Timing and Sizing Data
- ISO 26262-6 §9: Software Unit Testing (timing requirements)
- IEC 62304 §5.5.3: Software Unit Verification

**Methodology:**
- Welford, B.P. (1962). "Note on a method for calculating corrected sums of squares and products"
- Iglewicz, B. & Hoaglin, D. (1993). "How to Detect and Handle Outliers"
- Extreme Value Theory for WCET estimation

**Related Documents:**
- CT-MATH-001: certifiable-training mathematical foundations
- CM-MATH-001: certifiable-monitor mathematical foundations
- CD-MATH-001: certifiable-deploy mathematical foundations

---

## 13. Document Control

| Version | Date | Author | Change |
|---------|------|--------|--------|
| 1.0 | January 2026 | William Murray | Initial release |

---

*Built for safety. Designed for certification. Proven through testing.*

Copyright © 2026 The Murray Family Innovation Trust. All rights reserved.
