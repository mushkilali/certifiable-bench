/**
 * @file runner.c
 * @brief Benchmark execution engine implementation
 *
 * Implements SRS-003-RUNNER requirements with critical loop structure
 * per CB-MATH-001 §7.2 (Non-interference Invariant).
 *
 * Critical Loop Structure:
 *   t_start = cb_timer_now_ns()
 *   inference(input, output)
 *   t_end = cb_timer_now_ns()
 *   samples[i] = t_end - t_start
 *
 * No other operations inside the critical loop.
 *
 * @traceability SRS-003-RUNNER, CB-MATH-001 §7
 *
 * Copyright © 2026 The Murray Family Innovation Trust. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cb_runner.h"
#include "cb_timer.h"
#include "cb_metrics.h"
#include "cb_platform.h"
#include "cb_verify.h"

#include <string.h>
#include <time.h>

/*─────────────────────────────────────────────────────────────────────────────
 * Configuration API
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Initialise configuration with defaults
 * @satisfies RUNNER-F-001 through RUNNER-F-007
 */
cb_result_code_t cb_config_init(cb_config_t *config)
{
    if (config == NULL) {
        return CB_ERR_NULL_PTR;
    }

    memset(config, 0, sizeof(*config));

    /* RUNNER-F-002: Default warmup_iterations = 100 */
    config->warmup_iterations = 100;

    /* RUNNER-F-003: Default measure_iterations = 1000 */
    config->measure_iterations = 1000;

    /* RUNNER-F-004: Default batch_size = 1 */
    config->batch_size = 1;

    /* RUNNER-F-005: Default timer_source = CB_TIMER_AUTO */
    config->timer_source = CB_TIMER_AUTO;

    /* RUNNER-F-006: Default verify_outputs = true */
    config->verify_outputs = true;

    /* RUNNER-F-007: Default monitor_environment = true */
    config->monitor_environment = true;

    /* Histogram defaults */
    config->collect_histogram = false;
    config->histogram_bins = 100;
    config->histogram_min_ns = 0;
    config->histogram_max_ns = 10 * CB_NS_PER_MS;  /* 10ms */

    /* Paths default to NULL */
    config->model_path = NULL;
    config->data_path = NULL;
    config->golden_path = NULL;
    config->output_path = NULL;

    return CB_OK;
}

/**
 * @brief Validate configuration
 * @satisfies RUNNER-F-008, RUNNER-F-009, RUNNER-NF-012
 */
cb_result_code_t cb_config_validate(const cb_config_t *config)
{
    if (config == NULL) {
        return CB_ERR_NULL_PTR;
    }

    /* RUNNER-F-008: measure_iterations must be > 0 */
    if (config->measure_iterations == 0) {
        return CB_ERR_INVALID_CONFIG;
    }

    /* RUNNER-F-009: batch_size must be > 0 */
    if (config->batch_size == 0) {
        return CB_ERR_INVALID_CONFIG;
    }

    /* RUNNER-NF-012: measure_iterations must not exceed CB_MAX_SAMPLES */
    if (config->measure_iterations > CB_MAX_SAMPLES) {
        return CB_ERR_INVALID_CONFIG;
    }

    /* Histogram validation */
    if (config->collect_histogram) {
        if (config->histogram_bins == 0 || config->histogram_bins > CB_MAX_HISTOGRAM) {
            return CB_ERR_INVALID_CONFIG;
        }
        if (config->histogram_max_ns <= config->histogram_min_ns) {
            return CB_ERR_INVALID_CONFIG;
        }
    }

    return CB_OK;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Runner Lifecycle
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Initialise benchmark runner with caller-provided buffer
 * @satisfies RUNNER-F-014, RUNNER-NF-010, RUNNER-NF-011
 */
cb_result_code_t cb_runner_init(cb_runner_t *runner,
                                 const cb_config_t *config,
                                 uint64_t *sample_buffer,
                                 uint32_t buffer_capacity)
{
    cb_result_code_t rc;

    if (runner == NULL || config == NULL || sample_buffer == NULL) {
        return CB_ERR_NULL_PTR;
    }

    /* Validate configuration first */
    rc = cb_config_validate(config);
    if (rc != CB_OK) {
        return rc;
    }

    /* Check buffer is large enough */
    if (buffer_capacity < config->measure_iterations) {
        return CB_ERR_INVALID_CONFIG;
    }

    /* Clear state */
    memset(runner, 0, sizeof(*runner));

    /* Copy configuration */
    memcpy(&runner->config, config, sizeof(cb_config_t));

    /* RUNNER-F-014, RUNNER-NF-010: Use caller-provided sample buffer */
    runner->samples = sample_buffer;
    runner->sample_capacity = buffer_capacity;

    /* Initialise timer */
    cb_timer_source_t actual = cb_timer_init(config->timer_source);
    if (actual == CB_TIMER_AUTO) {
        /* Timer init failed - no suitable timer found */
        runner->samples = NULL;
        return CB_ERR_TIMER_INIT;
    }

    /* Initialise verification context if enabled */
    if (config->verify_outputs) {
        cb_verify_ctx_init(&runner->verify_ctx);
    }

    /* Clear fault flags */
    cb_fault_clear(&runner->faults);

    runner->samples_collected = 0;
    runner->initialised = true;
    runner->warmup_complete = false;

    return CB_OK;
}

/**
 * @brief Execute warmup phase
 * @satisfies RUNNER-F-011, RUNNER-F-030 through RUNNER-F-033
 */
cb_result_code_t cb_runner_warmup(cb_runner_t *runner,
                                   cb_inference_fn fn,
                                   void *ctx,
                                   const void *input,
                                   uint32_t input_size,
                                   void *output,
                                   uint32_t output_size)
{
    uint32_t i;
    cb_result_code_t rc;

    (void)input_size;  /* Used for documentation, may be used in future */
    (void)output_size; /* Used for documentation, may be used in future */

    if (runner == NULL || fn == NULL) {
        return CB_ERR_NULL_PTR;
    }
    if (input == NULL && input_size > 0) {
        return CB_ERR_NULL_PTR;
    }
    if (output == NULL && output_size > 0) {
        return CB_ERR_NULL_PTR;
    }

    if (!runner->initialised) {
        return CB_ERR_INVALID_CONFIG;
    }

    /* RUNNER-F-030: Warmup executes same inference path as measurement */
    /* RUNNER-F-031: Warmup does NOT record timing */
    for (i = 0; i < runner->config.warmup_iterations; i++) {
        rc = fn(ctx, input, output);
        if (rc != CB_OK) {
            /* RUNNER-F-033: Warmup failure aborts benchmark */
            return rc;
        }
    }

    /* Capture environment at end of warmup (start of measurement) */
    if (runner->config.monitor_environment) {
        cb_env_snapshot(&runner->env_start);
    }

    /* Record benchmark start time */
    runner->benchmark_start_ns = cb_timer_now_ns();

    runner->warmup_complete = true;
    return CB_OK;
}

/**
 * @brief Execute measurement phase
 *
 * CRITICAL LOOP - No operations other than timer and inference.
 *
 * @satisfies RUNNER-F-010, RUNNER-F-012 through RUNNER-F-023
 */
cb_result_code_t cb_runner_execute(cb_runner_t *runner,
                                    cb_inference_fn fn,
                                    void *ctx,
                                    const void *input,
                                    uint32_t input_size,
                                    void *output,
                                    uint32_t output_size)
{
    uint32_t i;
    uint64_t t_start, t_end;
    cb_result_code_t rc;

    if (runner == NULL || fn == NULL) {
        return CB_ERR_NULL_PTR;
    }
    if (input == NULL && input_size > 0) {
        return CB_ERR_NULL_PTR;
    }
    if (output == NULL && output_size > 0) {
        return CB_ERR_NULL_PTR;
    }

    if (!runner->initialised) {
        return CB_ERR_INVALID_CONFIG;
    }

    /* Run warmup if not already done */
    if (!runner->warmup_complete) {
        rc = cb_runner_warmup(runner, fn, ctx, input, input_size, output, output_size);
        if (rc != CB_OK) {
            return rc;
        }
    }

    /* RUNNER-F-012: Measurement loop - warmup NOT included in results */
    for (i = 0; i < runner->config.measure_iterations; i++) {
        /*═══════════════════════════════════════════════════════════════════
         * CRITICAL LOOP START (RUNNER-F-020)
         * Only timer reads and inference call inside this section.
         *═══════════════════════════════════════════════════════════════════*/
        t_start = cb_timer_now_ns();                    /* RUNNER-F-016 */
        rc = fn(ctx, input, output);                    /* Inference */
        t_end = cb_timer_now_ns();                      /* RUNNER-F-016 */
        /*═══════════════════════════════════════════════════════════════════
         * CRITICAL LOOP END
         *═══════════════════════════════════════════════════════════════════*/

        /* Store sample (RUNNER-F-013) */
        runner->samples[i] = t_end - t_start;

        /* Check for timer wraparound */
        if (t_end < t_start) {
            runner->faults.timer_error = 1;
        }

        /* RUNNER-F-022: Verification OUTSIDE critical loop */
        if (runner->config.verify_outputs && output_size > 0) {
            cb_verify_ctx_update(&runner->verify_ctx, output, output_size);
        }

        /* RUNNER-F-074: Continue even if inference fails (collect timing data) */
        if (rc != CB_OK) {
            runner->faults.verify_fail = 1;
        }
    }

    runner->samples_collected = runner->config.measure_iterations;

    return CB_OK;
}

/**
 * @brief Retrieve benchmark results
 * @satisfies RUNNER-F-040 through RUNNER-F-052
 */
cb_result_code_t cb_runner_get_result(const cb_runner_t *runner,
                                       cb_result_t *result)
{
    cb_env_snapshot_t env_end;
    cb_env_stats_t env_stats;
    cb_fault_flags_t stats_faults;
    uint64_t total_latency_ns;
    uint32_t i;

    if (runner == NULL || result == NULL) {
        return CB_ERR_NULL_PTR;
    }

    if (!runner->initialised || runner->samples_collected == 0) {
        return CB_ERR_INVALID_CONFIG;
    }

    /* Clear result */
    memset(result, 0, sizeof(*result));

    /* RUNNER-F-040, RUNNER-F-041: Platform identification */
    strncpy(result->platform, cb_platform_name(), CB_MAX_PLATFORM - 1);
    cb_cpu_model(result->cpu_model, CB_MAX_CPU_MODEL);
    result->cpu_freq_mhz = cb_cpu_freq_mhz();

    /* Configuration echo */
    result->warmup_iterations = runner->config.warmup_iterations;
    result->measure_iterations = runner->config.measure_iterations;
    result->batch_size = runner->config.batch_size;

    /* RUNNER-F-042, RUNNER-F-023: Compute statistics AFTER all iterations */
    cb_fault_clear(&stats_faults);

    /* Sort samples in place - benchmark is complete, original order not needed */
    cb_compute_stats(runner->samples, runner->samples_collected,
                     &result->latency, &stats_faults);

    /* RUNNER-F-043, RUNNER-F-050, RUNNER-F-051: Throughput calculation */
    /* Total latency = sum of all samples */
    total_latency_ns = 0;
    for (i = 0; i < runner->samples_collected; i++) {
        total_latency_ns += runner->samples[i];
    }

    /* RUNNER-F-050: inferences_per_sec = (iterations × 10^9) / total_latency_ns */
    if (total_latency_ns > 0) {
        result->throughput.inferences_per_sec =
            ((uint64_t)runner->samples_collected * CB_NS_PER_SEC) / total_latency_ns;
    }

    /* RUNNER-F-051: samples_per_sec = inferences_per_sec × batch_size */
    result->throughput.samples_per_sec =
        result->throughput.inferences_per_sec * runner->config.batch_size;

    result->throughput.batch_size = runner->config.batch_size;

    /* Environment data */
    if (runner->config.monitor_environment) {
        cb_env_snapshot(&env_end);
        cb_env_compute_stats(&runner->env_start, &env_end, &env_stats);
        result->environment = env_stats;
        result->env_stable = cb_env_check_stable(&env_stats);

        if (!result->env_stable) {
            result->faults.thermal_drift = 1;
        }
    }

    /* RUNNER-F-044: Benchmark duration */
    result->benchmark_start_ns = runner->benchmark_start_ns;
    result->benchmark_end_ns = cb_timer_now_ns();
    result->benchmark_duration_ns = result->benchmark_end_ns - result->benchmark_start_ns;

    /* RUNNER-F-045: Unix timestamp */
    result->timestamp_unix = (uint64_t)time(NULL);

    /* Verification results */
    if (runner->config.verify_outputs) {
        /* Finalise output hash */
        cb_verify_ctx_t verify_copy = runner->verify_ctx;
        cb_verify_ctx_final(&verify_copy, &result->output_hash);
        result->determinism_verified = !runner->faults.verify_fail;
        result->verification_failures = runner->faults.verify_fail ? 1 : 0;

        /* Compute result binding hash */
        cb_compute_result_binding(&result->output_hash,
                                  result->platform,
                                  0,  /* config_hash - could be computed */
                                  &result->latency,
                                  result->timestamp_unix,
                                  &result->result_hash);
    }

    /* RUNNER-F-075: Propagate all faults */
    result->faults = runner->faults;

    /* Merge stats faults */
    if (stats_faults.overflow) result->faults.overflow = 1;
    if (stats_faults.div_zero) result->faults.div_zero = 1;

    return CB_OK;
}

/**
 * @brief Clean up runner resources
 * Does not free sample buffer - caller owns it.
 */
void cb_runner_cleanup(cb_runner_t *runner)
{
    if (runner == NULL) {
        return;
    }

    /* Clear pointer but don't free - caller owns the buffer */
    runner->samples = NULL;
    runner->initialised = false;
    runner->warmup_complete = false;
    runner->sample_capacity = 0;
    runner->samples_collected = 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Convenience API
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Run complete inference benchmark
 * @satisfies RUNNER-F-010
 */
cb_result_code_t cb_run_benchmark(const cb_config_t *config,
                                   cb_inference_fn fn,
                                   void *ctx,
                                   const void *input,
                                   uint32_t input_size,
                                   void *output,
                                   uint32_t output_size,
                                   uint64_t *sample_buffer,
                                   uint32_t buffer_capacity,
                                   cb_result_t *result)
{
    cb_runner_t runner;
    cb_result_code_t rc;

    /* Initialise with caller-provided buffer */
    rc = cb_runner_init(&runner, config, sample_buffer, buffer_capacity);
    if (rc != CB_OK) {
        return rc;
    }

    /* Warmup */
    rc = cb_runner_warmup(&runner, fn, ctx, input, input_size, output, output_size);
    if (rc != CB_OK) {
        cb_runner_cleanup(&runner);
        return rc;
    }

    /* Execute */
    rc = cb_runner_execute(&runner, fn, ctx, input, input_size, output, output_size);
    if (rc != CB_OK) {
        cb_runner_cleanup(&runner);
        return rc;
    }

    /* Get results */
    rc = cb_runner_get_result(&runner, result);

    /* Cleanup */
    cb_runner_cleanup(&runner);

    return rc;
}
