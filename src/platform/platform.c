/**
 * @file platform.c
 * @brief Platform detection and environmental monitoring implementation
 *
 * Implements SRS-006-PLATFORM requirements for platform identification,
 * hardware counters, and environmental monitoring.
 *
 * @traceability SRS-006-PLATFORM, CB-MATH-001 §9
 *
 * Copyright © 2026 The Murray Family Innovation Trust. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#define _GNU_SOURCE  /* For some Linux-specific features */

#include "cb_platform.h"
#include "cb_timer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef __linux__
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#endif

/*─────────────────────────────────────────────────────────────────────────────
 * Constants
 *─────────────────────────────────────────────────────────────────────────────*/

/** Maximum line length when reading system files */
#define MAX_LINE_LEN 256

/** Number of hardware counters we track */
#define NUM_HW_COUNTERS 6

/*─────────────────────────────────────────────────────────────────────────────
 * Linux System File Paths (SRS-006-PLATFORM §7.1)
 *─────────────────────────────────────────────────────────────────────────────*/

#ifdef __linux__
static const char *CPUINFO_PATH = "/proc/cpuinfo";
static const char *CPU_FREQ_PATH = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq";
static const char *CPU_TEMP_PATH = "/sys/class/thermal/thermal_zone0/temp";
static const char *THROTTLE_PATH = "/sys/devices/system/cpu/cpu0/thermal_throttle/core_throttle_count";
#endif

/*─────────────────────────────────────────────────────────────────────────────
 * Global State
 *─────────────────────────────────────────────────────────────────────────────*/

static bool g_platform_initialized = false;

#ifdef __linux__
/** File descriptors for perf_event counters */
static int g_perf_fds[NUM_HW_COUNTERS] = {-1, -1, -1, -1, -1, -1};
static bool g_hwcounters_started = false;
static bool g_hwcounters_available = false;
#endif

/*─────────────────────────────────────────────────────────────────────────────
 * Helper: Read integer from sysfs file
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Read a single integer value from a sysfs file
 * @param path File path
 * @param value Output value
 * @return true on success
 */
static bool read_sysfs_int(const char *path, int64_t *value)
{
    FILE *fp;
    char line[MAX_LINE_LEN];
    
    fp = fopen(path, "r");
    if (fp == NULL) {
        return false;
    }
    
    if (fgets(line, sizeof(line), fp) == NULL) {
        fclose(fp);
        return false;
    }
    
    fclose(fp);
    
    *value = strtoll(line, NULL, 10);
    return true;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Platform Identification (SRS-006-PLATFORM §4.1)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @satisfies PLATFORM-F-001 through PLATFORM-F-005
 */
const char *cb_platform_name(void)
{
#if defined(__x86_64__) || defined(_M_X64) || defined(__amd64__)
    return "x86_64";
#elif defined(__aarch64__) || defined(_M_ARM64)
    return "aarch64";
#elif defined(__riscv) && (__riscv_xlen == 64)
    return "riscv64";
#elif defined(__riscv) && (__riscv_xlen == 32)
    return "riscv32";
#elif defined(__i386__) || defined(_M_IX86)
    return "i386";
#elif defined(__arm__)
    return "arm";
#else
    return "unknown";
#endif
}

/**
 * @satisfies PLATFORM-F-006 through PLATFORM-F-009
 */
cb_result_code_t cb_cpu_model(char *buffer, uint32_t size)
{
    if (buffer == NULL || size == 0) {
        return CB_ERR_NULL_PTR;
    }
    
    buffer[0] = '\0';
    
#ifdef __linux__
    /* Read from /proc/cpuinfo (PLATFORM-F-007) */
    FILE *fp = fopen(CPUINFO_PATH, "r");
    if (fp == NULL) {
        strncpy(buffer, "unknown", size - 1);
        buffer[size - 1] = '\0';
        return CB_OK;
    }
    
    char line[MAX_LINE_LEN];
    while (fgets(line, sizeof(line), fp) != NULL) {
        /* Look for "model name" on x86 or "Model" on ARM */
        if (strncmp(line, "model name", 10) == 0 ||
            strncmp(line, "Model", 5) == 0) {
            char *colon = strchr(line, ':');
            if (colon != NULL) {
                /* Skip colon and whitespace */
                colon++;
                while (*colon == ' ' || *colon == '\t') colon++;
                
                /* Remove trailing newline */
                size_t len = strlen(colon);
                if (len > 0 && colon[len - 1] == '\n') {
                    colon[len - 1] = '\0';
                }
                
                /* Copy to buffer, truncating if necessary (PLATFORM-F-009) */
                strncpy(buffer, colon, size - 1);
                buffer[size - 1] = '\0';
                break;
            }
        }
    }
    
    fclose(fp);
    
    if (buffer[0] == '\0') {
        strncpy(buffer, "unknown", size - 1);
        buffer[size - 1] = '\0';
    }
    
#elif defined(__APPLE__)
    /* macOS: use sysctl (PLATFORM-F-008) */
    FILE *fp = popen("sysctl -n machdep.cpu.brand_string 2>/dev/null", "r");
    if (fp != NULL) {
        if (fgets(buffer, (int)size, fp) != NULL) {
            /* Remove trailing newline */
            size_t len = strlen(buffer);
            if (len > 0 && buffer[len - 1] == '\n') {
                buffer[len - 1] = '\0';
            }
        }
        pclose(fp);
    }
    
    if (buffer[0] == '\0') {
        strncpy(buffer, "unknown", size - 1);
        buffer[size - 1] = '\0';
    }
    
#else
    strncpy(buffer, "unknown", size - 1);
    buffer[size - 1] = '\0';
#endif
    
    return CB_OK;
}

/**
 * @satisfies PLATFORM-F-010
 */
uint32_t cb_cpu_freq_mhz(void)
{
#ifdef __linux__
    int64_t freq_khz;
    
    if (read_sysfs_int(CPU_FREQ_PATH, &freq_khz)) {
        /* scaling_cur_freq is in kHz, convert to MHz */
        return (uint32_t)(freq_khz / 1000);
    }
    
    /* Fallback: try /proc/cpuinfo */
    FILE *fp = fopen(CPUINFO_PATH, "r");
    if (fp != NULL) {
        char line[MAX_LINE_LEN];
        while (fgets(line, sizeof(line), fp) != NULL) {
            if (strncmp(line, "cpu MHz", 7) == 0) {
                char *colon = strchr(line, ':');
                if (colon != NULL) {
                    double mhz = strtod(colon + 1, NULL);
                    fclose(fp);
                    return (uint32_t)mhz;
                }
            }
        }
        fclose(fp);
    }
    
#elif defined(__APPLE__)
    FILE *fp = popen("sysctl -n hw.cpufrequency 2>/dev/null", "r");
    if (fp != NULL) {
        char line[MAX_LINE_LEN];
        if (fgets(line, sizeof(line), fp) != NULL) {
            int64_t freq_hz = strtoll(line, NULL, 10);
            pclose(fp);
            return (uint32_t)(freq_hz / 1000000);
        }
        pclose(fp);
    }
#endif
    
    return 0;  /* Unavailable */
}

/*─────────────────────────────────────────────────────────────────────────────
 * Hardware Performance Counters (SRS-006-PLATFORM §4.2)
 *─────────────────────────────────────────────────────────────────────────────*/

#ifdef __linux__
/**
 * @brief Open a perf_event counter
 */
static int perf_event_open(struct perf_event_attr *attr, pid_t pid,
                           int cpu, int group_fd, unsigned long flags)
{
    return (int)syscall(__NR_perf_event_open, attr, pid, cpu, group_fd, flags);
}

/**
 * @brief Try to open hardware counters
 */
static bool try_open_hwcounters(void)
{
    struct perf_event_attr pe;
    int i;
    
    /* Counter configurations */
    static const struct {
        uint32_t type;
        uint64_t config;
    } counter_configs[NUM_HW_COUNTERS] = {
        { PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES },
        { PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS },
        { PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_REFERENCES },
        { PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES },
        { PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_INSTRUCTIONS },
        { PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_MISSES }
    };
    
    /* Close any existing counters */
    for (i = 0; i < NUM_HW_COUNTERS; i++) {
        if (g_perf_fds[i] >= 0) {
            close(g_perf_fds[i]);
            g_perf_fds[i] = -1;
        }
    }
    
    /* Open each counter */
    for (i = 0; i < NUM_HW_COUNTERS; i++) {
        memset(&pe, 0, sizeof(pe));
        pe.type = counter_configs[i].type;
        pe.config = counter_configs[i].config;
        pe.size = sizeof(pe);
        pe.disabled = 1;
        pe.exclude_kernel = 1;
        pe.exclude_hv = 1;
        
        g_perf_fds[i] = perf_event_open(&pe, 0, -1, -1, 0);
        
        if (g_perf_fds[i] < 0) {
            /* Clean up and return failure */
            int j;
            for (j = 0; j < i; j++) {
                close(g_perf_fds[j]);
                g_perf_fds[j] = -1;
            }
            return false;
        }
    }
    
    return true;
}
#endif

/**
 * @satisfies PLATFORM-F-020
 */
bool cb_hwcounters_available(void)
{
#ifdef __linux__
    return g_hwcounters_available;
#else
    return false;
#endif
}

/**
 * @satisfies PLATFORM-F-021
 */
cb_result_code_t cb_hwcounters_start(void)
{
#ifdef __linux__
    int i;
    
    if (!g_hwcounters_available) {
        return CB_ERR_HWCOUNTERS;
    }
    
    if (g_hwcounters_started) {
        return CB_ERR_HWCOUNTERS;  /* Already started */
    }
    
    /* Reset and enable all counters */
    for (i = 0; i < NUM_HW_COUNTERS; i++) {
        if (g_perf_fds[i] >= 0) {
            ioctl(g_perf_fds[i], PERF_EVENT_IOC_RESET, 0);
            ioctl(g_perf_fds[i], PERF_EVENT_IOC_ENABLE, 0);
        }
    }
    
    g_hwcounters_started = true;
    return CB_OK;
#else
    return CB_ERR_HWCOUNTERS;
#endif
}

/**
 * @satisfies PLATFORM-F-022 through PLATFORM-F-026
 */
cb_result_code_t cb_hwcounters_stop(cb_hwcounters_t *counters)
{
    if (counters == NULL) {
        return CB_ERR_NULL_PTR;
    }
    
    /* Clear output */
    memset(counters, 0, sizeof(*counters));
    
#ifdef __linux__
    int i;
    uint64_t values[NUM_HW_COUNTERS] = {0};
    
    if (!g_hwcounters_started) {
        counters->available = false;
        return CB_ERR_HWCOUNTERS;
    }
    
    /* Disable and read all counters */
    for (i = 0; i < NUM_HW_COUNTERS; i++) {
        if (g_perf_fds[i] >= 0) {
            ioctl(g_perf_fds[i], PERF_EVENT_IOC_DISABLE, 0);
            
            ssize_t ret = read(g_perf_fds[i], &values[i], sizeof(uint64_t));
            if (ret != sizeof(uint64_t)) {
                values[i] = 0;
            }
        }
    }
    
    g_hwcounters_started = false;
    
    /* Populate output structure */
    counters->available = true;
    counters->cycles = values[0];
    counters->instructions = values[1];
    counters->cache_refs = values[2];
    counters->cache_misses = values[3];
    counters->branch_refs = values[4];
    counters->branch_misses = values[5];
    
    /* Compute IPC in Q16.16 (PLATFORM-F-024) */
    if (counters->cycles > 0) {
        counters->ipc_q16 = (uint32_t)((counters->instructions << 16) / counters->cycles);
    } else {
        counters->ipc_q16 = 0;
    }
    
    /* Compute cache miss rate in Q16.16 (PLATFORM-F-025) */
    if (counters->cache_refs > 0) {
        counters->cache_miss_rate_q16 = (uint32_t)((counters->cache_misses << 16) / counters->cache_refs);
    } else {
        counters->cache_miss_rate_q16 = 0;
    }
    
    return CB_OK;
#else
    counters->available = false;
    return CB_ERR_HWCOUNTERS;
#endif
}

/*─────────────────────────────────────────────────────────────────────────────
 * Environmental Monitoring (SRS-006-PLATFORM §4.6)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Read current CPU frequency in Hz
 */
static uint64_t read_cpu_freq_hz(void)
{
#ifdef __linux__
    int64_t freq_khz;
    
    if (read_sysfs_int(CPU_FREQ_PATH, &freq_khz)) {
        return (uint64_t)freq_khz * 1000;  /* kHz to Hz */
    }
#endif
    
    return 0;
}

/**
 * @brief Read current CPU temperature in millidegrees Celsius
 */
static int32_t read_cpu_temp_mC(void)
{
#ifdef __linux__
    int64_t temp;
    
    if (read_sysfs_int(CPU_TEMP_PATH, &temp)) {
        /* Linux thermal_zone reports in millidegrees already */
        return (int32_t)temp;
    }
#endif
    
    return 0;
}

/**
 * @brief Read throttle event count
 */
static uint32_t read_throttle_count(void)
{
#ifdef __linux__
    int64_t count;
    
    if (read_sysfs_int(THROTTLE_PATH, &count)) {
        return (uint32_t)count;
    }
#endif
    
    return 0;
}

/**
 * @satisfies PLATFORM-F-060 through PLATFORM-F-064
 */
cb_result_code_t cb_env_snapshot(cb_env_snapshot_t *snapshot)
{
    if (snapshot == NULL) {
        return CB_ERR_NULL_PTR;
    }
    
    /* Get monotonic timestamp (PLATFORM-F-064) */
    snapshot->timestamp_ns = cb_timer_now_ns();
    
    /* Get CPU frequency (PLATFORM-F-061) */
    snapshot->cpu_freq_hz = read_cpu_freq_hz();
    
    /* Get CPU temperature (PLATFORM-F-062) */
    snapshot->cpu_temp_mC = read_cpu_temp_mC();
    
    /* Get throttle count (PLATFORM-F-063) */
    snapshot->throttle_count = read_throttle_count();
    
    return CB_OK;
}

/**
 * @satisfies PLATFORM-F-065 through PLATFORM-F-068
 */
cb_result_code_t cb_env_compute_stats(const cb_env_snapshot_t *start,
                                      const cb_env_snapshot_t *end,
                                      cb_env_stats_t *stats)
{
    if (start == NULL || end == NULL || stats == NULL) {
        return CB_ERR_NULL_PTR;
    }
    
    /* Copy snapshots */
    stats->start = *start;
    stats->end = *end;
    
    /* Compute min/max frequency (PLATFORM-F-066) */
    stats->min_freq_hz = (start->cpu_freq_hz < end->cpu_freq_hz) ?
                         start->cpu_freq_hz : end->cpu_freq_hz;
    stats->max_freq_hz = (start->cpu_freq_hz > end->cpu_freq_hz) ?
                         start->cpu_freq_hz : end->cpu_freq_hz;
    
    /* Compute min/max temperature (PLATFORM-F-067) */
    stats->min_temp_mC = (start->cpu_temp_mC < end->cpu_temp_mC) ?
                         start->cpu_temp_mC : end->cpu_temp_mC;
    stats->max_temp_mC = (start->cpu_temp_mC > end->cpu_temp_mC) ?
                         start->cpu_temp_mC : end->cpu_temp_mC;
    
    /* Compute total throttle events (PLATFORM-F-068) */
    if (end->throttle_count >= start->throttle_count) {
        stats->total_throttle_events = end->throttle_count - start->throttle_count;
    } else {
        /* Counter wrapped or reset */
        stats->total_throttle_events = 0;
    }
    
    return CB_OK;
}

/**
 * @satisfies PLATFORM-F-090 through PLATFORM-F-094
 */
bool cb_env_check_stable(const cb_env_stats_t *stats)
{
    if (stats == NULL) {
        return false;
    }
    
    /* If no frequency data, assume stable (graceful degradation) */
    if (stats->start.cpu_freq_hz == 0) {
        return true;
    }
    
    /*
     * Check for frequency drop > 5% (PLATFORM-F-091)
     * Use integer math per PLATFORM-F-094:
     * end_freq × 100 >= start_freq × 95
     */
    uint64_t end_scaled = stats->end.cpu_freq_hz * 100;
    uint64_t threshold = stats->start.cpu_freq_hz * 95;
    
    if (end_scaled < threshold) {
        return false;  /* Frequency dropped > 5% */
    }
    
    /* Check for throttle events (PLATFORM-F-092) */
    if (stats->total_throttle_events > 0) {
        return false;
    }
    
    return true;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Platform Initialisation
 *─────────────────────────────────────────────────────────────────────────────*/

cb_result_code_t cb_platform_init(void)
{
    if (g_platform_initialized) {
        return CB_OK;
    }
    
#ifdef __linux__
    /* Try to open hardware counters */
    g_hwcounters_available = try_open_hwcounters();
#endif
    
    g_platform_initialized = true;
    return CB_OK;
}
