/**
 * @file test_report.c
 * @brief Unit tests for cb_report API
 *
 * Tests all SRS-005-REPORT requirements including JSON/CSV output,
 * loading, and cross-platform comparison.
 *
 * @traceability SRS-005-REPORT §10
 *
 * Copyright © 2026 The Murray Family Innovation Trust. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cb_report.h"
#include "cb_verify.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
#define TEST_ASSERT_STR_EQ(a, b, msg) TEST_ASSERT(strcmp((a), (b)) == 0, msg)

#define RUN_TEST(fn) do { \
    printf("  %s\n", #fn); \
    int result = fn(); \
    if (result == 0) { \
        printf("    PASS\n"); \
    } \
} while(0)

/*─────────────────────────────────────────────────────────────────────────────
 * Test Helpers
 *─────────────────────────────────────────────────────────────────────────────*/

static void create_test_result(cb_result_t *result)
{
    memset(result, 0, sizeof(*result));

    strncpy(result->platform, "x86_64", CB_MAX_PLATFORM - 1);
    strncpy(result->cpu_model, "Test CPU @ 3.00GHz", CB_MAX_CPU_MODEL - 1);
    result->cpu_freq_mhz = 3000;

    result->warmup_iterations = 100;
    result->measure_iterations = 1000;
    result->batch_size = 1;

    result->latency.min_ns = 1000000;
    result->latency.max_ns = 5000000;
    result->latency.mean_ns = 2000000;
    result->latency.median_ns = 1900000;
    result->latency.p95_ns = 3500000;
    result->latency.p99_ns = 4200000;
    result->latency.stddev_ns = 500000;
    result->latency.variance_ns2 = 250000000000ULL;
    result->latency.sample_count = 1000;
    result->latency.outlier_count = 5;
    result->latency.wcet_observed_ns = 5000000;
    result->latency.wcet_bound_ns = 8000000;

    result->throughput.inferences_per_sec = 500;
    result->throughput.samples_per_sec = 500;
    result->throughput.bytes_per_sec = 2000000;
    result->throughput.batch_size = 1;

    result->determinism_verified = true;
    result->verification_failures = 0;
    cb_compute_hash("test output", 11, &result->output_hash);
    cb_compute_hash("test result", 11, &result->result_hash);

    result->env_stable = true;
    result->environment.start.cpu_freq_hz = 3000000000ULL;
    result->environment.end.cpu_freq_hz = 2950000000ULL;
    result->environment.min_freq_hz = 2900000000ULL;
    result->environment.max_freq_hz = 3000000000ULL;
    result->environment.start.cpu_temp_mC = 45000;
    result->environment.end.cpu_temp_mC = 52000;
    result->environment.min_temp_mC = 45000;
    result->environment.max_temp_mC = 55000;
    result->environment.total_throttle_events = 0;

    result->benchmark_start_ns = 1000000000000ULL;
    result->benchmark_end_ns = 1002000000000ULL;
    result->benchmark_duration_ns = 2000000000ULL;
    result->timestamp_unix = 1737500000ULL;

    cb_fault_clear(&result->faults);
}

static int file_contains(const char *path, const char *needle)
{
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;

    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, needle)) {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Test: JSON Output (REPORT-F-001..015)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Test cb_write_json creates file
 * @satisfies REPORT-F-001
 */
static int test_write_json_creates_file(void)
{
    cb_result_t result;
    cb_result_code_t rc;
    const char *path = "/tmp/cb_test_result.json";

    create_test_result(&result);

    rc = cb_write_json(&result, path);
    TEST_ASSERT_EQ(rc, CB_OK, "write_json should succeed");
    TEST_ASSERT(access(path, F_OK) == 0, "file should exist");

    unlink(path);
    return 0;
}

/**
 * @brief Test JSON includes version field
 * @satisfies REPORT-F-002
 */
static int test_json_has_version(void)
{
    cb_result_t result;
    const char *path = "/tmp/cb_test_result.json";

    create_test_result(&result);
    cb_write_json(&result, path);

    TEST_ASSERT(file_contains(path, "\"version\": \"1.0\""), "should have version");

    unlink(path);
    return 0;
}

/**
 * @brief Test JSON includes platform
 * @satisfies REPORT-F-003
 */
static int test_json_has_platform(void)
{
    cb_result_t result;
    const char *path = "/tmp/cb_test_result.json";

    create_test_result(&result);
    cb_write_json(&result, path);

    TEST_ASSERT(file_contains(path, "\"platform\": \"x86_64\""), "should have platform");

    unlink(path);
    return 0;
}

/**
 * @brief Test JSON includes CPU model
 * @satisfies REPORT-F-004
 */
static int test_json_has_cpu_model(void)
{
    cb_result_t result;
    const char *path = "/tmp/cb_test_result.json";

    create_test_result(&result);
    cb_write_json(&result, path);

    TEST_ASSERT(file_contains(path, "\"cpu_model\":"), "should have cpu_model");

    unlink(path);
    return 0;
}

/**
 * @brief Test JSON includes config section
 * @satisfies REPORT-F-005
 */
static int test_json_has_config(void)
{
    cb_result_t result;
    const char *path = "/tmp/cb_test_result.json";

    create_test_result(&result);
    cb_write_json(&result, path);

    TEST_ASSERT(file_contains(path, "\"config\":"), "should have config section");
    TEST_ASSERT(file_contains(path, "\"warmup_iterations\":"), "should have warmup");
    TEST_ASSERT(file_contains(path, "\"measure_iterations\":"), "should have measure");

    unlink(path);
    return 0;
}

/**
 * @brief Test JSON includes latency section
 * @satisfies REPORT-F-006
 */
static int test_json_has_latency(void)
{
    cb_result_t result;
    const char *path = "/tmp/cb_test_result.json";

    create_test_result(&result);
    cb_write_json(&result, path);

    TEST_ASSERT(file_contains(path, "\"latency\":"), "should have latency section");
    TEST_ASSERT(file_contains(path, "\"min_ns\":"), "should have min_ns");
    TEST_ASSERT(file_contains(path, "\"p99_ns\":"), "should have p99_ns");

    unlink(path);
    return 0;
}

/**
 * @brief Test JSON includes throughput
 * @satisfies REPORT-F-007
 */
static int test_json_has_throughput(void)
{
    cb_result_t result;
    const char *path = "/tmp/cb_test_result.json";

    create_test_result(&result);
    cb_write_json(&result, path);

    TEST_ASSERT(file_contains(path, "\"throughput\":"), "should have throughput");
    TEST_ASSERT(file_contains(path, "\"inferences_per_sec\":"), "should have inferences_per_sec");

    unlink(path);
    return 0;
}

/**
 * @brief Test JSON includes verification
 * @satisfies REPORT-F-008
 */
static int test_json_has_verification(void)
{
    cb_result_t result;
    const char *path = "/tmp/cb_test_result.json";

    create_test_result(&result);
    cb_write_json(&result, path);

    TEST_ASSERT(file_contains(path, "\"verification\":"), "should have verification");
    TEST_ASSERT(file_contains(path, "\"determinism_verified\":"), "should have determinism");
    TEST_ASSERT(file_contains(path, "\"output_hash\":"), "should have output_hash");

    unlink(path);
    return 0;
}

/**
 * @brief Test JSON includes faults
 * @satisfies REPORT-F-010
 */
static int test_json_has_faults(void)
{
    cb_result_t result;
    const char *path = "/tmp/cb_test_result.json";

    create_test_result(&result);
    cb_write_json(&result, path);

    TEST_ASSERT(file_contains(path, "\"faults\":"), "should have faults");
    TEST_ASSERT(file_contains(path, "\"overflow\":"), "should have overflow flag");

    unlink(path);
    return 0;
}

/**
 * @brief Test JSON includes timestamp
 * @satisfies REPORT-F-011
 */
static int test_json_has_timestamp(void)
{
    cb_result_t result;
    const char *path = "/tmp/cb_test_result.json";

    create_test_result(&result);
    cb_write_json(&result, path);

    TEST_ASSERT(file_contains(path, "\"timestamp_unix\":"), "should have timestamp");

    unlink(path);
    return 0;
}

/**
 * @brief Test write_json null handling
 * @satisfies REPORT-F-013
 */
static int test_write_json_null(void)
{
    cb_result_t result;
    cb_result_code_t rc;

    create_test_result(&result);

    rc = cb_write_json(NULL, "/tmp/test.json");
    TEST_ASSERT_EQ(rc, CB_ERR_NULL_PTR, "NULL result should fail");

    rc = cb_write_json(&result, NULL);
    TEST_ASSERT_EQ(rc, CB_ERR_NULL_PTR, "NULL path should fail");

    return 0;
}

/**
 * @brief Test write_json returns error on bad path
 * @satisfies REPORT-F-013
 */
static int test_write_json_io_error(void)
{
    cb_result_t result;
    cb_result_code_t rc;

    create_test_result(&result);

    rc = cb_write_json(&result, "/nonexistent/path/result.json");
    TEST_ASSERT_EQ(rc, CB_ERR_IO, "bad path should return IO error");

    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Test: CSV Output (REPORT-F-020..026)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Test cb_write_csv creates file with header
 * @satisfies REPORT-F-020, REPORT-F-021
 */
static int test_write_csv_creates_file(void)
{
    cb_result_t result;
    cb_result_code_t rc;
    const char *path = "/tmp/cb_test_result.csv";

    create_test_result(&result);

    rc = cb_write_csv(&result, path);
    TEST_ASSERT_EQ(rc, CB_OK, "write_csv should succeed");
    TEST_ASSERT(access(path, F_OK) == 0, "file should exist");
    TEST_ASSERT(file_contains(path, "platform,cpu_model"), "should have header");

    unlink(path);
    return 0;
}

/**
 * @brief Test CSV has data row
 * @satisfies REPORT-F-022
 */
static int test_csv_has_data_row(void)
{
    cb_result_t result;
    const char *path = "/tmp/cb_test_result.csv";

    create_test_result(&result);
    cb_write_csv(&result, path);

    TEST_ASSERT(file_contains(path, "x86_64,"), "should have platform in data");

    unlink(path);
    return 0;
}

/**
 * @brief Test CSV columns
 * @satisfies REPORT-F-023
 */
static int test_csv_columns(void)
{
    cb_result_t result;
    const char *path = "/tmp/cb_test_result.csv";

    create_test_result(&result);
    cb_write_csv(&result, path);

    /* Check header has expected columns */
    TEST_ASSERT(file_contains(path, "min_ns"), "should have min_ns column");
    TEST_ASSERT(file_contains(path, "p99_ns"), "should have p99_ns column");
    TEST_ASSERT(file_contains(path, "inferences_per_sec"), "should have throughput column");
    TEST_ASSERT(file_contains(path, "output_hash"), "should have hash column");

    unlink(path);
    return 0;
}

/**
 * @brief Test write_csv null handling
 * @satisfies REPORT-F-026
 */
static int test_write_csv_null(void)
{
    cb_result_t result;
    cb_result_code_t rc;

    create_test_result(&result);

    rc = cb_write_csv(NULL, "/tmp/test.csv");
    TEST_ASSERT_EQ(rc, CB_ERR_NULL_PTR, "NULL result should fail");

    rc = cb_write_csv(&result, NULL);
    TEST_ASSERT_EQ(rc, CB_ERR_NULL_PTR, "NULL path should fail");

    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Test: JSON Loading (REPORT-F-030..035)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Test JSON roundtrip (write then load)
 * @satisfies REPORT-F-030, REPORT-F-031
 */
static int test_json_roundtrip(void)
{
    cb_result_t original, loaded;
    cb_result_code_t rc;
    const char *path = "/tmp/cb_test_roundtrip.json";

    create_test_result(&original);
    cb_write_json(&original, path);

    rc = cb_load_json(path, &loaded);
    TEST_ASSERT_EQ(rc, CB_OK, "load_json should succeed");

    /* Verify key fields */
    TEST_ASSERT_STR_EQ(loaded.platform, original.platform, "platform should match");
    TEST_ASSERT_EQ(loaded.latency.min_ns, original.latency.min_ns, "min_ns should match");
    TEST_ASSERT_EQ(loaded.latency.p99_ns, original.latency.p99_ns, "p99_ns should match");
    TEST_ASSERT_EQ(loaded.throughput.inferences_per_sec,
                   original.throughput.inferences_per_sec, "throughput should match");
    TEST_ASSERT_EQ(loaded.timestamp_unix, original.timestamp_unix, "timestamp should match");
    TEST_ASSERT(cb_hash_equal(&loaded.output_hash, &original.output_hash), "hash should match");

    unlink(path);
    return 0;
}

/**
 * @brief Test load_json with missing file
 * @satisfies REPORT-F-032
 */
static int test_load_json_missing_file(void)
{
    cb_result_t result;
    cb_result_code_t rc;

    rc = cb_load_json("/nonexistent/path/result.json", &result);
    TEST_ASSERT_EQ(rc, CB_ERR_IO, "missing file should return IO error");

    return 0;
}

/**
 * @brief Test load_json null handling
 */
static int test_load_json_null(void)
{
    cb_result_t result;
    cb_result_code_t rc;

    rc = cb_load_json(NULL, &result);
    TEST_ASSERT_EQ(rc, CB_ERR_NULL_PTR, "NULL path should fail");

    rc = cb_load_json("/tmp/test.json", NULL);
    TEST_ASSERT_EQ(rc, CB_ERR_NULL_PTR, "NULL result should fail");

    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Test: Comparison (REPORT-F-040..049)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Test compare_results with identical results
 * @satisfies REPORT-F-040, REPORT-F-041, REPORT-F-042, REPORT-F-043
 */
static int test_compare_identical(void)
{
    cb_result_t result_a, result_b;
    cb_comparison_t comparison;
    cb_result_code_t rc;

    create_test_result(&result_a);
    create_test_result(&result_b);

    rc = cb_compare_results(&result_a, &result_b, &comparison);
    TEST_ASSERT_EQ(rc, CB_OK, "compare should succeed");

    /* REPORT-F-042: outputs_identical should be true */
    TEST_ASSERT(comparison.outputs_identical, "outputs should be identical");

    /* REPORT-F-043: comparable should be true */
    TEST_ASSERT(comparison.comparable, "should be comparable");

    return 0;
}

/**
 * @brief Test compare_results with different outputs
 * @satisfies REPORT-F-044
 */
static int test_compare_different_outputs(void)
{
    cb_result_t result_a, result_b;
    cb_comparison_t comparison;
    cb_result_code_t rc;

    create_test_result(&result_a);
    create_test_result(&result_b);

    /* Change output hash */
    cb_compute_hash("different output", 16, &result_b.output_hash);

    rc = cb_compare_results(&result_a, &result_b, &comparison);
    TEST_ASSERT_EQ(rc, CB_OK, "compare should succeed");

    TEST_ASSERT(!comparison.outputs_identical, "outputs should differ");
    TEST_ASSERT(!comparison.comparable, "should not be comparable");

    /* REPORT-F-044: Performance fields should be zeroed */
    TEST_ASSERT_EQ(comparison.latency_diff_ns, 0, "latency_diff should be 0");
    TEST_ASSERT_EQ(comparison.latency_ratio_q16, 0, "latency_ratio should be 0");

    return 0;
}

/**
 * @brief Test latency_diff calculation
 * @satisfies REPORT-F-045
 */
static int test_compare_latency_diff(void)
{
    cb_result_t result_a, result_b;
    cb_comparison_t comparison;

    create_test_result(&result_a);
    create_test_result(&result_b);

    result_a.latency.p99_ns = 1000000;  /* 1ms */
    result_b.latency.p99_ns = 1500000;  /* 1.5ms */

    cb_compare_results(&result_a, &result_b, &comparison);

    /* REPORT-F-045: diff = B.p99 - A.p99 = 500000 */
    TEST_ASSERT_EQ(comparison.latency_diff_ns, 500000, "latency diff should be 500000");

    return 0;
}

/**
 * @brief Test latency_ratio calculation
 * @satisfies REPORT-F-046
 */
static int test_compare_latency_ratio(void)
{
    cb_result_t result_a, result_b;
    cb_comparison_t comparison;

    create_test_result(&result_a);
    create_test_result(&result_b);

    result_a.latency.p99_ns = 1000000;  /* 1ms */
    result_b.latency.p99_ns = 2000000;  /* 2ms */

    cb_compare_results(&result_a, &result_b, &comparison);

    /* REPORT-F-046: ratio = (2000000 << 16) / 1000000 = 2.0 in Q16.16 = 131072 */
    TEST_ASSERT_EQ(comparison.latency_ratio_q16, 131072, "latency ratio should be 2.0 (Q16.16)");

    return 0;
}

/**
 * @brief Test throughput_diff calculation
 * @satisfies REPORT-F-047
 */
static int test_compare_throughput_diff(void)
{
    cb_result_t result_a, result_b;
    cb_comparison_t comparison;

    create_test_result(&result_a);
    create_test_result(&result_b);

    result_a.throughput.inferences_per_sec = 1000;
    result_b.throughput.inferences_per_sec = 800;

    cb_compare_results(&result_a, &result_b, &comparison);

    /* REPORT-F-047: diff = B - A = -200 */
    TEST_ASSERT_EQ(comparison.throughput_diff, -200, "throughput diff should be -200");

    return 0;
}

/**
 * @brief Test division by zero handling
 * @satisfies REPORT-F-049
 */
static int test_compare_div_zero(void)
{
    cb_result_t result_a, result_b;
    cb_comparison_t comparison;

    create_test_result(&result_a);
    create_test_result(&result_b);

    result_a.latency.p99_ns = 0;  /* Division by zero */
    result_b.latency.p99_ns = 1000000;

    cb_compare_results(&result_a, &result_b, &comparison);

    /* REPORT-F-049: ratio should be 0 on div by zero */
    TEST_ASSERT_EQ(comparison.latency_ratio_q16, 0, "ratio should be 0 on div by zero");

    return 0;
}

/**
 * @brief Test compare_results null handling
 */
static int test_compare_null(void)
{
    cb_result_t result;
    cb_comparison_t comparison;
    cb_result_code_t rc;

    create_test_result(&result);

    rc = cb_compare_results(NULL, &result, &comparison);
    TEST_ASSERT_EQ(rc, CB_ERR_NULL_PTR, "NULL result_a should fail");

    rc = cb_compare_results(&result, NULL, &comparison);
    TEST_ASSERT_EQ(rc, CB_ERR_NULL_PTR, "NULL result_b should fail");

    rc = cb_compare_results(&result, &result, NULL);
    TEST_ASSERT_EQ(rc, CB_ERR_NULL_PTR, "NULL comparison should fail");

    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Test: Human-Readable Output (REPORT-F-050..058)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Test print_summary runs without crash
 * @satisfies REPORT-F-050
 */
static int test_print_summary_no_crash(void)
{
    cb_result_t result;
    FILE *fp;

    create_test_result(&result);

    fp = fopen("/tmp/cb_test_summary.txt", "w");
    TEST_ASSERT(fp != NULL, "should open file");

    cb_print_summary_fp(&result, fp);
    fclose(fp);

    /* Check output contains expected sections */
    TEST_ASSERT(file_contains("/tmp/cb_test_summary.txt", "Platform:"), "should have platform");
    TEST_ASSERT(file_contains("/tmp/cb_test_summary.txt", "Latency:"), "should have latency");
    TEST_ASSERT(file_contains("/tmp/cb_test_summary.txt", "Throughput:"), "should have throughput");

    unlink("/tmp/cb_test_summary.txt");
    return 0;
}

/**
 * @brief Test print_comparison runs without crash
 * @satisfies REPORT-F-056
 */
static int test_print_comparison_no_crash(void)
{
    cb_result_t result_a, result_b;
    cb_comparison_t comparison;
    FILE *fp;

    create_test_result(&result_a);
    create_test_result(&result_b);
    cb_compare_results(&result_a, &result_b, &comparison);

    fp = fopen("/tmp/cb_test_comparison.txt", "w");
    TEST_ASSERT(fp != NULL, "should open file");

    cb_print_comparison_fp(&comparison, fp);
    fclose(fp);

    /* Check output contains expected sections */
    TEST_ASSERT(file_contains("/tmp/cb_test_comparison.txt", "Bit Identity:"),
                "should have bit identity");

    unlink("/tmp/cb_test_comparison.txt");
    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Test: Determinism (REPORT-NF-001)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Test JSON output is deterministic
 * @satisfies REPORT-NF-001
 */
static int test_json_determinism(void)
{
    cb_result_t result;
    FILE *fp1, *fp2;
    char buf1[4096], buf2[4096];
    size_t len1, len2;

    create_test_result(&result);

    /* Write twice */
    cb_write_json(&result, "/tmp/cb_test_det1.json");
    cb_write_json(&result, "/tmp/cb_test_det2.json");

    /* Read both */
    fp1 = fopen("/tmp/cb_test_det1.json", "r");
    fp2 = fopen("/tmp/cb_test_det2.json", "r");
    TEST_ASSERT(fp1 && fp2, "should open both files");

    len1 = fread(buf1, 1, sizeof(buf1), fp1);
    len2 = fread(buf2, 1, sizeof(buf2), fp2);
    fclose(fp1);
    fclose(fp2);

    /* Compare */
    TEST_ASSERT_EQ(len1, len2, "file lengths should match");
    TEST_ASSERT(memcmp(buf1, buf2, len1) == 0, "file contents should be identical");

    unlink("/tmp/cb_test_det1.json");
    unlink("/tmp/cb_test_det2.json");
    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Main
 *─────────────────────────────────────────────────────────────────────────────*/

int main(void)
{
    printf("═══════════════════════════════════════════════════════════════════\n");
    printf(" certifiable-bench Report Tests\n");
    printf(" @traceability SRS-005-REPORT\n");
    printf("═══════════════════════════════════════════════════════════════════\n\n");

    printf("§4.1 JSON Output (REPORT-F-001..015)\n");
    RUN_TEST(test_write_json_creates_file);
    RUN_TEST(test_json_has_version);
    RUN_TEST(test_json_has_platform);
    RUN_TEST(test_json_has_cpu_model);
    RUN_TEST(test_json_has_config);
    RUN_TEST(test_json_has_latency);
    RUN_TEST(test_json_has_throughput);
    RUN_TEST(test_json_has_verification);
    RUN_TEST(test_json_has_faults);
    RUN_TEST(test_json_has_timestamp);
    RUN_TEST(test_write_json_null);
    RUN_TEST(test_write_json_io_error);

    printf("\n§4.2 CSV Output (REPORT-F-020..026)\n");
    RUN_TEST(test_write_csv_creates_file);
    RUN_TEST(test_csv_has_data_row);
    RUN_TEST(test_csv_columns);
    RUN_TEST(test_write_csv_null);

    printf("\n§4.3 JSON Loading (REPORT-F-030..035)\n");
    RUN_TEST(test_json_roundtrip);
    RUN_TEST(test_load_json_missing_file);
    RUN_TEST(test_load_json_null);

    printf("\n§4.4 Comparison (REPORT-F-040..049)\n");
    RUN_TEST(test_compare_identical);
    RUN_TEST(test_compare_different_outputs);
    RUN_TEST(test_compare_latency_diff);
    RUN_TEST(test_compare_latency_ratio);
    RUN_TEST(test_compare_throughput_diff);
    RUN_TEST(test_compare_div_zero);
    RUN_TEST(test_compare_null);

    printf("\n§4.5 Human-Readable Output (REPORT-F-050..058)\n");
    RUN_TEST(test_print_summary_no_crash);
    RUN_TEST(test_print_comparison_no_crash);

    printf("\n§5.1 Determinism (REPORT-NF-001)\n");
    RUN_TEST(test_json_determinism);

    printf("\n═══════════════════════════════════════════════════════════════════\n");
    printf(" Results: %d/%d tests passed\n", g_tests_passed, g_tests_run);
    printf("═══════════════════════════════════════════════════════════════════\n");

    return (g_tests_failed > 0) ? 1 : 0;
}
