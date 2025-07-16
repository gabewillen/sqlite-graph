/*
** SQLite Graph Database Extension - Simple CREATE Node Test
**
** This test verifies that CREATE node operations work correctly
** without requiring the Unity test framework.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "sqlite3.h"
#include "graph.h"
#include "graph-vtab.h"
#include "cypher.h"
#include "cypher-executor.h"
#include "cypher-write.h"

/* Declare graphExtensionLoad since we don't have graph-extension.h */
int graphExtensionLoad(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi);

/* Test counters */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

/* Simple test macros */
#define TEST_ASSERT(condition) do { \
    tests_run++; \
    if (condition) { \
        tests_passed++; \
        printf("."); \
    } else { \
        tests_failed++; \
        printf("F"); \
        printf("\nAssertion failed at %s:%d: %s\n", __FILE__, __LINE__, #condition); \
    } \
} while(0)

/* Test basic node creation */
void test_create_node_basic(void) {
    sqlite3 *db = NULL;
    GraphVtab *graph = NULL;
    ExecutionContext *execCtx = NULL;
    CypherWriteContext *writeCtx = NULL;
    CreateNodeOp *op = NULL;
    int rc;
    
    printf("\nTesting basic node creation...\n");
    
    /* Open in-memory database */
    rc = sqlite3_open(":memory:", &db);
    TEST_ASSERT(rc == SQLITE_OK);
    TEST_ASSERT(db != NULL);
    
    /* Create minimal GraphVtab for testing */
    graph = (GraphVtab*)sqlite3_malloc(sizeof(GraphVtab));
    TEST_ASSERT(graph != NULL);
    memset(graph, 0, sizeof(GraphVtab));
    graph->pDb = db;
    
    /* Create execution context */
    execCtx = executionContextCreate(db, graph);
    TEST_ASSERT(execCtx != NULL);
    
    /* Create write context */
    writeCtx = cypherWriteContextCreate(db, graph, execCtx);
    TEST_ASSERT(writeCtx != NULL);
    
    /* Create node operation */
    op = cypherCreateNodeOpCreate();
    TEST_ASSERT(op != NULL);
    
    /* Set variable name */
    op->zVariable = sqlite3_mprintf("n");
    TEST_ASSERT(op->zVariable != NULL);
    
    /* Execute CREATE node */
    rc = cypherCreateNode(writeCtx, op);
    TEST_ASSERT(rc == SQLITE_OK);
    
    /* Verify node ID was assigned */
    TEST_ASSERT(op->iCreatedNodeId > 0);
    printf("Created node with ID: %lld\n", op->iCreatedNodeId);
    
    /* Verify variable was bound */
    CypherValue *value = executionContextGet(execCtx, "n");
    TEST_ASSERT(value != NULL);
    TEST_ASSERT(value->type == CYPHER_VALUE_NODE);
    TEST_ASSERT(value->u.iNodeId == op->iCreatedNodeId);
    
    /* Cleanup */
    cypherCreateNodeOpDestroy(op);
    cypherWriteContextDestroy(writeCtx);
    executionContextDestroy(execCtx);
    sqlite3_free(graph);
    sqlite3_close(db);
}

/* Test node creation with labels */
void test_create_node_with_labels(void) {
    sqlite3 *db = NULL;
    GraphVtab *graph = NULL;
    ExecutionContext *execCtx = NULL;
    CypherWriteContext *writeCtx = NULL;
    CreateNodeOp *op = NULL;
    int rc;
    
    printf("\nTesting node creation with labels...\n");
    
    /* Setup */
    rc = sqlite3_open(":memory:", &db);
    TEST_ASSERT(rc == SQLITE_OK);
    
    graph = (GraphVtab*)sqlite3_malloc(sizeof(GraphVtab));
    TEST_ASSERT(graph != NULL);
    memset(graph, 0, sizeof(GraphVtab));
    graph->pDb = db;
    
    execCtx = executionContextCreate(db, graph);
    TEST_ASSERT(execCtx != NULL);
    
    writeCtx = cypherWriteContextCreate(db, graph, execCtx);
    TEST_ASSERT(writeCtx != NULL);
    
    /* Create node with labels */
    op = cypherCreateNodeOpCreate();
    TEST_ASSERT(op != NULL);
    
    op->zVariable = sqlite3_mprintf("person");
    op->nLabels = 2;
    op->azLabels = sqlite3_malloc(2 * sizeof(char*));
    TEST_ASSERT(op->azLabels != NULL);
    op->azLabels[0] = sqlite3_mprintf("Person");
    op->azLabels[1] = sqlite3_mprintf("Actor");
    
    /* Execute */
    rc = cypherCreateNode(writeCtx, op);
    TEST_ASSERT(rc == SQLITE_OK);
    TEST_ASSERT(op->iCreatedNodeId > 0);
    
    printf("Created node with labels [Person, Actor], ID: %lld\n", op->iCreatedNodeId);
    
    /* Cleanup */
    cypherCreateNodeOpDestroy(op);
    cypherWriteContextDestroy(writeCtx);
    executionContextDestroy(execCtx);
    sqlite3_free(graph);
    sqlite3_close(db);
}

/* Test node creation with properties */
void test_create_node_with_properties(void) {
    sqlite3 *db = NULL;
    GraphVtab *graph = NULL;
    ExecutionContext *execCtx = NULL;
    CypherWriteContext *writeCtx = NULL;
    CreateNodeOp *op = NULL;
    int rc;
    
    printf("\nTesting node creation with properties...\n");
    
    /* Setup */
    rc = sqlite3_open(":memory:", &db);
    TEST_ASSERT(rc == SQLITE_OK);
    
    graph = (GraphVtab*)sqlite3_malloc(sizeof(GraphVtab));
    TEST_ASSERT(graph != NULL);
    memset(graph, 0, sizeof(GraphVtab));
    graph->pDb = db;
    
    execCtx = executionContextCreate(db, graph);
    TEST_ASSERT(execCtx != NULL);
    
    writeCtx = cypherWriteContextCreate(db, graph, execCtx);
    TEST_ASSERT(writeCtx != NULL);
    
    /* Create node with properties */
    op = cypherCreateNodeOpCreate();
    TEST_ASSERT(op != NULL);
    
    op->zVariable = sqlite3_mprintf("user");
    op->nLabels = 1;
    op->azLabels = sqlite3_malloc(sizeof(char*));
    op->azLabels[0] = sqlite3_mprintf("User");
    
    /* Set properties */
    op->nProperties = 3;
    op->azPropNames = sqlite3_malloc(3 * sizeof(char*));
    op->apPropValues = sqlite3_malloc(3 * sizeof(CypherValue*));
    TEST_ASSERT(op->azPropNames != NULL);
    TEST_ASSERT(op->apPropValues != NULL);
    
    /* Property 1: name (string) */
    op->azPropNames[0] = sqlite3_mprintf("name");
    op->apPropValues[0] = cypherValueCreate(CYPHER_VALUE_STRING);
    cypherValueSetString(op->apPropValues[0], "Alice");
    
    /* Property 2: age (integer) */
    op->azPropNames[1] = sqlite3_mprintf("age");
    op->apPropValues[1] = cypherValueCreate(CYPHER_VALUE_INTEGER);
    cypherValueSetInteger(op->apPropValues[1], 30);
    
    /* Property 3: score (float) */
    op->azPropNames[2] = sqlite3_mprintf("score");
    op->apPropValues[2] = cypherValueCreate(CYPHER_VALUE_FLOAT);
    cypherValueSetFloat(op->apPropValues[2], 95.5);
    
    /* Execute */
    rc = cypherCreateNode(writeCtx, op);
    TEST_ASSERT(rc == SQLITE_OK);
    TEST_ASSERT(op->iCreatedNodeId > 0);
    
    printf("Created User node with properties {name: \"Alice\", age: 30, score: 95.5}, ID: %lld\n", 
           op->iCreatedNodeId);
    
    /* Cleanup */
    cypherCreateNodeOpDestroy(op);
    cypherWriteContextDestroy(writeCtx);
    executionContextDestroy(execCtx);
    sqlite3_free(graph);
    sqlite3_close(db);
}

/* Test CREATE node iterator creation */
void test_create_node_iterator(void) {
    sqlite3 *db = NULL;
    GraphVtab *graph = NULL;
    ExecutionContext *execCtx = NULL;
    CypherWriteContext *writeCtx = NULL;
    CreateNodeOp *op = NULL;
    CypherWriteIterator *iter = NULL;
    int rc;
    
    printf("\nTesting CREATE node iterator...\n");
    
    /* Setup */
    rc = sqlite3_open(":memory:", &db);
    TEST_ASSERT(rc == SQLITE_OK);
    
    graph = (GraphVtab*)sqlite3_malloc(sizeof(GraphVtab));
    TEST_ASSERT(graph != NULL);
    memset(graph, 0, sizeof(GraphVtab));
    graph->pDb = db;
    
    execCtx = executionContextCreate(db, graph);
    TEST_ASSERT(execCtx != NULL);
    
    writeCtx = cypherWriteContextCreate(db, graph, execCtx);
    TEST_ASSERT(writeCtx != NULL);
    
    /* Create node operation */
    op = cypherCreateNodeOpCreate();
    TEST_ASSERT(op != NULL);
    op->zVariable = sqlite3_mprintf("n");
    
    /* Create iterator */
    iter = cypherCreateNodeIteratorCreate(writeCtx, op);
    TEST_ASSERT(iter != NULL);
    TEST_ASSERT(iter->pWriteCtx == writeCtx);
    TEST_ASSERT(iter->pOperationData == op);
    
    /* Write iterators don't need typical iterator functions */
    TEST_ASSERT(iter->base.xOpen == NULL);
    TEST_ASSERT(iter->base.xNext == NULL);
    TEST_ASSERT(iter->base.xClose == NULL);
    TEST_ASSERT(iter->base.xDestroy == NULL);
    
    printf("CREATE node iterator created successfully\n");
    
    /* Cleanup */
    sqlite3_free(iter);
    cypherCreateNodeOpDestroy(op);
    cypherWriteContextDestroy(writeCtx);
    executionContextDestroy(execCtx);
    sqlite3_free(graph);
    sqlite3_close(db);
}

/* Main test runner */
int main(void) {
    printf("SQLite Graph Extension - CREATE Node Tests\n");
    printf("==========================================\n");
    
    /* Run tests */
    test_create_node_basic();
    test_create_node_with_labels();
    test_create_node_with_properties();
    test_create_node_iterator();
    
    /* Print summary */
    printf("\n\nTest Summary:\n");
    printf("Tests run:    %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    
    if (tests_failed == 0) {
        printf("\nAll tests passed!\n");
        return 0;
    } else {
        printf("\n%d tests failed!\n", tests_failed);
        return 1;
    }
}