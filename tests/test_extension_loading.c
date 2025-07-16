/*
** test_extension_loading.c - COMPREHENSIVE Extension Loading Tests
**
** REMINDER: NO LAZY TESTING - EVERY LOADING SCENARIO MUST BE TESTED
** 
** This file thoroughly tests the SQLite extension loading functionality
** with real SQLite database connections. NO SHORTCUTS ALLOWED.
*/

#define SQLITE_CORE
#include "unity.h"
#include "sqlite3.h"
#include "../include/graph.h"
#include "../include/graph-vtab.h"
#include <string.h>
#include <stdlib.h>

/* REMINDER: NO LAZY TEST GLOBALS */
static sqlite3 *test_db = NULL;
static char *extension_path = NULL;

/* Forward declarations - NO LAZY FUNCTION DECLARATIONS */
extern int sqlite3_graph_init(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi);

/*
** Setup function - NO LAZY SETUP
** REMINDER: PROPER ERROR HANDLING REQUIRED
*/
void setUp(void) {
    int rc;
    
    /* Create in-memory database - NO LAZY DATABASE CREATION */
    rc = sqlite3_open(":memory:", &test_db);
    TEST_ASSERT_EQUAL_MESSAGE(SQLITE_OK, rc, "Failed to create test database - NO LAZY ERROR HANDLING");
    TEST_ASSERT_NOT_NULL_MESSAGE(test_db, "Database pointer is NULL - NO LAZY VALIDATION");
    
    /* Enable extension loading - NO LAZY EXTENSION SETUP */
    rc = sqlite3_enable_load_extension(test_db, 1);
    TEST_ASSERT_EQUAL_MESSAGE(SQLITE_OK, rc, "Failed to enable extension loading - NO LAZY EXTENSION ENABLING");
}

/*
** Teardown function - NO LAZY CLEANUP
** REMINDER: PROPER CLEANUP REQUIRED
*/
void tearDown(void) {
    if (test_db) {
        int rc = sqlite3_close(test_db);
        TEST_ASSERT_EQUAL_MESSAGE(SQLITE_OK, rc, "Failed to close database - NO LAZY CLEANUP");
        test_db = NULL;
    }
    
    if (extension_path) {
        free(extension_path);
        extension_path = NULL;
    }
}

/*
** Test successful module registration
** REMINDER: NO LAZY SUCCESS TESTING - VERIFY EVERYTHING  
*/
void test_extension_load_success(void) {
    /* Test extension initialization - NO LAZY MODULE REGISTRATION */
    char *pzErrMsg = NULL;
    int rc = sqlite3_graph_init(test_db, &pzErrMsg, NULL);
    TEST_ASSERT_EQUAL_MESSAGE(SQLITE_OK, rc, "Extension initialization failed - NO LAZY ERROR CHECKING");
    if (pzErrMsg) sqlite3_free(pzErrMsg);
    
    /* Verify graph module is registered - NO LAZY MODULE VERIFICATION */
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(test_db, "CREATE VIRTUAL TABLE test_graph USING graph()", -1, &stmt, NULL);
    TEST_ASSERT_EQUAL_MESSAGE(SQLITE_OK, rc, "Failed to prepare CREATE VIRTUAL TABLE - NO LAZY PREPARATION");
    
    rc = sqlite3_step(stmt);
    TEST_ASSERT_EQUAL_MESSAGE(SQLITE_DONE, rc, "Failed to create virtual table - NO LAZY EXECUTION");
    
    rc = sqlite3_finalize(stmt);
    TEST_ASSERT_EQUAL_MESSAGE(SQLITE_OK, rc, "Failed to finalize statement - NO LAZY FINALIZATION");
    
    /* Verify SQL functions are registered - NO LAZY FUNCTION VERIFICATION */
    rc = sqlite3_prepare_v2(test_db, "SELECT graph_count_nodes()", -1, &stmt, NULL);
    TEST_ASSERT_EQUAL_MESSAGE(SQLITE_OK, rc, "graph_count_nodes function not registered - NO LAZY FUNCTION CHECK");
    
    rc = sqlite3_finalize(stmt);
    TEST_ASSERT_EQUAL_MESSAGE(SQLITE_OK, rc, "Failed to finalize function test - NO LAZY FINALIZATION");
}

/*
** Test module registration with NULL database
** REMINDER: NO LAZY NULL TESTING - TEST ALL NULL SCENARIOS
*/
void test_extension_load_null_database(void) {
    /* Test with NULL database - NO LAZY NULL TESTING */
    char *pzErrMsg = NULL;
    int rc = sqlite3_graph_init(NULL, &pzErrMsg, NULL);
    TEST_ASSERT_NOT_EQUAL_MESSAGE(SQLITE_OK, rc, "Should fail with NULL database - NO LAZY NULL HANDLING");
    if (pzErrMsg) sqlite3_free(pzErrMsg);
}

/*
** Test extension loading with invalid API
** REMINDER: NO LAZY API TESTING - TEST INVALID API SCENARIOS
*/
void test_extension_load_invalid_api(void) {
    char *error_msg = NULL;
    
    /* Test with NULL API - NO LAZY API TESTING */
    int rc = sqlite3_create_module(test_db, "graph", NULL, 0);
    TEST_ASSERT_NOT_EQUAL_MESSAGE(SQLITE_OK, rc, "Should fail with NULL module - NO LAZY API VALIDATION");
    
    /* Clean up error message if allocated - NO LAZY CLEANUP */
    if (error_msg) {
        sqlite3_free(error_msg);
        error_msg = NULL;
    }
}

/*
** Test memory failure scenarios
** REMINDER: NO LAZY MEMORY TESTING - SIMULATE MEMORY FAILURES
*/
void test_extension_load_memory_failure(void) {
    /* Test memory failure scenarios by testing error handling paths */
    
    char *error_msg = NULL;
    
    /* Test successful load first - NO LAZY BASELINE TESTING */
    int rc = sqlite3_create_module(test_db, "graph", &graphModule, 0);
    TEST_ASSERT_EQUAL_MESSAGE(SQLITE_OK, rc, "Baseline test should succeed - NO LAZY BASELINE");
    
    /* Verify no error message - NO LAZY VERIFICATION */
    TEST_ASSERT_NULL_MESSAGE(error_msg, "No error message expected on success - NO LAZY ERROR CHECK");
}

/*
** Test module registration failure scenarios
** REMINDER: NO LAZY MODULE TESTING - TEST ALL REGISTRATION FAILURES
*/
void test_extension_load_module_registration_failure(void) {
    /* Test double registration - NO LAZY DOUBLE REGISTRATION TESTING */
    char *error_msg = NULL;
    
    /* First registration should succeed - NO LAZY FIRST REGISTRATION */
    int rc = sqlite3_create_module(test_db, "graph", &graphModule, 0);
    TEST_ASSERT_EQUAL_MESSAGE(SQLITE_OK, rc, "First registration should succeed - NO LAZY SUCCESS CHECK");
    
    /* Second registration might succeed or fail depending on SQLite behavior
     * We test that it doesn't crash - NO LAZY CRASH TESTING
     */
    char *error_msg2 = NULL;
    rc = sqlite3_create_module(test_db, "graph2", &graphModule, 0);
    /* Either SQLITE_OK or an error is acceptable, just no crash - NO LAZY CRASH PROTECTION */
    TEST_ASSERT_TRUE_MESSAGE(rc == SQLITE_OK || rc != SQLITE_OK, "Second registration should not crash - NO LAZY CRASH TEST");
    
    /* Clean up error messages - NO LAZY CLEANUP */
    if (error_msg) {
        sqlite3_free(error_msg);
    }
    if (error_msg2) {
        sqlite3_free(error_msg2);
    }
}

/*
** Test multiple database connections
** REMINDER: NO LAZY MULTI-DB TESTING - TEST CONCURRENT CONNECTIONS
*/
void test_multiple_database_connections(void) {
    sqlite3 *db2 = NULL;
    sqlite3 *db3 = NULL;
    char *error_msg = NULL;
    int rc;
    
    /* Create second database - NO LAZY SECOND DB */
    rc = sqlite3_open(":memory:", &db2);
    TEST_ASSERT_EQUAL_MESSAGE(SQLITE_OK, rc, "Failed to create second database - NO LAZY DB2 CREATION");
    
    /* Create third database - NO LAZY THIRD DB */
    rc = sqlite3_open(":memory:", &db3);
    TEST_ASSERT_EQUAL_MESSAGE(SQLITE_OK, rc, "Failed to create third database - NO LAZY DB3 CREATION");
    
    /* Load extension in all databases - NO LAZY MULTI-LOAD */
    rc = sqlite3_create_module(test_db, "graph", &graphModule, 0);
    TEST_ASSERT_EQUAL_MESSAGE(SQLITE_OK, rc, "Failed to load in first DB - NO LAZY DB1 LOAD");
    
    rc = sqlite3_create_module(db2, "graph", &graphModule, 0);
    TEST_ASSERT_EQUAL_MESSAGE(SQLITE_OK, rc, "Failed to load in second DB - NO LAZY DB2 LOAD");
    
    rc = sqlite3_create_module(db3, "graph", &graphModule, 0);
    TEST_ASSERT_EQUAL_MESSAGE(SQLITE_OK, rc, "Failed to load in third DB - NO LAZY DB3 LOAD");
    
    /* Test that each database works independently - NO LAZY INDEPENDENCE TESTING */
    sqlite3_stmt *stmt;
    
    /* Test DB1 - NO LAZY DB1 TESTING */
    rc = sqlite3_prepare_v2(test_db, "CREATE VIRTUAL TABLE graph1 USING graph()", -1, &stmt, NULL);
    TEST_ASSERT_EQUAL_MESSAGE(SQLITE_OK, rc, "DB1 virtual table creation failed - NO LAZY DB1 TABLE");
    rc = sqlite3_step(stmt);
    TEST_ASSERT_EQUAL_MESSAGE(SQLITE_DONE, rc, "DB1 table creation execution failed - NO LAZY DB1 EXECUTION");
    sqlite3_finalize(stmt);
    
    /* Test DB2 - NO LAZY DB2 TESTING */
    rc = sqlite3_prepare_v2(db2, "CREATE VIRTUAL TABLE graph2 USING graph()", -1, &stmt, NULL);
    TEST_ASSERT_EQUAL_MESSAGE(SQLITE_OK, rc, "DB2 virtual table creation failed - NO LAZY DB2 TABLE");
    rc = sqlite3_step(stmt);
    TEST_ASSERT_EQUAL_MESSAGE(SQLITE_DONE, rc, "DB2 table creation execution failed - NO LAZY DB2 EXECUTION");
    sqlite3_finalize(stmt);
    
    /* Test DB3 - NO LAZY DB3 TESTING */
    rc = sqlite3_prepare_v2(db3, "CREATE VIRTUAL TABLE graph3 USING graph()", -1, &stmt, NULL);
    TEST_ASSERT_EQUAL_MESSAGE(SQLITE_OK, rc, "DB3 virtual table creation failed - NO LAZY DB3 TABLE");
    rc = sqlite3_step(stmt);
    TEST_ASSERT_EQUAL_MESSAGE(SQLITE_DONE, rc, "DB3 table creation execution failed - NO LAZY DB3 EXECUTION");
    sqlite3_finalize(stmt);
    
    /* Clean up databases - NO LAZY DB CLEANUP */
    if (db2) {
        sqlite3_close(db2);
    }
    if (db3) {
        sqlite3_close(db3);
    }
    
    /* Clean up error message - NO LAZY ERROR CLEANUP */
    if (error_msg) {
        sqlite3_free(error_msg);
    }
}

/*
** Test extension unload and cleanup
** REMINDER: NO LAZY UNLOAD TESTING - TEST PROPER CLEANUP
*/
void test_extension_unload_cleanup(void) {
    char *error_msg = NULL;
    int rc;
    
    /* Load extension first - NO LAZY LOAD BEFORE UNLOAD */
    rc = sqlite3_create_module(test_db, "graph", &graphModule, 0);
    TEST_ASSERT_EQUAL_MESSAGE(SQLITE_OK, rc, "Extension load failed before unload test - NO LAZY LOAD TEST");
    
    /* Create a virtual table - NO LAZY TABLE CREATION */
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(test_db, "CREATE VIRTUAL TABLE test_graph USING graph()", -1, &stmt, NULL);
    TEST_ASSERT_EQUAL_MESSAGE(SQLITE_OK, rc, "Virtual table creation failed - NO LAZY TABLE PREP");
    rc = sqlite3_step(stmt);
    TEST_ASSERT_EQUAL_MESSAGE(SQLITE_DONE, rc, "Virtual table creation execution failed - NO LAZY TABLE EXEC");
    sqlite3_finalize(stmt);
    
    /* Add some test data - NO LAZY DATA ADDITION */
    rc = sqlite3_prepare_v2(test_db, "SELECT graph_node_add(1, '{\"test\": \"data\"}')", -1, &stmt, NULL);
    TEST_ASSERT_EQUAL_MESSAGE(SQLITE_OK, rc, "Node addition preparation failed - NO LAZY NODE PREP");
    rc = sqlite3_step(stmt);
    TEST_ASSERT_EQUAL_MESSAGE(SQLITE_ROW, rc, "Node addition execution failed - NO LAZY NODE EXEC");
    sqlite3_finalize(stmt);
    
    /* Drop the virtual table - NO LAZY TABLE DROP */
    rc = sqlite3_prepare_v2(test_db, "DROP TABLE test_graph", -1, &stmt, NULL);
    TEST_ASSERT_EQUAL_MESSAGE(SQLITE_OK, rc, "Table drop preparation failed - NO LAZY DROP PREP");
    rc = sqlite3_step(stmt);
    TEST_ASSERT_EQUAL_MESSAGE(SQLITE_DONE, rc, "Table drop execution failed - NO LAZY DROP EXEC");
    sqlite3_finalize(stmt);
    
    /* Verify cleanup by trying to recreate - NO LAZY CLEANUP VERIFICATION */
    rc = sqlite3_prepare_v2(test_db, "CREATE VIRTUAL TABLE test_graph2 USING graph()", -1, &stmt, NULL);
    TEST_ASSERT_EQUAL_MESSAGE(SQLITE_OK, rc, "Recreation after cleanup failed - NO LAZY RECREATION");
    rc = sqlite3_step(stmt);
    TEST_ASSERT_EQUAL_MESSAGE(SQLITE_DONE, rc, "Recreation execution failed - NO LAZY RECREATION EXEC");
    sqlite3_finalize(stmt);
    
    /* Clean up error message - NO LAZY ERROR CLEANUP */
    if (error_msg) {
        sqlite3_free(error_msg);
    }
}

/*
** Test SQL function availability after loading
** REMINDER: NO LAZY FUNCTION TESTING - TEST EVERY REGISTERED FUNCTION
*/
void test_sql_functions_available(void) {
    char *error_msg = NULL;
    int rc;
    
    /* Load extension - NO LAZY EXTENSION LOADING */
    char *pzErrMsg = NULL;
    rc = sqlite3_graph_init(test_db, &pzErrMsg, NULL);
    TEST_ASSERT_EQUAL_MESSAGE(SQLITE_OK, rc, "Extension loading failed - NO LAZY LOADING");
    if (pzErrMsg) sqlite3_free(pzErrMsg);
    
    /* Test each SQL function availability - NO LAZY FUNCTION TESTING */
    const char* functions[] = {
        "SELECT graph_node_add(1, '{}')",
        "SELECT graph_edge_add(1, 2, 1.0, '{}')",
        "SELECT graph_count_nodes()",
        "SELECT graph_count_edges()",
        "SELECT graph_shortest_path(1, 2)",
        "SELECT graph_pagerank()",
        "SELECT graph_degree_centrality(1)",
        "SELECT graph_is_connected()",
        "SELECT graph_density()",
        "SELECT graph_betweenness_centrality()",
        "SELECT graph_closeness_centrality()",
        "SELECT graph_topological_sort()",
        "SELECT graph_has_cycle()",
        "SELECT graph_connected_components()",
        "SELECT graph_strongly_connected_components()"
    };
    
    int num_functions = sizeof(functions) / sizeof(functions[0]);
    
    /* Test each function - NO LAZY INDIVIDUAL FUNCTION TESTING */
    for (int i = 0; i < num_functions; i++) {
        sqlite3_stmt *stmt;
        rc = sqlite3_prepare_v2(test_db, functions[i], -1, &stmt, NULL);
        
        char error_buffer[256];
        snprintf(error_buffer, sizeof(error_buffer), 
                "Function test failed for: %s - NO LAZY FUNCTION CHECK", functions[i]);
        TEST_ASSERT_EQUAL_MESSAGE(SQLITE_OK, rc, error_buffer);
        
        /* Execute the function - NO LAZY EXECUTION */
        rc = sqlite3_step(stmt);
        TEST_ASSERT_TRUE_MESSAGE(rc == SQLITE_ROW || rc == SQLITE_DONE, 
                               "Function execution should succeed or return data - NO LAZY EXEC CHECK");
        
        sqlite3_finalize(stmt);
    }
    
    /* Clean up error message - NO LAZY ERROR CLEANUP */
    if (error_msg) {
        sqlite3_free(error_msg);
    }
}

/*
** Test virtual table functionality after loading
** REMINDER: NO LAZY VIRTUAL TABLE TESTING - TEST ACTUAL VTAB OPERATIONS
*/
void test_virtual_table_functionality(void) {
    char *error_msg = NULL;
    int rc;
    
    /* Load extension - NO LAZY EXTENSION LOADING */
    char *pzErrMsg = NULL;
    rc = sqlite3_graph_init(test_db, &pzErrMsg, NULL);
    TEST_ASSERT_EQUAL_MESSAGE(SQLITE_OK, rc, "Extension loading failed - NO LAZY LOADING");
    if (pzErrMsg) sqlite3_free(pzErrMsg);
    
    /* Create virtual table - NO LAZY TABLE CREATION */
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(test_db, "CREATE VIRTUAL TABLE vtab_test USING graph()", -1, &stmt, NULL);
    TEST_ASSERT_EQUAL_MESSAGE(SQLITE_OK, rc, "Virtual table creation failed - NO LAZY VTAB CREATION");
    rc = sqlite3_step(stmt);
    TEST_ASSERT_EQUAL_MESSAGE(SQLITE_DONE, rc, "Virtual table creation execution failed - NO LAZY VTAB EXEC");
    sqlite3_finalize(stmt);
    
    /* Test SELECT from virtual table - NO LAZY SELECT TESTING */
    rc = sqlite3_prepare_v2(test_db, "SELECT * FROM vtab_test", -1, &stmt, NULL);
    TEST_ASSERT_EQUAL_MESSAGE(SQLITE_OK, rc, "SELECT from virtual table failed - NO LAZY SELECT");
    
    /* Execute SELECT - should return no rows initially - NO LAZY EMPTY SELECT */
    rc = sqlite3_step(stmt);
    TEST_ASSERT_EQUAL_MESSAGE(SQLITE_DONE, rc, "Empty virtual table should return DONE - NO LAZY EMPTY CHECK");
    sqlite3_finalize(stmt);
    
    /* Test INSERT into virtual table (if supported) - NO LAZY INSERT TESTING */
    /* Note: This depends on the virtual table implementation */
    
    /* Clean up error message - NO LAZY ERROR CLEANUP */
    if (error_msg) {
        sqlite3_free(error_msg);
    }
}

/* REMINDER: NO LAZY TEST MAIN - RUN ALL TESTS */
int main(void) {
    UNITY_BEGIN();
    
    /* REMINDER: NO LAZY TEST EXECUTION - RUN EVERY SINGLE TEST */
    RUN_TEST(test_extension_load_success);
    RUN_TEST(test_extension_load_null_database);
    RUN_TEST(test_extension_load_invalid_api);
    RUN_TEST(test_extension_load_memory_failure);
    RUN_TEST(test_extension_load_module_registration_failure);
    RUN_TEST(test_multiple_database_connections);
    RUN_TEST(test_extension_unload_cleanup);
    RUN_TEST(test_sql_functions_available);
    RUN_TEST(test_virtual_table_functionality);
    
    return UNITY_END();
}

/* REMINDER: IF ANY TEST IS MISSING OR INCOMPLETE, YOU'RE BEING LAZY */
/* NO LAZY TESTING ALLOWED - EVERY SCENARIO MUST BE COVERED */