/**
 * @file timer.c
 * @brief Timer subsystem implementation
 *
 * Implements SRS-001-TIMER requirements with POSIX backend.
 * Additional backends (RDTSC, CNTVCT, RISC-V) can be added via
 * compile-time platform detection.
 *
 * @traceability SRS-001-TIMER
 *
 * Copyright © 2026 The Murray Family Innovation Trust. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Required for clock_gettime, CLOCK_MONOTONIC on POSIX */
#define _POSIX_C_SOURCE 199309L

#include "cb_timer.h"

#include <time.h>
#include <string.h>

/*─────────────────────────────────────────────────────────────────────────────
 * Constants
 *─────────────────────────────────────────────────────────────────────────────*/

/** Number of calibration iterations for overhead measurement */
#define CALIBRATION_ITERATIONS 1000

/*─────────────────────────────────────────────────────────────────────────────
 * Global State (CB-STRUCT-001 §8)
 *─────────────────────────────────────────────────────────────────────────────*/

/** Timer state — initialised by cb_timer_init() */
static cb_timer_state_t g_timer_state;

/** Timer fault flags */
static cb_fault_flags_t g_timer_faults;

/** Initialisation flag */
static bool g_timer_initialized = false;

/*─────────────────────────────────────────────────────────────────────────────
 * Backend Names
 *─────────────────────────────────────────────────────────────────────────────*/

static const char *g_timer_names[] = {
    "auto",
    "posix (CLOCK_MONOTONIC)",
    "x86_64 (RDTSC)",
    "arm64 (CNTVCT_EL0)",
    "risc-v (cycle CSR)"
};

/*─────────────────────────────────────────────────────────────────────────────
 * POSIX Backend (SRS-001-TIMER §5.1)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Read POSIX CLOCK_MONOTONIC time
 *
 * @return Timestamp in nanoseconds, or 0 on failure
 *
 * @satisfies TIMER-P-001, TIMER-P-002
 * @traceability SRS-001-TIMER §5.1
 */
static uint64_t posix_now_ns(void)
{
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        g_timer_faults.timer_error = 1;
        return 0;
    }

    /* Convert to nanoseconds — overflow safe for ~584 years */
    return (uint64_t)ts.tv_sec * CB_NS_PER_SEC + (uint64_t)ts.tv_nsec;
}

/**
 * @brief Get POSIX clock resolution
 *
 * @return Resolution in nanoseconds, or 1 on failure
 *
 * @traceability SRS-001-TIMER §5.1
 */
static uint64_t posix_resolution_ns(void)
{
    struct timespec ts;

    if (clock_getres(CLOCK_MONOTONIC, &ts) != 0) {
        return 1;  /* Assume 1ns if query fails */
    }

    uint64_t res = (uint64_t)ts.tv_sec * CB_NS_PER_SEC + (uint64_t)ts.tv_nsec;
    return (res > 0) ? res : 1;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Platform Detection
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Check if RDTSC is available and invariant
 *
 * @return true if x86 invariant TSC is available
 *
 * @satisfies TIMER-P-012
 * @traceability SRS-001-TIMER §5.2
 */
static bool rdtsc_available(void)
{
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    /* TODO: Implement CPUID check for invariant TSC */
    /* For now, fall back to POSIX */
    return false;
#else
    return false;
#endif
}

/**
 * @brief Check if ARM64 CNTVCT is available
 *
 * @return true if ARM64 virtual counter is available
 *
 * @traceability SRS-001-TIMER §5.3
 */
static bool cntvct_available(void)
{
#if defined(__aarch64__) || defined(_M_ARM64)
    /* TODO: Implement ARM64 backend */
    return false;
#else
    return false;
#endif
}

/**
 * @brief Check if RISC-V cycle CSR is available
 *
 * @return true if RISC-V cycle counter is available
 *
 * @traceability SRS-001-TIMER §5.4
 */
static bool riscv_available(void)
{
#if defined(__riscv)
    /* TODO: Implement RISC-V backend */
    return false;
#else
    return false;
#endif
}

/*─────────────────────────────────────────────────────────────────────────────
 * Calibration
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Measure timer call overhead
 *
 * Takes minimum of many measurements to get true overhead
 * (avoiding cache/scheduling noise).
 *
 * @return Minimum observed overhead in nanoseconds
 *
 * @satisfies TIMER-NF-003, TIMER-F-033
 * @traceability SRS-001-TIMER §6.1
 */
static uint64_t calibrate_overhead(void)
{
    uint64_t min_overhead = UINT64_MAX;
    uint64_t start, end;
    int i;

    for (i = 0; i < CALIBRATION_ITERATIONS; i++) {
        start = posix_now_ns();
        end = posix_now_ns();

        if (start == 0 || end == 0) {
            continue;  /* Timer error, skip this sample */
        }

        uint64_t delta = end - start;
        if (delta < min_overhead) {
            min_overhead = delta;
        }
    }

    /* If all reads failed, return 0 */
    return (min_overhead == UINT64_MAX) ? 0 : min_overhead;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Public API Implementation (SRS-001-TIMER §7)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @satisfies TIMER-F-001, TIMER-F-002, TIMER-F-003, TIMER-F-004, TIMER-F-005
 */
cb_timer_source_t cb_timer_init(cb_timer_source_t source)
{
    /* Clear state */
    memset(&g_timer_state, 0, sizeof(g_timer_state));
    cb_fault_clear(&g_timer_faults);

    cb_timer_source_t selected = CB_TIMER_POSIX;

    /* Auto-select: priority is platform cycle counter → POSIX */
    if (source == CB_TIMER_AUTO) {
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
        if (rdtsc_available()) {
            selected = CB_TIMER_RDTSC;
        }
#elif defined(__aarch64__) || defined(_M_ARM64)
        if (cntvct_available()) {
            selected = CB_TIMER_CNTVCT;
        }
#elif defined(__riscv)
        if (riscv_available()) {
            selected = CB_TIMER_RISCV_CYCLE;
        }
#endif
        /* Default to POSIX if no cycle counter available */
    } else if (source == CB_TIMER_POSIX) {
        selected = CB_TIMER_POSIX;
    } else if (source == CB_TIMER_RDTSC && rdtsc_available()) {
        selected = CB_TIMER_RDTSC;
    } else if (source == CB_TIMER_CNTVCT && cntvct_available()) {
        selected = CB_TIMER_CNTVCT;
    } else if (source == CB_TIMER_RISCV_CYCLE && riscv_available()) {
        selected = CB_TIMER_RISCV_CYCLE;
    } else {
        /* Requested source unavailable, fall back to POSIX */
        selected = CB_TIMER_POSIX;
    }

    /* Verify selected timer works */
    uint64_t test_time = posix_now_ns();
    if (test_time == 0) {
        g_timer_faults.timer_error = 1;
        /* Continue anyway — state will reflect error */
    }

    /* Get resolution */
    g_timer_state.resolution_ns = posix_resolution_ns();

    /* Calibrate overhead */
    g_timer_state.calibration_ns = calibrate_overhead();

    /* For POSIX, frequency is not applicable */
    g_timer_state.freq_hz = 0;

    g_timer_state.source = selected;
    g_timer_initialized = true;

    return selected;
}

/**
 * @satisfies TIMER-F-010, TIMER-F-011, TIMER-F-012, TIMER-F-013, TIMER-F-014
 */
uint64_t cb_timer_now_ns(void)
{
    if (!g_timer_initialized) {
        g_timer_faults.timer_error = 1;
        return 0;
    }

    /* Currently only POSIX is implemented */
    switch (g_timer_state.source) {
    case CB_TIMER_POSIX:
        return posix_now_ns();

    case CB_TIMER_RDTSC:
        /* TODO: Implement RDTSC backend */
        return posix_now_ns();

    case CB_TIMER_CNTVCT:
        /* TODO: Implement CNTVCT backend */
        return posix_now_ns();

    case CB_TIMER_RISCV_CYCLE:
        /* TODO: Implement RISC-V backend */
        return posix_now_ns();

    default:
        g_timer_faults.timer_error = 1;
        return 0;
    }
}

/**
 * @satisfies TIMER-F-020, TIMER-F-021, TIMER-F-022
 */
uint64_t cb_timer_resolution_ns(void)
{
    if (!g_timer_initialized) {
        return 0;
    }
    return g_timer_state.resolution_ns;
}

/**
 * @satisfies TIMER-F-030, TIMER-F-031, TIMER-F-032
 */
uint64_t cb_cycles_to_ns(uint64_t cycles)
{
    if (!g_timer_initialized) {
        return 0;
    }

    /* For POSIX backend, cycles == nanoseconds */
    if (g_timer_state.freq_hz == 0) {
        return cycles;
    }

    /* For cycle counter backends: ns = cycles * 1e9 / freq_hz */
    /* Use 128-bit arithmetic to avoid overflow */
    /* cycles * CB_NS_PER_SEC / freq_hz */

    /* Split to avoid overflow: (cycles / freq_hz) * 1e9 + remainder */
    uint64_t whole_secs = cycles / g_timer_state.freq_hz;
    uint64_t remainder = cycles % g_timer_state.freq_hz;

    /* Overflow check for whole_secs * CB_NS_PER_SEC */
    if (whole_secs > UINT64_MAX / CB_NS_PER_SEC) {
        g_timer_faults.overflow = 1;
        return UINT64_MAX;
    }

    uint64_t ns_from_whole = whole_secs * CB_NS_PER_SEC;
    uint64_t ns_from_remainder = (remainder * CB_NS_PER_SEC) / g_timer_state.freq_hz;

    /* Check for final overflow */
    if (ns_from_whole > UINT64_MAX - ns_from_remainder) {
        g_timer_faults.overflow = 1;
        return UINT64_MAX;
    }

    return ns_from_whole + ns_from_remainder;
}

/**
 * @satisfies TIMER-F-040, TIMER-F-041
 */
const char *cb_timer_name(void)
{
    if (!g_timer_initialized) {
        return "uninitialised";
    }

    if ((unsigned)g_timer_state.source < sizeof(g_timer_names) / sizeof(g_timer_names[0])) {
        return g_timer_names[g_timer_state.source];
    }

    return "unknown";
}

const cb_timer_state_t *cb_timer_state(void)
{
    return &g_timer_state;
}

uint64_t cb_timer_calibration_ns(void)
{
    if (!g_timer_initialized) {
        return 0;
    }
    return g_timer_state.calibration_ns;
}

cb_fault_flags_t *cb_timer_faults(void)
{
    return &g_timer_faults;
}
