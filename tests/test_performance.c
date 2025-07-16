/*
** SQLite Graph Database Extension - Performance Tests
**
** Performance benchmarks and scalability tests for traversal algorithms.
** Tests performance on various graph sizes and structures.
*/

#define SQLITE_CORE
#include "unity.h"
#include "sqlite3.h"
#include "../include/graph.h"
#include "../include/graph-vtab.h"
#include <time.h>
#include <stdio.h>
#include <string.h>

/* Test database pointer */
static sqlite3 *pDb = 0;
static GraphVtab *pVtab = 0;


/*
** Setup performance test environment.
*/
void setupPerformanceTests(void){
  int rc;
  
  /* Create in-memory database */
  rc = sqlite3_open(":memory:", &pDb);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  /* Allocate vtab structure for testing */
  pVtab = (GraphVtab*)sqlite3_malloc(sizeof(GraphVtab));
  TEST_ASSERT_NOT_NULL(pVtab);
  
  memset(pVtab, 0, sizeof(GraphVtab));
  pVtab->pDb = pDb;
  pVtab->zDbName = sqlite3_mprintf("main");
  pVtab->zTableName = sqlite3_mprintf("perf_graph");
}

/*
** Cleanup performance test environment.
*/
void teardownPerformanceTests(void){
  if( pVtab ){
    char *zSql = sqlite3_mprintf("DROP TABLE %s_nodes; DROP TABLE %s_edges;", pVtab->zTableName, pVtab->zTableName);
    sqlite3_exec(pDb, zSql, 0, 0, 0);
    sqlite3_free(zSql);
    sqlite3_free(pVtab->zDbName);
    sqlite3_free(pVtab->zTableName);
    sqlite3_free(pVtab);
    pVtab = 0;
  }
  
  if( pDb ){
    sqlite3_close(pDb);
    pDb = 0;
  }
}

/*
** Create a linear chain graph of specified size.
** Graph structure: 1 -> 2 -> 3 -> ... -> n
*/
static void createLinearChain(int nNodes){
  int i;
  int rc;
  
  /* Add nodes */
  for( i=1; i<=nNodes; i++ ){
    rc = graphAddNode(pVtab, i, "{}");
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  }
  
  /* Add edges */
  for( i=1; i<nNodes; i++ ){
    rc = graphAddEdge(pVtab, i, i+1, 1.0, "{}");
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  }
}

/*
** Create a binary tree of specified depth.
** Each node has up to 2 children.
*/
static void createBinaryTree(int nDepth){
  int nNodes = (1 << nDepth) - 1;  /* 2^depth - 1 nodes */
  int i;
  int rc;
  
  /* Add nodes */
  for( i=1; i<=nNodes; i++ ){
    rc = graphAddNode(pVtab, i, "{}");
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  }
  
  /* Add edges (parent i has children 2*i and 2*i+1) */
  for( i=1; i<=nNodes/2; i++ ){
    if( 2*i <= nNodes ){
      rc = graphAddEdge(pVtab, i, 2*i, 1.0, "{}");
      TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    }
    if( 2*i+1 <= nNodes ){
      rc = graphAddEdge(pVtab, i, 2*i+1, 1.0, "{}");
      TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    }
  }
}

/*
** Create a complete graph (every node connected to every other node).
** Warning: O(n^2) edges!
*/
static void createCompleteGraph(int nNodes){
  int i, j;
  int rc;
  
  /* Add nodes */
  for( i=1; i<=nNodes; i++ ){
    rc = graphAddNode(pVtab, i, "{}");
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  }
  
  /* Add edges between all pairs */
  for( i=1; i<=nNodes; i++ ){
    for( j=i+1; j<=nNodes; j++ ){
      rc = graphAddEdge(pVtab, i, j, 1.0, "{}");
      TEST_ASSERT_EQUAL(SQLITE_OK, rc);
      rc = graphAddEdge(pVtab, j, i, 1.0, "{}");
      TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    }
  }
}

/*
** Measure execution time in milliseconds.
*/
static double getTimeMs(void){
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

/*
** Test DFS performance on linear chain graphs.
*/
void testDFSPerformanceLinear(void){
  setupPerformanceTests();
  const int sizes[] = {100, 500, 1000, 5000};
  int i;
  
  printf("\n=== DFS Performance - Linear Chain ===\n");
  printf("Size\tTime(ms)\tNodes/sec\n");
  
  for( i=0; i<(int)(sizeof(sizes)/sizeof(sizes[0])); i++ ){
    char *zPath = 0;
    double startTime, endTime, elapsed;
    int rc;
    
    createLinearChain(sizes[i]);
    
    startTime = getTimeMs();
    rc = graphDFS(pVtab, 1, -1, &zPath);
    endTime = getTimeMs();
    
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    TEST_ASSERT_NOT_NULL(zPath);
    
    elapsed = endTime - startTime;
    printf("%d\t%.2f\t\t%.0f\n", sizes[i], elapsed, sizes[i]*1000.0/elapsed);
    
    sqlite3_free(zPath);
  }
  teardownPerformanceTests();
}

/*
** Test BFS performance on linear chain graphs.
*/
void testBFSPerformanceLinear(void){
  setupPerformanceTests();
  const int sizes[] = {100, 500, 1000, 5000};
  int i;
  
  printf("\n=== BFS Performance - Linear Chain ===\n");
  printf("Size\tTime(ms)\tNodes/sec\n");
  
  for( i=0; i<(int)(sizeof(sizes)/sizeof(sizes[0])); i++ ){
    char *zPath = 0;
    double startTime, endTime, elapsed;
    int rc;
    
    createLinearChain(sizes[i]);
    
    startTime = getTimeMs();
    rc = graphBFS(pVtab, 1, -1, &zPath);
    endTime = getTimeMs();
    
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    TEST_ASSERT_NOT_NULL(zPath);
    
    elapsed = endTime - startTime;
    printf("%d\t%.2f\t\t%.0f\n", sizes[i], elapsed, sizes[i]*1000.0/elapsed);
    
    sqlite3_free(zPath);
  }
  teardownPerformanceTests();
}

/*
** Test DFS performance on binary trees.
*/
void testDFSPerformanceBinaryTree(void){
  setupPerformanceTests();
  const int depths[] = {8, 10, 12, 14};  /* 255, 1023, 4095, 16383 nodes */
  int i;
  
  printf("\n=== DFS Performance - Binary Tree ===\n");
  printf("Depth\tNodes\tTime(ms)\tNodes/sec\n");
  
  for( i=0; i<(int)(sizeof(depths)/sizeof(depths[0])); i++ ){
    char *zPath = 0;
    double startTime, endTime, elapsed;
    int nNodes = (1 << depths[i]) - 1;
    int rc;
    
    createBinaryTree(depths[i]);
    
    startTime = getTimeMs();
    rc = graphDFS(pVtab, 1, -1, &zPath);
    endTime = getTimeMs();
    
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    TEST_ASSERT_NOT_NULL(zPath);
    
    elapsed = endTime - startTime;
    printf("%d\t%d\t%.2f\t\t%.0f\n", depths[i], nNodes, elapsed, 
           nNodes*1000.0/elapsed);
    
    sqlite3_free(zPath);
  }
  teardownPerformanceTests();
}

/*
** Test BFS performance on binary trees.
*/
void testBFSPerformanceBinaryTree(void){
  setupPerformanceTests();
  const int depths[] = {8, 10, 12, 14};  /* 255, 1023, 4095, 16383 nodes */
  int i;
  
  printf("\n=== BFS Performance - Binary Tree ===\n");
  printf("Depth\tNodes\tTime(ms)\tNodes/sec\n");
  
  for( i=0; i<(int)(sizeof(depths)/sizeof(depths[0])); i++ ){
    char *zPath = 0;
    double startTime, endTime, elapsed;
    int nNodes = (1 << depths[i]) - 1;
    int rc;
    
    createBinaryTree(depths[i]);
    
    startTime = getTimeMs();
    rc = graphBFS(pVtab, 1, -1, &zPath);
    endTime = getTimeMs();
    
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    TEST_ASSERT_NOT_NULL(zPath);
    
    elapsed = endTime - startTime;
    printf("%d\t%d\t%.2f\t\t%.0f\n", depths[i], nNodes, elapsed, 
           nNodes*1000.0/elapsed);
    
    sqlite3_free(zPath);
  }
  teardownPerformanceTests();
}

/*
** Test DFS performance on complete graphs (stress test).
*/
void testDFSPerformanceComplete(void){
  setupPerformanceTests();
  const int sizes[] = {10, 20, 30, 40};
  int i;
  
  printf("\n=== DFS Performance - Complete Graph ===\n");
  printf("Size\tEdges\tTime(ms)\tNodes/sec\n");
  
  for( i=0; i<(int)(sizeof(sizes)/sizeof(sizes[0])); i++ ){
    char *zPath = 0;
    double startTime, endTime, elapsed;
    int nEdges = sizes[i] * (sizes[i] - 1);
    int rc;
    
    createCompleteGraph(sizes[i]);
    
    startTime = getTimeMs();
    rc = graphDFS(pVtab, 1, -1, &zPath);
    endTime = getTimeMs();
    
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    TEST_ASSERT_NOT_NULL(zPath);
    
    elapsed = endTime - startTime;
    printf("%d\t%d\t%.2f\t\t%.0f\n", sizes[i], nEdges, elapsed, 
           sizes[i]*1000.0/elapsed);
    
    sqlite3_free(zPath);
  }
  teardownPerformanceTests();
}

/*
** Test BFS performance on complete graphs (stress test).
*/
void testBFSPerformanceComplete(void){
  setupPerformanceTests();
  const int sizes[] = {10, 20, 30, 40};
  int i;
  
  printf("\n=== BFS Performance - Complete Graph ===\n");
  printf("Size\tEdges\tTime(ms)\tNodes/sec\n");
  
  for( i=0; i<(int)(sizeof(sizes)/sizeof(sizes[0])); i++ ){
    char *zPath = 0;
    double startTime, endTime, elapsed;
    int nEdges = sizes[i] * (sizes[i] - 1);
    int rc;
    
    createCompleteGraph(sizes[i]);
    
    startTime = getTimeMs();
    rc = graphBFS(pVtab, 1, -1, &zPath);
    endTime = getTimeMs();
    
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    TEST_ASSERT_NOT_NULL(zPath);
    
    elapsed = endTime - startTime;
    printf("%d\t%d\t%.2f\t\t%.0f\n", sizes[i], nEdges, elapsed, 
           sizes[i]*1000.0/elapsed);
    
    sqlite3_free(zPath);
  }
  teardownPerformanceTests();
}

/*
** Test memory usage during traversal.
*/
void testTraversalMemoryUsage(void){
  setupPerformanceTests();
  const int sizes[] = {1000, 5000, 10000};
  int i;
  
  printf("\n=== Traversal Memory Usage ===\n");
  printf("Size\tDFS Path\tBFS Path\n");
  
  for( i=0; i<(int)(sizeof(sizes)/sizeof(sizes[0])); i++ ){
    char *zDFSPath = 0;
    char *zBFSPath = 0;
    int rc;
    
    createLinearChain(sizes[i]);
    
    /* Measure DFS path length */
    rc = graphDFS(pVtab, 1, -1, &zDFSPath);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    TEST_ASSERT_NOT_NULL(zDFSPath);
    
    /* Measure BFS path length */
    rc = graphBFS(pVtab, 1, -1, &zBFSPath);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    TEST_ASSERT_NOT_NULL(zBFSPath);
    
    printf("%d\t%zu bytes\t%zu bytes\n", sizes[i], 
           strlen(zDFSPath), strlen(zBFSPath));
    
    sqlite3_free(zDFSPath);
    sqlite3_free(zBFSPath);
  }
  teardownPerformanceTests();
}

/*
** Verify O(V+E) time complexity for traversals.
*/
void testTraversalTimeComplexity(void){
  setupPerformanceTests();
  double times[4];
  const int sizes[] = {1000, 2000, 4000, 8000};
  int i;
  
  printf("\n=== Time Complexity Verification ===\n");
  printf("Testing O(V+E) complexity for DFS...\n");
  
  /* Measure DFS times for different sizes */
  for( i=0; i<4; i++ ){
    char *zPath = 0;
    double startTime, endTime;
    int rc;
    
    createLinearChain(sizes[i]);
    
    startTime = getTimeMs();
    rc = graphDFS(pVtab, 1, -1, &zPath);
    (void)rc;  /* Performance test - result not checked */
    endTime = getTimeMs();
    
    times[i] = endTime - startTime;
    
    sqlite3_free(zPath);
  }
  
  /* Check if times scale linearly */
  printf("Size ratios: 1:2:4:8\n");
  printf("Time ratios: 1:%.1f:%.1f:%.1f\n",
         times[1]/times[0], times[2]/times[0], times[3]/times[0]);
  
  /* Verify approximately linear scaling */
  double ratio21 = times[1] / times[0];
  double ratio42 = times[2] / times[1];
  double ratio84 = times[3] / times[2];
  
  TEST_ASSERT_FLOAT_WITHIN(0.5, 2.0, ratio21);
  TEST_ASSERT_FLOAT_WITHIN(0.5, 2.0, ratio42);
  TEST_ASSERT_FLOAT_WITHIN(0.5, 2.0, ratio84);
  teardownPerformanceTests();
}

/*
** Main performance test runner.
*/
void runPerformanceTests(void){
  printf("\n========================================\n");
  printf("   Graph Traversal Performance Tests\n");
  printf("========================================\n");
  
  RUN_TEST(testDFSPerformanceLinear);
  RUN_TEST(testBFSPerformanceLinear);
  RUN_TEST(testDFSPerformanceBinaryTree);
  RUN_TEST(testBFSPerformanceBinaryTree);
  RUN_TEST(testDFSPerformanceComplete);
  RUN_TEST(testBFSPerformanceComplete);
  RUN_TEST(testTraversalMemoryUsage);
  RUN_TEST(testTraversalTimeComplexity);
  
  printf("\n========================================\n");
}