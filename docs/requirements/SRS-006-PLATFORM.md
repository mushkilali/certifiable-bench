# SRS-006-PLATFORM: Platform Detection and Environmental Monitoring Requirements

**Component:** certifiable-bench  
**Module:** Platform (`cb_platform.h`, `src/platform/`)  
**Version:** 1.0  
**Status:** Draft  
**Date:** January 2026  
**Traceability:** CB-MATH-001 §9, CB-STRUCT-001 §6, §7

---

## 1. Purpose

This document specifies requirements for the platform detection and environmental monitoring module, which identifies hardware capabilities and monitors thermal/frequency state during benchmarking.

---

## 2. Scope

The platform module provides:
- Platform and CPU identification
- Hardware performance counter access
- CPU frequency monitoring
- Temperature monitoring
- Thermal throttle detection
- Environmental stability assessment

---

## 3. Definitions

| Term | Definition |
|------|------------|
| Platform | CPU architecture identifier (x86_64, aarch64, riscv64) |
| HPM | Hardware Performance Monitor |
| PMU | Performance Monitoring Unit |
| Throttling | CPU frequency reduction due to thermal or power limits |
| Thermal drift | Frequency change > 5% during benchmark |

---

## 4. Functional Requirements

### 4.1 Platform Identification

| ID | Requirement | Verification |
|----|-------------|--------------|
| PLATFORM-F-001 | The module SHALL provide a function `cb_platform_name()` returning platform identifier. | Test |
| PLATFORM-F-002 | `cb_platform_name()` SHALL return "x86_64" on x86-64 systems. | Test |
| PLATFORM-F-003 | `cb_platform_name()` SHALL return "aarch64" on ARM64 systems. | Test |
| PLATFORM-F-004 | `cb_platform_name()` SHALL return "riscv64" on RISC-V 64-bit systems. | Test |
| PLATFORM-F-005 | `cb_platform_name()` SHALL return "unknown" on unrecognised platforms. | Test |
| PLATFORM-F-006 | The module SHALL provide a function `cb_cpu_model()` returning CPU model string. | Test |
| PLATFORM-F-007 | `cb_cpu_model()` SHALL read from /proc/cpuinfo on Linux. | Inspection |
| PLATFORM-F-008 | `cb_cpu_model()` SHALL read from sysctl on macOS. | Inspection |
| PLATFORM-F-009 | `cb_cpu_model()` SHALL truncate output to fit buffer size. | Test |
| PLATFORM-F-010 | The module SHALL provide a function `cb_cpu_freq_mhz()` returning nominal CPU frequency. | Test |

### 4.2 Hardware Performance Counters

| ID | Requirement | Verification |
|----|-------------|--------------|
| PLATFORM-F-020 | The module SHALL provide a function `cb_hwcounters_available()` to check counter availability. | Test |
| PLATFORM-F-021 | The module SHALL provide a function `cb_hwcounters_start()` to begin counter collection. | Test |
| PLATFORM-F-022 | The module SHALL provide a function `cb_hwcounters_stop()` to end collection and read values. | Test |
| PLATFORM-F-023 | Hardware counters SHALL include: cycles, instructions, cache_misses, branch_misses. | Test |
| PLATFORM-F-024 | IPC (instructions per cycle) SHALL be computed as: (instructions << 16) / cycles (Q16.16). | Test |
| PLATFORM-F-025 | Cache miss rate SHALL be computed as: (cache_misses << 16) / cache_refs (Q16.16). | Test |
| PLATFORM-F-026 | If counters unavailable, `cb_hwcounters_t.available` SHALL be false. | Test |
| PLATFORM-F-027 | Counter access failure SHALL NOT cause benchmark failure. | Test |

### 4.3 x86_64 Hardware Counters

| ID | Requirement | Verification |
|----|-------------|--------------|
| PLATFORM-F-030 | x86_64 backend SHALL use perf_event_open() on Linux. | Inspection |
| PLATFORM-F-031 | x86_64 backend SHALL gracefully handle permission denied errors. | Test |
| PLATFORM-F-032 | If perf_event unavailable, counters SHALL be marked unavailable. | Test |

### 4.4 ARM64 Hardware Counters

| ID | Requirement | Verification |
|----|-------------|--------------|
| PLATFORM-F-040 | ARM64 backend SHALL use perf_event_open() on Linux. | Inspection |
| PLATFORM-F-041 | ARM64 backend SHALL use PMU registers where kernel access is available. | Inspection |

### 4.5 RISC-V Hardware Counters

| ID | Requirement | Verification |
|----|-------------|--------------|
| PLATFORM-F-050 | RISC-V backend SHALL use HPM CSRs where available. | Inspection |
| PLATFORM-F-051 | RISC-V backend SHALL fall back to perf_event on Linux. | Inspection |

### 4.6 Environmental Monitoring (CB-MATH-001 §9)

| ID | Requirement | Verification |
|----|-------------|--------------|
| PLATFORM-F-060 | The module SHALL provide a function `cb_env_snapshot()` to capture environmental state. | Test |
| PLATFORM-F-061 | Snapshot SHALL include CPU frequency in Hz. | Test |
| PLATFORM-F-062 | Snapshot SHALL include CPU temperature in millidegrees Celsius. | Test |
| PLATFORM-F-063 | Snapshot SHALL include cumulative throttle event count. | Test |
| PLATFORM-F-064 | Snapshot SHALL include monotonic timestamp. | Test |
| PLATFORM-F-065 | The module SHALL provide a function `cb_env_compute_stats()` to compute environmental statistics from start/end snapshots. | Test |
| PLATFORM-F-066 | Statistics SHALL include min/max frequency observed. | Test |
| PLATFORM-F-067 | Statistics SHALL include min/max temperature observed. | Test |
| PLATFORM-F-068 | Statistics SHALL include total throttle events. | Test |

### 4.7 Frequency Monitoring

| ID | Requirement | Verification |
|----|-------------|--------------|
| PLATFORM-F-070 | On Linux, frequency SHALL be read from /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq. | Inspection |
| PLATFORM-F-071 | If frequency file unavailable, frequency SHALL be reported as 0. | Test |
| PLATFORM-F-072 | Frequency reading failure SHALL NOT cause benchmark failure. | Test |

### 4.8 Temperature Monitoring

| ID | Requirement | Verification |
|----|-------------|--------------|
| PLATFORM-F-080 | On Linux, temperature SHALL be read from /sys/class/thermal/thermal_zone0/temp. | Inspection |
| PLATFORM-F-081 | Temperature SHALL be converted to millidegrees Celsius. | Test |
| PLATFORM-F-082 | If temperature file unavailable, temperature SHALL be reported as 0. | Test |
| PLATFORM-F-083 | Temperature reading failure SHALL NOT cause benchmark failure. | Test |

### 4.9 Stability Assessment (CB-MATH-001 §9.3)

| ID | Requirement | Verification |
|----|-------------|--------------|
| PLATFORM-F-090 | The module SHALL provide a function `cb_env_is_stable()` to assess stability. | Test |
| PLATFORM-F-091 | Environment SHALL be considered unstable if end_freq < 0.95 × start_freq. | Test |
| PLATFORM-F-092 | Environment SHALL be considered unstable if throttle_events > 0. | Test |
| PLATFORM-F-093 | If environment is unstable, `thermal_drift` fault flag SHALL be set. | Test |
| PLATFORM-F-094 | Stability assessment SHALL use integer arithmetic (multiply then divide). | Inspection |

---

## 5. Non-Functional Requirements

### 5.1 Performance

| ID | Requirement | Verification |
|----|-------------|--------------|
| PLATFORM-NF-001 | `cb_env_snapshot()` overhead SHALL be < 1 ms. | Test |
| PLATFORM-NF-002 | Environmental monitoring SHALL NOT impact benchmark timing (called outside critical loop). | Inspection |
| PLATFORM-NF-003 | Hardware counter start/stop overhead SHALL be < 10 µs. | Test |

### 5.2 Robustness

| ID | Requirement | Verification |
|----|-------------|--------------|
| PLATFORM-NF-010 | All platform functions SHALL handle missing/inaccessible system files gracefully. | Test |
| PLATFORM-NF-011 | Permission errors SHALL result in "unavailable" status, not failure. | Test |
| PLATFORM-NF-012 | Platform module SHALL NOT require root/admin privileges for basic operation. | Test |

### 5.3 Portability

| ID | Requirement | Verification |
|----|-------------|--------------|
| PLATFORM-NF-020 | Platform detection SHALL work on Linux (glibc and musl). | Test |
| PLATFORM-NF-021 | Platform detection SHALL work on macOS. | Test |
| PLATFORM-NF-022 | Platform detection SHALL work on FreeBSD. | Test |
| PLATFORM-NF-023 | Environmental monitoring MAY be reduced functionality on non-Linux systems. | Inspection |

---

## 6. Interface Definition

```c
/**
 * @brief Get platform identifier
 * @return "x86_64", "aarch64", "riscv64", or "unknown"
 *
 * @satisfies PLATFORM-F-001 through PLATFORM-F-005
 */
const char *cb_platform_name(void);

/**
 * @brief Get CPU model string
 * @param buffer Output buffer
 * @param size Buffer size
 *
 * @satisfies PLATFORM-F-006 through PLATFORM-F-009
 */
void cb_cpu_model(char *buffer, uint32_t size);

/**
 * @brief Get nominal CPU frequency
 * @return Frequency in MHz, or 0 if unavailable
 *
 * @satisfies PLATFORM-F-010
 */
uint32_t cb_cpu_freq_mhz(void);

/**
 * @brief Check if hardware counters are available
 * @return true if available
 *
 * @satisfies PLATFORM-F-020
 */
bool cb_hwcounters_available(void);

/**
 * @brief Start hardware counter collection
 * @return CB_OK on success, CB_ERR_HWCOUNTERS if unavailable
 *
 * @satisfies PLATFORM-F-021
 */
cb_result_code_t cb_hwcounters_start(void);

/**
 * @brief Stop collection and read hardware counters
 * @param counters Output counter values
 * @return CB_OK on success
 *
 * @satisfies PLATFORM-F-022 through PLATFORM-F-026
 */
cb_result_code_t cb_hwcounters_stop(cb_hwcounters_t *counters);

/**
 * @brief Capture environmental snapshot
 * @param snapshot Output snapshot
 * @return CB_OK on success
 *
 * @satisfies PLATFORM-F-060 through PLATFORM-F-064
 */
cb_result_code_t cb_env_snapshot(cb_env_snapshot_t *snapshot);

/**
 * @brief Compute environmental statistics from snapshots
 * @param start Start snapshot
 * @param end End snapshot
 * @param stats Output statistics
 *
 * @satisfies PLATFORM-F-065 through PLATFORM-F-068
 */
void cb_env_compute_stats(const cb_env_snapshot_t *start,
                           const cb_env_snapshot_t *end,
                           cb_env_stats_t *stats);

/**
 * @brief Check environmental stability
 * @param stats Environmental statistics
 * @return true if stable
 *
 * @satisfies PLATFORM-F-090 through PLATFORM-F-094
 */
bool cb_env_is_stable(const cb_env_stats_t *stats);
```

---

## 7. System File Paths

### 7.1 Linux

| Data | Path |
|------|------|
| CPU frequency | /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq |
| CPU temperature | /sys/class/thermal/thermal_zone0/temp |
| CPU model | /proc/cpuinfo |
| Throttle events | /sys/devices/system/cpu/cpu0/thermal_throttle/count |

### 7.2 macOS

| Data | Method |
|------|--------|
| CPU model | sysctl -n machdep.cpu.brand_string |
| CPU frequency | sysctl -n hw.cpufrequency |
| Temperature | IOKit (SMC) |

### 7.3 FreeBSD

| Data | Method |
|------|--------|
| CPU model | sysctl -n hw.model |
| CPU frequency | sysctl -n dev.cpu.0.freq |
| Temperature | sysctl -n dev.cpu.0.temperature |

---

## 8. Stability Check Implementation

```c
bool cb_env_is_stable(const cb_env_stats_t *stats) {
    /* Check for frequency drop > 5% */
    /* Use integer math: end_freq * 100 >= start_freq * 95 */
    uint64_t end_scaled = stats->end.cpu_freq_hz * 100;
    uint64_t threshold = stats->start.cpu_freq_hz * 95;
    
    if (end_scaled < threshold) {
        return false;  /* Frequency dropped > 5% */
    }
    
    /* Check for throttle events */
    if (stats->total_throttle_events > 0) {
        return false;
    }
    
    return true;
}
```

---

## 9. Traceability Matrix

| Requirement | CB-MATH-001 | CB-STRUCT-001 | Test |
|-------------|-------------|---------------|------|
| PLATFORM-F-001 | — | §10 | test_platform_name |
| PLATFORM-F-006 | — | §10 | test_cpu_model |
| PLATFORM-F-020 | — | §7 | test_hwcounters_avail |
| PLATFORM-F-060 | §9.2 | §6 | test_env_snapshot |
| PLATFORM-F-090 | §9.3 | §6 | test_env_stable |
| PLATFORM-NF-010 | — | — | test_graceful_failure |

---

## 10. Document Control

| Version | Date | Author | Change |
|---------|------|--------|--------|
| 1.0 | January 2026 | William Murray | Initial release |

---

*Built for safety. Designed for certification. Proven through testing.*

Copyright © 2026 The Murray Family Innovation Trust. All rights reserved.
