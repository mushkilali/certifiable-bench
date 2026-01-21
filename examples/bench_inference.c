/**
 * @file bench_inference.c
 * @project Certifiable Bench
 * @brief Main benchmark executable.
 *
 * @traceability SRS-003-RUNNER
 * @compliance MISRA-C:2012
 *
 * @author William Murray
 * @copyright Copyright (c) 2026 The Murray Family Innovation Trust. All rights reserved.
 * @license GPL-3.0 or Commercial
 */

#include <stdio.h>
#include <string.h>

#include "cb_types.h"
#include "cb_timer.h"
#include "cb_runner.h"
#include "cb_report.h"
#include "cb_platform.h"
#include "cb_verify.h"

/*─────────────────────────────────────────────────────────────────────────────
 * Sample Buffer (statically allocated)
 *─────────────────────────────────────────────────────────────────────────────*/

#define MAX_SAMPLES 10000
static uint64_t g_sample_buffer[MAX_SAMPLES];

/*─────────────────────────────────────────────────────────────────────────────
 * Mock Inference Function (replace with certifiable-inference integration)
 *─────────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Mock inference function for demonstration
 *
 * In production, this would call certifiable-inference's forward pass.
 * For now, it performs a deterministic computation.
 */
static cb_result_code_t mock_inference(void *ctx, const void *input, void *output)
{
    (void)ctx;
    const uint8_t *in = (const uint8_t *)input;
    uint8_t *out = (uint8_t *)output;

    /* Deterministic transformation: XOR with position and rotate */
    for (uint32_t i = 0; i < 1024; i++) {
        out[i] = (uint8_t)((in[i] ^ (uint8_t)i) + 0x5A);
    }

    return CB_OK;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Main
 *─────────────────────────────────────────────────────────────────────────────*/

static void print_usage(const char *prog)
{
    printf("Usage: %s [options]\n", prog);
    printf("\nOptions:\n");
    printf("  --iterations N     Measurement iterations (default: 1000, max: %d)\n", MAX_SAMPLES);
    printf("  --warmup N         Warmup iterations (default: 100)\n");
    printf("  --batch N          Batch size (default: 1)\n");
    printf("  --output PATH      Output JSON path (default: stdout summary)\n");
    printf("  --csv PATH         Output CSV path\n");
    printf("  --compare PATH     Compare with previous JSON result\n");
    printf("  --help             Show this help\n");
    printf("\nExample:\n");
    printf("  %s --iterations 5000 --output result.json\n", prog);
}

static int parse_int(const char *str)
{
    int val = 0;
    while (*str >= '0' && *str <= '9') {
        val = val * 10 + (*str - '0');
        str++;
    }
    return val;
}

int main(int argc, char *argv[])
{
    cb_config_t config;
    cb_result_t result;
    cb_result_code_t rc;
    const char *output_json = NULL;
    const char *output_csv = NULL;
    const char *compare_path = NULL;
    int i;

    /* Initialise with defaults */
    cb_config_init(&config);

    /* Parse arguments */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--iterations") == 0 && i + 1 < argc) {
            config.measure_iterations = (uint32_t)parse_int(argv[++i]);
        } else if (strcmp(argv[i], "--warmup") == 0 && i + 1 < argc) {
            config.warmup_iterations = (uint32_t)parse_int(argv[++i]);
        } else if (strcmp(argv[i], "--batch") == 0 && i + 1 < argc) {
            config.batch_size = (uint32_t)parse_int(argv[++i]);
        } else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            output_json = argv[++i];
        } else if (strcmp(argv[i], "--csv") == 0 && i + 1 < argc) {
            output_csv = argv[++i];
        } else if (strcmp(argv[i], "--compare") == 0 && i + 1 < argc) {
            compare_path = argv[++i];
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    /* Check buffer capacity */
    if (config.measure_iterations > MAX_SAMPLES) {
        fprintf(stderr, "Error: iterations (%u) exceeds buffer capacity (%d)\n",
                config.measure_iterations, MAX_SAMPLES);
        return 1;
    }

    /* Validate config */
    rc = cb_config_validate(&config);
    if (rc != CB_OK) {
        fprintf(stderr, "Invalid configuration: %d\n", rc);
        return 1;
    }

    /* Print banner */
    printf("═══════════════════════════════════════════════════════════════════\n");
    printf("  certifiable-bench v1.0.0\n");
    printf("═══════════════════════════════════════════════════════════════════\n\n");

    /* Initialise timer */
    cb_timer_init(CB_TIMER_AUTO);
    printf("Timer:       %s (resolution: %lu ns)\n",
           cb_timer_name(), (unsigned long)cb_timer_resolution_ns());

    /* Platform info */
    char cpu_model[128];
    cb_cpu_model(cpu_model, sizeof(cpu_model));
    printf("Platform:    %s\n", cb_platform_name());
    printf("CPU:         %s\n", cpu_model);
    printf("Frequency:   %u MHz\n", cb_cpu_freq_mhz());
    printf("Iterations:  %u warmup, %u measure\n\n",
           config.warmup_iterations, config.measure_iterations);

    /* Prepare test data */
    uint8_t input[1024];
    uint8_t output[1024];
    for (uint32_t j = 0; j < sizeof(input); j++) {
        input[j] = (uint8_t)(j & 0xFF);
    }

    /* Run benchmark with caller-provided buffer */
    printf("Running benchmark...\n");
    rc = cb_run_benchmark(&config, mock_inference, NULL,
                          input, sizeof(input),
                          output, sizeof(output),
                          g_sample_buffer, MAX_SAMPLES,
                          &result);

    if (rc != CB_OK) {
        fprintf(stderr, "Benchmark failed: %d\n", rc);
        return 1;
    }

    /* Print summary */
    printf("\n");
    cb_print_summary(&result);

    /* Write JSON output */
    if (output_json != NULL) {
        rc = cb_write_json(&result, output_json);
        if (rc == CB_OK) {
            printf("\nJSON written to: %s\n", output_json);
        } else {
            fprintf(stderr, "Failed to write JSON: %d\n", rc);
        }
    }

    /* Write CSV output */
    if (output_csv != NULL) {
        rc = cb_write_csv(&result, output_csv);
        if (rc == CB_OK) {
            printf("CSV written to: %s\n", output_csv);
        } else {
            fprintf(stderr, "Failed to write CSV: %d\n", rc);
        }
    }

    /* Compare with previous result */
    if (compare_path != NULL) {
        cb_result_t baseline;
        cb_comparison_t comparison;

        rc = cb_load_json(compare_path, &baseline);
        if (rc != CB_OK) {
            fprintf(stderr, "Failed to load baseline: %d\n", rc);
        } else {
            cb_compare_results(&baseline, &result, &comparison);
            printf("\n");
            cb_print_comparison(&comparison);
        }
    }

    return 0;
}
