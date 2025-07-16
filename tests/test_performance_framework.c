/*
** SQLite Graph Database Extension - Performance Test Framework
**
** Performance testing framework for graph operations benchmarking.
*/

#define SQLITE_CORE
#include "unity.h"
#include "sqlite3.h"
#include "../include/graph.h"
#include "../include/graph-vtab.h"
#include <time.h>
#include <stdio.h>

typedef struct {
  const char *name;
  void (*setup)(void);
  void (*test)(void);
  void (*teardown)(void);
  double target_ms;
} PerfTest;

static sqlite3 *pDb = 0;
static GraphVtab *pVtab = 0;

static void runPerfTest(PerfTest *test) {
  clock_t start, end;
  double elapsed_ms;
  
  if (test->setup) test->setup();
  
  start = clock();
  test->test();
  end = clock();
  
  elapsed_ms = ((double)(end - start) / CLOCKS_PER_SEC) * 1000;
  
  if (test->teardown) test->teardown();
  
  printf("Performance test '%s' took %.2fms (target: %.2fms)\n",
         test->name, elapsed_ms, test->target_ms);
  
  TEST_ASSERT_MESSAGE(elapsed_ms <= test->target_ms,
    "Performance test '%s' took %.2fms (target: %.2fms)");
}

static void setupPerfTests(void) {
  int rc = sqlite3_open(":memory:", &pDb);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  pVtab = (GraphVtab*)sqlite3_malloc(sizeof(GraphVtab));
  TEST_ASSERT_NOT_NULL(pVtab);
  
  memset(pVtab, 0, sizeof(GraphVtab));
  pVtab->pDb = pDb;
  pVtab->zDbName = sqlite3_mprintf("main");
  pVtab->zTableName = sqlite3_mprintf("perf_graph");
}

static void teardownPerfTests(void) {
  if (pVtab) {
    while (pVtab->pNodeList) {
      GraphNode *pNode = pVtab->pNodeList;
      pVtab->pNodeList = pNode->pNext;
      sqlite3_free(pNode->zProperties);
      sqlite3_free(pNode);
    }
    
    while (pVtab->pEdgeList) {
      GraphEdge *pEdge = pVtab->pEdgeList;
      pVtab->pEdgeList = pEdge->pNext;
      sqlite3_free(pEdge->zProperties);
      sqlite3_free(pEdge);
    }
    
    sqlite3_free(pVtab->zDbName);
    sqlite3_free(pVtab->zTableName);
    sqlite3_free(pVtab);
    pVtab = 0;
  }
  
  if (pDb) {
    sqlite3_close(pDb);
    pDb = 0;
  }
}

static void create_10k_nodes(void) {
  for (int i = 0; i < 10000; i++) {
    char *zProp = sqlite3_mprintf("{\"id\":%d}", i);
    int rc = graphAddNode(pVtab, i, zProp);
    sqlite3_free(zProp);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  }
}

static void create_1k_edges(void) {
  // First create some nodes
  for (int i = 0; i < 100; i++) {
    char *zProp = sqlite3_mprintf("{\"id\":%d}", i);
    int rc = graphAddNode(pVtab, i, zProp);
    sqlite3_free(zProp);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  }
  
  // Then create edges
  for (int i = 0; i < 1000; i++) {
    int from = i % 100;
    int to = (i + 1) % 100;
    int rc = graphAddEdge(pVtab, from, to, 1.0, "{}");
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  }
}

void test_graph_create_performance(void) {
  /* Create 10000 nodes in < 100ms */
  PerfTest test = {
    .name = "Create 10k nodes",
    .setup = setupPerfTests,
    .test = create_10k_nodes,
    .teardown = teardownPerfTests,
    .target_ms = 100.0
  };
  runPerfTest(&test);
}

void test_graph_edge_performance(void) {
  /* Create 1000 edges in < 50ms */
  PerfTest test = {
    .name = "Create 1k edges",
    .setup = setupPerfTests,
    .test = create_1k_edges,
    .teardown = teardownPerfTests,
    .target_ms = 50.0
  };
  runPerfTest(&test);
}

void runPerformanceFrameworkTests(void) {
  printf("\n========================================\n");
  printf("   Graph Performance Framework Tests\n");
  printf("========================================\n");
  
  RUN_TEST(test_graph_create_performance);
  RUN_TEST(test_graph_edge_performance);
  
  printf("========================================\n");
}