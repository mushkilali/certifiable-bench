/**
 * @file cb_platform.h
 * @brief Platform detection and environmental monitoring API
 *
 * Provides platform identification, hardware counter access, and
 * environmental monitoring for thermal/frequency characterisation.
 *
 * @traceability SRS-006-PLATFORM
 *
 * Copyright © 2026 The Murray Family Innovation Trust. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CB_PLATFORM_H
#define CB_PLATFORM_H

#include "cb_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*─────────────────────────────────────────────────────────────────────────────
 * Platform Identification (SRS-006-PLATFORM §4.1)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Get platform architecture identifier
 *
 * Returns a static string identifying the CPU architecture.
 *
 * @return "x86_64", "aarch64", "riscv64", or "unknown"
 *
 * @satisfies PLATFORM-F-001 through PLATFORM-F-005
 * @traceability SRS-006-PLATFORM §4.1
 */
const char *cb_platform_name(void);

/**
 * @brief Get CPU model string
 *
 * Reads CPU model from system (e.g., /proc/cpuinfo on Linux).
 * Output is truncated to fit buffer size.
 *
 * @param buffer Output buffer for CPU model string
 * @param size   Buffer size in bytes
 * @return CB_OK on success, CB_ERR_NULL_PTR if buffer is NULL
 *
 * @satisfies PLATFORM-F-006 through PLATFORM-F-009
 * @traceability SRS-006-PLATFORM §4.1
 */
cb_result_code_t cb_cpu_model(char *buffer, uint32_t size);

/**
 * @brief Get nominal CPU frequency
 *
 * Returns the current or nominal CPU frequency.
 *
 * @return Frequency in MHz, or 0 if unavailable
 *
 * @satisfies PLATFORM-F-010
 * @traceability SRS-006-PLATFORM §4.1
 */
uint32_t cb_cpu_freq_mhz(void);

/*─────────────────────────────────────────────────────────────────────────────
 * Hardware Performance Counters (SRS-006-PLATFORM §4.2)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Check if hardware performance counters are available
 *
 * Tests whether the system supports hardware counter access.
 * May require appropriate permissions (e.g., perf_event on Linux).
 *
 * @return true if counters are available and accessible
 *
 * @satisfies PLATFORM-F-020
 * @traceability SRS-006-PLATFORM §4.2
 */
bool cb_hwcounters_available(void);

/**
 * @brief Start hardware counter collection
 *
 * Begins collecting hardware performance counters. Must be paired
 * with cb_hwcounters_stop().
 *
 * @return CB_OK on success
 * @return CB_ERR_HWCOUNTERS if counters unavailable or already started
 *
 * @satisfies PLATFORM-F-021
 * @traceability SRS-006-PLATFORM §4.2
 */
cb_result_code_t cb_hwcounters_start(void);

/**
 * @brief Stop collection and read hardware counters
 *
 * Stops counter collection and populates the output structure.
 * Computes derived metrics (IPC, cache miss rate) in Q16.16 format.
 *
 * @param counters Output counter values
 * @return CB_OK on success
 * @return CB_ERR_NULL_PTR if counters is NULL
 * @return CB_ERR_HWCOUNTERS if counters weren't started
 *
 * @satisfies PLATFORM-F-022 through PLATFORM-F-026
 * @traceability SRS-006-PLATFORM §4.2
 */
cb_result_code_t cb_hwcounters_stop(cb_hwcounters_t *counters);

/*─────────────────────────────────────────────────────────────────────────────
 * Environmental Monitoring (SRS-006-PLATFORM §4.6, CB-MATH-001 §9)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Capture environmental snapshot
 *
 * Reads current CPU frequency, temperature, and throttle state.
 * On systems where some data is unavailable, those fields are set to 0.
 *
 * @param snapshot Output snapshot structure
 * @return CB_OK on success
 * @return CB_ERR_NULL_PTR if snapshot is NULL
 *
 * @satisfies PLATFORM-F-060 through PLATFORM-F-064
 * @traceability SRS-006-PLATFORM §4.6, CB-MATH-001 §9.2
 */
cb_result_code_t cb_env_snapshot(cb_env_snapshot_t *snapshot);

/**
 * @brief Compute environmental statistics from start/end snapshots
 *
 * Computes min/max frequency, min/max temperature, and total
 * throttle events from two snapshots.
 *
 * @param start Start snapshot (taken before benchmark)
 * @param end   End snapshot (taken after benchmark)
 * @param stats Output statistics structure
 * @return CB_OK on success
 * @return CB_ERR_NULL_PTR if any pointer is NULL
 *
 * @satisfies PLATFORM-F-065 through PLATFORM-F-068
 * @traceability SRS-006-PLATFORM §4.6, CB-MATH-001 §9.2
 */
cb_result_code_t cb_env_compute_stats(const cb_env_snapshot_t *start,
                                      const cb_env_snapshot_t *end,
                                      cb_env_stats_t *stats);

/**
 * @brief Check environmental stability (CB-MATH-001 §9.3)
 *
 * Assesses whether the hardware state was stable during benchmarking.
 * Returns false if:
 * - Frequency dropped more than 5% (thermal throttling)
 * - Any throttle events occurred
 *
 * Uses integer arithmetic: end_freq × 100 >= start_freq × 95
 *
 * @param stats Environmental statistics
 * @return true if environment was stable
 *
 * @satisfies PLATFORM-F-090 through PLATFORM-F-094
 * @traceability SRS-006-PLATFORM §4.9, CB-MATH-001 §9.3
 */
bool cb_env_check_stable(const cb_env_stats_t *stats);

/*─────────────────────────────────────────────────────────────────────────────
 * Platform Initialisation
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Initialise platform subsystem
 *
 * Detects platform capabilities and prepares monitoring systems.
 * Should be called once at startup.
 *
 * @return CB_OK on success
 *
 * @traceability SRS-006-PLATFORM
 */
cb_result_code_t cb_platform_init(void);

#ifdef __cplusplus
}
#endif

#endif /* CB_PLATFORM_H */
