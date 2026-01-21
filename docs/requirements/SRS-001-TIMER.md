# SRS-001-TIMER: Timer Subsystem Requirements

**Component:** certifiable-bench  
**Module:** Timer (`cb_timer.h`, `src/timer/`)  
**Version:** 1.0  
**Status:** Draft  
**Date:** January 2026  
**Traceability:** CB-MATH-001 §3, §7.2

---

## 1. Purpose

This document specifies requirements for the timer subsystem, which provides high-resolution monotonic timestamps for latency measurement across multiple platforms.

---

## 2. Scope

The timer subsystem provides:
- Platform-abstracted timestamp acquisition
- Multiple timer backends (POSIX, RDTSC, CNTVCT, RISC-V)
- Timer resolution and calibration
- Cycle-to-nanosecond conversion

---

## 3. Definitions

| Term | Definition |
|------|------------|
| Monotonic | Timestamps never decrease between successive reads |
| Resolution | Smallest measurable time interval |
| Calibration overhead | Time consumed by the timer read operation itself |
| TSC | Time Stamp Counter (x86) |
| CNTVCT | Counter-timer Virtual Count register (ARM64) |

---

## 4. Functional Requirements

### 4.1 Timer Initialisation

| ID | Requirement | Verification |
|----|-------------|--------------|
| TIMER-F-001 | The timer subsystem SHALL provide a function `cb_timer_init()` to initialise the timer. | Test |
| TIMER-F-002 | `cb_timer_init()` SHALL accept a preferred timer source parameter. | Test |
| TIMER-F-003 | `cb_timer_init()` SHALL return the actual timer source selected. | Test |
| TIMER-F-004 | When `CB_TIMER_AUTO` is specified, the subsystem SHALL select the highest-resolution available timer. | Test |
| TIMER-F-005 | Timer selection priority for `CB_TIMER_AUTO` SHALL be: platform-specific cycle counter → POSIX clock_gettime. | Inspection |

### 4.2 Timestamp Acquisition

| ID | Requirement | Verification |
|----|-------------|--------------|
| TIMER-F-010 | The timer subsystem SHALL provide a function `cb_timer_now_ns()` returning the current timestamp in nanoseconds. | Test |
| TIMER-F-011 | `cb_timer_now_ns()` SHALL return a monotonically increasing value. | Test |
| TIMER-F-012 | `cb_timer_now_ns()` SHALL NOT allocate memory. | Inspection |
| TIMER-F-013 | `cb_timer_now_ns()` SHALL NOT make system calls on platforms with cycle counter support (after initialisation). | Inspection |
| TIMER-F-014 | Successive calls to `cb_timer_now_ns()` SHALL return values with a difference ≥ 0. | Test |

### 4.3 Timer Resolution

| ID | Requirement | Verification |
|----|-------------|--------------|
| TIMER-F-020 | The timer subsystem SHALL provide a function `cb_timer_resolution_ns()` returning timer resolution in nanoseconds. | Test |
| TIMER-F-021 | The reported resolution SHALL be ≤ 1000 ns (1 µs) on supported platforms. | Test |
| TIMER-F-022 | The reported resolution SHALL be determined at initialisation, not hardcoded. | Inspection |

### 4.4 Cycle Conversion

| ID | Requirement | Verification |
|----|-------------|--------------|
| TIMER-F-030 | The timer subsystem SHALL provide a function `cb_cycles_to_ns()` for converting cycle counts to nanoseconds. | Test |
| TIMER-F-031 | Cycle-to-nanosecond conversion SHALL use integer arithmetic only. | Inspection |
| TIMER-F-032 | Conversion SHALL use calibrated frequency, not nominal frequency. | Test |
| TIMER-F-033 | Frequency calibration SHALL occur during `cb_timer_init()`. | Inspection |

### 4.5 Timer Identification

| ID | Requirement | Verification |
|----|-------------|--------------|
| TIMER-F-040 | The timer subsystem SHALL provide a function `cb_timer_name()` returning a human-readable timer source name. | Test |
| TIMER-F-041 | `cb_timer_name()` SHALL return a static string (no allocation). | Inspection |

---

## 5. Platform Requirements

### 5.1 POSIX Backend

| ID | Requirement | Verification |
|----|-------------|--------------|
| TIMER-P-001 | The POSIX backend SHALL use `clock_gettime(CLOCK_MONOTONIC)`. | Inspection |
| TIMER-P-002 | The POSIX backend SHALL be available on all POSIX-compliant systems. | Test |
| TIMER-P-003 | The POSIX backend SHALL set `timer_error` fault flag if `clock_gettime()` fails. | Test |

### 5.2 x86_64 Backend (RDTSC)

| ID | Requirement | Verification |
|----|-------------|--------------|
| TIMER-P-010 | The x86_64 backend SHALL use the RDTSC instruction. | Inspection |
| TIMER-P-011 | The x86_64 backend SHALL use RDTSCP or LFENCE;RDTSC for serialisation. | Inspection |
| TIMER-P-012 | The x86_64 backend SHALL detect invariant TSC support via CPUID. | Test |
| TIMER-P-013 | If invariant TSC is not available, the backend SHALL fall back to POSIX. | Test |
| TIMER-P-014 | TSC frequency SHALL be calibrated against CLOCK_MONOTONIC at initialisation. | Test |

### 5.3 ARM64 Backend (CNTVCT)

| ID | Requirement | Verification |
|----|-------------|--------------|
| TIMER-P-020 | The ARM64 backend SHALL use the CNTVCT_EL0 register. | Inspection |
| TIMER-P-021 | The ARM64 backend SHALL read CNTFRQ_EL0 for counter frequency. | Inspection |
| TIMER-P-022 | The ARM64 backend SHALL use ISB for serialisation before reading CNTVCT_EL0. | Inspection |

### 5.4 RISC-V Backend

| ID | Requirement | Verification |
|----|-------------|--------------|
| TIMER-P-030 | The RISC-V backend SHALL use the `cycle` CSR (or `time` CSR if cycle unavailable). | Inspection |
| TIMER-P-031 | The RISC-V backend SHALL handle 32-bit cycle CSR reads on RV32 (concatenating cycleh). | Inspection |
| TIMER-P-032 | Counter frequency SHALL be calibrated against POSIX at initialisation if not provided by platform. | Test |

---

## 6. Non-Functional Requirements

### 6.1 Performance

| ID | Requirement | Verification |
|----|-------------|--------------|
| TIMER-NF-001 | `cb_timer_now_ns()` call overhead SHALL be < 100 ns on cycle counter backends. | Test |
| TIMER-NF-002 | `cb_timer_now_ns()` call overhead SHALL be < 1000 ns on POSIX backend. | Test |
| TIMER-NF-003 | Timer overhead SHALL be measured and stored in `cb_timer_state_t.calibration_ns`. | Test |

### 6.2 Determinism

| ID | Requirement | Verification |
|----|-------------|--------------|
| TIMER-NF-010 | Timer initialisation SHALL be deterministic given the same hardware state. | Test |
| TIMER-NF-011 | The timer subsystem SHALL NOT introduce measurement variability beyond hardware limits. | Analysis |

### 6.3 Fault Handling

| ID | Requirement | Verification |
|----|-------------|--------------|
| TIMER-NF-020 | Timer read failures SHALL set the `timer_error` fault flag. | Test |
| TIMER-NF-021 | Timestamp wrap-around SHALL be detected and flagged as `timer_error`. | Test |
| TIMER-NF-022 | The timer subsystem SHALL NOT crash on timer read failure. | Test |

---

## 7. Interface Definition

```c
/**
 * @brief Initialise timer subsystem
 * @param source Preferred timer source (CB_TIMER_AUTO recommended)
 * @return Actual timer source selected
 *
 * @satisfies TIMER-F-001, TIMER-F-002, TIMER-F-003, TIMER-F-004
 */
cb_timer_source_t cb_timer_init(cb_timer_source_t source);

/**
 * @brief Get current timestamp in nanoseconds (monotonic)
 * @return Timestamp in nanoseconds
 *
 * @satisfies TIMER-F-010, TIMER-F-011, TIMER-F-012
 */
uint64_t cb_timer_now_ns(void);

/**
 * @brief Get timer resolution in nanoseconds
 * @return Resolution
 *
 * @satisfies TIMER-F-020, TIMER-F-021
 */
uint64_t cb_timer_resolution_ns(void);

/**
 * @brief Convert cycles to nanoseconds
 * @param cycles Cycle count
 * @return Nanoseconds
 *
 * @satisfies TIMER-F-030, TIMER-F-031, TIMER-F-032
 */
uint64_t cb_cycles_to_ns(uint64_t cycles);

/**
 * @brief Get name of current timer source
 * @return Timer name string
 *
 * @satisfies TIMER-F-040, TIMER-F-041
 */
const char *cb_timer_name(void);
```

---

## 8. Traceability Matrix

| Requirement | CB-MATH-001 | CB-STRUCT-001 | Test |
|-------------|-------------|---------------|------|
| TIMER-F-001 | — | §8 | test_timer_init |
| TIMER-F-010 | §7.2 | §8 | test_timer_now |
| TIMER-F-011 | §7.2 | — | test_timer_monotonic |
| TIMER-F-020 | — | §8 | test_timer_resolution |
| TIMER-F-030 | §3 | — | test_cycles_to_ns |
| TIMER-NF-001 | §7.2 | — | test_timer_overhead |
| TIMER-NF-020 | §5 | §3 | test_timer_fault |

---

## 9. Document Control

| Version | Date | Author | Change |
|---------|------|--------|--------|
| 1.0 | January 2026 | William Murray | Initial release |

---

*Built for safety. Designed for certification. Proven through testing.*

Copyright © 2026 The Murray Family Innovation Trust. All rights reserved.
