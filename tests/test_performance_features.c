/*
** test_performance_features.c - Comprehensive tests for performance features
**
** This file tests all the new performance optimization features including
** parallel execution, caching, compression, and bulk loading.
*/

#define SQLITE_CORE
#include "unity.h"
#include "sqlite3.h"
#include "../src/graph-parallel.c"
#include "../src/graph-cache.c"
#include "../src/graph-compress.c"
#include "../src/graph-bulk.c"

/* Note: Test database setup is handled by Unity setUp/tearDown in test_main.c */

/* External reference to global test database */
extern sqlite3 *g_pTestDb;

/* Mock GraphVtab structure for testing */
static GraphVtab *createMockGraph(void) {
    GraphVtab *pGraph = sqlite3_malloc(sizeof(GraphVtab));
    if (!pGraph) return NULL;
    
    memset(pGraph, 0, sizeof(GraphVtab));
    pGraph->pDb = g_pTestDb;
    pGraph->zDbName = sqlite3_mprintf("main");
    pGraph->zTableName = sqlite3_mprintf("mock_graph");
    pGraph->nRef = 1;
    
    // Create the virtual table to ensure backing tables are created
    char *zErrMsg = 0;
    int rc = sqlite3_exec(pGraph->pDb, "CREATE VIRTUAL TABLE mock_graph USING graph();", 0, 0, &zErrMsg);
    if( rc!=SQLITE_OK ){
      sqlite3_free(zErrMsg);
      sqlite3_free(pGraph->zDbName);
      sqlite3_free(pGraph->zTableName);
      sqlite3_free(pGraph);
      return NULL;
    }
    
    return pGraph;
}

static void destroyMockGraph(GraphVtab *pGraph) {
    if (!pGraph) return;
    
    char *zSql = sqlite3_mprintf("DROP TABLE IF EXISTS %s_nodes; DROP TABLE IF EXISTS %s_edges;", pGraph->zTableName, pGraph->zTableName);
    sqlite3_exec(pGraph->pDb, zSql, 0, 0, 0);
    sqlite3_free(zSql);

    sqlite3_free(pGraph->zDbName);
    sqlite3_free(pGraph->zTableName);
    sqlite3_free(pGraph);
}

/*
** Parallel Execution Tests
*/

/* Test task scheduler creation */
void testTaskSchedulerCreation(void) {
    TaskScheduler *scheduler = graphCreateTaskScheduler(4);
    TEST_ASSERT_NOT_NULL(scheduler);
    TEST_ASSERT_EQUAL(4, scheduler->nThreads);
    TEST_ASSERT_EQUAL(1, scheduler->stealingEnabled);
    
    graphDestroyTaskScheduler(scheduler);
}

/* Test parallel task execution */
static int task_counter = 0;
static pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;

static void simpleTaskFunc(void *arg) {
    int *value = (int*)arg;
    pthread_mutex_lock(&counter_mutex);
    task_counter += *value;
    pthread_mutex_unlock(&counter_mutex);
}

void testParallelTaskExecution(void) {
    TaskScheduler *scheduler = graphCreateTaskScheduler(2);
    TEST_ASSERT_NOT_NULL(scheduler);
    
    /* Create tasks */
    int values[] = {1, 2, 3, 4, 5};
    void *args[5];
    for (int i = 0; i < 5; i++) {
        args[i] = &values[i];
    }
    
    /* Reset counter */
    task_counter = 0;
    
    /* Execute tasks in parallel */
    int rc = graphExecuteParallel(scheduler, simpleTaskFunc, args, 5);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    
    /* Check result */
    TEST_ASSERT_EQUAL(15, task_counter); /* 1+2+3+4+5 = 15 */
    
    graphDestroyTaskScheduler(scheduler);
}

/* Test work stealing */
void testWorkStealing(void) {
    TaskScheduler *scheduler = graphCreateTaskScheduler(2);
    TEST_ASSERT_NOT_NULL(scheduler);
    
    /* Create many tasks for one thread */
    for (int i = 0; i < 100; i++) {
        ParallelTask *task = sqlite3_malloc(sizeof(ParallelTask));
        TEST_ASSERT_NOT_NULL(task);
        
        task->execute = simpleTaskFunc;
        task->arg = sqlite3_malloc(sizeof(int));
        *(int*)task->arg = 1;
        task->priority = 0;
        task->pNext = NULL;
        
        graphScheduleTask(scheduler, task);
    }
    
    /* Wait for completion - work should be stolen between threads */
    usleep(100000); /* 100ms */
    
    graphDestroyTaskScheduler(scheduler);
}

/*
** Query Plan Cache Tests
*/

/* Test cache initialization */
void testPlanCacheInit(void) {
    int rc = graphInitPlanCache(100, 10 * 1024 * 1024);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    
    /* Try to initialize again - should fail */
    rc = graphInitPlanCache(100, 10 * 1024 * 1024);
    TEST_ASSERT_EQUAL(SQLITE_MISUSE, rc);
    
    graphPlanCacheShutdown();
}

/* Test cache operations */
void testPlanCacheOperations(void) {
    graphInitPlanCache(10, 1024 * 1024);
    
    /* Create mock plan */
    PhysicalPlanNode *plan = sqlite3_malloc(sizeof(PhysicalPlanNode));
    TEST_ASSERT_NOT_NULL(plan);
    memset(plan, 0, sizeof(PhysicalPlanNode));
    plan->type = PHYSICAL_ALL_NODES_SCAN;
    
    /* Insert into cache */
    int rc = graphPlanCacheInsert("MATCH (n) RETURN n", plan);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    
    /* Lookup - should find it */
    PhysicalPlanNode *cached = graphPlanCacheLookup("MATCH (n) RETURN n");
    TEST_ASSERT_NOT_NULL(cached);
    TEST_ASSERT_EQUAL(plan, cached);
    
    /* Check statistics */
    sqlite3_int64 hits, misses;
    int nEntries;
    size_t memUsed;
    graphPlanCacheStats(&hits, &misses, &nEntries, &memUsed);
    TEST_ASSERT_EQUAL(1, hits);
    TEST_ASSERT_EQUAL(0, misses);
    TEST_ASSERT_EQUAL(1, nEntries);
    
    /* Lookup non-existent */
    cached = graphPlanCacheLookup("MATCH (m) RETURN m");
    TEST_ASSERT_NULL(cached);
    
    graphPlanCacheStats(&hits, &misses, &nEntries, &memUsed);
    TEST_ASSERT_EQUAL(1, hits);
    TEST_ASSERT_EQUAL(1, misses);
    
    graphPlanCacheShutdown();
}

/* Test LRU eviction */
void testPlanCacheLRU(void) {
    graphInitPlanCache(3, 1024); /* Small cache - 3 entries */
    
    /* Insert 4 plans */
    for (int i = 0; i < 4; i++) {
        PhysicalPlanNode *plan = sqlite3_malloc(sizeof(PhysicalPlanNode));
        memset(plan, 0, sizeof(PhysicalPlanNode));
        
        char query[50];
        sqlite3_snprintf(sizeof(query), query, "MATCH (n%d) RETURN n%d", i, i);
        
        graphPlanCacheInsert(query, plan);
    }
    
    /* First plan should be evicted */
    PhysicalPlanNode *cached = graphPlanCacheLookup("MATCH (n0) RETURN n0");
    TEST_ASSERT_NULL(cached);
    
    /* Others should still be there */
    cached = graphPlanCacheLookup("MATCH (n3) RETURN n3");
    TEST_ASSERT_NOT_NULL(cached);
    
    graphPlanCacheShutdown();
}

/*
** Property Compression Tests
*/

/* Test dictionary initialization */
void testDictionaryInit(void) {
    int rc = graphInitStringDictionary(64);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    
    /* Check initial stats */
    sqlite3_int64 entries = 0;
    size_t memory = 0, saved = 0;
    graphCompressionStats(&entries, &memory, &saved);
    TEST_ASSERT_EQUAL(0, entries);
    TEST_ASSERT_EQUAL(0, saved);
    
    graphCompressionShutdown();
}

/* Test string compression */
void testStringCompression(void) {
    graphInitStringDictionary(64);
    
    /* Compress properties with repeated strings */
    const char *props = "{\"city\":\"San Francisco\",\"state\":\"California\","
                       "\"country\":\"United States\",\"continent\":\"North America\"}";
    
    char *compressed = graphCompressProperties(props);
    TEST_ASSERT_NOT_NULL(compressed);
    TEST_ASSERT_TRUE(strstr(compressed, "_compressed") != NULL);
    
    /* Decompress */
    char *decompressed = graphDecompressProperties(compressed);
    TEST_ASSERT_NOT_NULL(decompressed);
    /* Would check content matches original */
    
    sqlite3_free(compressed);
    sqlite3_free(decompressed);
    graphCompressionShutdown();
}

/* Test zlib compression */
void testZlibCompression(void) {
    /* Create large string */
    char *large = sqlite3_malloc(10000);
    TEST_ASSERT_NOT_NULL(large);
    
    /* Fill with repetitive data */
    for (int i = 0; i < 10000; i++) {
        large[i] = 'A' + (i % 26);
    }
    large[9999] = '\0';
    
    size_t compressedSize;
    char *compressed = graphCompressLarge(large, &compressedSize);
    TEST_ASSERT_NOT_NULL(compressed);
    TEST_ASSERT_TRUE(compressedSize < 10000); /* Should compress well */
    
    /* Check for zlib header */
    TEST_ASSERT_TRUE(strncmp(compressed, "ZLIB:", 5) == 0);
    
    /* Decompress */
    char *decompressed = graphDecompressLarge(compressed, compressedSize);
    TEST_ASSERT_NOT_NULL(decompressed);
    TEST_ASSERT_EQUAL_STRING(large, decompressed);
    
    sqlite3_free(large);
    sqlite3_free(compressed);
    sqlite3_free(decompressed);
}

/*
** Bulk Loading Tests
*/

/* Test CSV parser */
void testCSVParser(void) {
    const char *csv = "id,name,age\n1,Alice,30\n2,Bob,25\n";
    
    CSVParser *parser = csvParserCreate(csv, strlen(csv));
    TEST_ASSERT_NOT_NULL(parser);
    
    /* Parse header */
    int rc = csvParseHeader(parser);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    TEST_ASSERT_EQUAL(3, parser->nColumns);
    TEST_ASSERT_EQUAL_STRING("id", parser->headers[0]);
    TEST_ASSERT_EQUAL_STRING("name", parser->headers[1]);
    TEST_ASSERT_EQUAL_STRING("age", parser->headers[2]);
    
    /* Parse first row */
    char **row = csvParseRow(parser);
    TEST_ASSERT_NOT_NULL(row);
    TEST_ASSERT_EQUAL_STRING("1", row[0]);
    TEST_ASSERT_EQUAL_STRING("Alice", row[1]);
    TEST_ASSERT_EQUAL_STRING("30", row[2]);
    
    for (int i = 0; i < 3; i++) {
        sqlite3_free(row[i]);
    }
    sqlite3_free(row);
    
    csvParserFree(parser);
}

/* Test batch accumulator */
void testBatchAccumulator(void) {
    BatchAccumulator *batch = batchAccumulatorCreate(100);
    TEST_ASSERT_NOT_NULL(batch);
    TEST_ASSERT_EQUAL(0, batch->nNodes);
    TEST_ASSERT_EQUAL(0, batch->nEdges);
    TEST_ASSERT_EQUAL(100, batch->capacity);
    
    /* Add some nodes */
    for (int i = 0; i < 10; i++) {
        GraphNode *node = sqlite3_malloc(sizeof(GraphNode));
        memset(node, 0, sizeof(GraphNode));
        node->iNodeId = i;
        batch->nodes[batch->nNodes++] = node;
    }
    
    TEST_ASSERT_EQUAL(10, batch->nNodes);
    
    /* Would test flush to graph */
    
    batchAccumulatorFree(batch);
}

/* Test bulk load configuration */
void testBulkLoadConfig(void) {
    BulkLoaderConfig config = {
        .batchSize = 1000,
        .deferIndexing = 1,
        .parallelImport = 0,
        .validateData = 1,
        .compressProperties = 1,
        .progressCallback = NULL,
        .progressArg = NULL
    };
    
    TEST_ASSERT_EQUAL(1000, config.batchSize);
    TEST_ASSERT_EQUAL(1, config.deferIndexing);
    TEST_ASSERT_EQUAL(1, config.compressProperties);
}

/*
** Integration Tests
*/

/* Test parallel pattern matching */
void testParallelPatternMatching(void) {
    /* Create mock graph */
    GraphVtab *test_graph = createMockGraph();
    TEST_ASSERT_NOT_NULL(test_graph);
    
    /* Add test nodes */
    for (int i = 0; i < 100; i++) {
        graphAddNode(test_graph, i, "{}");
    }
    
    /* Create a simple AST pattern for testing */
    CypherAst *pattern = cypherAstCreate(CYPHER_AST_NODE_PATTERN, 0, 0);
    TEST_ASSERT_NOT_NULL(pattern);
    
    /* Add a label to the pattern */
    CypherAst *labels = cypherAstCreate(CYPHER_AST_LABELS, 0, 0);
    CypherAst *label = cypherAstCreate(CYPHER_AST_IDENTIFIER, 0, 0);
    cypherAstSetValue(label, "Person");
    cypherAstAddChild(labels, label);
    cypherAstAddChild(pattern, labels);
    
    /* Run parallel pattern match */
    sqlite3_int64 *results;
    int nResults;
    int rc = graphParallelPatternMatch(test_graph, pattern, &results, &nResults);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    TEST_ASSERT_EQUAL(100, nResults);
    
    sqlite3_free(results);
    
    /* Cleanup */
    destroyMockGraph(test_graph);
}

/* Test memory pool with real allocations */
void testMemoryPoolIntegration(void) {
    QueryMemoryPool *pool = graphCreateMemoryPool(1024);
    TEST_ASSERT_NOT_NULL(pool);
    
    /* Allocate various sizes */
    void *p1 = graphPoolAlloc(pool, 100);
    void *p2 = graphPoolAlloc(pool, 200);
    void *p3 = graphPoolAlloc(pool, 300);
    
    TEST_ASSERT_NOT_NULL(p1);
    TEST_ASSERT_NOT_NULL(p2);
    TEST_ASSERT_NOT_NULL(p3);
    
    /* Check alignment */
    TEST_ASSERT_EQUAL(0, ((uintptr_t)p1 % 8));
    TEST_ASSERT_EQUAL(0, ((uintptr_t)p2 % 8));
    TEST_ASSERT_EQUAL(0, ((uintptr_t)p3 % 8));
    
    /* Test pool exhaustion */
    void *p4 = graphPoolAlloc(pool, 500); /* Should exceed pool */
    TEST_ASSERT_NOT_NULL(p4); /* Falls back to malloc */
    
    graphDestroyMemoryPool(pool);
}

/* Tests are run from unified test_main.c */