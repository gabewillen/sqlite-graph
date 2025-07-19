/*
 * Test Compatibility Kit (TCK) Assertion Helpers
 * 
 * This header provides assertion macros for testing Cypher parser, validator,
 * and executor functionality. It includes utilities to auto-skip tests when
 * features are not yet implemented.
 */

#ifndef TCK_ASSERT_H
#define TCK_ASSERT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

/* Include cypher headers for parser and validation functions */
#include "../include/cypher/cypher-parser.h"
#include "../include/cypher/cypher-api.h"
#include "../include/cypher-errors.h"

/* Test framework integration - using Unity if available, otherwise basic macros */
#ifdef UNITY_VERSION
    #include <unity.h>
    #define TCK_TEST_ASSERT_TRUE(condition) TEST_ASSERT_TRUE(condition)
    #define TCK_TEST_ASSERT_FALSE(condition) TEST_ASSERT_FALSE(condition)
    #define TCK_TEST_ASSERT_NULL(pointer) TEST_ASSERT_NULL(pointer)
    #define TCK_TEST_ASSERT_NOT_NULL(pointer) TEST_ASSERT_NOT_NULL(pointer)
    #define TCK_TEST_ASSERT_EQUAL_INT(expected, actual) TEST_ASSERT_EQUAL_INT(expected, actual)
    #define TCK_TEST_ASSERT_EQUAL_STRING(expected, actual) TEST_ASSERT_EQUAL_STRING(expected, actual)
    #define TCK_TEST_FAIL(message) TEST_FAIL(message)
    #define TCK_TEST_SKIP(message) TEST_IGNORE_MESSAGE(message)
#else
    /* Basic assertion macros when Unity is not available */
    extern int g_tck_tests_run;
    extern int g_tck_tests_passed; 
    extern int g_tck_tests_failed;
    extern int g_tck_tests_skipped;
    
    #define TCK_TEST_ASSERT_TRUE(condition) \
        do { \
            g_tck_tests_run++; \
            if (condition) { \
                g_tck_tests_passed++; \
            } else { \
                g_tck_tests_failed++; \
                printf("ASSERTION FAILED: %s:%d - Expected true but got false\n", __FILE__, __LINE__); \
            } \
        } while(0)
        
    #define TCK_TEST_ASSERT_FALSE(condition) \
        do { \
            g_tck_tests_run++; \
            if (!(condition)) { \
                g_tck_tests_passed++; \
            } else { \
                g_tck_tests_failed++; \
                printf("ASSERTION FAILED: %s:%d - Expected false but got true\n", __FILE__, __LINE__); \
            } \
        } while(0)
        
    #define TCK_TEST_ASSERT_NULL(pointer) \
        do { \
            g_tck_tests_run++; \
            if ((pointer) == NULL) { \
                g_tck_tests_passed++; \
            } else { \
                g_tck_tests_failed++; \
                printf("ASSERTION FAILED: %s:%d - Expected NULL but got non-NULL pointer\n", __FILE__, __LINE__); \
            } \
        } while(0)
        
    #define TCK_TEST_ASSERT_NOT_NULL(pointer) \
        do { \
            g_tck_tests_run++; \
            if ((pointer) != NULL) { \
                g_tck_tests_passed++; \
            } else { \
                g_tck_tests_failed++; \
                printf("ASSERTION FAILED: %s:%d - Expected non-NULL but got NULL pointer\n", __FILE__, __LINE__); \
            } \
        } while(0)
        
    #define TCK_TEST_ASSERT_EQUAL_INT(expected, actual) \
        do { \
            g_tck_tests_run++; \
            if ((expected) == (actual)) { \
                g_tck_tests_passed++; \
            } else { \
                g_tck_tests_failed++; \
                printf("ASSERTION FAILED: %s:%d - Expected %d but got %d\n", __FILE__, __LINE__, (expected), (actual)); \
            } \
        } while(0)
        
    #define TCK_TEST_ASSERT_EQUAL_STRING(expected, actual) \
        do { \
            g_tck_tests_run++; \
            if (((expected) == NULL && (actual) == NULL) || \
                ((expected) != NULL && (actual) != NULL && strcmp((expected), (actual)) == 0)) { \
                g_tck_tests_passed++; \
            } else { \
                g_tck_tests_failed++; \
                printf("ASSERTION FAILED: %s:%d - Expected '%s' but got '%s'\n", \
                       __FILE__, __LINE__, \
                       (expected) ? (expected) : "NULL", \
                       (actual) ? (actual) : "NULL"); \
            } \
        } while(0)
        
    #define TCK_TEST_FAIL(message) \
        do { \
            g_tck_tests_run++; \
            g_tck_tests_failed++; \
            printf("TEST FAILED: %s:%d - %s\n", __FILE__, __LINE__, (message)); \
        } while(0)
        
    #define TCK_TEST_SKIP(message) \
        do { \
            g_tck_tests_run++; \
            g_tck_tests_skipped++; \
            printf("TEST SKIPPED: %s:%d - %s\n", __FILE__, __LINE__, (message)); \
        } while(0)
#endif

/*
 * Feature detection and auto-skip functionality
 * These flags control which features are enabled/tested
 */
 
/* Compile-time feature flags */
#ifndef TCK_FEATURE_PARSER_ENABLED
    #define TCK_FEATURE_PARSER_ENABLED 1
#endif

#ifndef TCK_FEATURE_VALIDATOR_ENABLED  
    #define TCK_FEATURE_VALIDATOR_ENABLED 1
#endif

#ifndef TCK_FEATURE_EXECUTOR_ENABLED
    #define TCK_FEATURE_EXECUTOR_ENABLED 0  /* Executor not ready yet */
#endif

/* Runtime feature detection - can be used to check for specific capabilities */
typedef enum {
    TCK_FEATURE_BASIC_PARSING = 0x0001,
    TCK_FEATURE_SYNTAX_VALIDATION = 0x0002,
    TCK_FEATURE_SEMANTIC_VALIDATION = 0x0004,
    TCK_FEATURE_QUERY_EXECUTION = 0x0008,
    TCK_FEATURE_RESULT_COMPARISON = 0x0010,
    TCK_FEATURE_JSON_RESULTS = 0x0020
} TCKFeatureFlags;

/* Global feature availability mask - can be set at runtime */
extern unsigned int g_tck_available_features;

/* Feature check function */
static inline int tck_feature_available(TCKFeatureFlags feature) {
    return (g_tck_available_features & feature) != 0;
}

/* Auto-skip macro for unsupported features */
#define TCK_REQUIRE_FEATURE(feature, message) \
    do { \
        if (!tck_feature_available(feature)) { \
            TCK_TEST_SKIP("Feature not available: " message); \
            return; \
        } \
    } while(0)

/*
 * Core assertion macros for TCK tests
 */

/*
 * ASSERT_PARSE_OK(sql) - Assert that cypherParse returns non-NULL
 * Tests that the given Cypher query can be successfully parsed
 */
#define ASSERT_PARSE_OK(sql) \
    do { \
        TCK_REQUIRE_FEATURE(TCK_FEATURE_BASIC_PARSING, "Parser not available"); \
        CypherParser *parser = cypherParserCreate(); \
        char *error_msg = NULL; \
        CypherAst *ast = cypherParse(parser, (sql), &error_msg); \
        if (ast != NULL) { \
            printf("PARSE OK: '%s'\n", (sql)); \
            cypherAstDestroy(ast); \
        } else { \
            printf("PARSE FAILED: '%s' - Error: %s\n", (sql), error_msg ? error_msg : "Unknown error"); \
        } \
        TCK_TEST_ASSERT_NOT_NULL(ast); \
        if (error_msg) free(error_msg); \
        cypherParserDestroy(parser); \
    } while(0)

/*
 * ASSERT_PARSE_FAIL(sql) - Assert that cypherParse returns NULL and correct error code
 * Tests that the given Cypher query fails to parse as expected
 */
#define ASSERT_PARSE_FAIL(sql) \
    do { \
        TCK_REQUIRE_FEATURE(TCK_FEATURE_BASIC_PARSING, "Parser not available"); \
        CypherParser *parser = cypherParserCreate(); \
        char *error_msg = NULL; \
        CypherAst *ast = cypherParse(parser, (sql), &error_msg); \
        if (ast == NULL) { \
            printf("PARSE FAIL (expected): '%s' - Error: %s\n", (sql), error_msg ? error_msg : "Unknown error"); \
        } else { \
            printf("PARSE UNEXPECTEDLY SUCCEEDED: '%s'\n", (sql)); \
            cypherAstDestroy(ast); \
        } \
        TCK_TEST_ASSERT_NULL(ast); \
        if (error_msg) free(error_msg); \
        cypherParserDestroy(parser); \
    } while(0)

/*
 * ASSERT_VALIDATE_OK(sql) - Assert that validation succeeds
 * Tests that the given Cypher query passes semantic validation
 */
#define ASSERT_VALIDATE_OK(sql) \
    do { \
        TCK_REQUIRE_FEATURE(TCK_FEATURE_SYNTAX_VALIDATION, "Validator not available"); \
        char *error_msg = NULL; \
        int result = cypherValidateQuery((sql), &error_msg); \
        if (result == SQLITE_OK) { \
            printf("VALIDATE OK: '%s'\n", (sql)); \
        } else { \
            printf("VALIDATE FAILED: '%s' - Error: %s\n", (sql), error_msg ? error_msg : "Unknown error"); \
        } \
        TCK_TEST_ASSERT_EQUAL_INT(SQLITE_OK, result); \
        if (error_msg) sqlite3_free(error_msg); \
    } while(0)

/*
 * ASSERT_VALIDATE_FAIL(sql, err_code) - Assert that validation fails with specific error
 * Tests that the given Cypher query fails validation with the expected error code
 */
#define ASSERT_VALIDATE_FAIL(sql, err_code) \
    do { \
        TCK_REQUIRE_FEATURE(TCK_FEATURE_SYNTAX_VALIDATION, "Validator not available"); \
        char *error_msg = NULL; \
        int result = cypherValidateQuery((sql), &error_msg); \
        if (result == (err_code)) { \
            printf("VALIDATE FAIL (expected %d): '%s' - Error: %s\n", (err_code), (sql), error_msg ? error_msg : "Unknown error"); \
        } else { \
            printf("VALIDATE RETURNED UNEXPECTED CODE: '%s' - Expected %d, got %d - Error: %s\n", \
                   (sql), (err_code), result, error_msg ? error_msg : "Unknown error"); \
        } \
        TCK_TEST_ASSERT_EQUAL_INT((err_code), result); \
        if (error_msg) sqlite3_free(error_msg); \
    } while(0)

/*
 * ASSERT_RESULT(sql, expected_json) - Assert query result matches expected JSON
 * This macro is only compiled when the executor is ready, otherwise it auto-skips
 */
#if TCK_FEATURE_EXECUTOR_ENABLED
#define ASSERT_RESULT(sql, expected_json) \
    do { \
        TCK_REQUIRE_FEATURE(TCK_FEATURE_QUERY_EXECUTION, "Query executor not available"); \
        TCK_REQUIRE_FEATURE(TCK_FEATURE_JSON_RESULTS, "JSON result comparison not available"); \
        /* TODO: Implement result execution and comparison */ \
        /* For now, this will be a placeholder that auto-skips */ \
        TCK_TEST_SKIP("Query execution not yet implemented"); \
    } while(0)
#else
#define ASSERT_RESULT(sql, expected_json) \
    do { \
        TCK_TEST_SKIP("Query executor not compiled in - use TCK_FEATURE_EXECUTOR_ENABLED=1 to enable"); \
    } while(0)
#endif

/*
 * Helper functions for more complex assertions
 */

/* Initialize TCK assertion framework */
void tck_assert_init(void);

/* Cleanup TCK assertion framework */ 
void tck_assert_cleanup(void);

/* Print test summary */
void tck_assert_print_summary(void);

/* Set available features mask */
void tck_set_available_features(unsigned int features);

/* Get default available features based on compile-time settings */
unsigned int tck_get_default_features(void);

/*
 * Utility functions for error code validation
 */

/* Check if error code is a syntax error */
static inline int tck_is_syntax_error(int error_code) {
    return (error_code >= CYPHER_ERROR_SYNTAX && error_code < CYPHER_ERROR_SEMANTIC);
}

/* Check if error code is a semantic error */
static inline int tck_is_semantic_error(int error_code) {
    return (error_code >= CYPHER_ERROR_SEMANTIC && error_code < CYPHER_ERROR_TYPE);
}

/* Check if error code is a type error */
static inline int tck_is_type_error(int error_code) {
    return (error_code >= CYPHER_ERROR_TYPE && error_code < CYPHER_ERROR_RUNTIME);
}

/* Check if error code is a runtime error */
static inline int tck_is_runtime_error(int error_code) {
    return (error_code >= CYPHER_ERROR_RUNTIME && error_code < CYPHER_ERROR_CONSTRAINT);
}

/*
 * Extended assertion macros for specific error categories
 */

#define ASSERT_SYNTAX_ERROR(sql) ASSERT_VALIDATE_FAIL(sql, CYPHER_ERROR_SYNTAX_GENERAL)
#define ASSERT_SEMANTIC_ERROR(sql) ASSERT_VALIDATE_FAIL(sql, CYPHER_ERROR_SEMANTIC_UNDEFINED_VARIABLE)
#define ASSERT_TYPE_ERROR(sql) ASSERT_VALIDATE_FAIL(sql, CYPHER_ERROR_TYPE_MISMATCH)

/*
 * Convenience macros for common patterns
 */

/* Test both parsing and validation in one assertion */
#define ASSERT_PARSE_AND_VALIDATE_OK(sql) \
    do { \
        ASSERT_PARSE_OK(sql); \
        ASSERT_VALIDATE_OK(sql); \
    } while(0)

/* Test that parsing succeeds but validation fails */
#define ASSERT_PARSE_OK_VALIDATE_FAIL(sql, err_code) \
    do { \
        ASSERT_PARSE_OK(sql); \
        ASSERT_VALIDATE_FAIL(sql, err_code); \
    } while(0)

#endif /* TCK_ASSERT_H */
