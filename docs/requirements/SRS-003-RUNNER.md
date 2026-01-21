# SRS-003-RUNNER: Benchmark Runner Requirements

**Component:** certifiable-bench  
**Module:** Runner (`cb_runner.h`, `src/runner/`)  
**Version:** 1.0  
**Status:** Draft  
**Date:** January 2026  
**Traceability:** CB-MATH-001 §7

---

## 1. Purpose

This document specifies requirements for the benchmark runner, which orchestrates warmup, measurement, and verification phases while preserving the non-interference invariant.

---

## 2. Scope

The runner module provides:
- Configuration initialisation and validation
- Warmup phase execution
- Measurement phase execution
- Integration with verification subsystem
- Result aggregation
- Fault handling

---

## 3. Definitions

| Term | Definition |
|------|------------|
| Warmup phase | Iterations executed before measurement to stabilise caches and branch predictors |
| Measurement phase | Iterations where latency is recorded |
| Critical loop | The timed section containing only inference execution |
| Non-interference | Benchmark harness overhead < 1% of measured time |

---

## 4. Functional Requirements

### 4.1 Configuration

| ID | Requirement | Verification |
|----|-------------|--------------|
| RUNNER-F-001 | The module SHALL provide a function `cb_config_init()` to initialise configuration with defaults. | Test |
| RUNNER-F-002 | Default `warmup_iterations` SHALL be 100. | Test |
| RUNNER-F-003 | Default `measure_iterations` SHALL be 1000. | Test |
| RUNNER-F-004 | Default `batch_size` SHALL be 1. | Test |
| RUNNER-F-005 | Default `timer_source` SHALL be `CB_TIMER_AUTO`. | Test |
| RUNNER-F-006 | Default `verify_outputs` SHALL be true. | Test |
| RUNNER-F-007 | Default `monitor_environment` SHALL be true. | Test |
| RUNNER-F-008 | Configuration validation SHALL reject `measure_iterations` = 0. | Test |
| RUNNER-F-009 | Configuration validation SHALL reject `batch_size` = 0. | Test |

### 4.2 Benchmark Execution

| ID | Requirement | Verification |
|----|-------------|--------------|
| RUNNER-F-010 | The module SHALL provide a function `cb_run_inference_bench()` to execute benchmarks. | Test |
| RUNNER-F-011 | `cb_run_inference_bench()` SHALL execute warmup iterations before measurement. | Test |
| RUNNER-F-012 | Warmup iterations SHALL NOT be included in timing results. | Test |
| RUNNER-F-013 | Each measurement iteration SHALL record a single latency sample. | Test |
| RUNNER-F-014 | The runner SHALL allocate sample buffer before the critical loop. | Inspection |
| RUNNER-F-015 | The runner SHALL NOT allocate memory during the measurement phase. | Inspection |
| RUNNER-F-016 | Timer reads SHALL occur only at loop boundaries (before/after inference call). | Inspection |

### 4.3 Critical Loop Structure

| ID | Requirement | Verification |
|----|-------------|--------------|
| RUNNER-F-020 | The critical loop SHALL have the structure: read_timer → inference → read_timer → store_delta. | Inspection |
| RUNNER-F-021 | No function calls other than inference and timer reads SHALL occur inside the critical loop. | Inspection |
| RUNNER-F-022 | Verification (output hashing) SHALL occur outside the critical loop timing. | Inspection |
| RUNNER-F-023 | Statistics computation SHALL occur after all iterations complete. | Inspection |

### 4.4 Warmup Phase

| ID | Requirement | Verification |
|----|-------------|--------------|
| RUNNER-F-030 | Warmup iterations SHALL execute the same inference path as measurement iterations. | Inspection |
| RUNNER-F-031 | Warmup phase SHALL NOT record timing data. | Test |
| RUNNER-F-032 | Warmup phase MAY verify outputs if `verify_outputs` is enabled. | Test |
| RUNNER-F-033 | Warmup phase failure SHALL abort the benchmark with appropriate error. | Test |

### 4.5 Result Population

| ID | Requirement | Verification |
|----|-------------|--------------|
| RUNNER-F-040 | The runner SHALL populate `cb_result_t.platform` with platform identifier. | Test |
| RUNNER-F-041 | The runner SHALL populate `cb_result_t.cpu_model` with CPU identification string. | Test |
| RUNNER-F-042 | The runner SHALL populate `cb_result_t.latency` with computed statistics. | Test |
| RUNNER-F-043 | The runner SHALL populate `cb_result_t.throughput` with computed throughput. | Test |
| RUNNER-F-044 | The runner SHALL populate `cb_result_t.benchmark_duration_ns` with total benchmark time. | Test |
| RUNNER-F-045 | The runner SHALL populate `cb_result_t.timestamp_unix` with benchmark start time. | Test |

### 4.6 Throughput Calculation

| ID | Requirement | Verification |
|----|-------------|--------------|
| RUNNER-F-050 | `inferences_per_sec` SHALL be calculated as: (iterations × 10^9) / total_latency_ns. | Test |
| RUNNER-F-051 | `samples_per_sec` SHALL be calculated as: inferences_per_sec × batch_size. | Test |
| RUNNER-F-052 | Throughput calculation SHALL use integer arithmetic. | Inspection |

### 4.7 Per-Layer Benchmarking

| ID | Requirement | Verification |
|----|-------------|--------------|
| RUNNER-F-060 | The module SHALL provide a function `cb_run_layer_bench()` for per-layer timing. | Test |
| RUNNER-F-061 | `cb_run_layer_bench()` SHALL produce one `cb_result_t` per model layer. | Test |
| RUNNER-F-062 | Sum of per-layer latencies SHALL approximate total inference latency. | Test |

---

## 5. Fault Handling Requirements

| ID | Requirement | Verification |
|----|-------------|--------------|
| RUNNER-F-070 | If model loading fails, `cb_run_inference_bench()` SHALL return `CB_ERR_MODEL_LOAD`. | Test |
| RUNNER-F-071 | If data loading fails, `cb_run_inference_bench()` SHALL return `CB_ERR_DATA_LOAD`. | Test |
| RUNNER-F-072 | If timer initialisation fails, `cb_run_inference_bench()` SHALL return `CB_ERR_TIMER_INIT`. | Test |
| RUNNER-F-073 | If verification fails during measurement, `verify_fail` fault flag SHALL be set. | Test |
| RUNNER-F-074 | If verification fails, measurement SHALL continue to collect timing data. | Test |
| RUNNER-F-075 | All fault flags SHALL be propagated to `cb_result_t.faults`. | Test |
| RUNNER-F-076 | The runner SHALL NOT crash on any recoverable error condition. | Test |

---

## 6. Non-Functional Requirements

### 6.1 Non-Interference (CB-MATH-001 §7.2)

| ID | Requirement | Verification |
|----|-------------|--------------|
| RUNNER-NF-001 | Benchmark harness overhead SHALL be < 1% of measured inference time. | Test |
| RUNNER-NF-002 | Timer read overhead SHALL be characterised and documented. | Test |
| RUNNER-NF-003 | No system calls SHALL occur inside the critical loop (except timer reads on POSIX). | Inspection |

### 6.2 Memory

| ID | Requirement | Verification |
|----|-------------|--------------|
| RUNNER-NF-010 | Sample buffer SHALL be pre-allocated before measurement phase. | Inspection |
| RUNNER-NF-011 | Maximum sample buffer size SHALL be CB_MAX_SAMPLES (1,000,000). | Inspection |
| RUNNER-NF-012 | If iterations exceed CB_MAX_SAMPLES, configuration validation SHALL fail. | Test |

### 6.3 Determinism

| ID | Requirement | Verification |
|----|-------------|--------------|
| RUNNER-NF-020 | Given identical inputs and hardware state, benchmark results SHALL be reproducible within measurement noise. | Test |
| RUNNER-NF-021 | Iteration order SHALL be deterministic (sequential, no randomisation). | Inspection |

---

## 7. Interface Definition

```c
/**
 * @brief Initialise configuration with defaults
 * @param config Configuration to initialise
 *
 * @satisfies RUNNER-F-001 through RUNNER-F-007
 */
void cb_config_init(cb_config_t *config);

/**
 * @brief Validate configuration
 * @param config Configuration to validate
 * @return CB_OK if valid, error code otherwise
 *
 * @satisfies RUNNER-F-008, RUNNER-F-009
 */
cb_result_code_t cb_config_validate(const cb_config_t *config);

/**
 * @brief Run inference benchmark
 * @param config Benchmark configuration
 * @param model_path Path to model bundle (.cbf)
 * @param data_path Path to test data
 * @param result Output result
 * @return CB_OK on success
 *
 * @satisfies RUNNER-F-010 through RUNNER-F-045
 */
cb_result_code_t cb_run_inference_bench(const cb_config_t *config,
                                         const char *model_path,
                                         const char *data_path,
                                         cb_result_t *result);

/**
 * @brief Run per-layer benchmark
 * @param config Configuration
 * @param model_path Model path
 * @param data_path Data path
 * @param layer_results Output array (one per layer)
 * @param max_layers Maximum layers
 * @param num_layers Actual layers found
 * @return CB_OK on success
 *
 * @satisfies RUNNER-F-060 through RUNNER-F-062
 */
cb_result_code_t cb_run_layer_bench(const cb_config_t *config,
                                     const char *model_path,
                                     const char *data_path,
                                     cb_result_t *layer_results,
                                     uint32_t max_layers,
                                     uint32_t *num_layers);
```

---

## 8. Critical Loop Pseudocode

```c
/* Pre-measurement setup (outside critical section) */
samples = allocate_buffer(config->measure_iterations);
timer_init(config->timer_source);
model = load_model(model_path);
data = load_data(data_path);
verify_ctx_init(&verify_ctx);

/* Warmup phase (untimed) */
for (i = 0; i < config->warmup_iterations; i++) {
    inference_run(model, data[i % num_inputs], output);
}

/* Measurement phase (critical loop) */
for (i = 0; i < config->measure_iterations; i++) {
    /* === CRITICAL LOOP START === */
    t_start = cb_timer_now_ns();
    inference_run(model, data[i % num_inputs], output);
    t_end = cb_timer_now_ns();
    /* === CRITICAL LOOP END === */

    samples[i] = t_end - t_start;

    /* Verification outside critical timing */
    if (config->verify_outputs) {
        verify_ctx_update(&verify_ctx, output, output_size);
    }
}

/* Post-measurement analysis */
cb_compute_stats(samples, config->measure_iterations, &result->latency);
```

---

## 9. Traceability Matrix

| Requirement | CB-MATH-001 | CB-STRUCT-001 | Test |
|-------------|-------------|---------------|------|
| RUNNER-F-001 | — | §9 | test_config_init |
| RUNNER-F-010 | §7 | §10 | test_run_bench |
| RUNNER-F-015 | §7.2 | — | inspection |
| RUNNER-F-020 | §7.2 | — | inspection |
| RUNNER-NF-001 | §7.2 | — | test_overhead |
| RUNNER-F-070 | §5 | §3 | test_fault_handling |

---

## 10. Document Control

| Version | Date | Author | Change |
|---------|------|--------|--------|
| 1.0 | January 2026 | William Murray | Initial release |

---

*Built for safety. Designed for certification. Proven through testing.*

Copyright © 2026 The Murray Family Innovation Trust. All rights reserved.
