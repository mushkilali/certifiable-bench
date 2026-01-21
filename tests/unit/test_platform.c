/**
 * @file test_platform.c
 * @brief Unit tests for cb_platform API
 *
 * Tests all platform functions against SRS-006-PLATFORM requirements.
 * Note: Some tests may have reduced functionality on non-Linux systems.
 *
 * @traceability SRS-006-PLATFORM §9
 *
 * Copyright © 2026 The Murray Family Innovation Trust. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cb_platform.h"
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
 * Test: Platform Identification (SRS-006-PLATFORM §4.1)
 * Traceability: PLATFORM-F-001 through PLATFORM-F-010
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Test cb_platform_name returns valid platform string
 * @satisfies PLATFORM-F-001 through PLATFORM-F-005
 */
static int test_platform_name(void)
{
    const char *name = cb_platform_name();
    
    TEST_ASSERT_NE(name, NULL, "platform name should not be NULL");
    TEST_ASSERT_GT(strlen(name), 0, "platform name should not be empty");
    
    /* Should be one of the known platforms */
    int known = (strcmp(name, "x86_64") == 0 ||
                 strcmp(name, "aarch64") == 0 ||
                 strcmp(name, "riscv64") == 0 ||
                 strcmp(name, "riscv32") == 0 ||
                 strcmp(name, "i386") == 0 ||
                 strcmp(name, "arm") == 0 ||
                 strcmp(name, "unknown") == 0);
    
    TEST_ASSERT(known, "platform name should be a known value");
    
    printf("    Platform: %s\n", name);
    
    return 0;
}

/**
 * @brief Test cb_platform_name is consistent
 */
static int test_platform_name_consistent(void)
{
    const char *name1 = cb_platform_name();
    const char *name2 = cb_platform_name();
    
    TEST_ASSERT_EQ(name1, name2, "platform name should be consistent");
    
    return 0;
}

/**
 * @brief Test cb_cpu_model returns model string
 * @satisfies PLATFORM-F-006 through PLATFORM-F-009
 */
static int test_cpu_model(void)
{
    char buffer[128];
    cb_result_code_t rc;
    
    rc = cb_cpu_model(buffer, sizeof(buffer));
    
    TEST_ASSERT_EQ(rc, CB_OK, "cpu_model should succeed");
    TEST_ASSERT_GT(strlen(buffer), 0, "cpu model should not be empty");
    
    printf("    CPU Model: %s\n", buffer);
    
    return 0;
}

/**
 * @brief Test cb_cpu_model handles NULL
 * @satisfies PLATFORM-F-006
 */
static int test_cpu_model_null(void)
{
    cb_result_code_t rc;
    
    rc = cb_cpu_model(NULL, 128);
    TEST_ASSERT_EQ(rc, CB_ERR_NULL_PTR, "NULL buffer should return error");
    
    return 0;
}

/**
 * @brief Test cb_cpu_model truncates long strings
 * @satisfies PLATFORM-F-009
 */
static int test_cpu_model_truncate(void)
{
    char buffer[16];  /* Small buffer */
    cb_result_code_t rc;
    
    rc = cb_cpu_model(buffer, sizeof(buffer));
    
    TEST_ASSERT_EQ(rc, CB_OK, "cpu_model should succeed with small buffer");
    TEST_ASSERT_LT(strlen(buffer), sizeof(buffer), "should not overflow buffer");
    TEST_ASSERT_EQ(buffer[sizeof(buffer) - 1], '\0', "should be null-terminated");
    
    return 0;
}

/**
 * @brief Test cb_cpu_freq_mhz returns frequency
 * @satisfies PLATFORM-F-010
 */
static int test_cpu_freq(void)
{
    uint32_t freq = cb_cpu_freq_mhz();
    
    /* Frequency may be 0 if unavailable, that's okay */
    if (freq > 0) {
        /* Sanity check: should be between 100 MHz and 10 GHz */
        TEST_ASSERT_GE(freq, 100, "frequency should be >= 100 MHz");
        TEST_ASSERT_LE(freq, 10000, "frequency should be <= 10000 MHz");
        
        printf("    CPU Frequency: %u MHz\n", freq);
    } else {
        printf("    CPU Frequency: unavailable\n");
    }
    
    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Test: Hardware Performance Counters (SRS-006-PLATFORM §4.2)
 * Traceability: PLATFORM-F-020 through PLATFORM-F-027
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Test cb_hwcounters_available
 * @satisfies PLATFORM-F-020
 */
static int test_hwcounters_available(void)
{
    bool avail = cb_hwcounters_available();
    
    /* Just test that it returns without crashing */
    printf("    Hardware counters: %s\n", avail ? "available" : "unavailable");
    
    return 0;
}

/**
 * @brief Test hardware counter start/stop cycle
 * @satisfies PLATFORM-F-021, PLATFORM-F-022
 */
static int test_hwcounters_cycle(void)
{
    cb_hwcounters_t counters;
    cb_result_code_t rc;
    
    if (!cb_hwcounters_available()) {
        printf("    (skipped - counters unavailable)\n");
        return 0;
    }
    
    /* Start */
    rc = cb_hwcounters_start();
    TEST_ASSERT_EQ(rc, CB_OK, "hwcounters_start should succeed");
    
    /* Do some work */
    volatile int sum = 0;
    for (int i = 0; i < 100000; i++) {
        sum += i;
    }
    (void)sum;
    
    /* Stop */
    rc = cb_hwcounters_stop(&counters);
    TEST_ASSERT_EQ(rc, CB_OK, "hwcounters_stop should succeed");
    
    /* Check counters */
    TEST_ASSERT_EQ(counters.available, true, "counters should be available");
    TEST_ASSERT_GT(counters.cycles, 0, "should have counted cycles");
    TEST_ASSERT_GT(counters.instructions, 0, "should have counted instructions");
    
    printf("    Cycles: %lu, Instructions: %lu, IPC: %u.%02u\n",
           (unsigned long)counters.cycles,
           (unsigned long)counters.instructions,
           counters.ipc_q16 >> 16,
           ((counters.ipc_q16 & 0xFFFF) * 100) >> 16);
    
    return 0;
}

/**
 * @brief Test cb_hwcounters_stop NULL handling
 * @satisfies PLATFORM-F-022
 */
static int test_hwcounters_stop_null(void)
{
    cb_result_code_t rc;
    
    rc = cb_hwcounters_stop(NULL);
    TEST_ASSERT_EQ(rc, CB_ERR_NULL_PTR, "NULL counters should return error");
    
    return 0;
}

/**
 * @brief Test counter access failure doesn't crash
 * @satisfies PLATFORM-F-027
 */
static int test_hwcounters_graceful(void)
{
    cb_hwcounters_t counters;
    
    /* Even if counters unavailable, stop should not crash */
    (void)cb_hwcounters_stop(&counters);
    
    /* available should be set appropriately */
    if (!cb_hwcounters_available()) {
        TEST_ASSERT_EQ(counters.available, false, "should report unavailable");
    }
    
    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Test: Environmental Monitoring (SRS-006-PLATFORM §4.6)
 * Traceability: PLATFORM-F-060 through PLATFORM-F-068
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Test cb_env_snapshot captures environmental state
 * @satisfies PLATFORM-F-060 through PLATFORM-F-064
 */
static int test_env_snapshot(void)
{
    cb_env_snapshot_t snapshot;
    cb_result_code_t rc;
    
    rc = cb_env_snapshot(&snapshot);
    
    TEST_ASSERT_EQ(rc, CB_OK, "env_snapshot should succeed");
    TEST_ASSERT_GT(snapshot.timestamp_ns, 0, "should have timestamp");
    
    printf("    Timestamp: %lu ns\n", (unsigned long)snapshot.timestamp_ns);
    printf("    CPU Freq: %lu Hz\n", (unsigned long)snapshot.cpu_freq_hz);
    printf("    CPU Temp: %d mC (%d.%d°C)\n",
           snapshot.cpu_temp_mC,
           snapshot.cpu_temp_mC / 1000,
           (snapshot.cpu_temp_mC % 1000) / 100);
    printf("    Throttle count: %u\n", snapshot.throttle_count);
    
    return 0;
}

/**
 * @brief Test cb_env_snapshot NULL handling
 */
static int test_env_snapshot_null(void)
{
    cb_result_code_t rc;
    
    rc = cb_env_snapshot(NULL);
    TEST_ASSERT_EQ(rc, CB_ERR_NULL_PTR, "NULL snapshot should return error");
    
    return 0;
}

/**
 * @brief Test cb_env_compute_stats
 * @satisfies PLATFORM-F-065 through PLATFORM-F-068
 */
static int test_env_compute_stats(void)
{
    cb_env_snapshot_t start, end;
    cb_env_stats_t stats;
    cb_result_code_t rc;
    
    /* Take start snapshot */
    rc = cb_env_snapshot(&start);
    TEST_ASSERT_EQ(rc, CB_OK, "start snapshot should succeed");
    
    /* Do some work to let time pass */
    volatile int sum = 0;
    for (int i = 0; i < 1000000; i++) {
        sum += i;
    }
    (void)sum;
    
    /* Take end snapshot */
    rc = cb_env_snapshot(&end);
    TEST_ASSERT_EQ(rc, CB_OK, "end snapshot should succeed");
    
    /* Compute stats */
    rc = cb_env_compute_stats(&start, &end, &stats);
    TEST_ASSERT_EQ(rc, CB_OK, "compute_stats should succeed");
    
    /* Verify snapshots are copied */
    TEST_ASSERT_EQ(stats.start.timestamp_ns, start.timestamp_ns, "start should be copied");
    TEST_ASSERT_EQ(stats.end.timestamp_ns, end.timestamp_ns, "end should be copied");
    
    /* Min/max should be set */
    TEST_ASSERT_LE(stats.min_freq_hz, stats.max_freq_hz, "min <= max freq");
    TEST_ASSERT_LE(stats.min_temp_mC, stats.max_temp_mC, "min <= max temp");
    
    return 0;
}

/**
 * @brief Test cb_env_compute_stats NULL handling
 */
static int test_env_compute_stats_null(void)
{
    cb_env_snapshot_t snapshot;
    cb_env_stats_t stats;
    cb_result_code_t rc;
    
    (void)cb_env_snapshot(&snapshot);
    
    rc = cb_env_compute_stats(NULL, &snapshot, &stats);
    TEST_ASSERT_EQ(rc, CB_ERR_NULL_PTR, "NULL start should return error");
    
    rc = cb_env_compute_stats(&snapshot, NULL, &stats);
    TEST_ASSERT_EQ(rc, CB_ERR_NULL_PTR, "NULL end should return error");
    
    rc = cb_env_compute_stats(&snapshot, &snapshot, NULL);
    TEST_ASSERT_EQ(rc, CB_ERR_NULL_PTR, "NULL stats should return error");
    
    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Test: Stability Assessment (SRS-006-PLATFORM §4.9)
 * Traceability: PLATFORM-F-090 through PLATFORM-F-094
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Test cb_env_check_stable with stable environment
 * @satisfies PLATFORM-F-090, PLATFORM-F-091, PLATFORM-F-092
 */
static int test_env_stable(void)
{
    cb_env_stats_t stats;
    
    memset(&stats, 0, sizeof(stats));
    
    /* Stable: same frequency, no throttle */
    stats.start.cpu_freq_hz = 3000000000;  /* 3 GHz */
    stats.end.cpu_freq_hz = 3000000000;
    stats.total_throttle_events = 0;
    
    TEST_ASSERT_EQ(cb_env_check_stable(&stats), true, "same freq should be stable");
    
    /* Stable: small frequency drop (< 5%) */
    stats.end.cpu_freq_hz = 2900000000;  /* 2.9 GHz = 3.3% drop */
    TEST_ASSERT_EQ(cb_env_check_stable(&stats), true, "< 5% drop should be stable");
    
    return 0;
}

/**
 * @brief Test cb_env_check_stable with unstable environment
 * @satisfies PLATFORM-F-091, PLATFORM-F-092
 */
static int test_env_unstable(void)
{
    cb_env_stats_t stats;
    
    memset(&stats, 0, sizeof(stats));
    
    /* Unstable: frequency drop > 5% */
    stats.start.cpu_freq_hz = 3000000000;  /* 3 GHz */
    stats.end.cpu_freq_hz = 2800000000;    /* 2.8 GHz = 6.7% drop */
    stats.total_throttle_events = 0;
    
    TEST_ASSERT_EQ(cb_env_check_stable(&stats), false, "> 5% drop should be unstable");
    
    /* Unstable: throttle events */
    stats.end.cpu_freq_hz = 3000000000;  /* Same freq */
    stats.total_throttle_events = 1;
    
    TEST_ASSERT_EQ(cb_env_check_stable(&stats), false, "throttle should be unstable");
    
    return 0;
}

/**
 * @brief Test cb_env_check_stable with no frequency data
 */
static int test_env_stable_no_data(void)
{
    cb_env_stats_t stats;
    
    memset(&stats, 0, sizeof(stats));
    
    /* No frequency data - should assume stable (graceful degradation) */
    stats.start.cpu_freq_hz = 0;
    stats.end.cpu_freq_hz = 0;
    stats.total_throttle_events = 0;
    
    TEST_ASSERT_EQ(cb_env_check_stable(&stats), true, "no data should assume stable");
    
    return 0;
}

/**
 * @brief Test cb_env_check_stable NULL handling
 */
static int test_env_stable_null(void)
{
    TEST_ASSERT_EQ(cb_env_check_stable(NULL), false, "NULL should return false");
    
    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Test: Robustness (SRS-006-PLATFORM §5.2)
 * Traceability: PLATFORM-NF-010 through PLATFORM-NF-012
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Test all platform functions handle missing data gracefully
 * @satisfies PLATFORM-NF-010, PLATFORM-NF-011
 */
static int test_graceful_failure(void)
{
    /* These should all complete without crashing, even on minimal systems */
    
    const char *name = cb_platform_name();
    TEST_ASSERT_NE(name, NULL, "platform_name should not crash");
    
    char buffer[64];
    (void)cb_cpu_model(buffer, sizeof(buffer));
    /* Should not crash even if cpuinfo unavailable */
    
    (void)cb_cpu_freq_mhz();
    /* Should return 0 if unavailable, not crash */
    
    (void)cb_hwcounters_available();
    /* Should return false if unavailable, not crash */
    
    cb_env_snapshot_t snapshot;
    (void)cb_env_snapshot(&snapshot);
    /* Should succeed with zeros if data unavailable */
    
    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Test: Performance (SRS-006-PLATFORM §5.1)
 * Traceability: PLATFORM-NF-001, PLATFORM-NF-003
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Test cb_env_snapshot overhead
 * @satisfies PLATFORM-NF-001
 */
static int test_env_snapshot_overhead(void)
{
    cb_env_snapshot_t snapshot;
    uint64_t start, end;
    uint64_t min_overhead = UINT64_MAX;
    int i;
    
    /* Measure overhead over many iterations */
    for (i = 0; i < 100; i++) {
        start = cb_timer_now_ns();
        (void)cb_env_snapshot(&snapshot);
        end = cb_timer_now_ns();
        
        uint64_t overhead = end - start;
        if (overhead < min_overhead) {
            min_overhead = overhead;
        }
    }
    
    /* Should be < 1 ms = 1,000,000 ns */
    TEST_ASSERT_LT(min_overhead, 1000000, "snapshot overhead should be < 1 ms");
    
    printf("    Snapshot overhead: %lu ns\n", (unsigned long)min_overhead);
    
    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Main
 *─────────────────────────────────────────────────────────────────────────────*/

int main(void)
{
    printf("═══════════════════════════════════════════════════════════════════\n");
    printf(" certifiable-bench Platform Tests\n");
    printf(" @traceability SRS-006-PLATFORM\n");
    printf("═══════════════════════════════════════════════════════════════════\n\n");
    
    /* Initialise timer (required for env_snapshot) */
    cb_timer_init(CB_TIMER_AUTO);
    
    /* Initialise platform */
    cb_platform_init();
    
    printf("§4.1 Platform Identification (PLATFORM-F-001..010)\n");
    RUN_TEST(test_platform_name);
    RUN_TEST(test_platform_name_consistent);
    RUN_TEST(test_cpu_model);
    RUN_TEST(test_cpu_model_null);
    RUN_TEST(test_cpu_model_truncate);
    RUN_TEST(test_cpu_freq);
    
    printf("\n§4.2 Hardware Counters (PLATFORM-F-020..027)\n");
    RUN_TEST(test_hwcounters_available);
    RUN_TEST(test_hwcounters_cycle);
    RUN_TEST(test_hwcounters_stop_null);
    RUN_TEST(test_hwcounters_graceful);
    
    printf("\n§4.6 Environmental Monitoring (PLATFORM-F-060..068)\n");
    RUN_TEST(test_env_snapshot);
    RUN_TEST(test_env_snapshot_null);
    RUN_TEST(test_env_compute_stats);
    RUN_TEST(test_env_compute_stats_null);
    
    printf("\n§4.9 Stability Assessment (PLATFORM-F-090..094)\n");
    RUN_TEST(test_env_stable);
    RUN_TEST(test_env_unstable);
    RUN_TEST(test_env_stable_no_data);
    RUN_TEST(test_env_stable_null);
    
    printf("\n§5.1 Performance (PLATFORM-NF-001..003)\n");
    RUN_TEST(test_env_snapshot_overhead);
    
    printf("\n§5.2 Robustness (PLATFORM-NF-010..012)\n");
    RUN_TEST(test_graceful_failure);
    
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
