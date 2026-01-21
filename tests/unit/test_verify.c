/**
 * @file test_verify.c
 * @brief Unit tests for cb_verify API
 *
 * Tests SHA-256 implementation against NIST test vectors and
 * verifies all SRS-004-VERIFY requirements.
 *
 * @traceability SRS-004-VERIFY §10
 *
 * Copyright © 2026 The Murray Family Innovation Trust. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cb_verify.h"
#include "cb_platform.h"

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
#define TEST_ASSERT_STR_EQ(a, b, msg) TEST_ASSERT(strcmp((a), (b)) == 0, msg)

#define RUN_TEST(fn) do { \
    printf("  %s\n", #fn); \
    int result = fn(); \
    if (result == 0) { \
        printf("    PASS\n"); \
    } \
} while(0)

/*─────────────────────────────────────────────────────────────────────────────
 * Test: SHA-256 NIST Test Vectors (FIPS 180-4)
 * Traceability: VERIFY-F-004, VERIFY-NF-001, VERIFY-NF-002
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Test SHA-256 with empty string
 *
 * NIST test vector: SHA-256("")
 * Expected: e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
 */
static int test_sha256_empty(void)
{
    cb_hash_t hash;
    char hex[65];
    const char *expected = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";

    cb_compute_hash("", 0, &hash);
    cb_hash_to_hex(&hash, hex);

    TEST_ASSERT_STR_EQ(hex, expected, "SHA-256 of empty string");
    printf("    SHA-256(\"\") = %s\n", hex);

    return 0;
}

/**
 * @brief Test SHA-256 with "abc"
 *
 * NIST test vector: SHA-256("abc")
 * Expected: ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad
 */
static int test_sha256_abc(void)
{
    cb_hash_t hash;
    char hex[65];
    const char *expected = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";

    cb_compute_hash("abc", 3, &hash);
    cb_hash_to_hex(&hash, hex);

    TEST_ASSERT_STR_EQ(hex, expected, "SHA-256 of 'abc'");
    printf("    SHA-256(\"abc\") = %s\n", hex);

    return 0;
}

/**
 * @brief Test SHA-256 with longer message
 *
 * NIST test vector: SHA-256("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq")
 * Expected: 248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1
 */
static int test_sha256_448bit(void)
{
    cb_hash_t hash;
    char hex[65];
    const char *input = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    const char *expected = "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1";

    cb_compute_hash(input, (uint32_t)strlen(input), &hash);
    cb_hash_to_hex(&hash, hex);

    TEST_ASSERT_STR_EQ(hex, expected, "SHA-256 of 448-bit message");

    return 0;
}

/**
 * @brief Test SHA-256 with 896-bit message
 *
 * NIST test vector: SHA-256("abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu")
 * Expected: cf5b16a778af8380036ce59e7b0492370b249b11e8f07a51afac45037afee9d1
 */
static int test_sha256_896bit(void)
{
    cb_hash_t hash;
    char hex[65];
    const char *input = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";
    const char *expected = "cf5b16a778af8380036ce59e7b0492370b249b11e8f07a51afac45037afee9d1";

    cb_compute_hash(input, (uint32_t)strlen(input), &hash);
    cb_hash_to_hex(&hash, hex);

    TEST_ASSERT_STR_EQ(hex, expected, "SHA-256 of 896-bit message");

    return 0;
}

/**
 * @brief Test SHA-256 with 1 million 'a' characters
 *
 * NIST test vector: SHA-256("a" × 1000000)
 * Expected: cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0
 */
static int test_sha256_million_a(void)
{
    cb_verify_ctx_t ctx;
    cb_hash_t hash;
    char hex[65];
    const char *expected = "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0";
    char block[1000];
    int i;

    /* Create a block of 1000 'a' characters */
    memset(block, 'a', sizeof(block));

    /* Hash 1000 blocks of 1000 'a's = 1,000,000 'a's */
    cb_verify_ctx_init(&ctx);
    for (i = 0; i < 1000; i++) {
        cb_verify_ctx_update(&ctx, block, sizeof(block));
    }
    cb_verify_ctx_final(&ctx, &hash);

    cb_hash_to_hex(&hash, hex);

    TEST_ASSERT_STR_EQ(hex, expected, "SHA-256 of million 'a' characters");
    printf("    SHA-256(\"a\" × 1000000) verified\n");

    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Test: Streaming Hash API (SRS-004-VERIFY §4.1)
 * Traceability: VERIFY-F-001 through VERIFY-F-007
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Test streaming hash produces same result as one-shot
 * @satisfies VERIFY-F-001, VERIFY-F-002, VERIFY-F-003
 */
static int test_streaming_equivalence(void)
{
    const char *data = "Hello, certifiable-bench!";
    cb_hash_t hash_oneshot, hash_streaming;
    cb_verify_ctx_t ctx;

    /* One-shot */
    cb_compute_hash(data, (uint32_t)strlen(data), &hash_oneshot);

    /* Streaming - one byte at a time */
    cb_verify_ctx_init(&ctx);
    for (size_t i = 0; i < strlen(data); i++) {
        cb_verify_ctx_update(&ctx, data + i, 1);
    }
    cb_verify_ctx_final(&ctx, &hash_streaming);

    TEST_ASSERT(cb_hash_equal(&hash_oneshot, &hash_streaming),
                "streaming should equal one-shot");

    return 0;
}

/**
 * @brief Test streaming with various chunk sizes
 * @satisfies VERIFY-F-005
 */
static int test_streaming_chunks(void)
{
    const char *data = "The quick brown fox jumps over the lazy dog";
    uint32_t len = (uint32_t)strlen(data);
    cb_hash_t hash_oneshot, hash_chunked;
    cb_verify_ctx_t ctx;
    uint32_t chunk_sizes[] = {1, 7, 13, 64, 100};
    int c;

    cb_compute_hash(data, len, &hash_oneshot);

    for (c = 0; c < 5; c++) {
        uint32_t chunk = chunk_sizes[c];
        uint32_t offset = 0;

        cb_verify_ctx_init(&ctx);
        while (offset < len) {
            uint32_t remaining = len - offset;
            uint32_t this_chunk = (remaining < chunk) ? remaining : chunk;
            cb_verify_ctx_update(&ctx, data + offset, this_chunk);
            offset += this_chunk;
        }
        cb_verify_ctx_final(&ctx, &hash_chunked);

        TEST_ASSERT(cb_hash_equal(&hash_oneshot, &hash_chunked),
                    "chunked hash should match one-shot");
    }

    return 0;
}

/**
 * @brief Test init/update/final null handling
 * @satisfies VERIFY-F-001
 */
static int test_streaming_null(void)
{
    cb_verify_ctx_t ctx;
    cb_hash_t hash;
    cb_result_code_t rc;

    rc = cb_verify_ctx_init(NULL);
    TEST_ASSERT_EQ(rc, CB_ERR_NULL_PTR, "init NULL should fail");

    cb_verify_ctx_init(&ctx);

    /* NULL data with size 0 should be OK */
    rc = cb_verify_ctx_update(&ctx, NULL, 0);
    TEST_ASSERT_EQ(rc, CB_OK, "update NULL with size 0 should be OK");

    rc = cb_verify_ctx_final(NULL, &hash);
    TEST_ASSERT_EQ(rc, CB_ERR_NULL_PTR, "final NULL ctx should fail");

    rc = cb_verify_ctx_final(&ctx, NULL);
    TEST_ASSERT_EQ(rc, CB_ERR_NULL_PTR, "final NULL hash should fail");

    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Test: Hash Comparison (SRS-004-VERIFY §4.3)
 * Traceability: VERIFY-F-023, VERIFY-F-024, VERIFY-NF-020
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Test hash equality
 * @satisfies VERIFY-F-023, VERIFY-F-024
 */
static int test_hash_equal(void)
{
    cb_hash_t a, b;

    /* Same hash */
    cb_compute_hash("test", 4, &a);
    cb_compute_hash("test", 4, &b);
    TEST_ASSERT(cb_hash_equal(&a, &b), "same data should produce equal hashes");

    /* Different hash */
    cb_compute_hash("test", 4, &a);
    cb_compute_hash("Test", 4, &b);  /* Different case */
    TEST_ASSERT(!cb_hash_equal(&a, &b), "different data should produce different hashes");

    /* Single bit difference */
    cb_compute_hash("test", 4, &a);
    memcpy(&b, &a, sizeof(b));
    b.bytes[31] ^= 0x01;  /* Flip one bit */
    TEST_ASSERT(!cb_hash_equal(&a, &b), "single bit difference should be detected");

    return 0;
}

/**
 * @brief Test hash equality null handling
 */
static int test_hash_equal_null(void)
{
    cb_hash_t a;

    cb_compute_hash("test", 4, &a);

    TEST_ASSERT(!cb_hash_equal(NULL, &a), "NULL first arg should return false");
    TEST_ASSERT(!cb_hash_equal(&a, NULL), "NULL second arg should return false");
    TEST_ASSERT(!cb_hash_equal(NULL, NULL), "NULL both args should return false");

    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Test: Hex Conversion (SRS-004-VERIFY §4.6)
 * Traceability: VERIFY-F-052, VERIFY-F-053
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Test hex conversion roundtrip
 * @satisfies VERIFY-F-052, VERIFY-F-053
 */
static int test_hex_roundtrip(void)
{
    cb_hash_t original, restored;
    char hex[65];
    cb_result_code_t rc;

    cb_compute_hash("roundtrip test", 14, &original);

    rc = cb_hash_to_hex(&original, hex);
    TEST_ASSERT_EQ(rc, CB_OK, "to_hex should succeed");
    TEST_ASSERT_EQ(strlen(hex), 64, "hex should be 64 characters");

    rc = cb_hash_from_hex(hex, &restored);
    TEST_ASSERT_EQ(rc, CB_OK, "from_hex should succeed");

    TEST_ASSERT(cb_hash_equal(&original, &restored), "roundtrip should preserve hash");

    return 0;
}

/**
 * @brief Test hex conversion is lowercase
 * @satisfies VERIFY-F-053
 */
static int test_hex_lowercase(void)
{
    cb_hash_t hash;
    char hex[65];
    size_t i;

    cb_compute_hash("lowercase", 9, &hash);
    cb_hash_to_hex(&hash, hex);

    for (i = 0; i < 64; i++) {
        bool is_digit = (hex[i] >= '0' && hex[i] <= '9');
        bool is_lower = (hex[i] >= 'a' && hex[i] <= 'f');
        TEST_ASSERT(is_digit || is_lower, "hex should be lowercase");
    }

    return 0;
}

/**
 * @brief Test hex from uppercase
 */
static int test_hex_from_uppercase(void)
{
    cb_hash_t hash;
    cb_result_code_t rc;
    const char *upper = "BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD";

    rc = cb_hash_from_hex(upper, &hash);
    TEST_ASSERT_EQ(rc, CB_OK, "from_hex should accept uppercase");

    return 0;
}

/**
 * @brief Test hex conversion null handling
 */
static int test_hex_null(void)
{
    cb_hash_t hash;
    char hex[65];
    cb_result_code_t rc;

    rc = cb_hash_to_hex(NULL, hex);
    TEST_ASSERT_EQ(rc, CB_ERR_NULL_PTR, "to_hex NULL hash should fail");

    rc = cb_hash_to_hex(&hash, NULL);
    TEST_ASSERT_EQ(rc, CB_ERR_NULL_PTR, "to_hex NULL hex should fail");

    rc = cb_hash_from_hex(NULL, &hash);
    TEST_ASSERT_EQ(rc, CB_ERR_NULL_PTR, "from_hex NULL hex should fail");

    rc = cb_hash_from_hex("abc", NULL);
    TEST_ASSERT_EQ(rc, CB_ERR_NULL_PTR, "from_hex NULL hash should fail");

    return 0;
}

/**
 * @brief Test hex from invalid string
 */
static int test_hex_invalid(void)
{
    cb_hash_t hash;
    cb_result_code_t rc;

    /* Too short */
    rc = cb_hash_from_hex("abc", &hash);
    TEST_ASSERT_EQ(rc, CB_ERR_INVALID_CONFIG, "too short should fail");

    /* Invalid characters */
    rc = cb_hash_from_hex("zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz", &hash);
    TEST_ASSERT_EQ(rc, CB_ERR_INVALID_CONFIG, "invalid chars should fail");

    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Test: Golden Reference (SRS-004-VERIFY §4.2)
 * Traceability: VERIFY-F-010 through VERIFY-F-015
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Test golden save and load roundtrip
 * @satisfies VERIFY-F-010, VERIFY-F-011, VERIFY-F-012, VERIFY-F-013
 */
static int test_golden_roundtrip(void)
{
    cb_golden_ref_t original, loaded;
    cb_result_code_t rc;
    const char *path = "/tmp/cb_test_golden.json";

    memset(&original, 0, sizeof(original));
    memset(&loaded, 0, sizeof(loaded));

    /* Create golden reference */
    cb_compute_hash("golden output", 13, &original.output_hash);
    original.sample_count = 1000;
    original.output_size = 40;
    strncpy(original.platform, "x86_64", sizeof(original.platform) - 1);

    /* Save */
    rc = cb_golden_save(path, &original);
    TEST_ASSERT_EQ(rc, CB_OK, "golden_save should succeed");

    /* Load */
    rc = cb_golden_load(path, &loaded);
    TEST_ASSERT_EQ(rc, CB_OK, "golden_load should succeed");

    /* Verify */
    TEST_ASSERT(cb_hash_equal(&original.output_hash, &loaded.output_hash),
                "hash should match");
    TEST_ASSERT_EQ(original.sample_count, loaded.sample_count, "sample_count should match");

    /* Cleanup */
    remove(path);

    return 0;
}

/**
 * @brief Test golden load with missing file
 * @satisfies VERIFY-F-014
 */
static int test_golden_load_missing(void)
{
    cb_golden_ref_t golden;
    cb_result_code_t rc;

    rc = cb_golden_load("/nonexistent/path/golden.json", &golden);
    TEST_ASSERT_EQ(rc, CB_ERR_GOLDEN_LOAD, "missing file should fail");

    return 0;
}

/**
 * @brief Test golden verify
 * @satisfies VERIFY-F-020, VERIFY-F-021
 */
static int test_golden_verify(void)
{
    cb_golden_ref_t golden;
    cb_hash_t computed;

    memset(&golden, 0, sizeof(golden));

    /* Setup golden */
    cb_compute_hash("expected output", 15, &golden.output_hash);

    /* Matching */
    cb_compute_hash("expected output", 15, &computed);
    TEST_ASSERT(cb_golden_verify(&computed, &golden), "matching should verify");

    /* Non-matching */
    cb_compute_hash("different output", 16, &computed);
    TEST_ASSERT(!cb_golden_verify(&computed, &golden), "different should not verify");

    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Test: Result Binding (SRS-004-VERIFY §4.5)
 * Traceability: VERIFY-F-040 through VERIFY-F-047
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Test result binding computation
 * @satisfies VERIFY-F-040 through VERIFY-F-047
 */
static int test_result_binding(void)
{
    cb_hash_t output_hash, result_hash1, result_hash2;
    cb_latency_stats_t stats;
    cb_result_code_t rc;

    /* Setup */
    cb_compute_hash("inference output", 16, &output_hash);
    memset(&stats, 0, sizeof(stats));
    stats.min_ns = 1000;
    stats.max_ns = 5000;
    stats.mean_ns = 2500;
    stats.p99_ns = 4500;

    /* Compute binding */
    rc = cb_compute_result_binding(&output_hash, "x86_64", 0x12345678,
                                   &stats, 1700000000, &result_hash1);
    TEST_ASSERT_EQ(rc, CB_OK, "result_binding should succeed");

    /* Same inputs should produce same hash (deterministic) */
    rc = cb_compute_result_binding(&output_hash, "x86_64", 0x12345678,
                                   &stats, 1700000000, &result_hash2);
    TEST_ASSERT_EQ(rc, CB_OK, "second binding should succeed");
    TEST_ASSERT(cb_hash_equal(&result_hash1, &result_hash2),
                "same inputs should produce same binding");

    /* Different timestamp should produce different hash */
    rc = cb_compute_result_binding(&output_hash, "x86_64", 0x12345678,
                                   &stats, 1700000001, &result_hash2);
    TEST_ASSERT(!cb_hash_equal(&result_hash1, &result_hash2),
                "different timestamp should produce different binding");

    return 0;
}

/**
 * @brief Test result binding null handling
 */
static int test_result_binding_null(void)
{
    cb_hash_t output_hash, result_hash;
    cb_latency_stats_t stats;
    cb_result_code_t rc;

    memset(&stats, 0, sizeof(stats));
    cb_compute_hash("test", 4, &output_hash);

    rc = cb_compute_result_binding(NULL, "x86_64", 0, &stats, 0, &result_hash);
    TEST_ASSERT_EQ(rc, CB_ERR_NULL_PTR, "NULL output_hash should fail");

    rc = cb_compute_result_binding(&output_hash, NULL, 0, &stats, 0, &result_hash);
    TEST_ASSERT_EQ(rc, CB_ERR_NULL_PTR, "NULL platform should fail");

    rc = cb_compute_result_binding(&output_hash, "x86_64", 0, NULL, 0, &result_hash);
    TEST_ASSERT_EQ(rc, CB_ERR_NULL_PTR, "NULL stats should fail");

    rc = cb_compute_result_binding(&output_hash, "x86_64", 0, &stats, 0, NULL);
    TEST_ASSERT_EQ(rc, CB_ERR_NULL_PTR, "NULL result_hash should fail");

    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Test: Determinism (SRS-004-VERIFY §5.1)
 * Traceability: VERIFY-NF-001
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Test cross-platform determinism (same code, same results)
 * @satisfies VERIFY-NF-001
 */
static int test_determinism(void)
{
    cb_hash_t hash;
    char hex[65];

    /* These expected values are computed per FIPS 180-4 and should
     * be identical on all platforms */

    /* Test 1 */
    cb_compute_hash("determinism", 11, &hash);
    cb_hash_to_hex(&hash, hex);
    TEST_ASSERT_STR_EQ(hex, "f723e6c99c64713e0d5b95252a3f9bf7ba658a168d8de4cea791fa97a48d81b8",
                       "determinism test 1");

    /* Test 2 - binary data */
    uint8_t binary[] = {0x00, 0x01, 0x02, 0x03, 0xFF, 0xFE, 0xFD, 0xFC};
    cb_compute_hash(binary, sizeof(binary), &hash);
    cb_hash_to_hex(&hash, hex);
    TEST_ASSERT_STR_EQ(hex, "fed271e1776a1c254c9e8ea187937d24418e1d01781eee828507725de159dd58",
                       "determinism test 2 (binary)");

    return 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Main
 *─────────────────────────────────────────────────────────────────────────────*/

int main(void)
{
    printf("═══════════════════════════════════════════════════════════════════\n");
    printf(" certifiable-bench Verify Tests\n");
    printf(" @traceability SRS-004-VERIFY\n");
    printf("═══════════════════════════════════════════════════════════════════\n\n");

    printf("§4.1 SHA-256 NIST Test Vectors (VERIFY-F-004, VERIFY-NF-002)\n");
    RUN_TEST(test_sha256_empty);
    RUN_TEST(test_sha256_abc);
    RUN_TEST(test_sha256_448bit);
    RUN_TEST(test_sha256_896bit);
    RUN_TEST(test_sha256_million_a);

    printf("\n§4.1 Streaming Hash API (VERIFY-F-001..007)\n");
    RUN_TEST(test_streaming_equivalence);
    RUN_TEST(test_streaming_chunks);
    RUN_TEST(test_streaming_null);

    printf("\n§4.3 Hash Comparison (VERIFY-F-023..024, VERIFY-NF-020)\n");
    RUN_TEST(test_hash_equal);
    RUN_TEST(test_hash_equal_null);

    printf("\n§4.6 Hex Conversion (VERIFY-F-052..053)\n");
    RUN_TEST(test_hex_roundtrip);
    RUN_TEST(test_hex_lowercase);
    RUN_TEST(test_hex_from_uppercase);
    RUN_TEST(test_hex_null);
    RUN_TEST(test_hex_invalid);

    printf("\n§4.2 Golden Reference (VERIFY-F-010..015)\n");
    RUN_TEST(test_golden_roundtrip);
    RUN_TEST(test_golden_load_missing);
    RUN_TEST(test_golden_verify);

    printf("\n§4.5 Result Binding (VERIFY-F-040..047)\n");
    RUN_TEST(test_result_binding);
    RUN_TEST(test_result_binding_null);

    printf("\n§5.1 Determinism (VERIFY-NF-001)\n");
    RUN_TEST(test_determinism);

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
