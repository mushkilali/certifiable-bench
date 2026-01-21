/**
 * @file verify.c
 * @brief Verification and SHA-256 hashing implementation
 *
 * Implements SRS-004-VERIFY requirements with FIPS 180-4 compliant SHA-256.
 * No external dependencies — pure C implementation for portability.
 *
 * @traceability SRS-004-VERIFY, CB-MATH-001 §7.1, §8
 *
 * Copyright © 2026 The Murray Family Innovation Trust. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cb_verify.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*─────────────────────────────────────────────────────────────────────────────
 * Internal SHA-256 State Structure
 *
 * This is stored within the cb_verify_ctx_t.state[128] array.
 *─────────────────────────────────────────────────────────────────────────────*/

typedef struct {
    uint32_t h[8];             /* Hash state */
    uint8_t buffer[64];        /* Message block buffer */
    uint32_t buffer_len;       /* Bytes in buffer */
    uint64_t total_len;        /* Total bytes processed */
} sha256_state_t;

/*─────────────────────────────────────────────────────────────────────────────
 * SHA-256 Constants (FIPS 180-4)
 *─────────────────────────────────────────────────────────────────────────────*/

static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static const uint32_t H0[8] = {
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};

/*─────────────────────────────────────────────────────────────────────────────
 * SHA-256 Helper Macros
 *─────────────────────────────────────────────────────────────────────────────*/

#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define EP1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SIG0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
#define SIG1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

/*─────────────────────────────────────────────────────────────────────────────
 * SHA-256 Block Processing
 *─────────────────────────────────────────────────────────────────────────────*/

static void sha256_transform(uint32_t state[8], const uint8_t block[64])
{
    uint32_t a, b, c, d, e, f, g, h;
    uint32_t W[64];
    uint32_t t1, t2;
    int i;

    for (i = 0; i < 16; i++) {
        W[i] = ((uint32_t)block[i * 4 + 0] << 24) |
               ((uint32_t)block[i * 4 + 1] << 16) |
               ((uint32_t)block[i * 4 + 2] << 8) |
               ((uint32_t)block[i * 4 + 3]);
    }
    for (i = 16; i < 64; i++) {
        W[i] = SIG1(W[i - 2]) + W[i - 7] + SIG0(W[i - 15]) + W[i - 16];
    }

    a = state[0]; b = state[1]; c = state[2]; d = state[3];
    e = state[4]; f = state[5]; g = state[6]; h = state[7];

    for (i = 0; i < 64; i++) {
        t1 = h + EP1(e) + CH(e, f, g) + K[i] + W[i];
        t2 = EP0(a) + MAJ(a, b, c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }

    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Internal SHA-256 Functions
 *─────────────────────────────────────────────────────────────────────────────*/

static sha256_state_t *get_sha_state(cb_verify_ctx_t *ctx)
{
    return (sha256_state_t *)ctx->state;
}

static void sha256_init(sha256_state_t *s)
{
    memcpy(s->h, H0, sizeof(H0));
    memset(s->buffer, 0, sizeof(s->buffer));
    s->buffer_len = 0;
    s->total_len = 0;
}

static void sha256_update(sha256_state_t *s, const uint8_t *data, uint32_t len)
{
    uint32_t remaining;

    s->total_len += len;

    if (s->buffer_len > 0) {
        remaining = 64 - s->buffer_len;
        if (len < remaining) {
            memcpy(s->buffer + s->buffer_len, data, len);
            s->buffer_len += len;
            return;
        }
        memcpy(s->buffer + s->buffer_len, data, remaining);
        sha256_transform(s->h, s->buffer);
        data += remaining;
        len -= remaining;
        s->buffer_len = 0;
    }

    while (len >= 64) {
        sha256_transform(s->h, data);
        data += 64;
        len -= 64;
    }

    if (len > 0) {
        memcpy(s->buffer, data, len);
        s->buffer_len = len;
    }
}

static void sha256_final(sha256_state_t *s, cb_hash_t *hash)
{
    uint8_t pad[64];
    uint64_t bit_len = s->total_len * 8;
    uint32_t pad_len;
    int i;

    pad_len = (s->buffer_len < 56) ? (56 - s->buffer_len) : (120 - s->buffer_len);
    memset(pad, 0, sizeof(pad));
    pad[0] = 0x80;
    sha256_update(s, pad, pad_len);

    /* Append bit length in big-endian */
    pad[0] = (uint8_t)(bit_len >> 56);
    pad[1] = (uint8_t)(bit_len >> 48);
    pad[2] = (uint8_t)(bit_len >> 40);
    pad[3] = (uint8_t)(bit_len >> 32);
    pad[4] = (uint8_t)(bit_len >> 24);
    pad[5] = (uint8_t)(bit_len >> 16);
    pad[6] = (uint8_t)(bit_len >> 8);
    pad[7] = (uint8_t)(bit_len);
    sha256_update(s, pad, 8);

    for (i = 0; i < 8; i++) {
        hash->bytes[i * 4 + 0] = (uint8_t)(s->h[i] >> 24);
        hash->bytes[i * 4 + 1] = (uint8_t)(s->h[i] >> 16);
        hash->bytes[i * 4 + 2] = (uint8_t)(s->h[i] >> 8);
        hash->bytes[i * 4 + 3] = (uint8_t)(s->h[i]);
    }
}

/*─────────────────────────────────────────────────────────────────────────────
 * Public Streaming Hash API
 *─────────────────────────────────────────────────────────────────────────────*/

cb_result_code_t cb_verify_ctx_init(cb_verify_ctx_t *ctx)
{
    if (ctx == NULL) return CB_ERR_NULL_PTR;

    sha256_init(get_sha_state(ctx));
    ctx->bytes_hashed = 0;
    ctx->finalised = false;
    return CB_OK;
}

cb_result_code_t cb_verify_ctx_update(cb_verify_ctx_t *ctx, const void *data, uint32_t size)
{
    if (ctx == NULL) return CB_ERR_NULL_PTR;
    if (data == NULL && size > 0) return CB_ERR_NULL_PTR;
    if (ctx->finalised) return CB_ERR_INVALID_CONFIG;
    if (size == 0) return CB_OK;  /* Nothing to do */

    sha256_update(get_sha_state(ctx), (const uint8_t *)data, size);
    ctx->bytes_hashed += size;
    return CB_OK;
}

cb_result_code_t cb_verify_ctx_final(cb_verify_ctx_t *ctx, cb_hash_t *hash)
{
    if (ctx == NULL || hash == NULL) return CB_ERR_NULL_PTR;

    sha256_final(get_sha_state(ctx), hash);
    ctx->finalised = true;
    return CB_OK;
}

/*─────────────────────────────────────────────────────────────────────────────
 * One-Shot Hash
 *─────────────────────────────────────────────────────────────────────────────*/

cb_result_code_t cb_compute_hash(const void *data, uint32_t size, cb_hash_t *hash)
{
    cb_verify_ctx_t ctx;
    if (data == NULL || hash == NULL) return CB_ERR_NULL_PTR;

    cb_verify_ctx_init(&ctx);
    cb_verify_ctx_update(&ctx, data, size);
    cb_verify_ctx_final(&ctx, hash);
    return CB_OK;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Hash Comparison (constant-time)
 *─────────────────────────────────────────────────────────────────────────────*/

bool cb_hash_equal(const cb_hash_t *a, const cb_hash_t *b)
{
    uint8_t diff = 0;
    int i;
    if (a == NULL || b == NULL) return false;

    for (i = 0; i < CB_HASH_SIZE; i++) {
        diff |= a->bytes[i] ^ b->bytes[i];
    }
    return diff == 0;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Hex Conversion
 *─────────────────────────────────────────────────────────────────────────────*/

cb_result_code_t cb_hash_to_hex(const cb_hash_t *hash, char *hex_out)
{
    static const char hex_chars[] = "0123456789abcdef";
    int i;
    if (hash == NULL || hex_out == NULL) return CB_ERR_NULL_PTR;

    for (i = 0; i < CB_HASH_SIZE; i++) {
        hex_out[i * 2 + 0] = hex_chars[(hash->bytes[i] >> 4) & 0x0F];
        hex_out[i * 2 + 1] = hex_chars[hash->bytes[i] & 0x0F];
    }
    hex_out[CB_HASH_SIZE * 2] = '\0';
    return CB_OK;
}

static int hex_char_to_int(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

cb_result_code_t cb_hash_from_hex(const char *hex_in, cb_hash_t *hash)
{
    int i, hi, lo;
    if (hex_in == NULL || hash == NULL) return CB_ERR_NULL_PTR;
    if (strlen(hex_in) != CB_HASH_SIZE * 2) return CB_ERR_INVALID_CONFIG;

    for (i = 0; i < CB_HASH_SIZE; i++) {
        hi = hex_char_to_int(hex_in[i * 2]);
        lo = hex_char_to_int(hex_in[i * 2 + 1]);
        if (hi < 0 || lo < 0) return CB_ERR_INVALID_CONFIG;
        hash->bytes[i] = (uint8_t)((hi << 4) | lo);
    }
    return CB_OK;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Golden Reference File I/O
 *─────────────────────────────────────────────────────────────────────────────*/

static bool extract_json_string(const char *json, const char *key, char *out, size_t out_size)
{
    char search[128];
    const char *start, *end;
    size_t len;

    snprintf(search, sizeof(search), "\"%s\"", key);
    start = strstr(json, search);
    if (start == NULL) return false;

    start = strchr(start, ':');
    if (start == NULL) return false;
    start++;
    while (*start == ' ' || *start == '"') start++;

    end = start;
    while (*end && *end != '"' && *end != ',' && *end != '}') end++;

    len = (size_t)(end - start);
    if (len >= out_size) len = out_size - 1;
    memcpy(out, start, len);
    out[len] = '\0';
    return true;
}

static bool extract_json_uint(const char *json, const char *key, uint64_t *out)
{
    char buf[32];
    if (!extract_json_string(json, key, buf, sizeof(buf))) return false;
    *out = (uint64_t)strtoul(buf, NULL, 10);
    return true;
}

cb_result_code_t cb_golden_load(const char *path, cb_golden_ref_t *golden)
{
    FILE *fp;
    char json[2048], hash_hex[65];
    size_t bytes_read;
    uint64_t tmp;

    if (path == NULL || golden == NULL) return CB_ERR_NULL_PTR;
    memset(golden, 0, sizeof(*golden));

    fp = fopen(path, "r");
    if (fp == NULL) return CB_ERR_GOLDEN_LOAD;

    bytes_read = fread(json, 1, sizeof(json) - 1, fp);
    fclose(fp);
    if (bytes_read == 0) return CB_ERR_GOLDEN_LOAD;
    json[bytes_read] = '\0';

    if (!extract_json_string(json, "output_hash", hash_hex, sizeof(hash_hex)))
        return CB_ERR_GOLDEN_LOAD;
    if (cb_hash_from_hex(hash_hex, &golden->output_hash) != CB_OK)
        return CB_ERR_GOLDEN_LOAD;

    if (extract_json_uint(json, "sample_count", &tmp))
        golden->sample_count = (uint32_t)tmp;
    if (extract_json_uint(json, "output_size", &tmp))
        golden->output_size = (uint32_t)tmp;

    extract_json_string(json, "platform", golden->platform, sizeof(golden->platform));
    return CB_OK;
}

cb_result_code_t cb_verify_load_golden(const char *path, cb_golden_ref_t *golden)
{
    return cb_golden_load(path, golden);
}

cb_result_code_t cb_golden_save(const char *path, const cb_golden_ref_t *golden)
{
    FILE *fp;
    char hash_hex[65];

    if (path == NULL || golden == NULL) return CB_ERR_NULL_PTR;

    fp = fopen(path, "w");
    if (fp == NULL) return CB_ERR_GOLDEN_LOAD;

    cb_hash_to_hex(&golden->output_hash, hash_hex);

    fprintf(fp, "{\n");
    fprintf(fp, "  \"version\": \"1.0\",\n");
    fprintf(fp, "  \"format\": \"cb_golden_ref\",\n");
    fprintf(fp, "  \"output_hash\": \"%s\",\n", hash_hex);
    fprintf(fp, "  \"sample_count\": %u,\n", golden->sample_count);
    fprintf(fp, "  \"output_size\": %u,\n", golden->output_size);
    fprintf(fp, "  \"platform\": \"%s\"\n", golden->platform);
    fprintf(fp, "}\n");
    fclose(fp);
    return CB_OK;
}

cb_result_code_t cb_verify_save_golden(const char *path, const cb_golden_ref_t *golden)
{
    return cb_golden_save(path, golden);
}

/*─────────────────────────────────────────────────────────────────────────────
 * Golden Verification
 *─────────────────────────────────────────────────────────────────────────────*/

bool cb_golden_verify(const cb_hash_t *computed, const cb_golden_ref_t *golden)
{
    if (computed == NULL || golden == NULL) return false;
    /* Golden is valid if output_hash is non-zero */
    return cb_hash_equal(computed, &golden->output_hash);
}

/*─────────────────────────────────────────────────────────────────────────────
 * Result Binding
 *─────────────────────────────────────────────────────────────────────────────*/

static void write_le64(uint8_t *buf, uint64_t value)
{
    buf[0] = (uint8_t)(value);       buf[1] = (uint8_t)(value >> 8);
    buf[2] = (uint8_t)(value >> 16); buf[3] = (uint8_t)(value >> 24);
    buf[4] = (uint8_t)(value >> 32); buf[5] = (uint8_t)(value >> 40);
    buf[6] = (uint8_t)(value >> 48); buf[7] = (uint8_t)(value >> 56);
}

cb_result_code_t cb_compute_result_hash(const cb_hash_t *output_hash,
                                        const char *platform,
                                        uint64_t config_hash,
                                        const cb_latency_stats_t *stats,
                                        uint64_t timestamp_unix,
                                        cb_hash_t *result_hash)
{
    cb_verify_ctx_t ctx;
    uint8_t le64_buf[8];
    char platform_padded[32];
    static const char prefix[] = "CB:RESULT:v1";

    if (output_hash == NULL || platform == NULL || stats == NULL || result_hash == NULL)
        return CB_ERR_NULL_PTR;

    cb_verify_ctx_init(&ctx);
    cb_verify_ctx_update(&ctx, prefix, (uint32_t)strlen(prefix));
    cb_verify_ctx_update(&ctx, output_hash->bytes, CB_HASH_SIZE);

    memset(platform_padded, 0, sizeof(platform_padded));
    strncpy(platform_padded, platform, sizeof(platform_padded) - 1);
    cb_verify_ctx_update(&ctx, platform_padded, 32);

    write_le64(le64_buf, config_hash);
    cb_verify_ctx_update(&ctx, le64_buf, 8);

    write_le64(le64_buf, stats->min_ns);
    cb_verify_ctx_update(&ctx, le64_buf, 8);
    write_le64(le64_buf, stats->max_ns);
    cb_verify_ctx_update(&ctx, le64_buf, 8);
    write_le64(le64_buf, stats->mean_ns);
    cb_verify_ctx_update(&ctx, le64_buf, 8);
    write_le64(le64_buf, stats->p99_ns);
    cb_verify_ctx_update(&ctx, le64_buf, 8);

    write_le64(le64_buf, timestamp_unix);
    cb_verify_ctx_update(&ctx, le64_buf, 8);

    cb_verify_ctx_final(&ctx, result_hash);
    return CB_OK;
}

cb_result_code_t cb_compute_result_binding(const cb_hash_t *output_hash,
                                           const char *platform,
                                           uint64_t config_hash,
                                           const cb_latency_stats_t *stats,
                                           uint64_t timestamp_unix,
                                           cb_hash_t *result_hash)
{
    return cb_compute_result_hash(output_hash, platform, config_hash,
                                  stats, timestamp_unix, result_hash);
}
