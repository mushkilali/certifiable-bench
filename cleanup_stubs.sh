#!/bin/bash
#
# cleanup_stubs.sh - Remove scaffold stubs from certifiable-bench
#
# Run this on axioma-validator after extracting the final tarball
#

set -e

cd ~/certifiable-bench

echo "Removing source stubs..."

# src/metrics/ stubs
rm -fv src/metrics/histogram.c
rm -fv src/metrics/outlier.c
rm -fv src/metrics/stats.c

# src/platform/ stubs
rm -fv src/platform/detect.c
rm -fv src/platform/hwcounters_arm.c
rm -fv src/platform/hwcounters_riscv.c
rm -fv src/platform/hwcounters_x86.c

# src/report/ stubs
rm -fv src/report/compare.c
rm -fv src/report/csv.c
rm -fv src/report/json.c

# src/runner/ stubs
rm -fv src/runner/verify.c
rm -fv src/runner/warmup.c

# src/timer/ stubs
rm -fv src/timer/timer_cntvct.c
rm -fv src/timer/timer_posix.c
rm -fv src/timer/timer_rdtsc.c
rm -fv src/timer/timer_riscv.c

echo ""
echo "Removing test stubs..."

# tests/unit/ stubs
rm -fv tests/unit/test_bit_identity.c
rm -fv tests/unit/test_histogram.c
rm -fv tests/unit/test_outlier.c
rm -fv tests/unit/test_stats.c

echo ""
echo "Verifying clean structure..."

echo ""
echo "Source files (should be 6):"
find src -name "*.c" | sort

echo ""
echo "Test files (should be 6):"
find tests/unit -name "*.c" | sort

echo ""
echo "Header files (should be 7):"
find include -name "*.h" | sort

echo ""
echo "Done. Now rebuild:"
echo "  rm -rf build && mkdir build && cd build && cmake .. && make && ctest --output-on-failure"
