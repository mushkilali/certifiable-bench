# Contributing to Certifiable Bench

Thank you for your interest in contributing to certifiable-bench. This project is part of the Murray Deterministic Computing Platform (MDCP), designed for safety-critical ML systems.

## Contributor License Agreement

**All contributions require a signed CLA** before we can merge your changes.

Please read and sign the [Contributor License Agreement](CONTRIBUTOR-LICENSE-AGREEMENT.md) before submitting a pull request.

## Code Standards

### Language Requirements

- **C99** — No C11 or later features
- **No dynamic allocation** — All buffers statically allocated or caller-provided
- **No floating-point** — Integer-only arithmetic throughout
- **Bounded loops** — All loops must have provable termination
- **Full traceability** — Every function must trace to a specification

### Coding Style

```c
/**
 * @brief Brief description of function.
 * @param ctx Context pointer (must not be NULL)
 * @param samples Timing samples array
 * @param count Number of samples
 * @param faults Fault flag accumulator
 * @return CB_OK on success, error code otherwise
 * @traceability CB-MATH-001 §X.Y
 */
cb_result_code_t cb_function_name(cb_context_t *ctx,
                                   const uint64_t *samples,
                                   uint32_t count,
                                   cb_fault_flags_t *faults)
{
    if (!ctx || !samples || !faults) {
        return CB_ERR_NULL_PTR;
    }

    /* Implementation */

    return CB_OK;
}
```

### Key Principles

1. **NULL checks first** — Every pointer parameter checked at entry
2. **Fault propagation** — All operations that can fail must signal via fault flags
3. **Determinism** — Same inputs must produce same outputs across platforms
4. **No side effects** — Functions should not modify global state
5. **Constant-time where needed** — Hash comparisons must be constant-time
6. **Timing integrity** — Verification must happen outside the critical timing loop

### Prohibited Constructs

- `malloc()`, `calloc()`, `realloc()`, `free()`
- `float`, `double`, or any floating-point operations
- `goto` (except for cleanup patterns)
- Variable-length arrays (VLAs)
- Recursion without bounded depth proof
- Platform-specific headers (except behind `#ifdef`)

## Commit Messages

Use the conventional commit format:

```
type(scope): brief description

Detailed explanation if needed.

Traceability: CB-MATH-001 §X.Y
```

**Types:**
- `feat` — New feature
- `fix` — Bug fix
- `docs` — Documentation only
- `test` — Adding or updating tests
- `refactor` — Code change that neither fixes nor adds
- `chore` — Build, CI, or tooling changes

**Examples:**
```
feat(metrics): add WCET bound estimation

Implements worst-case execution time estimation using max + 6×stddev
per CB-MATH-001 §6.6.

Traceability: CB-MATH-001 §6.6, SRS-002-METRICS

fix(timer): correct resolution detection on ARM64

Previous implementation returned 0 for timer resolution on ARM64
due to missing CNTVCT frequency detection.

Traceability: SRS-001-TIMER §4.2
```

## Testing Requirements

### All Changes Must Pass

```bash
cd build
rm -rf * && cmake .. -DCMAKE_BUILD_TYPE=Release
make
ctest --output-on-failure
```

### New Features Require Tests

Add tests to the appropriate test file in `tests/unit/`:

```c
static int test_new_feature(void)
{
    /* Setup */
    cb_config_t config;
    cb_config_init(&config);

    /* Execute */
    cb_result_code_t rc = cb_new_function(&config, ...);

    /* Verify */
    TEST_ASSERT_EQ(rc, CB_OK, "function should succeed");
    TEST_ASSERT(/* expected condition */, "condition should hold");

    return 0;
}
```

### Cross-Platform Verification

For changes affecting determinism or comparison:

1. Run benchmark on first platform, save JSON
2. Run benchmark on second platform, save JSON
3. Use `cb_compare_results()` to verify bit-identity
4. Confirm performance ratios are computed correctly

## Pull Request Process

1. **Fork** the repository
2. **Create** a feature branch (`git checkout -b feat/my-feature`)
3. **Implement** changes following code standards
4. **Test** thoroughly on your platform
5. **Commit** with proper message format
6. **Push** to your fork
7. **Open** pull request with:
   - Clear description of changes
   - Traceability to specification
   - Test results
   - Platforms tested

## Areas for Contribution

### High Priority

- **RDTSC backend** — x86_64 cycle counter for sub-nanosecond precision
- **CNTVCT backend** — ARM64 cycle counter
- **RISC-V backend** — RISC-V cycle counter for Tenstorrent hardware
- **Cross-platform testing** — Verify on ARM64, RISC-V architectures

### Platform Support

- macOS `mach_absolute_time()` backend
- Windows `QueryPerformanceCounter()` backend
- Bare-metal timer backends for embedded

### Documentation

- Improve code comments
- Add examples to documentation
- Integration guides for certifiable-inference

### Testing

- Additional edge case tests
- Fuzz testing for statistics functions
- Long-running stability tests

## Module Overview

| Module | SRS | Key Functions |
|--------|-----|---------------|
| Timer | SRS-001 | `cb_timer_now_ns()`, `cb_timer_resolution_ns()` |
| Metrics | SRS-002 | `cb_compute_stats()`, `cb_detect_outliers()` |
| Runner | SRS-003 | `cb_run_benchmark()`, `cb_runner_execute()` |
| Verify | SRS-004 | `cb_compute_hash()`, `cb_hash_equal()` |
| Report | SRS-005 | `cb_write_json()`, `cb_compare_results()` |
| Platform | SRS-006 | `cb_platform_name()`, `cb_env_snapshot()` |

## Questions?

Open an issue for:
- Clarification on requirements
- Discussion of implementation approaches
- Bug reports
- Feature requests

---

*Thank you for helping make certifiable-bench better!*

Copyright © 2026 The Murray Family Innovation Trust. All rights reserved.
