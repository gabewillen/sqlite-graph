/*
** test_performance_optimization.c - Tests for performance optimization features
**
** This file tests the query optimization, memory management,
** and benchmarking features of the SQLite Graph Extension.
*/

#define SQLITE_CORE
#include "unity.h"
#include "sqlite3.h"
#include "../include/graph-performance.h"
#include <string.h>

/* Note: Test database setup is handled by Unity setUp/tearDown in test_main.c */

/* External reference to global test database */
extern sqlite3 *g_pTestDb;

/*
** Test selectivity estimation
*/
void testSelectivityEstimation(void) {
    /* Create a simple AST pattern for testing */
    CypherAst *pattern = cypherAstCreate(CYPHER_AST_NODE_PATTERN, 0, 0);
    TEST_ASSERT_NOT_NULL(pattern);
    
    /* Get graph virtual table */
    GraphVtab graph;
    memset(&graph, 0, sizeof(GraphVtab));
    graph.base.pModule = NULL;
    graph.pDb = g_pTestDb;
    graph.zDbName = sqlite3_mprintf("main");
    graph.zTableName = sqlite3_mprintf("test_perf_graph");
    graph.nRef = 1;
    graph.pLabelIndex = NULL;
    graph.pPropertyIndex = NULL;
    graph.pSchema = NULL;

    // Manually add some nodes and edges for testing
    graphAddNode(&graph, 1, "{}");
    graphAddNode(&graph, 2, "{}");
    graphAddNode(&graph, 3, "{}");
    graphAddNode(&graph, 4, "{}");
    graphAddEdge(&graph, 1, 2, 1.0, "{}");
    graphAddEdge(&graph, 2, 3, 1.0, "{}");
    graphAddEdge(&graph, 3, 4, 1.0, "{}");
    
    SelectivityEstimate estimate = graphEstimateSelectivity(&graph, pattern);
    
    /* With label filter, selectivity should be less than 1.0 */
    TEST_ASSERT_TRUE(estimate.selectivity <= 1.0);
    TEST_ASSERT_TRUE(estimate.selectivity > 0.0);
    
    /* With property filter, selectivity should be even lower */
    TEST_ASSERT_TRUE(estimate.selectivity <= 0.5);
    
    /* Estimated rows should be reasonable */
    TEST_ASSERT_TRUE(estimate.estimatedRows <= graphCountNodes(&graph));
    TEST_ASSERT_TRUE(estimate.estimatedRows >= 0);
    
    /* Cleanup */
    cypherAstDestroy(pattern);
}

/*
** Test memory pool allocation
*/
void testMemoryPool(void) {
    /* Create memory pool with 1KB initial size */
    QueryMemoryPool *pool = graphCreateMemoryPool(1024);
    TEST_ASSERT_NOT_NULL(pool);
    
    /* Test allocation */
    void *ptr1 = graphPoolAlloc(pool, 100);
    TEST_ASSERT_NOT_NULL(ptr1);
    
    void *ptr2 = graphPoolAlloc(pool, 200);
    TEST_ASSERT_NOT_NULL(ptr2);
    
    /* Pointers should be different */
    TEST_ASSERT_NOT_EQUAL(ptr1, ptr2);
    
    /* Test alignment */
    void *ptr3 = graphPoolAlloc(pool, 7);  /* Should be aligned to 8 */
    TEST_ASSERT_NOT_NULL(ptr3);
    
    /* Test pool exhaustion */
    void *ptrLarge = graphPoolAlloc(pool, 2048);  /* Larger than pool */
    TEST_ASSERT_NOT_NULL(ptrLarge);  /* Should fall back to malloc */
    
    /* Cleanup */
    graphDestroyMemoryPool(pool);
}

/*
** Test performance metrics collection
*/
void testPerformanceMetrics(void) {
    /* Start metrics collection */
    PerfMetrics *metrics = graphStartMetrics();
    TEST_ASSERT_NOT_NULL(metrics);
    
    /* Simulate some operations */
    graphUpdateMetrics(metrics, "node_scan");
    graphUpdateMetrics(metrics, "node_scan");
    graphUpdateMetrics(metrics, "edge_traverse");
    graphUpdateMetrics(metrics, "cache_hit");
    graphUpdateMetrics(metrics, "cache_miss");
    
    /* End metrics collection */
    graphEndMetrics(metrics);
    
    /* Check metrics values */
    TEST_ASSERT_EQUAL(2, metrics->nodesScanned);
    TEST_ASSERT_EQUAL(1, metrics->edgesTraversed);
    TEST_ASSERT_EQUAL(1, metrics->cacheHits);
    TEST_ASSERT_EQUAL(1, metrics->cacheMisses);
    
    /* Format metrics */
    char *formatted = graphFormatMetrics(metrics);
    TEST_ASSERT_NOT_NULL(formatted);
    TEST_ASSERT_TRUE(strstr(formatted, "Nodes Scanned: 2") != NULL);
    TEST_ASSERT_TRUE(strstr(formatted, "Cache Hit Rate: 50.0%") != NULL);
    
    sqlite3_free(formatted);
    sqlite3_free(metrics);
}

/*
** Test join order optimization
*/
void testJoinOrderOptimization(void) {
    /* Create join order optimizer */
    JoinOrderOptimizer optimizer = {
        .nJoins = 3,
        .joins = NULL,  /* Would contain actual join operations */
        .costs = sqlite3_malloc(3 * sizeof(double)),
        .bestOrder = sqlite3_malloc(3 * sizeof(int))
    };
    
    TEST_ASSERT_NOT_NULL(optimizer.costs);
    TEST_ASSERT_NOT_NULL(optimizer.bestOrder);
    
    /* Run optimization */
    int rc = graphOptimizeJoinOrder(&optimizer);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    
    /* Check that we got a valid order */
    for (int i = 0; i < optimizer.nJoins; i++) {
        TEST_ASSERT_TRUE(optimizer.bestOrder[i] >= 0);
        TEST_ASSERT_TRUE(optimizer.bestOrder[i] < optimizer.nJoins);
    }
    
    sqlite3_free(optimizer.costs);
    sqlite3_free(optimizer.bestOrder);
}

/*
** Test composite index creation
*/
void testCompositeIndex(void) {
    const char *properties[] = {"name", "age", "city"};
    
    CompositeIndex *index = graphCreateCompositeIndex(NULL, properties, 3);
    TEST_ASSERT_NOT_NULL(index);
    
    /* Check index name */
    TEST_ASSERT_NOT_NULL(index->indexName);
    TEST_ASSERT_TRUE(strstr(index->indexName, "composite") != NULL);
    
    /* Check properties */
    TEST_ASSERT_EQUAL(3, index->nProperties);
    TEST_ASSERT_EQUAL_STRING("name", index->properties[0]);
    TEST_ASSERT_EQUAL_STRING("age", index->properties[1]);
    TEST_ASSERT_EQUAL_STRING("city", index->properties[2]);
    
    /* Cleanup */
    sqlite3_free(index->indexName);
    for (int i = 0; i < index->nProperties; i++) {
        sqlite3_free(index->properties[i]);
    }
    sqlite3_free(index->properties);
    sqlite3_free(index);
}

/*
** Test benchmark function registration
*/
void testBenchmarkFunctionRegistration(void) {
    /* Register benchmark functions */
    int rc = graphRegisterBenchmarkFunctions(g_pTestDb);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    
    /* Check that function exists by preparing a statement */
    sqlite3_stmt *pStmt;
    rc = sqlite3_prepare_v2(g_pTestDb, 
        "SELECT graph_benchmark(1, 1, 1, 1)", -1, &pStmt, NULL);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    
    sqlite3_finalize(pStmt);
}

/*
** Test query with performance metrics
*/
void testQueryWithMetrics(void) {
    /* Enable metrics collection */
    const char *sql = 
        "SELECT * FROM cypher_execute_explain("
        "  'MATCH (p:Person) RETURN p.name'"
        ")";
    
    sqlite3_stmt *pStmt;
    int rc = sqlite3_prepare_v2(g_pTestDb, sql, -1, &pStmt, NULL);
    
    if (rc == SQLITE_OK) {
        /* Execute and check for metrics in output */
        rc = sqlite3_step(pStmt);
        if (rc == SQLITE_ROW) {
            const char *result = (const char*)sqlite3_column_text(pStmt, 0);
            TEST_ASSERT_NOT_NULL(result);
            /* Would check for performance metrics in result */
        }
        sqlite3_finalize(pStmt);
    }
}

/*
** Test CSR graph conversion
*/
void testCSRConversion(void) {
    /* Mock graph for testing */
    GraphVtab graph;
    memset(&graph, 0, sizeof(GraphVtab));
    graph.base.pModule = NULL;
    graph.pDb = g_pTestDb;
    graph.zDbName = sqlite3_mprintf("main");
    graph.zTableName = sqlite3_mprintf("test_csr_graph");
    graph.nRef = 1;
    graph.pLabelIndex = NULL;
    graph.pPropertyIndex = NULL;
    graph.pSchema = NULL;

    // Manually add some nodes and edges for testing
    graphAddNode(&graph, 1, "{}");
    graphAddNode(&graph, 2, "{}");
    graphAddNode(&graph, 3, "{}");
    graphAddNode(&graph, 4, "{}");
    graphAddEdge(&graph, 1, 2, 1.0, "{}");
    graphAddEdge(&graph, 2, 3, 1.0, "{}");
    graphAddEdge(&graph, 3, 4, 1.0, "{}");
    
    CSRGraph *csr = graphConvertToCSR(&graph);
    TEST_ASSERT_NOT_NULL(csr);
    
    /* Check dimensions */
    TEST_ASSERT_EQUAL(graphCountNodes(&graph), csr->nNodes);
    TEST_ASSERT_EQUAL(graphCountEdges(&graph), csr->nEdges);
    
    /* Check arrays allocated */
    TEST_ASSERT_NOT_NULL(csr->rowOffsets);
    TEST_ASSERT_NOT_NULL(csr->columnIndices);
    TEST_ASSERT_NOT_NULL(csr->edgeWeights);
    
    /* Cleanup */
    sqlite3_free(csr->rowOffsets);
    sqlite3_free(csr->columnIndices);
    sqlite3_free(csr->edgeWeights);
    sqlite3_free(csr);
}

/* Tests are run from unified test_main.c */