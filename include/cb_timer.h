/**
 * @file cb_timer.h
 * @brief High-resolution timer API for certifiable-bench
 *
 * Provides platform-abstracted timing with nanosecond resolution.
 * Supports POSIX, x86 RDTSC, ARM64 CNTVCT, and RISC-V cycle counters.
 *
 * @traceability SRS-001-TIMER
 *
 * Copyright © 2026 The Murray Family Innovation Trust. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CB_TIMER_H
#define CB_TIMER_H

#include "cb_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*─────────────────────────────────────────────────────────────────────────────
 * Timer Initialisation (SRS-001-TIMER §4.1)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Initialise timer subsystem
 *
 * Calibrates timer resolution and overhead. Must be called before
 * any other timer functions.
 *
 * When CB_TIMER_AUTO is specified, selects the highest-resolution
 * available timer with priority: platform cycle counter → POSIX.
 *
 * @param source Preferred timer source (CB_TIMER_AUTO recommended)
 * @return Actual timer source selected
 *
 * @satisfies TIMER-F-001, TIMER-F-002, TIMER-F-003, TIMER-F-004, TIMER-F-005
 * @traceability SRS-001-TIMER §4.1
 */
cb_timer_source_t cb_timer_init(cb_timer_source_t source);

/*─────────────────────────────────────────────────────────────────────────────
 * Timestamp Acquisition (SRS-001-TIMER §4.2)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Get current timestamp in nanoseconds (monotonic)
 *
 * Returns monotonically increasing timestamp. Does not allocate memory.
 * On cycle counter backends, does not make system calls after init.
 *
 * @return Timestamp in nanoseconds
 *
 * @satisfies TIMER-F-010, TIMER-F-011, TIMER-F-012, TIMER-F-013, TIMER-F-014
 * @traceability SRS-001-TIMER §4.2, CB-MATH-001 §7.2
 */
uint64_t cb_timer_now_ns(void);

/*─────────────────────────────────────────────────────────────────────────────
 * Timer Resolution (SRS-001-TIMER §4.3)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Get timer resolution in nanoseconds
 *
 * Resolution is determined at initialisation, not hardcoded.
 * SHALL be ≤ 1000 ns (1 µs) on supported platforms.
 *
 * @return Resolution in nanoseconds
 *
 * @satisfies TIMER-F-020, TIMER-F-021, TIMER-F-022
 * @traceability SRS-001-TIMER §4.3
 */
uint64_t cb_timer_resolution_ns(void);

/*─────────────────────────────────────────────────────────────────────────────
 * Cycle Conversion (SRS-001-TIMER §4.4)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Convert cycles to nanoseconds
 *
 * Uses calibrated frequency from initialisation.
 * Integer arithmetic only — no floating-point.
 *
 * @param cycles Cycle count to convert
 * @return Nanoseconds
 *
 * @satisfies TIMER-F-030, TIMER-F-031, TIMER-F-032, TIMER-F-033
 * @traceability SRS-001-TIMER §4.4
 */
uint64_t cb_cycles_to_ns(uint64_t cycles);

/*─────────────────────────────────────────────────────────────────────────────
 * Timer Identification (SRS-001-TIMER §4.5)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Get name of current timer source
 *
 * Returns a static string — no allocation.
 *
 * @return Timer name string (do not free)
 *
 * @satisfies TIMER-F-040, TIMER-F-041
 * @traceability SRS-001-TIMER §4.5
 */
const char *cb_timer_name(void);

/*─────────────────────────────────────────────────────────────────────────────
 * Timer State Access
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Get current timer state
 *
 * Returns pointer to internal timer state. Valid until next cb_timer_init().
 *
 * @return Pointer to timer state (do not free)
 *
 * @traceability CB-STRUCT-001 §8
 */
const cb_timer_state_t *cb_timer_state(void);

/**
 * @brief Get timer calibration overhead in nanoseconds
 *
 * Returns measured overhead of calling cb_timer_now_ns().
 *
 * @return Calibration overhead in nanoseconds
 *
 * @satisfies TIMER-NF-003
 * @traceability SRS-001-TIMER §6.1
 */
uint64_t cb_timer_calibration_ns(void);

/*─────────────────────────────────────────────────────────────────────────────
 * Fault Access
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Get timer fault flags
 *
 * Returns pointer to fault flags. Check after timer operations.
 *
 * @return Pointer to fault flags
 *
 * @satisfies TIMER-NF-020, TIMER-NF-021, TIMER-NF-022
 * @traceability SRS-001-TIMER §6.3
 */
cb_fault_flags_t *cb_timer_faults(void);

#ifdef __cplusplus
}
#endif

#endif /* CB_TIMER_H */
