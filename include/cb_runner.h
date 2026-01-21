/**
 * @file cb_runner.h
 * @brief Benchmark execution engine
 *
 * Orchestrates warmup, measurement, and verification phases while
 * preserving the non-interference invariant (CB-MATH-001 §7.2).
 *
 * @traceability SRS-003-RUNNER, CB-MATH-001 §7
 *
 * Copyright © 2026 The Murray Family Innovation Trust. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CB_RUNNER_H
#define CB_RUNNER_H

#include "cb_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*─────────────────────────────────────────────────────────────────────────────
 * Configuration API (SRS-003-RUNNER §4.1)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Initialise configuration with defaults
 *
 * Default values:
 *   - warmup_iterations: 100
 *   - measure_iterations: 1000
 *   - batch_size: 1
 *   - timer_source: CB_TIMER_AUTO
 *   - verify_outputs: true
 *   - monitor_environment: true
 *
 * @param config Configuration to initialise
 * @return CB_OK on success
 * @return CB_ERR_NULL_PTR if config is NULL
 *
 * @satisfies RUNNER-F-001 through RUNNER-F-007
 * @traceability SRS-003-RUNNER §4.1
 */
cb_result_code_t cb_config_init(cb_config_t *config);

/**
 * @brief Validate configuration
 *
 * Checks:
 *   - measure_iterations > 0
 *   - batch_size > 0
 *   - measure_iterations <= CB_MAX_SAMPLES
 *
 * @param config Configuration to validate
 * @return CB_OK if valid
 * @return CB_ERR_INVALID_CONFIG if invalid
 * @return CB_ERR_NULL_PTR if config is NULL
 *
 * @satisfies RUNNER-F-008, RUNNER-F-009, RUNNER-NF-012
 * @traceability SRS-003-RUNNER §4.1
 */
cb_result_code_t cb_config_validate(const cb_config_t *config);

/*─────────────────────────────────────────────────────────────────────────────
 * Runner Lifecycle API (SRS-003-RUNNER §4.2, §4.3)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Initialise benchmark runner with caller-provided buffer
 *
 * Prepares runner state using caller-provided sample buffer.
 * No dynamic allocation — all buffers must be provided by caller.
 * Must be called before warmup or execute.
 *
 * @param runner         Runner state to initialise
 * @param config         Benchmark configuration
 * @param sample_buffer  Caller-provided buffer for timing samples
 * @param buffer_capacity Number of samples buffer can hold (must be >= config->measure_iterations)
 * @return CB_OK on success
 * @return CB_ERR_NULL_PTR if runner, config, or sample_buffer is NULL
 * @return CB_ERR_INVALID_CONFIG if config validation fails or buffer too small
 *
 * @satisfies RUNNER-F-014, RUNNER-NF-010, RUNNER-NF-011
 * @traceability SRS-003-RUNNER §4.2
 */
cb_result_code_t cb_runner_init(cb_runner_t *runner,
                                 const cb_config_t *config,
                                 uint64_t *sample_buffer,
                                 uint32_t buffer_capacity);

/**
 * @brief Execute warmup phase
 *
 * Runs warmup_iterations iterations without recording timing.
 * Stabilises caches, branch predictors, and frequency scaling.
 *
 * @param runner     Initialised runner
 * @param fn         Inference function to call
 * @param ctx        User context passed to inference function
 * @param input      Input buffer
 * @param input_size Size of input buffer
 * @param output     Output buffer
 * @param output_size Size of output buffer
 * @return CB_OK on success
 * @return CB_ERR_NULL_PTR if any pointer is NULL
 * @return Other error codes from inference function
 *
 * @satisfies RUNNER-F-011, RUNNER-F-030 through RUNNER-F-033
 * @traceability SRS-003-RUNNER §4.4
 */
cb_result_code_t cb_runner_warmup(cb_runner_t *runner,
                                   cb_inference_fn fn,
                                   void *ctx,
                                   const void *input,
                                   uint32_t input_size,
                                   void *output,
                                   uint32_t output_size);

/**
 * @brief Execute measurement phase
 *
 * Runs measure_iterations iterations, recording latency for each.
 * Critical loop structure per CB-MATH-001 §7.2:
 *   t_start = timer_now()
 *   inference(input, output)
 *   t_end = timer_now()
 *   samples[i] = t_end - t_start
 *
 * Verification (if enabled) occurs outside the critical timing path.
 *
 * @param runner     Initialised runner (warmup should be complete)
 * @param fn         Inference function to call
 * @param ctx        User context passed to inference function
 * @param input      Input buffer
 * @param input_size Size of input buffer
 * @param output     Output buffer
 * @param output_size Size of output buffer
 * @return CB_OK on success
 * @return CB_ERR_NULL_PTR if any pointer is NULL
 * @return Other error codes from inference function
 *
 * @satisfies RUNNER-F-010, RUNNER-F-012 through RUNNER-F-023
 * @traceability SRS-003-RUNNER §4.2, §4.3
 */
cb_result_code_t cb_runner_execute(cb_runner_t *runner,
                                    cb_inference_fn fn,
                                    void *ctx,
                                    const void *input,
                                    uint32_t input_size,
                                    void *output,
                                    uint32_t output_size);

/**
 * @brief Retrieve benchmark results
 *
 * Computes statistics from collected samples and populates result structure.
 * Must be called after execute.
 *
 * @param runner Completed runner
 * @param result Output result structure
 * @return CB_OK on success
 * @return CB_ERR_NULL_PTR if runner or result is NULL
 * @return CB_ERR_INVALID_CONFIG if execute was not called
 *
 * @satisfies RUNNER-F-040 through RUNNER-F-045, RUNNER-F-050 through RUNNER-F-052
 * @traceability SRS-003-RUNNER §4.5, §4.6
 */
cb_result_code_t cb_runner_get_result(const cb_runner_t *runner,
                                       cb_result_t *result);

/**
 * @brief Clean up runner resources
 *
 * Resets runner state. Does not free sample buffer (caller owns it).
 *
 * @param runner Runner to clean up
 */
void cb_runner_cleanup(cb_runner_t *runner);

/*─────────────────────────────────────────────────────────────────────────────
 * Convenience API (SRS-003-RUNNER §4.2)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Run complete inference benchmark
 *
 * Convenience function that performs init → warmup → execute → get_result → cleanup.
 * Caller must provide sample buffer.
 *
 * @param config         Benchmark configuration
 * @param fn             Inference function to call
 * @param ctx            User context passed to inference function
 * @param input          Input buffer
 * @param input_size     Size of input buffer
 * @param output         Output buffer
 * @param output_size    Size of output buffer
 * @param sample_buffer  Caller-provided buffer for timing samples
 * @param buffer_capacity Number of samples buffer can hold
 * @param result         Output result structure
 * @return CB_OK on success
 * @return Error codes from underlying operations
 *
 * @satisfies RUNNER-F-010
 * @traceability SRS-003-RUNNER §4.2
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
                                   cb_result_t *result);

#ifdef __cplusplus
}
#endif

#endif /* CB_RUNNER_H */
