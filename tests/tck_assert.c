/*
 * Test Compatibility Kit (TCK) Assertion Helpers - Implementation
 * 
 * This file implements the helper functions and global variables
 * needed by the TCK assertion macros.
 */

#include "tck_assert.h"

/* Global test counters (used when Unity is not available) */
#ifndef UNITY_VERSION
int g_tck_tests_run = 0;
int g_tck_tests_passed = 0; 
int g_tck_tests_failed = 0;
int g_tck_tests_skipped = 0;
#endif

/* Global feature availability mask */
unsigned int g_tck_available_features = 0;

/*
 * Initialize TCK assertion framework
 */
void tck_assert_init(void) {
    #ifndef UNITY_VERSION
    g_tck_tests_run = 0;
    g_tck_tests_passed = 0;
    g_tck_tests_failed = 0;
    g_tck_tests_skipped = 0;
    #endif
    
    /* Set default available features based on compile-time settings */
    g_tck_available_features = tck_get_default_features();
    
    printf("TCK Assertion Framework initialized\n");
    printf("Available features: 0x%04x\n", g_tck_available_features);
}

/*
 * Cleanup TCK assertion framework
 */
void tck_assert_cleanup(void) {
    printf("TCK Assertion Framework cleanup complete\n");
}

/*
 * Print test summary
 */
void tck_assert_print_summary(void) {
    #ifdef UNITY_VERSION
    /* Unity handles its own summary */
    printf("Test summary handled by Unity framework\n");
    #else
    printf("\n=== TCK Test Summary ===\n");
    printf("Total tests: %d\n", g_tck_tests_run);
    printf("Passed: %d\n", g_tck_tests_passed);
    printf("Failed: %d\n", g_tck_tests_failed);
    printf("Skipped: %d\n", g_tck_tests_skipped);
    
    if (g_tck_tests_failed > 0) {
        printf("Result: FAILURE (%d tests failed)\n", g_tck_tests_failed);
    } else if (g_tck_tests_passed > 0) {
        printf("Result: SUCCESS (all %d tests passed)\n", g_tck_tests_passed);
    } else {
        printf("Result: NO TESTS RUN\n");
    }
    printf("========================\n");
    #endif
}

/*
 * Set available features mask
 */
void tck_set_available_features(unsigned int features) {
    g_tck_available_features = features;
    printf("TCK available features updated to: 0x%04x\n", g_tck_available_features);
}

/*
 * Get default available features based on compile-time settings
 */
unsigned int tck_get_default_features(void) {
    unsigned int features = 0;
    
    #if TCK_FEATURE_PARSER_ENABLED
    features |= TCK_FEATURE_BASIC_PARSING;
    #endif
    
    #if TCK_FEATURE_VALIDATOR_ENABLED
    features |= TCK_FEATURE_SYNTAX_VALIDATION;
    features |= TCK_FEATURE_SEMANTIC_VALIDATION;
    #endif
    
    #if TCK_FEATURE_EXECUTOR_ENABLED
    features |= TCK_FEATURE_QUERY_EXECUTION;
    features |= TCK_FEATURE_RESULT_COMPARISON;
    features |= TCK_FEATURE_JSON_RESULTS;
    #endif
    
    return features;
}
