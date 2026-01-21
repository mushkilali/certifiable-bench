# SRS-004-VERIFY: Verification Module Requirements

**Component:** certifiable-bench  
**Module:** Verify (`cb_verify.h`, `src/runner/verify.c`)  
**Version:** 1.0  
**Status:** Draft  
**Date:** January 2026  
**Traceability:** CB-MATH-001 §7.1, §8

---

## 1. Purpose

This document specifies requirements for the verification module, which ensures bit-identity of inference outputs during benchmarking through cryptographic hashing.

---

## 2. Scope

The verification module provides:
- Streaming output hash computation
- Golden reference loading and comparison
- Determinism gate (pass/fail based on hash match)
- Result binding (cryptographic seal on benchmark results)

---

## 3. Definitions

| Term | Definition |
|------|------------|
| Golden reference | Pre-computed expected output hash from a known-correct execution |
| Output hash | SHA-256 digest of all inference outputs concatenated |
| Result binding | Cryptographic commitment linking performance data to correctness proof |
| Determinism gate | Boolean flag indicating all outputs matched expected values |

---

## 4. Functional Requirements

### 4.1 Hash Computation

| ID | Requirement | Verification |
|----|-------------|--------------|
| VERIFY-F-001 | The module SHALL provide a function `cb_verify_ctx_init()` to initialise verification context. | Test |
| VERIFY-F-002 | The module SHALL provide a function `cb_verify_ctx_update()` to incrementally hash output data. | Test |
| VERIFY-F-003 | The module SHALL provide a function `cb_verify_ctx_final()` to finalise and retrieve hash. | Test |
| VERIFY-F-004 | Hash computation SHALL use SHA-256 algorithm. | Test |
| VERIFY-F-005 | Streaming hash SHALL NOT require storing all outputs in memory. | Inspection |
| VERIFY-F-006 | `cb_verify_ctx_update()` SHALL NOT allocate memory. | Inspection |
| VERIFY-F-007 | Hash state SHALL be stored in `cb_verify_ctx_t` structure. | Inspection |

### 4.2 Golden Reference

| ID | Requirement | Verification |
|----|-------------|--------------|
| VERIFY-F-010 | The module SHALL provide a function `cb_verify_load_golden()` to load golden reference. | Test |
| VERIFY-F-011 | Golden reference SHALL contain expected output hash. | Inspection |
| VERIFY-F-012 | Golden reference SHALL contain expected sample count. | Inspection |
| VERIFY-F-013 | Golden reference SHALL contain platform identifier that generated it. | Inspection |
| VERIFY-F-014 | `cb_verify_load_golden()` SHALL return `CB_ERR_GOLDEN_LOAD` on file read failure. | Test |
| VERIFY-F-015 | Golden reference format SHALL be documented and versioned. | Inspection |

### 4.3 Output Verification

| ID | Requirement | Verification |
|----|-------------|--------------|
| VERIFY-F-020 | The module SHALL provide a function `cb_verify_output()` to verify a single output. | Test |
| VERIFY-F-021 | `cb_verify_output()` SHALL compare output hash against expected hash. | Test |
| VERIFY-F-022 | Hash comparison SHALL be constant-time to prevent timing attacks. | Inspection |
| VERIFY-F-023 | The module SHALL provide a function `cb_hash_equal()` for hash comparison. | Test |
| VERIFY-F-024 | `cb_hash_equal()` SHALL return true only if all 32 bytes match. | Test |

### 4.4 Determinism Gate

| ID | Requirement | Verification |
|----|-------------|--------------|
| VERIFY-F-030 | If any output hash mismatches, `verify_fail` fault flag SHALL be set. | Test |
| VERIFY-F-031 | `cb_result_t.determinism_verified` SHALL be true only if all outputs matched. | Test |
| VERIFY-F-032 | `cb_result_t.verification_failures` SHALL count the number of mismatched outputs. | Test |
| VERIFY-F-033 | Verification failure SHALL NOT abort the benchmark (timing continues). | Test |
| VERIFY-F-034 | Final output hash SHALL be stored in `cb_result_t.output_hash`. | Test |

### 4.5 Result Binding (CB-MATH-001 §8.2)

| ID | Requirement | Verification |
|----|-------------|--------------|
| VERIFY-F-040 | The module SHALL provide a function `cb_compute_result_hash()` for result binding. | Test |
| VERIFY-F-041 | Result hash SHALL include output hash. | Test |
| VERIFY-F-042 | Result hash SHALL include platform identifier. | Test |
| VERIFY-F-043 | Result hash SHALL include configuration hash. | Test |
| VERIFY-F-044 | Result hash SHALL include key latency metrics (min, max, mean, p99). | Test |
| VERIFY-F-045 | Result hash SHALL include timestamp. | Test |
| VERIFY-F-046 | Result hash SHALL use the format specified in CB-MATH-001 §8.2. | Inspection |
| VERIFY-F-047 | Result hash SHALL be stored in `cb_result_t.result_hash`. | Test |

### 4.6 Standalone Hash Utilities

| ID | Requirement | Verification |
|----|-------------|--------------|
| VERIFY-F-050 | The module SHALL provide a function `cb_compute_hash()` for one-shot hashing. | Test |
| VERIFY-F-051 | `cb_compute_hash()` SHALL compute SHA-256 of arbitrary data buffer. | Test |
| VERIFY-F-052 | The module SHALL provide a function `cb_hash_to_hex()` for hex string conversion. | Test |
| VERIFY-F-053 | `cb_hash_to_hex()` output SHALL be 64 lowercase hex characters plus null terminator. | Test |

---

## 5. Non-Functional Requirements

### 5.1 Determinism

| ID | Requirement | Verification |
|----|-------------|--------------|
| VERIFY-NF-001 | Hash computation SHALL produce bit-identical results across all platforms. | Test |
| VERIFY-NF-002 | SHA-256 implementation SHALL follow FIPS 180-4 specification. | Inspection |
| VERIFY-NF-003 | No floating-point operations SHALL be used in hash computation. | Inspection |

### 5.2 Performance

| ID | Requirement | Verification |
|----|-------------|--------------|
| VERIFY-NF-010 | `cb_verify_ctx_update()` overhead SHALL be < 1 µs per KB of output data. | Test |
| VERIFY-NF-011 | Verification SHALL NOT significantly impact benchmark throughput (< 5% overhead). | Test |

### 5.3 Security

| ID | Requirement | Verification |
|----|-------------|--------------|
| VERIFY-NF-020 | Hash comparison SHALL be constant-time. | Inspection |
| VERIFY-NF-021 | Golden reference SHALL NOT be modifiable during benchmark execution. | Inspection |
| VERIFY-NF-022 | Result binding SHALL prevent tampering with performance data after signing. | Analysis |

---

## 6. Interface Definition

```c
/**
 * @brief Initialise verification context
 * @param ctx Context to initialise
 *
 * @satisfies VERIFY-F-001
 */
void cb_verify_ctx_init(cb_verify_ctx_t *ctx);

/**
 * @brief Update verification context with output data
 * @param ctx Verification context
 * @param data Output data
 * @param size Data size in bytes
 *
 * @satisfies VERIFY-F-002, VERIFY-F-006
 */
void cb_verify_ctx_update(cb_verify_ctx_t *ctx, const void *data, uint32_t size);

/**
 * @brief Finalise verification and retrieve hash
 * @param ctx Verification context
 * @param hash Output hash (32 bytes)
 *
 * @satisfies VERIFY-F-003, VERIFY-F-004
 */
void cb_verify_ctx_final(cb_verify_ctx_t *ctx, cb_hash_t *hash);

/**
 * @brief Load golden reference from file
 * @param path Path to golden reference file
 * @param golden Output golden reference structure
 * @return CB_OK on success
 *
 * @satisfies VERIFY-F-010 through VERIFY-F-015
 */
cb_result_code_t cb_verify_load_golden(const char *path, cb_golden_ref_t *golden);

/**
 * @brief Compare two hashes (constant-time)
 * @param a First hash
 * @param b Second hash
 * @return true if equal
 *
 * @satisfies VERIFY-F-023, VERIFY-F-024, VERIFY-NF-020
 */
bool cb_hash_equal(const cb_hash_t *a, const cb_hash_t *b);

/**
 * @brief Compute result binding hash
 * @param result Benchmark result (output_hash must be populated)
 *
 * @satisfies VERIFY-F-040 through VERIFY-F-047
 */
void cb_compute_result_hash(cb_result_t *result);

/**
 * @brief Compute hash of arbitrary data
 * @param data Input data
 * @param size Data size
 * @param hash Output hash
 *
 * @satisfies VERIFY-F-050, VERIFY-F-051
 */
void cb_compute_hash(const void *data, uint32_t size, cb_hash_t *hash);

/**
 * @brief Convert hash to hex string
 * @param hash Input hash
 * @param hex_out Output buffer (must be at least 65 bytes)
 *
 * @satisfies VERIFY-F-052, VERIFY-F-053
 */
void cb_hash_to_hex(const cb_hash_t *hash, char *hex_out);
```

---

## 7. Golden Reference File Format

```json
{
  "version": "1.0",
  "format": "cb_golden_ref",
  "output_hash": "48f4ecebc0eec79ab15fe694717a831e7218993502f84c66f0ef0f3d178c0138",
  "sample_count": 1000,
  "output_size": 40,
  "platform": "x86_64",
  "model_hash": "abc123...",
  "data_hash": "def456...",
  "generated": "2026-01-21T12:00:00Z"
}
```

---

## 8. Result Binding Format (CB-MATH-001 §8.2)

```
H_result = SHA256(
    "CB:RESULT:v1" ||
    H_outputs[32] ||
    platform[32] (null-padded) ||
    LE64(config_hash) ||
    LE64(min_ns) ||
    LE64(max_ns) ||
    LE64(mean_ns) ||
    LE64(p99_ns) ||
    LE64(timestamp_unix)
)
```

Total input: 7 + 32 + 32 + 8 + 8 + 8 + 8 + 8 + 8 = 119 bytes

---

## 9. Constant-Time Hash Comparison

```c
bool cb_hash_equal(const cb_hash_t *a, const cb_hash_t *b) {
    uint8_t diff = 0;
    for (int i = 0; i < CB_HASH_SIZE; i++) {
        diff |= a->bytes[i] ^ b->bytes[i];
    }
    return diff == 0;
}
```

---

## 10. Traceability Matrix

| Requirement | CB-MATH-001 | CB-STRUCT-001 | Test |
|-------------|-------------|---------------|------|
| VERIFY-F-001 | §8.1 | §5 | test_verify_init |
| VERIFY-F-004 | §8.1 | — | test_sha256 |
| VERIFY-F-010 | §8.3 | §5 | test_load_golden |
| VERIFY-F-023 | — | — | test_hash_equal |
| VERIFY-F-030 | §7.1 | §3 | test_determinism_gate |
| VERIFY-F-040 | §8.2 | §10 | test_result_binding |
| VERIFY-NF-001 | §8 | — | test_cross_platform |

---

## 11. Document Control

| Version | Date | Author | Change |
|---------|------|--------|--------|
| 1.0 | January 2026 | William Murray | Initial release |

---

*Built for safety. Designed for certification. Proven through testing.*

Copyright © 2026 The Murray Family Innovation Trust. All rights reserved.
