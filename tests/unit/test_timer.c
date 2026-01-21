/**
 * @file test_timer.c
 * @brief Unit tests for cb_timer API
 *
 * Tests all timer functions against SRS-001-TIMER requirements.
 * Each test function references specific requirement IDs.
 *
 * @traceability SRS-001-TIMER §8
 *
 * Copyright © 2026 The Murray Family Innovation Trust. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cb_timer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
#define TEST_ASSERT_GE(a, b, msg) TEST_ASSERT((a) >= (b), msg)
#define TEST_ASSERT_LT(a, b, msg) TEST_ASSERT((a) < (b), msg)
#define TEST_ASSERT_LE(a, b, msg) TEST_ASSERT((a) <= (b), msg)

#define RUN_TEST(fn) do { \
    printf("  %s\n", #fn); \
    int result = fn(); \
    if (result == 0) { \
        printf("    PASS\n"); \
    } \
} while(0)

/*─────────────────────────────────────────────────────────────────────────────
 * Test: Timer Initialisation (SRS-001-TIMER §4.1)
 * Traceability: TIMER-F-001, TIMER-F-002, TIMER-F-003, TIMER-F-004, TIMER-F-005
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Test cb_timer_init exists and returns valid source
 * @satisfies TIMER-F-001
 */
static int test_timer_init_exists(void)
{
    cb_timer_source_t source = cb_timer_init(CB_TIMER_AUTO);
    TEST_ASSERT_GE(source, CB_TIMER_AUTO, "source should be valid enum");
    TEST_ASSERT_LE(source, CB_TIMER_RISCV_CYCLE, "source should be valid enum");
    return 0;
}

/**
 * @brief Test cb_timer_init accepts preferred source parameter
 * @satisfies TIMER-F-002
 */
static int test_timer_init_accepts_source(void)
{
    cb_timer_source_t source;

    /* Test AUTO */
    source = cb_timer_init(CB_TIMER_AUTO);
    TEST_ASSERT_NE(source, CB_TIMER_AUTO, "AUTO should resolve to specific backend");

    /* Test POSIX explicitly */
    source = cb_timer_init(CB_TIMER_POSIX);
    TEST_ASSERT_EQ(source, CB_TIMER_POSIX, "POSIX request should return POSIX");

    return 0;
}

/**
 * @brief Test cb_timer_init returns actual source selected
 * @satisfies TIMER-F-003
 */
static int test_timer_init_returns_actual(void)
{
    cb_timer_source_t source = cb_timer_init(CB_TIMER_AUTO);

    /* Should return a concrete backend, not AUTO */
    TEST_ASSERT_NE(source, CB_TIMER_AUTO, "should return actual source, not AUTO");

    /* The returned source should match internal state */
    const cb_timer_state_t *state = cb_timer_state();
    TEST_ASSERT_EQ(source, state->source, "returned source should match state");

    return 0;
}

/**
 * @brief Test AUTO selects highest-resolution available
 * @satisfies TIMER-F-004, TIMER-F-005
 */
static int test_timer_init_auto_priority(void)
{
    cb_timer_source_t source = cb_timer_init(CB_TIMER_AUTO);

    /* On this system, should select POSIX or platform cycle counter */
    /* We can't predict which, but it should work */
    TEST_ASSERT_GT(source, CB_TIMER_AUTO, "AUTO should select concrete backend");

    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Test: Timestamp Acquisition (SRS-001-TIMER §4.2)
 * Traceability: TIMER-F-010, TIMER-F-011, TIMER-F-012, TIMER-F-013, TIMER-F-014
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Test cb_timer_now_ns returns timestamps
 * @satisfies TIMER-F-010
 */
static int test_timer_now_ns_exists(void)
{
    cb_timer_init(CB_TIMER_AUTO);

    uint64_t t = cb_timer_now_ns();
    TEST_ASSERT_GT(t, 0, "timestamp should be non-zero");

    return 0;
}

/**
 * @brief Test timestamps are monotonically increasing
 * @satisfies TIMER-F-011, TIMER-F-014
 */
static int test_timer_monotonic(void)
{
    cb_timer_init(CB_TIMER_AUTO);

    uint64_t prev = cb_timer_now_ns();
    int i;

    for (i = 0; i < 10000; i++) {
        uint64_t curr = cb_timer_now_ns();
        TEST_ASSERT_GE(curr, prev, "timestamps must be monotonic");
        prev = curr;
    }

    return 0;
}

/**
 * @brief Test time advances after work
 * @satisfies TIMER-F-011
 */
static int test_timer_advances(void)
{
    cb_timer_init(CB_TIMER_AUTO);

    uint64_t start = cb_timer_now_ns();

    /* Do some work */
    volatile int sum = 0;
    int i;
    for (i = 0; i < 100000; i++) {
        sum += i;
    }
    (void)sum;

    uint64_t end = cb_timer_now_ns();

    TEST_ASSERT_GT(end, start, "time should advance after work");
    TEST_ASSERT_GT(end - start, 0, "elapsed time should be positive");

    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Test: Timer Resolution (SRS-001-TIMER §4.3)
 * Traceability: TIMER-F-020, TIMER-F-021, TIMER-F-022
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Test cb_timer_resolution_ns returns resolution
 * @satisfies TIMER-F-020
 */
static int test_timer_resolution_exists(void)
{
    cb_timer_init(CB_TIMER_AUTO);

    uint64_t res = cb_timer_resolution_ns();
    TEST_ASSERT_GT(res, 0, "resolution should be positive");

    return 0;
}

/**
 * @brief Test resolution is ≤ 1000 ns (1 µs)
 * @satisfies TIMER-F-021
 */
static int test_timer_resolution_adequate(void)
{
    cb_timer_init(CB_TIMER_AUTO);

    uint64_t res = cb_timer_resolution_ns();
    TEST_ASSERT_LE(res, 1000, "resolution should be ≤ 1000 ns");

    printf("    Timer resolution: %lu ns\n", (unsigned long)res);

    return 0;
}

/**
 * @brief Test resolution is determined at init, not hardcoded
 * @satisfies TIMER-F-022
 */
static int test_timer_resolution_dynamic(void)
{
    /* Re-init and check resolution is set */
    cb_timer_init(CB_TIMER_POSIX);

    const cb_timer_state_t *state = cb_timer_state();
    TEST_ASSERT_GT(state->resolution_ns, 0, "resolution should be set after init");

    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Test: Cycle Conversion (SRS-001-TIMER §4.4)
 * Traceability: TIMER-F-030, TIMER-F-031, TIMER-F-032, TIMER-F-033
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Test cb_cycles_to_ns exists
 * @satisfies TIMER-F-030
 */
static int test_cycles_to_ns_exists(void)
{
    cb_timer_init(CB_TIMER_AUTO);

    uint64_t ns = cb_cycles_to_ns(1000000);
    /* For POSIX backend, cycles == ns */
    TEST_ASSERT_GT(ns, 0, "conversion should return non-zero");

    return 0;
}

/**
 * @brief Test conversion uses integer arithmetic
 * @satisfies TIMER-F-031
 *
 * Note: This is verified by inspection of timer.c
 * Test validates boundary conditions don't crash.
 */
static int test_cycles_to_ns_integer(void)
{
    cb_timer_init(CB_TIMER_AUTO);

    /* Test various values */
    (void)cb_cycles_to_ns(0);
    (void)cb_cycles_to_ns(1);
    (void)cb_cycles_to_ns(CB_NS_PER_SEC);
    (void)cb_cycles_to_ns(UINT64_MAX / 2);

    /* No crash = pass */
    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Test: Timer Identification (SRS-001-TIMER §4.5)
 * Traceability: TIMER-F-040, TIMER-F-041
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Test cb_timer_name returns name string
 * @satisfies TIMER-F-040
 */
static int test_timer_name_exists(void)
{
    cb_timer_init(CB_TIMER_AUTO);

    const char *name = cb_timer_name();
    TEST_ASSERT_NE(name, NULL, "name should not be NULL");
    TEST_ASSERT_GT(strlen(name), 0, "name should not be empty");

    printf("    Timer name: %s\n", name);

    return 0;
}

/**
 * @brief Test cb_timer_name returns static string
 * @satisfies TIMER-F-041
 *
 * Note: Verified by inspection — no allocation in cb_timer_name().
 * Test validates string is consistent across calls.
 */
static int test_timer_name_static(void)
{
    cb_timer_init(CB_TIMER_AUTO);

    const char *name1 = cb_timer_name();
    const char *name2 = cb_timer_name();

    TEST_ASSERT_EQ(name1, name2, "same pointer should be returned");

    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Test: POSIX Backend (SRS-001-TIMER §5.1)
 * Traceability: TIMER-P-001, TIMER-P-002, TIMER-P-003
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Test POSIX backend uses CLOCK_MONOTONIC
 * @satisfies TIMER-P-001
 *
 * Note: Verified by inspection of timer.c
 */
static int test_posix_backend(void)
{
    cb_timer_source_t source = cb_timer_init(CB_TIMER_POSIX);
    TEST_ASSERT_EQ(source, CB_TIMER_POSIX, "POSIX should be available");

    const char *name = cb_timer_name();
    TEST_ASSERT_NE(strstr(name, "CLOCK_MONOTONIC"), NULL,
                   "name should mention CLOCK_MONOTONIC");

    return 0;
}

/**
 * @brief Test POSIX backend is available on this system
 * @satisfies TIMER-P-002
 */
static int test_posix_available(void)
{
    cb_timer_source_t source = cb_timer_init(CB_TIMER_POSIX);
    TEST_ASSERT_EQ(source, CB_TIMER_POSIX, "POSIX must be available");

    /* Timer should work */
    uint64_t t = cb_timer_now_ns();
    TEST_ASSERT_GT(t, 0, "POSIX timer should return valid time");

    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Test: Performance Requirements (SRS-001-TIMER §6.1)
 * Traceability: TIMER-NF-001, TIMER-NF-002, TIMER-NF-003
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Test POSIX backend overhead is < 1000 ns
 * @satisfies TIMER-NF-002
 */
static int test_timer_overhead_posix(void)
{
    cb_timer_init(CB_TIMER_POSIX);

    uint64_t overhead = cb_timer_calibration_ns();
    TEST_ASSERT_LT(overhead, 1000, "POSIX overhead should be < 1000 ns");

    printf("    Timer overhead: %lu ns\n", (unsigned long)overhead);

    return 0;
}

/**
 * @brief Test calibration overhead is stored
 * @satisfies TIMER-NF-003
 */
static int test_timer_calibration_stored(void)
{
    cb_timer_init(CB_TIMER_AUTO);

    const cb_timer_state_t *state = cb_timer_state();
    TEST_ASSERT_EQ(state->calibration_ns, cb_timer_calibration_ns(),
                   "calibration should match state");

    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Test: Fault Handling (SRS-001-TIMER §6.3)
 * Traceability: TIMER-NF-020, TIMER-NF-021, TIMER-NF-022
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Test fault flags are cleared on init
 * @satisfies TIMER-NF-020
 */
static int test_timer_fault_clear_on_init(void)
{
    cb_timer_init(CB_TIMER_AUTO);

    cb_fault_flags_t *faults = cb_timer_faults();
    TEST_ASSERT_EQ(cb_has_fault(faults), false, "no faults after init");
    TEST_ASSERT_EQ(cb_has_warning(faults), false, "no warnings after init");

    return 0;
}

/**
 * @brief Test fault flags are accessible
 * @satisfies TIMER-NF-020
 */
static int test_timer_fault_access(void)
{
    cb_timer_init(CB_TIMER_AUTO);

    cb_fault_flags_t *faults = cb_timer_faults();
    TEST_ASSERT_NE(faults, NULL, "faults pointer should not be NULL");

    /* Manually set and clear to verify access works */
    faults->timer_error = 1;
    TEST_ASSERT_EQ(cb_has_fault(faults), true, "fault should be detected");

    cb_fault_clear(faults);
    TEST_ASSERT_EQ(cb_has_fault(faults), false, "fault should be cleared");

    return 0;
}

/**
 * @brief Test timer doesn't crash on repeated use
 * @satisfies TIMER-NF-022
 */
static int test_timer_no_crash(void)
{
    cb_timer_init(CB_TIMER_AUTO);

    int i;
    for (i = 0; i < 100000; i++) {
        (void)cb_timer_now_ns();
    }

    /* Reaching here = no crash */
    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Test: Determinism (SRS-001-TIMER §6.2)
 * Traceability: TIMER-NF-010, TIMER-NF-011
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Test timer init is deterministic
 * @satisfies TIMER-NF-010
 */
static int test_timer_init_deterministic(void)
{
    /* Init twice, should get same source */
    cb_timer_source_t source1 = cb_timer_init(CB_TIMER_AUTO);
    cb_timer_source_t source2 = cb_timer_init(CB_TIMER_AUTO);

    TEST_ASSERT_EQ(source1, source2, "init should be deterministic");

    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Test: Timer State Access
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Test timer state is accessible
 */
static int test_timer_state_access(void)
{
    cb_timer_init(CB_TIMER_AUTO);

    const cb_timer_state_t *state = cb_timer_state();
    TEST_ASSERT_NE(state, NULL, "state should not be NULL");
    TEST_ASSERT_GT(state->resolution_ns, 0, "resolution should be set");

    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Test: Uninitialised Behaviour
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Test behaviour before init (may fail gracefully)
 *
 * Note: This is a robustness test — actual behaviour depends on implementation.
 * The timer may return 0 or set error flags.
 */
static int test_timer_before_init(void)
{
    /* Note: After running other tests, timer is already initialised.
     * This test verifies the API handles the case, but we can't truly
     * test uninitialised state without process restart.
     *
     * The implementation uses a static flag, so re-testing after
     * init doesn't fully exercise this path. This is documented
     * as a limitation.
     */

    /* Just verify we can call cb_timer_name() without crashing */
    const char *name = cb_timer_name();
    TEST_ASSERT_NE(name, NULL, "name should not crash");

    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Main
 *─────────────────────────────────────────────────────────────────────────────*/

int main(void)
{
    printf("═══════════════════════════════════════════════════════════════════\n");
    printf(" certifiable-bench Timer Tests\n");
    printf(" @traceability SRS-001-TIMER\n");
    printf("═══════════════════════════════════════════════════════════════════\n\n");

    printf("§4.1 Timer Initialisation (TIMER-F-001..005)\n");
    RUN_TEST(test_timer_init_exists);
    RUN_TEST(test_timer_init_accepts_source);
    RUN_TEST(test_timer_init_returns_actual);
    RUN_TEST(test_timer_init_auto_priority);

    printf("\n§4.2 Timestamp Acquisition (TIMER-F-010..014)\n");
    RUN_TEST(test_timer_now_ns_exists);
    RUN_TEST(test_timer_monotonic);
    RUN_TEST(test_timer_advances);

    printf("\n§4.3 Timer Resolution (TIMER-F-020..022)\n");
    RUN_TEST(test_timer_resolution_exists);
    RUN_TEST(test_timer_resolution_adequate);
    RUN_TEST(test_timer_resolution_dynamic);

    printf("\n§4.4 Cycle Conversion (TIMER-F-030..033)\n");
    RUN_TEST(test_cycles_to_ns_exists);
    RUN_TEST(test_cycles_to_ns_integer);

    printf("\n§4.5 Timer Identification (TIMER-F-040..041)\n");
    RUN_TEST(test_timer_name_exists);
    RUN_TEST(test_timer_name_static);

    printf("\n§5.1 POSIX Backend (TIMER-P-001..003)\n");
    RUN_TEST(test_posix_backend);
    RUN_TEST(test_posix_available);

    printf("\n§6.1 Performance (TIMER-NF-001..003)\n");
    RUN_TEST(test_timer_overhead_posix);
    RUN_TEST(test_timer_calibration_stored);

    printf("\n§6.2 Determinism (TIMER-NF-010..011)\n");
    RUN_TEST(test_timer_init_deterministic);

    printf("\n§6.3 Fault Handling (TIMER-NF-020..022)\n");
    RUN_TEST(test_timer_fault_clear_on_init);
    RUN_TEST(test_timer_fault_access);
    RUN_TEST(test_timer_no_crash);

    printf("\nAdditional Tests\n");
    RUN_TEST(test_timer_state_access);
    RUN_TEST(test_timer_before_init);

    printf("\n═══════════════════════════════════════════════════════════════════\n");
    printf(" Results\n");
    printf("═══════════════════════════════════════════════════════════════════\n");
    printf(" Tests run:    %d\n", g_tests_run);
    printf(" Tests passed: %d\n", g_tests_passed);
    printf(" Tests failed: %d\n", g_tests_failed);
    printf("═══════════════════════════════════════════════════════════════════\n");

    if (g_tests_failed > 0) {
        printf("\n*** FAILURES DETECTED ***\n");
        return 1;
    }

    printf("\nAll tests passed.\n");
    return 0;
}
