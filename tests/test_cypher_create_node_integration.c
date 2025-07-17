/*
** SQLite Graph Database Extension - CREATE Node Integration Test
**
** This test verifies the ACTUAL cypherCreateNode implementation
** from cypher-write.c, not a stub or mock.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "sqlite3.h"
#include "graph.h"
#include "graph-vtab.h"
#include "cypher.h"
#include "cypher-executor.h"
#include "cypher-write.h"
#include "test_util.h" // Include common test utilities

/* Mock storage for verification */
static struct {
    sqlite3_int64 iNodeId;
    char *zLabels;
    char *zProperties;
    int bStored;
} g_lastStoredNode = {0, NULL, NULL, 0};

/* Override cypherStorageAddNode for testing purposes */
int cypherStorageAddNode(GraphVtab *pGraph, sqlite3_int64 iNodeId, 
                        const char **azLabels, int nLabels, 
                        const char *zProperties) {
    /* Mock implementation that records what would be stored */
    g_lastStoredNode.iNodeId = iNodeId;
    if (g_lastStoredNode.zLabels) sqlite3_free(g_lastStoredNode.zLabels);
    if (g_lastStoredNode.zProperties) sqlite3_free(g_lastStoredNode.zProperties);
    
    /* Build labels JSON for verification */
    if (nLabels > 0) {
        char *zLabels = sqlite3_malloc(256);
        strcpy(zLabels, "[");
        for (int i = 0; i < nLabels; i++) {
            if (i > 0) strcat(zLabels, ",");
            strcat(zLabels, "\"");
            strcat(zLabels, azLabels[i]);
            strcat(zLabels, "\"");
        }
        strcat(zLabels, "]");
        g_lastStoredNode.zLabels = zLabels;
    } else {
        g_lastStoredNode.zLabels = sqlite3_mprintf("[]");
    }
    
    g_lastStoredNode.zProperties = sqlite3_mprintf("%s", zProperties ? zProperties : "{}");
    g_lastStoredNode.bStored = 1;
    return SQLITE_OK;
}

/* Test functions */
static int g_tests_run = 0;
static int g_tests_passed = 0;

#define TEST_ASSERT(cond, msg) do { \
    g_tests_run++; \
    if (cond) { \
        g_tests_passed++; \
        printf("  PASS: %s\n", msg); \
    } else { \
        printf("  FAIL: %s (line %d)\n", msg, __LINE__); \
    } \
} while(0)

void test_create_node_basic(void) {
    printf("\nTest: Basic CREATE node\n");
    
    GraphVtab graph = {0};
    ExecutionContext *execCtx = executionContextCreate(NULL, &graph);
    CypherWriteContext *writeCtx = cypherWriteContextCreate(NULL, &graph, execCtx);
    
    CreateNodeOp *op = cypherCreateNodeOpCreate();
    op->zVariable = sqlite3_mprintf("n");
    
    /* Clear storage mock */
    g_lastStoredNode.bStored = 0;
    
    int rc = cypherCreateNode(writeCtx, op);
    
    TEST_ASSERT(rc == SQLITE_OK, "cypherCreateNode returns SQLITE_OK");
    TEST_ASSERT(op->iCreatedNodeId == 1, "Node ID assigned correctly");
    TEST_ASSERT(g_lastStoredNode.bStored == 1, "Storage function called");
    TEST_ASSERT(g_lastStoredNode.iNodeId == 1, "Correct node ID stored");
    TEST_ASSERT(strcmp(g_lastStoredNode.zLabels, "[]") == 0, "Empty labels stored");
    TEST_ASSERT(strcmp(g_lastStoredNode.zProperties, "{}") == 0, "Empty properties stored");
    
    /* Check variable binding */
    CypherValue *boundValue = executionContextGet(execCtx, "n");
    TEST_ASSERT(boundValue != NULL, "Variable bound in execution context");
    TEST_ASSERT(boundValue->type == CYPHER_VALUE_NODE, "Bound value is NODE type");
    TEST_ASSERT(boundValue->u.iNodeId == 1, "Bound node ID is correct");
    
    /* Check operation recorded */
    TEST_ASSERT(writeCtx->nOperations == 1, "Operation recorded");
    TEST_ASSERT(writeCtx->pOperations != NULL, "Operation list not empty");
    TEST_ASSERT(writeCtx->pOperations->type == CYPHER_WRITE_CREATE_NODE, "Correct operation type");
    
    cypherCreateNodeOpDestroy(op);
    cypherWriteContextDestroy(writeCtx);
    executionContextDestroy(execCtx);
}

void test_create_node_with_labels(void) {
    printf("\nTest: CREATE node with labels\n");
    
    GraphVtab graph = {0};
    ExecutionContext *execCtx = executionContextCreate(NULL, &graph);
    CypherWriteContext *writeCtx = cypherWriteContextCreate(NULL, &graph, execCtx);
    
    CreateNodeOp *op = cypherCreateNodeOpCreate();
    op->zVariable = sqlite3_mprintf("person");
    op->nLabels = 2;
    op->azLabels = sqlite3_malloc(2 * sizeof(char*));
    op->azLabels[0] = sqlite3_mprintf("Person");
    op->azLabels[1] = sqlite3_mprintf("Actor");
    
    g_lastStoredNode.bStored = 0;
    
    int rc = cypherCreateNode(writeCtx, op);
    
    TEST_ASSERT(rc == SQLITE_OK, "cypherCreateNode returns SQLITE_OK");
    TEST_ASSERT(g_lastStoredNode.bStored == 1, "Storage function called");
    TEST_ASSERT(strcmp(g_lastStoredNode.zLabels, "[\"Person\",\"Actor\"]") == 0, "Labels stored correctly");
    
    cypherCreateNodeOpDestroy(op);
    cypherWriteContextDestroy(writeCtx);
    executionContextDestroy(execCtx);
}

void test_create_node_with_properties(void) {
    printf("\nTest: CREATE node with properties\n");
    
    GraphVtab graph = {0};
    ExecutionContext *execCtx = executionContextCreate(NULL, &graph);
    CypherWriteContext *writeCtx = cypherWriteContextCreate(NULL, &graph, execCtx);
    
    CreateNodeOp *op = cypherCreateNodeOpCreate();
    op->zVariable = sqlite3_mprintf("user");
    op->nLabels = 1;
    op->azLabels = sqlite3_malloc(sizeof(char*));
    op->azLabels[0] = sqlite3_mprintf("User");
    
    op->nProperties = 3;
    op->azPropNames = sqlite3_malloc(3 * sizeof(char*));
    op->apPropValues = sqlite3_malloc(3 * sizeof(CypherValue*));
    
    op->azPropNames[0] = sqlite3_mprintf("name");
    op->apPropValues[0] = cypherValueCreate(CYPHER_VALUE_STRING);
    cypherValueSetString(op->apPropValues[0], "Alice");
    
    op->azPropNames[1] = sqlite3_mprintf("age");
    op->apPropValues[1] = cypherValueCreate(CYPHER_VALUE_INTEGER);
    cypherValueSetInteger(op->apPropValues[1], 30);
    
    op->azPropNames[2] = sqlite3_mprintf("score");
    op->apPropValues[2] = cypherValueCreate(CYPHER_VALUE_FLOAT);
    cypherValueSetFloat(op->apPropValues[2], 95.5);
    
    g_lastStoredNode.bStored = 0;
    
    int rc = cypherCreateNode(writeCtx, op);
    
    TEST_ASSERT(rc == SQLITE_OK, "cypherCreateNode returns SQLITE_OK");
    TEST_ASSERT(g_lastStoredNode.bStored == 1, "Storage function called");
    
    /* Verify properties JSON */
    TEST_ASSERT(strstr(g_lastStoredNode.zProperties, "\"name\":\"Alice\"") != NULL, "String property stored");
    TEST_ASSERT(strstr(g_lastStoredNode.zProperties, "\"age\":30") != NULL, "Integer property stored");
    TEST_ASSERT(strstr(g_lastStoredNode.zProperties, "\"score\":95.5") != NULL, "Float property stored");
    
    cypherCreateNodeOpDestroy(op);
    cypherWriteContextDestroy(writeCtx);
    executionContextDestroy(execCtx);
}

void test_create_node_error_cases(void) {
    printf("\nTest: CREATE node error cases\n");
    
    GraphVtab graph = {0};
    ExecutionContext *execCtx = executionContextCreate(NULL, &graph);
    CypherWriteContext *writeCtx = cypherWriteContextCreate(NULL, &graph, execCtx);
    CreateNodeOp *op = cypherCreateNodeOpCreate();
    
    /* Test NULL context */
    int rc = cypherCreateNode(NULL, op);
    TEST_ASSERT(rc == SQLITE_MISUSE, "NULL context returns SQLITE_MISUSE");
    
    /* Test NULL operation */
    rc = cypherCreateNode(writeCtx, NULL);
    TEST_ASSERT(rc == SQLITE_MISUSE, "NULL operation returns SQLITE_MISUSE");
    
    cypherCreateNodeOpDestroy(op);
    cypherWriteContextDestroy(writeCtx);
    executionContextDestroy(execCtx);
}

void test_create_multiple_nodes(void) {
    printf("\nTest: CREATE multiple nodes\n");
    
    GraphVtab graph = {0};
    ExecutionContext *execCtx = executionContextCreate(NULL, &graph);
    CypherWriteContext *writeCtx = cypherWriteContextCreate(NULL, &graph, execCtx);
    
    /* Create first node */
    CreateNodeOp *op1 = cypherCreateNodeOpCreate();
    op1->zVariable = sqlite3_mprintf("n1");
    int rc1 = cypherCreateNode(writeCtx, op1);
    sqlite3_int64 id1 = op1->iCreatedNodeId;
    
    /* Create second node */
    CreateNodeOp *op2 = cypherCreateNodeOpCreate();
    op2->zVariable = sqlite3_mprintf("n2");
    int rc2 = cypherCreateNode(writeCtx, op2);
    sqlite3_int64 id2 = op2->iCreatedNodeId;
    
    TEST_ASSERT(rc1 == SQLITE_OK && rc2 == SQLITE_OK, "Both nodes created successfully");
    TEST_ASSERT(id1 == 1 && id2 == 2, "Sequential node IDs assigned");
    TEST_ASSERT(writeCtx->nOperations == 2, "Two operations recorded");
    
    cypherCreateNodeOpDestroy(op1);
    cypherCreateNodeOpDestroy(op2);
    cypherWriteContextDestroy(writeCtx);
    executionContextDestroy(execCtx);
}

int main(void) {
    printf("=== SQLite Graph CREATE Node Integration Test ===\n");
    printf("Testing ACTUAL cypherCreateNode implementation\n");
    
    test_create_node_basic();
    test_create_node_with_labels();
    test_create_node_with_properties();
    test_create_node_error_cases();
    test_create_multiple_nodes();
    
    printf("\n=== Test Summary ===\n");
    printf("Total tests: %d\n", g_tests_run);
    printf("Passed: %d\n", g_tests_passed);
    printf("Failed: %d\n", g_tests_run - g_tests_passed);
    
    if (g_tests_passed == g_tests_run) {
        printf("\nSUCCESS: All tests passed!\n");
        return 0;
    } else {
        printf("\nFAILURE: %d tests failed!\n", g_tests_run - g_tests_passed);
        return 1;
    }
}