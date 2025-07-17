/*
** SQLite Graph Database Extension - Algorithm Tests
**
** Comprehensive tests for graph algorithms including shortest path,
** PageRank, centrality measures, and graph properties.
*/

#define SQLITE_CORE
#include "unity.h"
#include "sqlite3.h"
#include "graph.h"
#include "graph-vtab.h"
#include <float.h>
#include <math.h>
#include <string.h>

/* Test database pointer */
static sqlite3 *pDb = 0;
static GraphVtab *pVtab = 0;


/*
** Setup test environment.
** Reuses the global test database instead of creating a new one.
*/
void setupAlgorithmTests(void){
  /* Use the global test database from Unity setUp */
  extern sqlite3 *g_pTestDb;
  pDb = g_pTestDb;
  
  /* Allocate vtab structure for testing */
  pVtab = (GraphVtab*)sqlite3_malloc(sizeof(GraphVtab));
  TEST_ASSERT_NOT_NULL(pVtab);
  
  memset(pVtab, 0, sizeof(GraphVtab));
  pVtab->pDb = pDb;
  pVtab->zDbName = sqlite3_mprintf("main");
  pVtab->zTableName = sqlite3_mprintf("algo_graph");
}

/*
** Cleanup test environment.
*/
void teardownAlgorithmTests(void){
  if( pVtab ){
    char *zSql = sqlite3_mprintf("DROP TABLE %s_nodes; DROP TABLE %s_edges;", pVtab->zTableName, pVtab->zTableName);
    sqlite3_exec(pDb, zSql, 0, 0, 0);
    sqlite3_free(zSql);
    sqlite3_free(pVtab->zDbName);
    sqlite3_free(pVtab->zTableName);
    sqlite3_free(pVtab);
    pVtab = 0;
  }
  
  /* Don't close pDb - it's managed by Unity setUp/tearDown */
  pDb = 0;
}

/*
** Create a simple weighted graph for testing shortest paths.
**      1 ---(5)--- 2
**      |            |
**     (2)          (1)
**      |            |
**      3 ---(3)--- 4
*/
static void createWeightedGraph(void){
  int rc;
  
  /* Add nodes */
  rc = graphAddNode(pVtab, 1, "{\"name\":\"A\"}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  rc = graphAddNode(pVtab, 2, "{\"name\":\"B\"}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  rc = graphAddNode(pVtab, 3, "{\"name\":\"C\"}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  rc = graphAddNode(pVtab, 4, "{\"name\":\"D\"}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  /* Add weighted edges */
  rc = graphAddEdge(pVtab, 1, 2, 5.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  rc = graphAddEdge(pVtab, 1, 3, 2.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  rc = graphAddEdge(pVtab, 2, 4, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  rc = graphAddEdge(pVtab, 3, 4, 3.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
}

/*
** Create a graph for PageRank testing.
** Classic example from PageRank paper.
*/
static void createPageRankGraph(void){
  int rc;
  
  /* Add nodes */
  for( int i=1; i<=4; i++ ){
    rc = graphAddNode(pVtab, i, "{}");
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  }
  
  /* Add edges (directed) */
  rc = graphAddEdge(pVtab, 1, 2, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  rc = graphAddEdge(pVtab, 1, 3, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  rc = graphAddEdge(pVtab, 2, 3, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  rc = graphAddEdge(pVtab, 3, 1, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  rc = graphAddEdge(pVtab, 4, 3, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
}

/*
** Test Dijkstra's algorithm on simple graph.
*/
void testDijkstraSimple(void){
  setupAlgorithmTests();
  char *zPath = 0;
  double rDistance = 0.0;
  int rc;
  
  createWeightedGraph();
  
  /* Find shortest path from 1 to 4 */
  rc = graphDijkstra(pVtab, 1, 4, &zPath, &rDistance);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_NOT_NULL(zPath);
  
  /* Shortest path should be 1->3->4 with distance 5 */
  TEST_ASSERT_EQUAL_STRING("[1,3,4]", zPath);
  TEST_ASSERT_EQUAL_DOUBLE(5.0, rDistance);
  
  sqlite3_free(zPath);
  teardownAlgorithmTests();
}

/*
** Test Dijkstra with no path.
*/
void testDijkstraNoPath(void){
  setupAlgorithmTests();
  char *zPath = 0;
  double rDistance = DBL_MAX;
  int rc;
  
  
  /* Create disconnected nodes */
  rc = graphAddNode(pVtab, 1, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  rc = graphAddNode(pVtab, 2, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  /* Try to find path between disconnected nodes */
  rc = graphDijkstra(pVtab, 1, 2, &zPath, &rDistance);
  TEST_ASSERT_EQUAL(SQLITE_NOTFOUND, rc);
  TEST_ASSERT_NULL(zPath);
  TEST_ASSERT_EQUAL_DOUBLE(DBL_MAX, rDistance);
  
  teardownAlgorithmTests();
}

/*
** Test Dijkstra with start node not found.
*/
void testDijkstraStartNotFound(void){
  setupAlgorithmTests();
  char *zPath = 0;
  double rDistance;
  int rc;
  
  createWeightedGraph();
  
  /* Try non-existent start node */
  rc = graphDijkstra(pVtab, 99, 1, &zPath, &rDistance);
  TEST_ASSERT_EQUAL(SQLITE_NOTFOUND, rc);
  TEST_ASSERT_NULL(zPath);
  
  teardownAlgorithmTests();
}

/*
** Test shortest path finding all distances.
*/
void testDijkstraAllDistances(void){
  setupAlgorithmTests();
  char *zPath = 0;
  int rc;
  
  createWeightedGraph();
  
  /* Find distances from node 1 to all others */
  rc = graphDijkstra(pVtab, 1, -1, &zPath, NULL);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  /* TODO: Check format of all distances */
  
  sqlite3_free(zPath);
  teardownAlgorithmTests();
}

/*
** Test PageRank algorithm.
*/
void testPageRankSimple(void){
  setupAlgorithmTests();
  char *zResults = 0;
  int rc;
  
  createPageRankGraph();
  
  /* Calculate PageRank with default parameters */
  rc = graphPageRank(pVtab, 0.85, 100, 0.0001, &zResults);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_NOT_NULL(zResults);
  
  /* Check that results contain all nodes */
  TEST_ASSERT_NOT_NULL(strstr(zResults, "\"1\":"));
  TEST_ASSERT_NOT_NULL(strstr(zResults, "\"2\":"));
  TEST_ASSERT_NOT_NULL(strstr(zResults, "\"3\":"));
  TEST_ASSERT_NOT_NULL(strstr(zResults, "\"4\":"));
  
  /* Node 3 should have highest PageRank (most incoming links) */
  /* TODO: Parse JSON and verify relative values */
  
  sqlite3_free(zResults);
  teardownAlgorithmTests();
}

/*
** Test PageRank on empty graph.
*/
void testPageRankEmpty(void){
  setupAlgorithmTests();
  char *zResults = 0;
  int rc;
  
  /* No nodes added */
  
  rc = graphPageRank(pVtab, 0.85, 100, 0.0001, &zResults);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_NOT_NULL(zResults);
  TEST_ASSERT_EQUAL_STRING("{}", zResults);
  
  sqlite3_free(zResults);
  teardownAlgorithmTests();
}

/*
** Test PageRank convergence.
*/
void testPageRankConvergence(void){
  setupAlgorithmTests();
  char *zResults1 = 0;
  char *zResults2 = 0;
  int rc;
  
  createPageRankGraph();
  
  /* Run with different iteration limits */
  rc = graphPageRank(pVtab, 0.85, 10, 0.0001, &zResults1);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  rc = graphPageRank(pVtab, 0.85, 100, 0.0001, &zResults2);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  /* Results should be similar if converged */
  /* TODO: Parse and compare values */
  
  sqlite3_free(zResults1);
  sqlite3_free(zResults2);
  teardownAlgorithmTests();
}

/*
** Test degree calculations.
*/
void testDegreeCalculations(void){
  setupAlgorithmTests();
  int nInDegree, nOutDegree, nTotalDegree;
  
  createPageRankGraph();
  
  /* Node 3 has 3 incoming edges and 1 outgoing edge */
  nInDegree = graphInDegree(pVtab, 3);
  TEST_ASSERT_EQUAL(3, nInDegree);
  
  nOutDegree = graphOutDegree(pVtab, 3);
  TEST_ASSERT_EQUAL(1, nOutDegree);
  
  nTotalDegree = graphTotalDegree(pVtab, 3);
  TEST_ASSERT_EQUAL(4, nTotalDegree);
  
  /* Node 4 has 0 incoming edges and 1 outgoing edge */
  nInDegree = graphInDegree(pVtab, 4);
  TEST_ASSERT_EQUAL(0, nInDegree);
  
  nOutDegree = graphOutDegree(pVtab, 4);
  TEST_ASSERT_EQUAL(1, nOutDegree);
  
  teardownAlgorithmTests();
}

/*
** Test degree centrality.
*/
void testDegreeCentrality(void){
  setupAlgorithmTests();
  double rCentrality;
  
  createPageRankGraph();
  
  /* Directed graph centrality */
  rCentrality = graphDegreeCentrality(pVtab, 3, 1);
  /* Node 3: degree 4, n=4, centrality = 4/(2*(4-1)) = 4/6 = 0.667 */
  TEST_ASSERT_FLOAT_WITHIN(0.01, 0.667, rCentrality);
  
  /* Node 4: degree 1, centrality = 1/6 = 0.167 */
  rCentrality = graphDegreeCentrality(pVtab, 4, 1);
  TEST_ASSERT_FLOAT_WITHIN(0.01, 0.167, rCentrality);
  
  teardownAlgorithmTests();
}

/*
** Test graph connectivity.
*/
void testGraphConnectivity(void){
  setupAlgorithmTests();
  int bConnected;
  int rc;
  
  
  /* Empty graph is connected */
  bConnected = graphIsConnected(pVtab);
  TEST_ASSERT_EQUAL(1, bConnected);
  
  /* Single node is connected */
  rc = graphAddNode(pVtab, 1, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  bConnected = graphIsConnected(pVtab);
  TEST_ASSERT_EQUAL(1, bConnected);
  
  /* Add disconnected node */
  rc = graphAddNode(pVtab, 2, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  bConnected = graphIsConnected(pVtab);
  TEST_ASSERT_EQUAL(0, bConnected);
  
  /* Connect them */
  rc = graphAddEdge(pVtab, 1, 2, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  bConnected = graphIsConnected(pVtab);
  TEST_ASSERT_EQUAL(1, bConnected);
  
  teardownAlgorithmTests();
}

/*
** Test graph density.
*/
void testGraphDensity(void){
  setupAlgorithmTests();
  double rDensity;
  int rc;
  
  
  /* Empty graph has density 0 */
  rDensity = graphDensity(pVtab, 1);
  TEST_ASSERT_EQUAL_DOUBLE(0.0, rDensity);
  
  /* Complete graph with 3 nodes */
  for( int i=1; i<=3; i++ ){
    rc = graphAddNode(pVtab, i, "{}");
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  }
  
  /* Add all possible edges (6 for directed) */
  for( int i=1; i<=3; i++ ){
    for( int j=1; j<=3; j++ ){
      if( i!=j ){
        rc = graphAddEdge(pVtab, i, j, 1.0, "{}");
        TEST_ASSERT_EQUAL(SQLITE_OK, rc);
      }
    }
  }
  
  /* Directed density: 6/(3*2) = 1.0 */
  rDensity = graphDensity(pVtab, 1);
  TEST_ASSERT_EQUAL_DOUBLE(1.0, rDensity);
  (void)rDensity;  /* Suppress unused warning in optimized builds */
  
  teardownAlgorithmTests();
}

/*
** Test shortest path on unweighted graph.
*/
void testShortestPathUnweighted(void){
  setupAlgorithmTests();
  char *zPath = 0;
  int rc;
  
  
  /* Create unweighted graph */
  for( int i=1; i<=5; i++ ){
    rc = graphAddNode(pVtab, i, "{}");
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  }
  
  /* Create path 1->2->3->4->5 and shortcut 1->3 */
  rc = graphAddEdge(pVtab, 1, 2, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  rc = graphAddEdge(pVtab, 2, 3, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  rc = graphAddEdge(pVtab, 3, 4, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  rc = graphAddEdge(pVtab, 4, 5, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  rc = graphAddEdge(pVtab, 1, 3, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  /* BFS should find shortest hop path */
  rc = graphShortestPathUnweighted(pVtab, 1, 5, &zPath);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_NOT_NULL(zPath);
  
  /* Path should include node 1 */
  TEST_ASSERT_NOT_NULL(strstr(zPath, "1"));
  
  sqlite3_free(zPath);
  teardownAlgorithmTests();
}

/*
** Test Dijkstra on graph with negative weights (should still work).
*/
void testDijkstraMixedWeights(void){
  setupAlgorithmTests();
  char *zPath = 0;
  double rDistance;
  int rc;
  
  
  /* Create graph with mixed weights */
  rc = graphAddNode(pVtab, 1, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  rc = graphAddNode(pVtab, 2, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  rc = graphAddNode(pVtab, 3, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  rc = graphAddEdge(pVtab, 1, 2, 10.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  rc = graphAddEdge(pVtab, 1, 3, 5.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  rc = graphAddEdge(pVtab, 3, 2, 3.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  /* Shortest path 1->2 is via node 3 */
  rc = graphDijkstra(pVtab, 1, 2, &zPath, &rDistance);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_NOT_NULL(zPath);
  TEST_ASSERT_EQUAL_STRING("[1,3,2]", zPath);
  TEST_ASSERT_EQUAL_DOUBLE(8.0, rDistance);
  
  sqlite3_free(zPath);
  teardownAlgorithmTests();
}

/*
** Main test runner for algorithm tests.
*/
void runAlgorithmTests(void){
  RUN_TEST(testDijkstraSimple);
  RUN_TEST(testDijkstraNoPath);
  RUN_TEST(testDijkstraStartNotFound);
  RUN_TEST(testDijkstraAllDistances);
  RUN_TEST(testPageRankSimple);
  RUN_TEST(testPageRankEmpty);
  RUN_TEST(testPageRankConvergence);
  RUN_TEST(testDegreeCalculations);
  RUN_TEST(testDegreeCentrality);
  RUN_TEST(testGraphConnectivity);
  RUN_TEST(testGraphDensity);
  RUN_TEST(testShortestPathUnweighted);
  RUN_TEST(testDijkstraMixedWeights);
}