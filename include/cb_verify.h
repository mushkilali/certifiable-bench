/**
 * @file cb_verify.h
 * @brief Verification and cryptographic hashing API
 *
 * Provides SHA-256 hashing, golden reference comparison, and result binding
 * for deterministic verification of inference outputs.
 *
 * @traceability SRS-004-VERIFY, CB-MATH-001 §7.1, §8
 *
 * Copyright © 2026 The Murray Family Innovation Trust. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CB_VERIFY_H
#define CB_VERIFY_H

#include "cb_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*─────────────────────────────────────────────────────────────────────────────
 * Streaming Hash API (SRS-004-VERIFY §4.1)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Initialise verification context for streaming hash
 *
 * Prepares the context for incremental SHA-256 hashing.
 * Must be called before cb_verify_ctx_update().
 *
 * @param ctx Context to initialise
 * @return CB_OK on success
 * @return CB_ERR_NULL_PTR if ctx is NULL
 *
 * @satisfies VERIFY-F-001, VERIFY-F-007
 * @traceability SRS-004-VERIFY §4.1
 */
cb_result_code_t cb_verify_ctx_init(cb_verify_ctx_t *ctx);

/**
 * @brief Update verification context with output data
 *
 * Incrementally adds data to the hash computation.
 * Does not allocate memory. Can be called multiple times.
 *
 * @param ctx  Verification context (must be initialised)
 * @param data Output data to hash
 * @param size Data size in bytes
 * @return CB_OK on success
 * @return CB_ERR_NULL_PTR if ctx or data is NULL
 *
 * @satisfies VERIFY-F-002, VERIFY-F-005, VERIFY-F-006
 * @traceability SRS-004-VERIFY §4.1
 */
cb_result_code_t cb_verify_ctx_update(cb_verify_ctx_t *ctx,
                                      const void *data,
                                      uint32_t size);

/**
 * @brief Finalise verification and retrieve hash
 *
 * Completes the SHA-256 computation and outputs the digest.
 * After calling this, the context must be re-initialised to hash new data.
 *
 * @param ctx  Verification context
 * @param hash Output hash (32 bytes)
 * @return CB_OK on success
 * @return CB_ERR_NULL_PTR if ctx or hash is NULL
 *
 * @satisfies VERIFY-F-003, VERIFY-F-004
 * @traceability SRS-004-VERIFY §4.1
 */
cb_result_code_t cb_verify_ctx_final(cb_verify_ctx_t *ctx, cb_hash_t *hash);

/*─────────────────────────────────────────────────────────────────────────────
 * One-Shot Hash API (SRS-004-VERIFY §4.6)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Compute SHA-256 hash of arbitrary data (one-shot)
 *
 * Convenience function for hashing data in a single call.
 *
 * @param data Input data
 * @param size Data size in bytes
 * @param hash Output hash (32 bytes)
 * @return CB_OK on success
 * @return CB_ERR_NULL_PTR if data or hash is NULL
 *
 * @satisfies VERIFY-F-050, VERIFY-F-051
 * @traceability SRS-004-VERIFY §4.6
 */
cb_result_code_t cb_compute_hash(const void *data, uint32_t size, cb_hash_t *hash);

/*─────────────────────────────────────────────────────────────────────────────
 * Hash Comparison (SRS-004-VERIFY §4.3)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Compare two hashes (constant-time)
 *
 * Performs constant-time comparison to prevent timing attacks.
 * Returns true only if all 32 bytes match.
 *
 * @param a First hash
 * @param b Second hash
 * @return true if hashes are equal
 *
 * @satisfies VERIFY-F-023, VERIFY-F-024, VERIFY-NF-020
 * @traceability SRS-004-VERIFY §4.3, §9
 */
bool cb_hash_equal(const cb_hash_t *a, const cb_hash_t *b);

/*─────────────────────────────────────────────────────────────────────────────
 * Hash Utilities (SRS-004-VERIFY §4.6)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Convert hash to lowercase hex string
 *
 * Outputs 64 lowercase hexadecimal characters plus null terminator.
 *
 * @param hash    Input hash
 * @param hex_out Output buffer (must be at least 65 bytes)
 * @return CB_OK on success
 * @return CB_ERR_NULL_PTR if hash or hex_out is NULL
 *
 * @satisfies VERIFY-F-052, VERIFY-F-053
 * @traceability SRS-004-VERIFY §4.6
 */
cb_result_code_t cb_hash_to_hex(const cb_hash_t *hash, char *hex_out);

/**
 * @brief Parse hex string to hash
 *
 * Parses 64 hexadecimal characters into a hash structure.
 *
 * @param hex_in Input hex string (64 characters)
 * @param hash   Output hash
 * @return CB_OK on success
 * @return CB_ERR_NULL_PTR if hex_in or hash is NULL
 * @return CB_ERR_GOLDEN_LOAD if hex string is invalid
 *
 * @traceability SRS-004-VERIFY §4.6
 */
cb_result_code_t cb_hash_from_hex(const char *hex_in, cb_hash_t *hash);

/*─────────────────────────────────────────────────────────────────────────────
 * Golden Reference (SRS-004-VERIFY §4.2)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Load golden reference from file
 *
 * Reads a golden reference file containing expected output hash,
 * sample count, and metadata.
 *
 * @param path   Path to golden reference file
 * @param golden Output golden reference structure
 * @return CB_OK on success
 * @return CB_ERR_NULL_PTR if path or golden is NULL
 * @return CB_ERR_GOLDEN_LOAD on file read or parse failure
 *
 * @satisfies VERIFY-F-010 through VERIFY-F-015
 * @traceability SRS-004-VERIFY §4.2, §7
 */
cb_result_code_t cb_verify_load_golden(const char *path, cb_golden_ref_t *golden);

/**
 * @brief Save golden reference to file
 *
 * Writes a golden reference file for future verification.
 *
 * @param path   Path to output file
 * @param golden Golden reference to save
 * @return CB_OK on success
 * @return CB_ERR_NULL_PTR if path or golden is NULL
 * @return CB_ERR_GOLDEN_LOAD on file write failure
 *
 * @traceability SRS-004-VERIFY §4.2, §7
 */
cb_result_code_t cb_verify_save_golden(const char *path, const cb_golden_ref_t *golden);

/* Aliases for test compatibility */
cb_result_code_t cb_golden_load(const char *path, cb_golden_ref_t *golden);
cb_result_code_t cb_golden_save(const char *path, const cb_golden_ref_t *golden);
bool cb_golden_verify(const cb_hash_t *computed, const cb_golden_ref_t *golden);

/*─────────────────────────────────────────────────────────────────────────────
 * Result Binding (SRS-004-VERIFY §4.5, CB-MATH-001 §8.2)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Compute result binding hash
 *
 * Creates a cryptographic commitment binding performance data to
 * correctness proof. Format per CB-MATH-001 §8.2:
 *
 *   H_result = SHA256(
 *       "CB:RESULT:v1" ||
 *       H_outputs[32] ||
 *       platform[32] ||
 *       LE64(config_hash) ||
 *       LE64(min_ns) ||
 *       LE64(max_ns) ||
 *       LE64(mean_ns) ||
 *       LE64(p99_ns) ||
 *       LE64(timestamp_unix)
 *   )
 *
 * @param output_hash    Hash of inference outputs
 * @param platform       Platform identifier (will be null-padded to 32 bytes)
 * @param config_hash    Configuration hash
 * @param stats          Latency statistics
 * @param timestamp_unix Unix timestamp
 * @param result_hash    Output result binding hash
 * @return CB_OK on success
 * @return CB_ERR_NULL_PTR if any pointer is NULL
 *
 * @satisfies VERIFY-F-040 through VERIFY-F-047
 * @traceability SRS-004-VERIFY §4.5, §8, CB-MATH-001 §8.2
 */
cb_result_code_t cb_compute_result_hash(const cb_hash_t *output_hash,
                                        const char *platform,
                                        uint64_t config_hash,
                                        const cb_latency_stats_t *stats,
                                        uint64_t timestamp_unix,
                                        cb_hash_t *result_hash);

/* Alias for test compatibility */
cb_result_code_t cb_compute_result_binding(const cb_hash_t *output_hash,
                                           const char *platform,
                                           uint64_t config_hash,
                                           const cb_latency_stats_t *stats,
                                           uint64_t timestamp_unix,
                                           cb_hash_t *result_hash);

#ifdef __cplusplus
}
#endif

#endif /* CB_VERIFY_H */
