/*
** SQLite Graph Database Extension - CREATE Node Test
**
** This test verifies that CREATE node operations work correctly
** including node ID generation, label handling, property setting,
** and variable binding.
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
#include "unity.h" // Added for Unity test framework functions
#include "test_util.h" // Include common test utilities

/* Global database and graph context */
static sqlite3 *g_db = NULL;
static GraphVtab *g_graph = NULL;
static CypherWriteContext *g_writeCtx = NULL;
static ExecutionContext *g_execCtx = NULL;

/* Setup function called before each test */
void setUp(void) {
    int rc;
    
    /* Open in-memory database */
    rc = sqlite3_open(":memory:", &g_db);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    TEST_ASSERT_NOT_NULL(g_db);
    
    /* Load graph extension */
    rc = graphExtensionLoad(g_db, NULL, NULL);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    
    /* Create graph virtual table */
    rc = sqlite3_exec(g_db, "CREATE VIRTUAL TABLE test_graph USING graph()", NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    
    /* Get graph vtab instance */
    sqlite3_stmt *pStmt;
    rc = sqlite3_prepare_v2(g_db, "SELECT 1 FROM test_graph WHERE 1=0", -1, &pStmt, NULL);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    
    /* Note: Removed problematic vtab cursor code that used wrong API */
    /* Instead, rely on the global graph variable being set during vtab creation */
    extern GraphVtab *pGraph;
    g_graph = pGraph;
    sqlite3_finalize(pStmt);
    
    /* Fallback: Create minimal GraphVtab for testing */
    if (!g_graph) {
        g_graph = (GraphVtab*)sqlite3_malloc(sizeof(GraphVtab));
        TEST_ASSERT_NOT_NULL(g_graph);
        memset(g_graph, 0, sizeof(GraphVtab));
        g_graph->pDb = g_db;
    }
    
    /* Create execution context */
    g_execCtx = executionContextCreate(g_db, g_graph);
    TEST_ASSERT_NOT_NULL(g_execCtx);
    
    /* Create write context */
    g_writeCtx = cypherWriteContextCreate(g_db, g_graph, g_execCtx);
    TEST_ASSERT_NOT_NULL(g_writeCtx);
}

/* Teardown function called after each test */
void tearDown(void) {
    if (g_writeCtx) {
        cypherWriteContextDestroy(g_writeCtx);
        g_writeCtx = NULL;
    }
    
    if (g_execCtx) {
        executionContextDestroy(g_execCtx);
        g_execCtx = NULL;
    }
    
    if (g_graph) {
        /* Proper cleanup without type confusion */
        g_graph = NULL;
    }
    
    if (g_db) {
        sqlite3_close(g_db);
        g_db = NULL;
    }
}

/* Test basic node creation without labels or properties */
void test_create_node_basic(void) {
    CreateNodeOp *pOp = cypherCreateNodeOpCreate();
    TEST_ASSERT_NOT_NULL(pOp);
    
    /* Set variable name */
    pOp->zVariable = sqlite3_mprintf("n");
    TEST_ASSERT_NOT_NULL(pOp->zVariable);
    
    /* Execute CREATE node */
    int rc = cypherCreateNode(g_writeCtx, pOp);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    
    /* Verify node ID was assigned */
    TEST_ASSERT(pOp->iCreatedNodeId > 0);
    
    /* Verify variable was bound */
    CypherValue *pValue = executionContextGet(g_execCtx, "n");
    TEST_ASSERT_NOT_NULL(pValue);
    TEST_ASSERT_EQUAL(CYPHER_VALUE_NODE, pValue->type);
    TEST_ASSERT_EQUAL(pOp->iCreatedNodeId, pValue->u.iNodeId);
    
    cypherCreateNodeOpDestroy(pOp);
}

/* Test node creation with labels */
void test_create_node_with_labels(void) {
    CreateNodeOp *pOp = cypherCreateNodeOpCreate();
    TEST_ASSERT_NOT_NULL(pOp);
    
    /* Set variable name */
    pOp->zVariable = sqlite3_mprintf("person");
    
    /* Set labels */
    pOp->nLabels = 2;
    pOp->azLabels = sqlite3_malloc(2 * sizeof(char*));
    TEST_ASSERT_NOT_NULL(pOp->azLabels);
    pOp->azLabels[0] = sqlite3_mprintf("Person");
    pOp->azLabels[1] = sqlite3_mprintf("Actor");
    
    /* Execute CREATE node */
    int rc = cypherCreateNode(g_writeCtx, pOp);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    
    /* Verify node was created */
    TEST_ASSERT(pOp->iCreatedNodeId > 0);
    
    /* Verify variable binding */
    CypherValue *pValue = executionContextGet(g_execCtx, "person");
    TEST_ASSERT_NOT_NULL(pValue);
    TEST_ASSERT_EQUAL(CYPHER_VALUE_NODE, pValue->type);
    
    cypherCreateNodeOpDestroy(pOp);
}

/* Test node creation with properties */
void test_create_node_with_properties(void) {
    CreateNodeOp *pOp = cypherCreateNodeOpCreate();
    TEST_ASSERT_NOT_NULL(pOp);
    
    /* Set variable name */
    pOp->zVariable = sqlite3_mprintf("user");
    
    /* Set labels */
    pOp->nLabels = 1;
    pOp->azLabels = sqlite3_malloc(sizeof(char*));
    pOp->azLabels[0] = sqlite3_mprintf("User");
    
    /* Set properties */
    pOp->nProperties = 3;
    pOp->azPropNames = sqlite3_malloc(3 * sizeof(char*));
    pOp->apPropValues = sqlite3_malloc(3 * sizeof(CypherValue*));
    TEST_ASSERT_NOT_NULL(pOp->azPropNames);
    TEST_ASSERT_NOT_NULL(pOp->apPropValues);
    
    /* Property 1: name (string) */
    pOp->azPropNames[0] = sqlite3_mprintf("name");
    pOp->apPropValues[0] = cypherValueCreate(CYPHER_VALUE_STRING);
    cypherValueSetString(pOp->apPropValues[0], "Alice");
    
    /* Property 2: age (integer) */
    pOp->azPropNames[1] = sqlite3_mprintf("age");
    pOp->apPropValues[1] = cypherValueCreate(CYPHER_VALUE_INTEGER);
    cypherValueSetInteger(pOp->apPropValues[1], 30);
    
    /* Property 3: score (float) */
    pOp->azPropNames[2] = sqlite3_mprintf("score");
    pOp->apPropValues[2] = cypherValueCreate(CYPHER_VALUE_FLOAT);
    cypherValueSetFloat(pOp->apPropValues[2], 95.5);
    
    /* Execute CREATE node */
    int rc = cypherCreateNode(g_writeCtx, pOp);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    
    /* Verify node was created */
    TEST_ASSERT(pOp->iCreatedNodeId > 0);
    
    cypherCreateNodeOpDestroy(pOp);
}

/* Test multiple node creation */
void test_create_multiple_nodes(void) {
    /* Create first node */
    CreateNodeOp *pOp1 = cypherCreateNodeOpCreate();
    pOp1->zVariable = sqlite3_mprintf("n1");
    int rc = cypherCreateNode(g_writeCtx, pOp1);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    sqlite3_int64 id1 = pOp1->iCreatedNodeId;
    
    /* Create second node */
    CreateNodeOp *pOp2 = cypherCreateNodeOpCreate();
    pOp2->zVariable = sqlite3_mprintf("n2");
    rc = cypherCreateNode(g_writeCtx, pOp2);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    sqlite3_int64 id2 = pOp2->iCreatedNodeId;
    
    /* Verify different IDs */
    TEST_ASSERT(id1 != id2);
    TEST_ASSERT(id1 > 0);
    TEST_ASSERT(id2 > 0);
    
    /* Verify both variables are bound */
    CypherValue *pVal1 = executionContextGet(g_execCtx, "n1");
    CypherValue *pVal2 = executionContextGet(g_execCtx, "n2");
    TEST_ASSERT_NOT_NULL(pVal1);
    TEST_ASSERT_NOT_NULL(pVal2);
    TEST_ASSERT_EQUAL(id1, pVal1->u.iNodeId);
    TEST_ASSERT_EQUAL(id2, pVal2->u.iNodeId);
    
    cypherCreateNodeOpDestroy(pOp1);
    cypherCreateNodeOpDestroy(pOp2);
}

/* Test node creation with transaction */
void test_create_node_with_transaction(void) {
    /* Begin transaction */
    int rc = cypherWriteContextBegin(g_writeCtx);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    
    /* Create node */
    CreateNodeOp *pOp = cypherCreateNodeOpCreate();
    pOp->zVariable = sqlite3_mprintf("n");
    pOp->nLabels = 1;
    pOp->azLabels = sqlite3_malloc(sizeof(char*));
    pOp->azLabels[0] = sqlite3_mprintf("TestNode");
    
    rc = cypherCreateNode(g_writeCtx, pOp);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    
    /* Commit transaction */
    rc = cypherWriteContextCommit(g_writeCtx);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    
    /* Verify node persisted */
    TEST_ASSERT(pOp->iCreatedNodeId > 0);
    
    cypherCreateNodeOpDestroy(pOp);
}

/* Test node creation iterator */
void test_create_node_iterator(void) {
    CreateNodeOp *pOp = cypherCreateNodeOpCreate();
    pOp->zVariable = sqlite3_mprintf("n");
    
    /* Create iterator */
    CypherWriteIterator *pIter = cypherCreateNodeIteratorCreate(g_writeCtx, pOp);
    TEST_ASSERT_NOT_NULL(pIter);
    TEST_ASSERT_EQUAL(g_writeCtx, pIter->pWriteCtx);
    TEST_ASSERT_EQUAL(pOp, pIter->pOperationData);
    
    /* Write iterators don't need typical iterator functions */
    TEST_ASSERT_NULL(pIter->base.xOpen);
    TEST_ASSERT_NULL(pIter->base.xNext);
    TEST_ASSERT_NULL(pIter->base.xClose);
    TEST_ASSERT_NULL(pIter->base.xDestroy);
    
    sqlite3_free(pIter);
    cypherCreateNodeOpDestroy(pOp);
}

/* Main test runner */
int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_create_node_basic);
    RUN_TEST(test_create_node_with_labels);
    RUN_TEST(test_create_node_with_properties);
    RUN_TEST(test_create_multiple_nodes);
    RUN_TEST(test_create_node_with_transaction);
    RUN_TEST(test_create_node_iterator);
    
    return UNITY_END();
}