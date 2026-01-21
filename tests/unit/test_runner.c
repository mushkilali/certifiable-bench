/**
 * @file test_runner.c
 * @brief Unit tests for cb_runner API
 *
 * Tests all SRS-003-RUNNER requirements including configuration,
 * warmup, execution, and result generation.
 *
 * @traceability SRS-003-RUNNER §10
 *
 * Copyright © 2026 The Murray Family Innovation Trust. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cb_runner.h"
#include "cb_timer.h"
#include "cb_metrics.h"
#include "cb_verify.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*─────────────────────────────────────────────────────────────────────────────
 * Test Infrastructure
 *─────────────────────────────────────────────────────────────────────────────*/

static int g_tests_run = 0;
static int g_tests_passed = 0;
static int g_tests_failed = 0;

/* Shared sample buffer for tests */
#define TEST_SAMPLE_CAPACITY 1000
static uint64_t g_sample_buffer[TEST_SAMPLE_CAPACITY];

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

#define RUN_TEST(fn) do { \
    printf("  %s\n", #fn); \
    int result = fn(); \
    if (result == 0) { \
        printf("    PASS\n"); \
    } \
} while(0)

/*─────────────────────────────────────────────────────────────────────────────
 * Mock Inference Functions
 *─────────────────────────────────────────────────────────────────────────────*/

static uint32_t g_inference_count = 0;

static cb_result_code_t mock_inference_copy(void *ctx, const void *input, void *output)
{
    (void)ctx;
    if (input != NULL && output != NULL) {
        memcpy(output, input, 64);
    }
    g_inference_count++;
    return CB_OK;
}

static cb_result_code_t mock_inference_work(void *ctx, const void *input, void *output)
{
    volatile uint64_t sum = 0;
    uint64_t result_val;
    int i;
    (void)ctx;
    (void)input;
    for (i = 0; i < 1000; i++) {
        sum += (uint64_t)i * (uint64_t)i;
    }
    result_val = sum;  /* Copy out of volatile */
    if (output != NULL) {
        memcpy(output, &result_val, sizeof(result_val));
    }
    g_inference_count++;
    return CB_OK;
}

static cb_result_code_t mock_inference_fail(void *ctx, const void *input, void *output)
{
    (void)ctx; (void)input; (void)output;
    g_inference_count++;
    return CB_ERR_VERIFICATION;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Test: Configuration (RUNNER-F-001..009)
 *─────────────────────────────────────────────────────────────────────────────*/

static int test_config_defaults(void)
{
    cb_config_t config;
    cb_result_code_t rc = cb_config_init(&config);
    TEST_ASSERT_EQ(rc, CB_OK, "config_init should succeed");
    TEST_ASSERT_EQ(config.warmup_iterations, 100, "warmup default = 100");
    TEST_ASSERT_EQ(config.measure_iterations, 1000, "measure default = 1000");
    TEST_ASSERT_EQ(config.batch_size, 1, "batch_size default = 1");
    TEST_ASSERT_EQ(config.timer_source, CB_TIMER_AUTO, "timer default = AUTO");
    TEST_ASSERT(config.verify_outputs, "verify_outputs default = true");
    TEST_ASSERT(config.monitor_environment, "monitor_environment default = true");
    return 0;
}

static int test_config_init_null(void)
{
    TEST_ASSERT_EQ(cb_config_init(NULL), CB_ERR_NULL_PTR, "init(NULL) fails");
    return 0;
}

static int test_config_validate_valid(void)
{
    cb_config_t config;
    cb_config_init(&config);
    TEST_ASSERT_EQ(cb_config_validate(&config), CB_OK, "default config valid");
    return 0;
}

static int test_config_validate_zero_iterations(void)
{
    cb_config_t config;
    cb_config_init(&config);
    config.measure_iterations = 0;
    TEST_ASSERT_EQ(cb_config_validate(&config), CB_ERR_INVALID_CONFIG, "zero iterations invalid");
    return 0;
}

static int test_config_validate_zero_batch(void)
{
    cb_config_t config;
    cb_config_init(&config);
    config.batch_size = 0;
    TEST_ASSERT_EQ(cb_config_validate(&config), CB_ERR_INVALID_CONFIG, "zero batch invalid");
    return 0;
}

static int test_config_validate_too_many(void)
{
    cb_config_t config;
    cb_config_init(&config);
    config.measure_iterations = CB_MAX_SAMPLES + 1;
    TEST_ASSERT_EQ(cb_config_validate(&config), CB_ERR_INVALID_CONFIG, "too many iterations invalid");
    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Test: Runner Init (RUNNER-F-014, RUNNER-NF-010..012)
 *─────────────────────────────────────────────────────────────────────────────*/

static int test_runner_init_valid(void)
{
    cb_runner_t runner;
    cb_config_t config;
    cb_config_init(&config);
    config.measure_iterations = 100;
    cb_result_code_t rc = cb_runner_init(&runner, &config, g_sample_buffer, TEST_SAMPLE_CAPACITY);
    TEST_ASSERT_EQ(rc, CB_OK, "runner_init succeeds");
    TEST_ASSERT(runner.initialised, "runner initialised");
    TEST_ASSERT(runner.samples != NULL, "samples set");
    TEST_ASSERT_EQ(runner.sample_capacity, TEST_SAMPLE_CAPACITY, "capacity matches");
    cb_runner_cleanup(&runner);
    return 0;
}

static int test_runner_init_null(void)
{
    cb_runner_t runner;
    cb_config_t config;
    cb_config_init(&config);
    TEST_ASSERT_EQ(cb_runner_init(NULL, &config, g_sample_buffer, TEST_SAMPLE_CAPACITY), CB_ERR_NULL_PTR, "NULL runner fails");
    TEST_ASSERT_EQ(cb_runner_init(&runner, NULL, g_sample_buffer, TEST_SAMPLE_CAPACITY), CB_ERR_NULL_PTR, "NULL config fails");
    TEST_ASSERT_EQ(cb_runner_init(&runner, &config, NULL, TEST_SAMPLE_CAPACITY), CB_ERR_NULL_PTR, "NULL buffer fails");
    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Test: Warmup (RUNNER-F-011, RUNNER-F-030..033)
 *─────────────────────────────────────────────────────────────────────────────*/

static int test_warmup_count(void)
{
    cb_runner_t runner;
    cb_config_t config;
    uint8_t input[64], output[64];
    cb_config_init(&config);
    config.warmup_iterations = 50;
    config.measure_iterations = 10;
    cb_runner_init(&runner, &config, g_sample_buffer, TEST_SAMPLE_CAPACITY);
    g_inference_count = 0;
    cb_result_code_t rc = cb_runner_warmup(&runner, mock_inference_copy, NULL,
                                            input, sizeof(input), output, sizeof(output));
    TEST_ASSERT_EQ(rc, CB_OK, "warmup succeeds");
    TEST_ASSERT_EQ(g_inference_count, 50, "warmup runs 50 iterations");
    TEST_ASSERT(runner.warmup_complete, "warmup complete flag set");
    cb_runner_cleanup(&runner);
    return 0;
}

static int test_warmup_failure(void)
{
    cb_runner_t runner;
    cb_config_t config;
    uint8_t input[64], output[64];
    cb_config_init(&config);
    config.warmup_iterations = 10;
    config.measure_iterations = 10;
    cb_runner_init(&runner, &config, g_sample_buffer, TEST_SAMPLE_CAPACITY);
    cb_result_code_t rc = cb_runner_warmup(&runner, mock_inference_fail, NULL,
                                            input, sizeof(input), output, sizeof(output));
    TEST_ASSERT_NE(rc, CB_OK, "warmup with failing fn fails");
    TEST_ASSERT(!runner.warmup_complete, "warmup_complete false on failure");
    cb_runner_cleanup(&runner);
    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Test: Execute (RUNNER-F-010..023)
 *─────────────────────────────────────────────────────────────────────────────*/

static int test_execute_sample_count(void)
{
    cb_runner_t runner;
    cb_config_t config;
    uint8_t input[64], output[64];
    cb_config_init(&config);
    config.warmup_iterations = 5;
    config.measure_iterations = 100;
    config.verify_outputs = false;
    cb_runner_init(&runner, &config, g_sample_buffer, TEST_SAMPLE_CAPACITY);
    g_inference_count = 0;
    cb_runner_execute(&runner, mock_inference_copy, NULL,
                      input, sizeof(input), output, sizeof(output));
    TEST_ASSERT_EQ(g_inference_count, 105, "total = warmup + measure");
    TEST_ASSERT_EQ(runner.samples_collected, 100, "100 samples collected");
    cb_runner_cleanup(&runner);
    return 0;
}

static int test_execute_positive_samples(void)
{
    cb_runner_t runner;
    cb_config_t config;
    uint8_t input[64], output[64];
    uint32_t i;
    cb_config_init(&config);
    config.warmup_iterations = 5;
    config.measure_iterations = 50;
    config.verify_outputs = false;
    cb_runner_init(&runner, &config, g_sample_buffer, TEST_SAMPLE_CAPACITY);
    cb_runner_execute(&runner, mock_inference_work, NULL,
                      input, sizeof(input), output, sizeof(output));
    for (i = 0; i < runner.samples_collected; i++) {
        TEST_ASSERT_GT(runner.samples[i], 0, "samples positive");
    }
    cb_runner_cleanup(&runner);
    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Test: Results (RUNNER-F-040..052)
 *─────────────────────────────────────────────────────────────────────────────*/

static int test_result_platform(void)
{
    cb_runner_t runner;
    cb_config_t config;
    cb_result_t result;
    uint8_t input[64], output[64];
    cb_config_init(&config);
    config.warmup_iterations = 5;
    config.measure_iterations = 10;
    config.verify_outputs = false;
    config.monitor_environment = false;
    cb_runner_init(&runner, &config, g_sample_buffer, TEST_SAMPLE_CAPACITY);
    cb_runner_execute(&runner, mock_inference_copy, NULL, input, 64, output, 64);
    cb_runner_get_result(&runner, &result);
    TEST_ASSERT(strlen(result.platform) > 0, "platform set");
    cb_runner_cleanup(&runner);
    return 0;
}

static int test_result_statistics(void)
{
    cb_runner_t runner;
    cb_config_t config;
    cb_result_t result;
    uint8_t input[64], output[64];
    cb_config_init(&config);
    config.warmup_iterations = 5;
    config.measure_iterations = 100;
    config.verify_outputs = false;
    config.monitor_environment = false;
    cb_runner_init(&runner, &config, g_sample_buffer, TEST_SAMPLE_CAPACITY);
    cb_runner_execute(&runner, mock_inference_work, NULL, input, 64, output, 64);
    cb_runner_get_result(&runner, &result);
    TEST_ASSERT_GT(result.latency.min_ns, 0, "min > 0");
    TEST_ASSERT_GE(result.latency.max_ns, result.latency.min_ns, "max >= min");
    TEST_ASSERT_GE(result.latency.mean_ns, result.latency.min_ns, "mean >= min");
    TEST_ASSERT_EQ(result.latency.sample_count, 100, "sample_count = 100");
    cb_runner_cleanup(&runner);
    return 0;
}

static int test_result_throughput(void)
{
    cb_runner_t runner;
    cb_config_t config;
    cb_result_t result;
    uint8_t input[64], output[64];
    cb_config_init(&config);
    config.warmup_iterations = 5;
    config.measure_iterations = 100;
    config.batch_size = 4;
    config.verify_outputs = false;
    config.monitor_environment = false;
    cb_runner_init(&runner, &config, g_sample_buffer, TEST_SAMPLE_CAPACITY);
    cb_runner_execute(&runner, mock_inference_work, NULL, input, 64, output, 64);
    cb_runner_get_result(&runner, &result);
    TEST_ASSERT_GT(result.throughput.inferences_per_sec, 0, "throughput > 0");
    TEST_ASSERT_EQ(result.throughput.samples_per_sec,
                   result.throughput.inferences_per_sec * 4, "samples = inf * batch");
    TEST_ASSERT_EQ(result.throughput.batch_size, 4, "batch echoed");
    cb_runner_cleanup(&runner);
    return 0;
}

static int test_result_timing(void)
{
    cb_runner_t runner;
    cb_config_t config;
    cb_result_t result;
    uint8_t input[64], output[64];
    cb_config_init(&config);
    config.warmup_iterations = 5;
    config.measure_iterations = 10;
    config.verify_outputs = false;
    config.monitor_environment = false;
    cb_runner_init(&runner, &config, g_sample_buffer, TEST_SAMPLE_CAPACITY);
    cb_runner_execute(&runner, mock_inference_copy, NULL, input, 64, output, 64);
    cb_runner_get_result(&runner, &result);
    TEST_ASSERT_GT(result.benchmark_duration_ns, 0, "duration > 0");
    TEST_ASSERT_GT(result.timestamp_unix, 1700000000ULL, "timestamp recent");
    cb_runner_cleanup(&runner);
    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Test: Verification (RUNNER-F-022)
 *─────────────────────────────────────────────────────────────────────────────*/

static int test_verification_hash(void)
{
    cb_runner_t runner;
    cb_config_t config;
    cb_result_t result;
    uint8_t input[64], output[64];
    char hex[65];
    int is_zero = 1, i;
    memset(input, 0xAA, sizeof(input));
    cb_config_init(&config);
    config.warmup_iterations = 5;
    config.measure_iterations = 10;
    config.verify_outputs = true;
    config.monitor_environment = false;
    cb_runner_init(&runner, &config, g_sample_buffer, TEST_SAMPLE_CAPACITY);
    cb_runner_execute(&runner, mock_inference_copy, NULL, input, 64, output, 64);
    cb_runner_get_result(&runner, &result);
    for (i = 0; i < CB_HASH_SIZE; i++) {
        if (result.output_hash.bytes[i] != 0) { is_zero = 0; break; }
    }
    TEST_ASSERT(!is_zero, "output_hash non-zero");
    cb_hash_to_hex(&result.output_hash, hex);
    printf("    output_hash: %s\n", hex);
    cb_runner_cleanup(&runner);
    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Test: Convenience API
 *─────────────────────────────────────────────────────────────────────────────*/

static int test_run_benchmark(void)
{
    cb_config_t config;
    cb_result_t result;
    uint8_t input[64], output[64];
    cb_config_init(&config);
    config.warmup_iterations = 10;
    config.measure_iterations = 50;
    config.verify_outputs = false;
    config.monitor_environment = false;
    cb_result_code_t rc = cb_run_benchmark(&config, mock_inference_work, NULL,
                                            input, 64, output, 64,
                                            g_sample_buffer, TEST_SAMPLE_CAPACITY,
                                            &result);
    TEST_ASSERT_EQ(rc, CB_OK, "run_benchmark succeeds");
    TEST_ASSERT_GT(result.latency.min_ns, 0, "latency measured");
    TEST_ASSERT_EQ(result.latency.sample_count, 50, "correct samples");
    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Test: Cleanup
 *─────────────────────────────────────────────────────────────────────────────*/

static int test_cleanup_null(void)
{
    cb_runner_cleanup(NULL);  /* Should not crash */
    return 0;
}

static int test_cleanup_resets(void)
{
    cb_runner_t runner;
    cb_config_t config;
    cb_config_init(&config);
    config.measure_iterations = 10;
    cb_runner_init(&runner, &config, g_sample_buffer, TEST_SAMPLE_CAPACITY);
    TEST_ASSERT(runner.initialised, "initialised before cleanup");
    cb_runner_cleanup(&runner);
    TEST_ASSERT(!runner.initialised, "not initialised after cleanup");
    TEST_ASSERT(runner.samples == NULL, "samples NULL after cleanup");
    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Main
 *─────────────────────────────────────────────────────────────────────────────*/

int main(void)
{
    printf("═══════════════════════════════════════════════════════════════════\n");
    printf(" certifiable-bench Runner Tests\n");
    printf(" @traceability SRS-003-RUNNER\n");
    printf("═══════════════════════════════════════════════════════════════════\n\n");

    printf("§4.1 Configuration (RUNNER-F-001..009)\n");
    RUN_TEST(test_config_defaults);
    RUN_TEST(test_config_init_null);
    RUN_TEST(test_config_validate_valid);
    RUN_TEST(test_config_validate_zero_iterations);
    RUN_TEST(test_config_validate_zero_batch);
    RUN_TEST(test_config_validate_too_many);

    printf("\n§4.2 Runner Init (RUNNER-F-014, RUNNER-NF-010..012)\n");
    RUN_TEST(test_runner_init_valid);
    RUN_TEST(test_runner_init_null);

    printf("\n§4.4 Warmup (RUNNER-F-011, RUNNER-F-030..033)\n");
    RUN_TEST(test_warmup_count);
    RUN_TEST(test_warmup_failure);

    printf("\n§4.2-4.3 Execute (RUNNER-F-010..023)\n");
    RUN_TEST(test_execute_sample_count);
    RUN_TEST(test_execute_positive_samples);

    printf("\n§4.5-4.6 Results (RUNNER-F-040..052)\n");
    RUN_TEST(test_result_platform);
    RUN_TEST(test_result_statistics);
    RUN_TEST(test_result_throughput);
    RUN_TEST(test_result_timing);

    printf("\n§4.2 Verification (RUNNER-F-022)\n");
    RUN_TEST(test_verification_hash);

    printf("\nConvenience API\n");
    RUN_TEST(test_run_benchmark);

    printf("\nCleanup\n");
    RUN_TEST(test_cleanup_null);
    RUN_TEST(test_cleanup_resets);

    printf("\n═══════════════════════════════════════════════════════════════════\n");
    printf(" Results: %d/%d tests passed\n", g_tests_passed, g_tests_run);
    printf("═══════════════════════════════════════════════════════════════════\n");

    return (g_tests_failed > 0) ? 1 : 0;
}
