/**
 * @file cb_report.h
 * @brief Report generation API (JSON, CSV, comparison)
 *
 * Provides serialisation, deserialisation, and comparison functions
 * for benchmark results per CB-STRUCT-001 §14.
 *
 * @traceability SRS-005-REPORT, CB-STRUCT-001 §14
 *
 * Copyright © 2026 The Murray Family Innovation Trust. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CB_REPORT_H
#define CB_REPORT_H

#include "cb_types.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/*─────────────────────────────────────────────────────────────────────────────
 * JSON Output (SRS-005-REPORT §4.1)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Write results to JSON file
 *
 * Serialises benchmark result to JSON format per CB-STRUCT-001 §14.
 * Uses streaming output with 2-space indentation.
 *
 * @param result Benchmark result to serialise
 * @param path   Output file path
 * @return CB_OK on success
 * @return CB_ERR_NULL_PTR if result or path is NULL
 * @return CB_ERR_IO on file write failure
 *
 * @satisfies REPORT-F-001 through REPORT-F-015
 * @traceability SRS-005-REPORT §4.1
 */
cb_result_code_t cb_write_json(const cb_result_t *result, const char *path);

/**
 * @brief Write results to JSON stream
 *
 * Writes JSON to an already-open file stream.
 *
 * @param result Benchmark result to serialise
 * @param fp     Open file stream
 * @return CB_OK on success
 * @return CB_ERR_NULL_PTR if result or fp is NULL
 * @return CB_ERR_IO on write failure
 */
cb_result_code_t cb_write_json_fp(const cb_result_t *result, FILE *fp);

/*─────────────────────────────────────────────────────────────────────────────
 * CSV Output (SRS-005-REPORT §4.2)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Write results to CSV file
 *
 * Serialises benchmark result to CSV format with header row.
 *
 * @param result Benchmark result to serialise
 * @param path   Output file path
 * @return CB_OK on success
 * @return CB_ERR_NULL_PTR if result or path is NULL
 * @return CB_ERR_IO on file write failure
 *
 * @satisfies REPORT-F-020 through REPORT-F-026
 * @traceability SRS-005-REPORT §4.2
 */
cb_result_code_t cb_write_csv(const cb_result_t *result, const char *path);

/**
 * @brief Append result to existing CSV file (data row only)
 *
 * @param result Benchmark result to append
 * @param path   Existing CSV file path
 * @return CB_OK on success
 */
cb_result_code_t cb_append_csv(const cb_result_t *result, const char *path);

/*─────────────────────────────────────────────────────────────────────────────
 * JSON Loading (SRS-005-REPORT §4.3)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Load results from JSON file
 *
 * Deserialises benchmark result from JSON file.
 *
 * @param path   Input file path
 * @param result Output result structure
 * @return CB_OK on success
 * @return CB_ERR_NULL_PTR if path or result is NULL
 * @return CB_ERR_IO on file read failure
 * @return CB_ERR_INVALID_CONFIG on JSON parse error or missing required fields
 *
 * @satisfies REPORT-F-030 through REPORT-F-035
 * @traceability SRS-005-REPORT §4.3
 */
cb_result_code_t cb_load_json(const char *path, cb_result_t *result);

/*─────────────────────────────────────────────────────────────────────────────
 * Cross-Platform Comparison (SRS-005-REPORT §4.4)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Compare two benchmark results
 *
 * Compares results per CB-MATH-001 §8.3:
 *   Comparable(A, B) ⟺ H_outputs(A) = H_outputs(B)
 *
 * If outputs are not identical, performance comparison is not meaningful
 * and comparison fields are zeroed.
 *
 * @param result_a   Reference result (baseline)
 * @param result_b   Target result (comparison)
 * @param comparison Output comparison structure
 * @return CB_OK on success
 * @return CB_ERR_NULL_PTR if any pointer is NULL
 *
 * @satisfies REPORT-F-040 through REPORT-F-049
 * @traceability SRS-005-REPORT §4.4, CB-MATH-001 §8.3
 */
cb_result_code_t cb_compare_results(const cb_result_t *result_a,
                                     const cb_result_t *result_b,
                                     cb_comparison_t *comparison);

/*─────────────────────────────────────────────────────────────────────────────
 * Human-Readable Output (SRS-005-REPORT §4.5)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Print result summary to stdout
 *
 * Prints human-readable summary including platform, latency,
 * throughput, and verification status.
 *
 * @param result Benchmark result to print
 *
 * @satisfies REPORT-F-050 through REPORT-F-055
 * @traceability SRS-005-REPORT §4.5
 */
void cb_print_summary(const cb_result_t *result);

/**
 * @brief Print result summary to stream
 *
 * @param result Benchmark result to print
 * @param fp     Output stream
 */
void cb_print_summary_fp(const cb_result_t *result, FILE *fp);

/**
 * @brief Print comparison summary to stdout
 *
 * Prints human-readable comparison including bit-identity status,
 * latency difference/ratio, and throughput difference/ratio.
 *
 * @param comparison Comparison result to print
 *
 * @satisfies REPORT-F-056 through REPORT-F-058
 * @traceability SRS-005-REPORT §4.5
 */
void cb_print_comparison(const cb_comparison_t *comparison);

/**
 * @brief Print comparison summary to stream
 *
 * @param comparison Comparison result to print
 * @param fp         Output stream
 */
void cb_print_comparison_fp(const cb_comparison_t *comparison, FILE *fp);

#ifdef __cplusplus
}
#endif

#endif /* CB_REPORT_H */
