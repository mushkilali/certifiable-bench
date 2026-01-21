/**
 * @file report.c
 * @brief Report generation implementation (JSON, CSV, comparison)
 *
 * Implements SRS-005-REPORT requirements for result serialisation,
 * deserialisation, and cross-platform comparison.
 *
 * @traceability SRS-005-REPORT, CB-STRUCT-001 §14
 *
 * Copyright © 2026 The Murray Family Innovation Trust. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cb_report.h"
#include "cb_verify.h"

#include <stdio.h>
#include <stdlib.h>  /* strtoull, strtoll */
#include <string.h>
#include <inttypes.h>

/*─────────────────────────────────────────────────────────────────────────────
 * JSON Output Helpers
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Write hash as 64-character lowercase hex string
 * @satisfies REPORT-F-012
 */
static void write_hash_json(FILE *fp, const char *key, const cb_hash_t *hash, int indent, int comma)
{
    char hex[65];
    cb_hash_to_hex(hash, hex);
    fprintf(fp, "%*s\"%s\": \"%s\"%s\n", indent, "", key, hex, comma ? "," : "");
}

/**
 * @brief Write boolean as JSON true/false
 */
static void write_bool_json(FILE *fp, const char *key, bool value, int indent, int comma)
{
    fprintf(fp, "%*s\"%s\": %s%s\n", indent, "", key, value ? "true" : "false", comma ? "," : "");
}

/**
 * @brief Write uint64 as JSON number
 * @satisfies REPORT-NF-002 (integers only)
 */
static void write_u64_json(FILE *fp, const char *key, uint64_t value, int indent, int comma)
{
    fprintf(fp, "%*s\"%s\": %" PRIu64 "%s\n", indent, "", key, value, comma ? "," : "");
}

/**
 * @brief Write uint32 as JSON number
 */
static void write_u32_json(FILE *fp, const char *key, uint32_t value, int indent, int comma)
{
    fprintf(fp, "%*s\"%s\": %" PRIu32 "%s\n", indent, "", key, value, comma ? "," : "");
}

/**
 * @brief Write int32 as JSON number
 */
static void write_i32_json(FILE *fp, const char *key, int32_t value, int indent, int comma)
{
    fprintf(fp, "%*s\"%s\": %" PRId32 "%s\n", indent, "", key, value, comma ? "," : "");
}

/**
 * @brief Write string as JSON string (with escaping)
 */
static void write_str_json(FILE *fp, const char *key, const char *value, int indent, int comma)
{
    fprintf(fp, "%*s\"%s\": \"", indent, "", key);
    /* Simple escaping for common characters */
    for (const char *p = value; *p; p++) {
        switch (*p) {
            case '"':  fprintf(fp, "\\\""); break;
            case '\\': fprintf(fp, "\\\\"); break;
            case '\n': fprintf(fp, "\\n"); break;
            case '\r': fprintf(fp, "\\r"); break;
            case '\t': fprintf(fp, "\\t"); break;
            default:   fputc(*p, fp); break;
        }
    }
    fprintf(fp, "\"%s\n", comma ? "," : "");
}

/*─────────────────────────────────────────────────────────────────────────────
 * JSON Output (REPORT-F-001..015)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Write results to JSON stream
 * @satisfies REPORT-F-001 through REPORT-F-015, REPORT-NF-001..003
 */
cb_result_code_t cb_write_json_fp(const cb_result_t *result, FILE *fp)
{
    if (result == NULL || fp == NULL) {
        return CB_ERR_NULL_PTR;
    }

    /* REPORT-F-015: 2-space indentation */
    /* REPORT-NF-003: Fixed key order for determinism */

    fprintf(fp, "{\n");

    /* REPORT-F-002: Version */
    write_str_json(fp, "version", "1.0", 2, 1);

    /* REPORT-F-003: Platform */
    write_str_json(fp, "platform", result->platform, 2, 1);

    /* REPORT-F-004: CPU model */
    write_str_json(fp, "cpu_model", result->cpu_model, 2, 1);
    write_u32_json(fp, "cpu_freq_mhz", result->cpu_freq_mhz, 2, 1);

    /* REPORT-F-005: Configuration */
    fprintf(fp, "  \"config\": {\n");
    write_u32_json(fp, "warmup_iterations", result->warmup_iterations, 4, 1);
    write_u32_json(fp, "measure_iterations", result->measure_iterations, 4, 1);
    write_u32_json(fp, "batch_size", result->batch_size, 4, 0);
    fprintf(fp, "  },\n");

    /* REPORT-F-006: Latency statistics */
    fprintf(fp, "  \"latency\": {\n");
    write_u64_json(fp, "min_ns", result->latency.min_ns, 4, 1);
    write_u64_json(fp, "max_ns", result->latency.max_ns, 4, 1);
    write_u64_json(fp, "mean_ns", result->latency.mean_ns, 4, 1);
    write_u64_json(fp, "median_ns", result->latency.median_ns, 4, 1);
    write_u64_json(fp, "p95_ns", result->latency.p95_ns, 4, 1);
    write_u64_json(fp, "p99_ns", result->latency.p99_ns, 4, 1);
    write_u64_json(fp, "stddev_ns", result->latency.stddev_ns, 4, 1);
    write_u64_json(fp, "variance_ns2", result->latency.variance_ns2, 4, 1);
    write_u32_json(fp, "sample_count", result->latency.sample_count, 4, 1);
    write_u32_json(fp, "outlier_count", result->latency.outlier_count, 4, 1);
    write_u64_json(fp, "wcet_observed_ns", result->latency.wcet_observed_ns, 4, 1);
    write_u64_json(fp, "wcet_bound_ns", result->latency.wcet_bound_ns, 4, 0);
    fprintf(fp, "  },\n");

    /* REPORT-F-007: Throughput */
    fprintf(fp, "  \"throughput\": {\n");
    write_u64_json(fp, "inferences_per_sec", result->throughput.inferences_per_sec, 4, 1);
    write_u64_json(fp, "samples_per_sec", result->throughput.samples_per_sec, 4, 1);
    write_u64_json(fp, "bytes_per_sec", result->throughput.bytes_per_sec, 4, 1);
    write_u32_json(fp, "batch_size", result->throughput.batch_size, 4, 0);
    fprintf(fp, "  },\n");

    /* REPORT-F-008: Verification */
    fprintf(fp, "  \"verification\": {\n");
    write_bool_json(fp, "determinism_verified", result->determinism_verified, 4, 1);
    write_u32_json(fp, "verification_failures", result->verification_failures, 4, 1);
    write_hash_json(fp, "output_hash", &result->output_hash, 4, 1);
    write_hash_json(fp, "result_hash", &result->result_hash, 4, 0);
    fprintf(fp, "  },\n");

    /* REPORT-F-009: Environment */
    fprintf(fp, "  \"environment\": {\n");
    write_bool_json(fp, "stable", result->env_stable, 4, 1);
    write_u64_json(fp, "start_freq_hz", result->environment.start.cpu_freq_hz, 4, 1);
    write_u64_json(fp, "end_freq_hz", result->environment.end.cpu_freq_hz, 4, 1);
    write_u64_json(fp, "min_freq_hz", result->environment.min_freq_hz, 4, 1);
    write_u64_json(fp, "max_freq_hz", result->environment.max_freq_hz, 4, 1);
    write_i32_json(fp, "start_temp_mC", result->environment.start.cpu_temp_mC, 4, 1);
    write_i32_json(fp, "end_temp_mC", result->environment.end.cpu_temp_mC, 4, 1);
    write_i32_json(fp, "min_temp_mC", result->environment.min_temp_mC, 4, 1);
    write_i32_json(fp, "max_temp_mC", result->environment.max_temp_mC, 4, 1);
    write_u32_json(fp, "throttle_events", result->environment.total_throttle_events, 4, 0);
    fprintf(fp, "  },\n");

    /* REPORT-F-060..063: Histogram (if valid) */
    fprintf(fp, "  \"histogram\": {\n");
    write_bool_json(fp, "valid", result->histogram_valid, 4, 1);
    if (result->histogram_valid && result->histogram.bins != NULL) {
        write_u64_json(fp, "range_min_ns", result->histogram.range_min_ns, 4, 1);
        write_u64_json(fp, "range_max_ns", result->histogram.range_max_ns, 4, 1);
        write_u64_json(fp, "bin_width_ns", result->histogram.bin_width_ns, 4, 1);
        write_u32_json(fp, "num_bins", result->histogram.num_bins, 4, 1);
        write_u32_json(fp, "overflow_count", result->histogram.overflow_count, 4, 1);
        write_u32_json(fp, "underflow_count", result->histogram.underflow_count, 4, 1);
        /* Bins array */
        fprintf(fp, "    \"bins\": [");
        for (uint32_t i = 0; i < result->histogram.num_bins; i++) {
            if (i > 0) fprintf(fp, ", ");
            if (i % 20 == 0 && i > 0) fprintf(fp, "\n      ");
            fprintf(fp, "%" PRIu32, result->histogram.bins[i].count);
        }
        fprintf(fp, "]\n");
    } else {
        write_u64_json(fp, "range_min_ns", 0, 4, 1);
        write_u64_json(fp, "range_max_ns", 0, 4, 1);
        write_u32_json(fp, "num_bins", 0, 4, 0);
    }
    fprintf(fp, "  },\n");

    /* REPORT-F-010: Faults */
    fprintf(fp, "  \"faults\": {\n");
    write_bool_json(fp, "overflow", result->faults.overflow, 4, 1);
    write_bool_json(fp, "underflow", result->faults.underflow, 4, 1);
    write_bool_json(fp, "div_zero", result->faults.div_zero, 4, 1);
    write_bool_json(fp, "timer_error", result->faults.timer_error, 4, 1);
    write_bool_json(fp, "verify_fail", result->faults.verify_fail, 4, 1);
    write_bool_json(fp, "thermal_drift", result->faults.thermal_drift, 4, 0);
    fprintf(fp, "  },\n");

    /* Timing metadata */
    write_u64_json(fp, "benchmark_start_ns", result->benchmark_start_ns, 2, 1);
    write_u64_json(fp, "benchmark_end_ns", result->benchmark_end_ns, 2, 1);
    write_u64_json(fp, "benchmark_duration_ns", result->benchmark_duration_ns, 2, 1);

    /* REPORT-F-011: Timestamp */
    write_u64_json(fp, "timestamp_unix", result->timestamp_unix, 2, 0);

    fprintf(fp, "}\n");

    if (ferror(fp)) {
        return CB_ERR_IO;
    }

    return CB_OK;
}

/**
 * @brief Write results to JSON file
 * @satisfies REPORT-F-001, REPORT-F-013
 */
cb_result_code_t cb_write_json(const cb_result_t *result, const char *path)
{
    FILE *fp;
    cb_result_code_t rc;

    if (result == NULL || path == NULL) {
        return CB_ERR_NULL_PTR;
    }

    fp = fopen(path, "w");
    if (fp == NULL) {
        return CB_ERR_IO;
    }

    rc = cb_write_json_fp(result, fp);

    if (fclose(fp) != 0 && rc == CB_OK) {
        rc = CB_ERR_IO;
    }

    return rc;
}

/*─────────────────────────────────────────────────────────────────────────────
 * CSV Output (REPORT-F-020..026)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Write CSV header
 * @satisfies REPORT-F-021, REPORT-F-023
 */
static void write_csv_header(FILE *fp)
{
    fprintf(fp, "platform,cpu_model,min_ns,max_ns,mean_ns,median_ns,p95_ns,p99_ns,"
                "stddev_ns,inferences_per_sec,determinism_verified,output_hash,timestamp_unix\n");
}

/**
 * @brief Write CSV data row
 * @satisfies REPORT-F-022, REPORT-F-024, REPORT-F-025
 */
static void write_csv_row(FILE *fp, const cb_result_t *result)
{
    char hash_hex[65];
    cb_hash_to_hex(&result->output_hash, hash_hex);

    /* REPORT-F-025: Quote strings containing commas */
    fprintf(fp, "%s,\"%s\",", result->platform, result->cpu_model);

    fprintf(fp, "%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",",
            result->latency.min_ns, result->latency.max_ns,
            result->latency.mean_ns, result->latency.median_ns);

    fprintf(fp, "%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",",
            result->latency.p95_ns, result->latency.p99_ns, result->latency.stddev_ns);

    fprintf(fp, "%" PRIu64 ",%s,%s,%" PRIu64 "\n",
            result->throughput.inferences_per_sec,
            result->determinism_verified ? "true" : "false",
            hash_hex,
            result->timestamp_unix);
}

/**
 * @brief Write results to CSV file
 * @satisfies REPORT-F-020 through REPORT-F-026
 */
cb_result_code_t cb_write_csv(const cb_result_t *result, const char *path)
{
    FILE *fp;

    if (result == NULL || path == NULL) {
        return CB_ERR_NULL_PTR;
    }

    fp = fopen(path, "w");
    if (fp == NULL) {
        return CB_ERR_IO;
    }

    write_csv_header(fp);
    write_csv_row(fp, result);

    if (ferror(fp)) {
        fclose(fp);
        return CB_ERR_IO;
    }

    if (fclose(fp) != 0) {
        return CB_ERR_IO;
    }

    return CB_OK;
}

/**
 * @brief Append result to CSV file (data row only)
 */
cb_result_code_t cb_append_csv(const cb_result_t *result, const char *path)
{
    FILE *fp;

    if (result == NULL || path == NULL) {
        return CB_ERR_NULL_PTR;
    }

    fp = fopen(path, "a");
    if (fp == NULL) {
        return CB_ERR_IO;
    }

    write_csv_row(fp, result);

    if (ferror(fp)) {
        fclose(fp);
        return CB_ERR_IO;
    }

    if (fclose(fp) != 0) {
        return CB_ERR_IO;
    }

    return CB_OK;
}

/*─────────────────────────────────────────────────────────────────────────────
 * JSON Loading (REPORT-F-030..035)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Extract string value from JSON
 */
static int json_extract_string(const char *json, const char *key, char *out, size_t out_size)
{
    char search[128];
    const char *start, *end;
    size_t len;

    snprintf(search, sizeof(search), "\"%s\"", key);
    start = strstr(json, search);
    if (start == NULL) return 0;

    start = strchr(start, ':');
    if (start == NULL) return 0;
    start++;

    while (*start == ' ' || *start == '\t' || *start == '\n') start++;
    if (*start != '"') return 0;
    start++;

    end = start;
    while (*end && *end != '"') {
        if (*end == '\\' && *(end + 1)) end++;  /* Skip escaped char */
        end++;
    }

    len = (size_t)(end - start);
    if (len >= out_size) len = out_size - 1;
    memcpy(out, start, len);
    out[len] = '\0';
    return 1;
}

/**
 * @brief Extract uint64 value from JSON
 */
static int json_extract_u64(const char *json, const char *key, uint64_t *out)
{
    char search[128];
    const char *start;

    snprintf(search, sizeof(search), "\"%s\"", key);
    start = strstr(json, search);
    if (start == NULL) return 0;

    start = strchr(start, ':');
    if (start == NULL) return 0;
    start++;

    while (*start == ' ' || *start == '\t' || *start == '\n') start++;

    *out = strtoull(start, NULL, 10);
    return 1;
}

/**
 * @brief Extract int64 value from JSON
 */
static int json_extract_i64(const char *json, const char *key, int64_t *out)
{
    char search[128];
    const char *start;

    snprintf(search, sizeof(search), "\"%s\"", key);
    start = strstr(json, search);
    if (start == NULL) return 0;

    start = strchr(start, ':');
    if (start == NULL) return 0;
    start++;

    while (*start == ' ' || *start == '\t' || *start == '\n') start++;

    *out = strtoll(start, NULL, 10);
    return 1;
}

/**
 * @brief Extract uint32 value from JSON
 */
static int json_extract_u32(const char *json, const char *key, uint32_t *out)
{
    uint64_t tmp;
    if (!json_extract_u64(json, key, &tmp)) return 0;
    *out = (uint32_t)tmp;
    return 1;
}

/**
 * @brief Extract int32 value from JSON
 */
static int json_extract_i32(const char *json, const char *key, int32_t *out)
{
    int64_t tmp;
    if (!json_extract_i64(json, key, &tmp)) return 0;
    *out = (int32_t)tmp;
    return 1;
}

/**
 * @brief Extract boolean value from JSON
 */
static int json_extract_bool(const char *json, const char *key, bool *out)
{
    char search[128];
    const char *start;

    snprintf(search, sizeof(search), "\"%s\"", key);
    start = strstr(json, search);
    if (start == NULL) return 0;

    start = strchr(start, ':');
    if (start == NULL) return 0;
    start++;

    while (*start == ' ' || *start == '\t' || *start == '\n') start++;

    if (strncmp(start, "true", 4) == 0) {
        *out = true;
        return 1;
    } else if (strncmp(start, "false", 5) == 0) {
        *out = false;
        return 1;
    }
    return 0;
}

/** Maximum JSON file size for loading (64KB) */
#define CB_MAX_JSON_SIZE 65536

/**
 * @brief Load results from JSON file
 * @satisfies REPORT-F-030 through REPORT-F-035
 */
cb_result_code_t cb_load_json(const char *path, cb_result_t *result)
{
    FILE *fp;
    static char json[CB_MAX_JSON_SIZE];  /* Static buffer - not thread-safe */
    long file_size;
    size_t bytes_read;
    char hash_hex[65];

    if (path == NULL || result == NULL) {
        return CB_ERR_NULL_PTR;
    }

    /* Clear result */
    memset(result, 0, sizeof(*result));

    /* Read file */
    fp = fopen(path, "r");
    if (fp == NULL) {
        return CB_ERR_IO;
    }

    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (file_size <= 0 || file_size >= CB_MAX_JSON_SIZE) {
        fclose(fp);
        return CB_ERR_IO;
    }

    bytes_read = fread(json, 1, (size_t)file_size, fp);
    fclose(fp);

    if (bytes_read != (size_t)file_size) {
        return CB_ERR_IO;
    }
    json[file_size] = '\0';

    /* REPORT-F-035: Required fields */
    if (!json_extract_string(json, "platform", result->platform, sizeof(result->platform))) {
        return CB_ERR_INVALID_CONFIG;
    }

    /* REPORT-F-034: Optional fields - extract what's available */
    json_extract_string(json, "cpu_model", result->cpu_model, sizeof(result->cpu_model));
    json_extract_u32(json, "cpu_freq_mhz", &result->cpu_freq_mhz);

    /* Config */
    json_extract_u32(json, "warmup_iterations", &result->warmup_iterations);
    json_extract_u32(json, "measure_iterations", &result->measure_iterations);
    json_extract_u32(json, "batch_size", &result->batch_size);

    /* Latency - required */
    if (!json_extract_u64(json, "min_ns", &result->latency.min_ns)) {
        return CB_ERR_INVALID_CONFIG;
    }
    json_extract_u64(json, "max_ns", &result->latency.max_ns);
    json_extract_u64(json, "mean_ns", &result->latency.mean_ns);
    json_extract_u64(json, "median_ns", &result->latency.median_ns);
    json_extract_u64(json, "p95_ns", &result->latency.p95_ns);
    json_extract_u64(json, "p99_ns", &result->latency.p99_ns);
    json_extract_u64(json, "stddev_ns", &result->latency.stddev_ns);
    json_extract_u64(json, "variance_ns2", &result->latency.variance_ns2);
    json_extract_u32(json, "sample_count", &result->latency.sample_count);
    json_extract_u32(json, "outlier_count", &result->latency.outlier_count);
    json_extract_u64(json, "wcet_observed_ns", &result->latency.wcet_observed_ns);
    json_extract_u64(json, "wcet_bound_ns", &result->latency.wcet_bound_ns);

    /* Throughput */
    json_extract_u64(json, "inferences_per_sec", &result->throughput.inferences_per_sec);
    json_extract_u64(json, "samples_per_sec", &result->throughput.samples_per_sec);
    json_extract_u64(json, "bytes_per_sec", &result->throughput.bytes_per_sec);
    if (result->throughput.batch_size == 0) {
        json_extract_u32(json, "batch_size", &result->throughput.batch_size);
    }

    /* Verification */
    json_extract_bool(json, "determinism_verified", &result->determinism_verified);
    json_extract_u32(json, "verification_failures", &result->verification_failures);
    if (json_extract_string(json, "output_hash", hash_hex, sizeof(hash_hex))) {
        cb_hash_from_hex(hash_hex, &result->output_hash);
    }
    if (json_extract_string(json, "result_hash", hash_hex, sizeof(hash_hex))) {
        cb_hash_from_hex(hash_hex, &result->result_hash);
    }

    /* Environment */
    json_extract_bool(json, "stable", &result->env_stable);
    json_extract_u64(json, "start_freq_hz", &result->environment.start.cpu_freq_hz);
    json_extract_u64(json, "end_freq_hz", &result->environment.end.cpu_freq_hz);
    json_extract_u64(json, "min_freq_hz", &result->environment.min_freq_hz);
    json_extract_u64(json, "max_freq_hz", &result->environment.max_freq_hz);
    json_extract_i32(json, "start_temp_mC", &result->environment.start.cpu_temp_mC);
    json_extract_i32(json, "end_temp_mC", &result->environment.end.cpu_temp_mC);
    json_extract_i32(json, "min_temp_mC", &result->environment.min_temp_mC);
    json_extract_i32(json, "max_temp_mC", &result->environment.max_temp_mC);
    json_extract_u32(json, "throttle_events", &result->environment.total_throttle_events);

    /* Faults - use temp bool for bit-fields */
    {
        bool tmp;
        if (json_extract_bool(json, "overflow", &tmp)) result->faults.overflow = tmp ? 1 : 0;
        if (json_extract_bool(json, "underflow", &tmp)) result->faults.underflow = tmp ? 1 : 0;
        if (json_extract_bool(json, "div_zero", &tmp)) result->faults.div_zero = tmp ? 1 : 0;
        if (json_extract_bool(json, "timer_error", &tmp)) result->faults.timer_error = tmp ? 1 : 0;
        if (json_extract_bool(json, "verify_fail", &tmp)) result->faults.verify_fail = tmp ? 1 : 0;
        if (json_extract_bool(json, "thermal_drift", &tmp)) result->faults.thermal_drift = tmp ? 1 : 0;
    }

    /* Timing metadata */
    json_extract_u64(json, "benchmark_start_ns", &result->benchmark_start_ns);
    json_extract_u64(json, "benchmark_end_ns", &result->benchmark_end_ns);
    json_extract_u64(json, "benchmark_duration_ns", &result->benchmark_duration_ns);
    json_extract_u64(json, "timestamp_unix", &result->timestamp_unix);

    return CB_OK;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Cross-Platform Comparison (REPORT-F-040..049)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Compare two benchmark results
 * @satisfies REPORT-F-040 through REPORT-F-049
 */
cb_result_code_t cb_compare_results(const cb_result_t *result_a,
                                     const cb_result_t *result_b,
                                     cb_comparison_t *comparison)
{
    if (result_a == NULL || result_b == NULL || comparison == NULL) {
        return CB_ERR_NULL_PTR;
    }

    memset(comparison, 0, sizeof(*comparison));

    /* Copy platform identifiers - ensure null termination */
    memset(comparison->platform_a, 0, CB_MAX_PLATFORM);
    memset(comparison->platform_b, 0, CB_MAX_PLATFORM);
    snprintf(comparison->platform_a, CB_MAX_PLATFORM, "%s", result_a->platform);
    snprintf(comparison->platform_b, CB_MAX_PLATFORM, "%s", result_b->platform);

    /* REPORT-F-041, REPORT-F-042: Check output hash equality */
    comparison->outputs_identical = cb_hash_equal(&result_a->output_hash,
                                                   &result_b->output_hash);

    /* REPORT-F-043: Comparable only if outputs identical */
    comparison->comparable = comparison->outputs_identical;

    /* REPORT-F-044: If not comparable, zero performance fields (already zeroed) */
    if (!comparison->comparable) {
        return CB_OK;
    }

    /* REPORT-F-045: latency_diff_ns = B.p99 - A.p99 */
    comparison->latency_diff_ns = (int64_t)result_b->latency.p99_ns -
                                  (int64_t)result_a->latency.p99_ns;

    /* REPORT-F-046: latency_ratio_q16 = (B.p99 << 16) / A.p99 */
    /* REPORT-F-049: Division by zero results in ratio = 0 */
    if (result_a->latency.p99_ns > 0) {
        comparison->latency_ratio_q16 = (uint32_t)(
            (result_b->latency.p99_ns << 16) / result_a->latency.p99_ns
        );
    } else {
        comparison->latency_ratio_q16 = 0;
    }

    /* REPORT-F-047: throughput_diff = B - A */
    comparison->throughput_diff = (int64_t)result_b->throughput.inferences_per_sec -
                                  (int64_t)result_a->throughput.inferences_per_sec;

    /* REPORT-F-048: throughput_ratio_q16 */
    if (result_a->throughput.inferences_per_sec > 0) {
        comparison->throughput_ratio_q16 = (uint32_t)(
            (result_b->throughput.inferences_per_sec << 16) /
            result_a->throughput.inferences_per_sec
        );
    } else {
        comparison->throughput_ratio_q16 = 0;
    }

    /* WCET comparison */
    comparison->wcet_diff_ns = (int64_t)result_b->latency.wcet_bound_ns -
                               (int64_t)result_a->latency.wcet_bound_ns;
    if (result_a->latency.wcet_bound_ns > 0) {
        comparison->wcet_ratio_q16 = (uint32_t)(
            (result_b->latency.wcet_bound_ns << 16) / result_a->latency.wcet_bound_ns
        );
    } else {
        comparison->wcet_ratio_q16 = 0;
    }

    return CB_OK;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Human-Readable Output (REPORT-F-050..058)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Format number with thousand separators
 */
static void format_number(char *buf, size_t size, uint64_t value)
{
    char tmp[32];
    int len, i, j;

    snprintf(tmp, sizeof(tmp), "%" PRIu64, value);
    len = (int)strlen(tmp);

    j = 0;
    for (i = 0; i < len; i++) {
        if (i > 0 && (len - i) % 3 == 0) {
            if ((size_t)j < size - 1) buf[j++] = ',';
        }
        if ((size_t)j < size - 1) buf[j++] = tmp[i];
    }
    buf[j] = '\0';
}

/**
 * @brief Print result summary to stream
 * @satisfies REPORT-F-050 through REPORT-F-055
 */
void cb_print_summary_fp(const cb_result_t *result, FILE *fp)
{
    char buf[32];
    char hash_hex[65];

    if (result == NULL || fp == NULL) return;

    fprintf(fp, "══════════════════════════════════════════════════════════════════\n");
    fprintf(fp, "  Benchmark Summary\n");
    fprintf(fp, "══════════════════════════════════════════════════════════════════\n\n");

    /* REPORT-F-051: Platform and CPU model */
    fprintf(fp, "Platform:    %s\n", result->platform);
    fprintf(fp, "CPU Model:   %s\n", result->cpu_model);
    fprintf(fp, "CPU Freq:    %u MHz\n", result->cpu_freq_mhz);
    fprintf(fp, "Iterations:  %u warmup, %u measure (batch=%u)\n\n",
            result->warmup_iterations, result->measure_iterations, result->batch_size);

    /* REPORT-F-052: Latency metrics */
    fprintf(fp, "Latency:\n");
    format_number(buf, sizeof(buf), result->latency.min_ns);
    fprintf(fp, "  Min:       %s ns\n", buf);
    format_number(buf, sizeof(buf), result->latency.max_ns);
    fprintf(fp, "  Max:       %s ns\n", buf);
    format_number(buf, sizeof(buf), result->latency.mean_ns);
    fprintf(fp, "  Mean:      %s ns\n", buf);
    format_number(buf, sizeof(buf), result->latency.median_ns);
    fprintf(fp, "  Median:    %s ns\n", buf);
    format_number(buf, sizeof(buf), result->latency.p95_ns);
    fprintf(fp, "  P95:       %s ns\n", buf);
    format_number(buf, sizeof(buf), result->latency.p99_ns);
    fprintf(fp, "  P99:       %s ns\n", buf);
    format_number(buf, sizeof(buf), result->latency.stddev_ns);
    fprintf(fp, "  StdDev:    %s ns\n", buf);
    format_number(buf, sizeof(buf), result->latency.wcet_bound_ns);
    fprintf(fp, "  WCET Bound: %s ns\n\n", buf);

    /* REPORT-F-053: Throughput */
    fprintf(fp, "Throughput:\n");
    format_number(buf, sizeof(buf), result->throughput.inferences_per_sec);
    fprintf(fp, "  Inferences/sec:  %s\n", buf);
    format_number(buf, sizeof(buf), result->throughput.samples_per_sec);
    fprintf(fp, "  Samples/sec:     %s\n\n", buf);

    /* REPORT-F-054: Verification status */
    fprintf(fp, "Verification:\n");
    fprintf(fp, "  Determinism:     %s\n", result->determinism_verified ? "VERIFIED" : "FAILED");
    fprintf(fp, "  Failures:        %u\n", result->verification_failures);
    cb_hash_to_hex(&result->output_hash, hash_hex);
    fprintf(fp, "  Output Hash:     %s\n\n", hash_hex);

    /* REPORT-F-055: Fault flags */
    fprintf(fp, "Faults:\n");
    if (cb_has_fault(&result->faults)) {
        if (result->faults.overflow)    fprintf(fp, "  - OVERFLOW\n");
        if (result->faults.underflow)   fprintf(fp, "  - UNDERFLOW\n");
        if (result->faults.div_zero)    fprintf(fp, "  - DIV_ZERO\n");
        if (result->faults.timer_error) fprintf(fp, "  - TIMER_ERROR\n");
        if (result->faults.verify_fail) fprintf(fp, "  - VERIFY_FAIL\n");
    } else {
        fprintf(fp, "  None\n");
    }
    if (result->faults.thermal_drift) {
        fprintf(fp, "  Warning: THERMAL_DRIFT\n");
    }

    fprintf(fp, "\nEnvironment:\n");
    fprintf(fp, "  Stable:          %s\n", result->env_stable ? "Yes" : "No");
    format_number(buf, sizeof(buf), result->benchmark_duration_ns / 1000000);
    fprintf(fp, "  Duration:        %s ms\n", buf);

    fprintf(fp, "══════════════════════════════════════════════════════════════════\n");
}

/**
 * @brief Print result summary to stdout
 */
void cb_print_summary(const cb_result_t *result)
{
    cb_print_summary_fp(result, stdout);
}

/**
 * @brief Print comparison summary to stream
 * @satisfies REPORT-F-056 through REPORT-F-058
 */
void cb_print_comparison_fp(const cb_comparison_t *comparison, FILE *fp)
{
    char buf_diff[32];
    uint32_t ratio_int, ratio_frac;

    if (comparison == NULL || fp == NULL) return;

    fprintf(fp, "══════════════════════════════════════════════════════════════════\n");
    fprintf(fp, "  Cross-Platform Performance Comparison\n");
    fprintf(fp, "  Reference: %-12s  Target: %s\n", comparison->platform_a, comparison->platform_b);
    fprintf(fp, "══════════════════════════════════════════════════════════════════\n\n");

    /* REPORT-F-057: Bit identity status */
    if (comparison->outputs_identical) {
        fprintf(fp, "Bit Identity:  VERIFIED (outputs identical)\n\n");
    } else {
        fprintf(fp, "Bit Identity:  FAILED (outputs differ)\n");
        fprintf(fp, "\n  *** Performance comparison not meaningful ***\n\n");
        fprintf(fp, "══════════════════════════════════════════════════════════════════\n");
        return;
    }

    /* REPORT-F-058: Latency and throughput ratios */
    if (!comparison->comparable) {
        fprintf(fp, "  Results not comparable.\n");
        fprintf(fp, "══════════════════════════════════════════════════════════════════\n");
        return;
    }

    /* Latency comparison (we'd need result values, but we have diff/ratio) */
    fprintf(fp, "Latency (p99):\n");
    format_number(buf_diff, sizeof(buf_diff),
                  comparison->latency_diff_ns >= 0 ?
                  (uint64_t)comparison->latency_diff_ns :
                  (uint64_t)(-comparison->latency_diff_ns));
    fprintf(fp, "  Diff:    %s%s ns\n",
            comparison->latency_diff_ns >= 0 ? "+" : "-", buf_diff);

    ratio_int = comparison->latency_ratio_q16 >> 16;
    ratio_frac = ((comparison->latency_ratio_q16 & 0xFFFF) * 100) >> 16;
    fprintf(fp, "  Ratio:   %u.%02ux", ratio_int, ratio_frac);
    if (comparison->latency_diff_ns > 0) {
        fprintf(fp, " slower\n");
    } else if (comparison->latency_diff_ns < 0) {
        fprintf(fp, " faster\n");
    } else {
        fprintf(fp, " (equal)\n");
    }

    fprintf(fp, "\nThroughput:\n");
    format_number(buf_diff, sizeof(buf_diff),
                  comparison->throughput_diff >= 0 ?
                  (uint64_t)comparison->throughput_diff :
                  (uint64_t)(-comparison->throughput_diff));
    fprintf(fp, "  Diff:    %s%s inferences/sec\n",
            comparison->throughput_diff >= 0 ? "+" : "-", buf_diff);

    ratio_int = comparison->throughput_ratio_q16 >> 16;
    ratio_frac = ((comparison->throughput_ratio_q16 & 0xFFFF) * 100) >> 16;
    fprintf(fp, "  Ratio:   %u.%02ux\n", ratio_int, ratio_frac);

    fprintf(fp, "\n══════════════════════════════════════════════════════════════════\n");
}

/**
 * @brief Print comparison summary to stdout
 */
void cb_print_comparison(const cb_comparison_t *comparison)
{
    cb_print_comparison_fp(comparison, stdout);
}
