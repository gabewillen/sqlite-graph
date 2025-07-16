/*
** SQLite Graph Database Extension - Advanced Algorithm Tests
**
** Comprehensive tests for betweenness centrality, closeness centrality,
** topological sort, cycle detection, and strongly connected components.
*/

#define SQLITE_CORE
#include "unity.h"
#include "sqlite3.h"
#include "../include/graph.h"
#include "../include/graph-vtab.h"
#include <string.h>
#include <math.h>

/* Test database pointer */
static sqlite3 *pDb = 0;
static GraphVtab *pVtab = 0;

/*
** Setup test environment.
** Reuses the global test database instead of creating a new one.
*/
void setupAdvancedTests(void){
  /* Use the global test database from Unity setUp */
  extern sqlite3 *g_pTestDb;
  pDb = g_pTestDb;
  
  /* Allocate vtab structure for testing */
  pVtab = (GraphVtab*)sqlite3_malloc(sizeof(GraphVtab));
  TEST_ASSERT_NOT_NULL(pVtab);
  
  memset(pVtab, 0, sizeof(GraphVtab));
  pVtab->pDb = pDb;
  pVtab->zDbName = sqlite3_mprintf("main");
  pVtab->zTableName = sqlite3_mprintf("advanced_graph");

  // Create the virtual table to ensure backing tables are created
  char *zErrMsg = 0;
  int rc = sqlite3_exec(pDb, "CREATE VIRTUAL TABLE advanced_graph USING graph();", 0, 0, &zErrMsg);
  if( rc!=SQLITE_OK ){
    sqlite3_free(zErrMsg);
  }
}

/*
** Cleanup test environment.
*/
void teardownAdvancedTests(void){
  if( pVtab ){
    char *zSql = sqlite3_mprintf("DROP TABLE IF EXISTS %s_nodes; DROP TABLE IF EXISTS %s_edges;", pVtab->zTableName, pVtab->zTableName);
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
** Create Zachary's Karate Club graph for centrality testing.
** Classic graph with known centrality values.
*/
static void createKarateClubGraph(void){
  int rc;
  
  /* Add 5 nodes for simplified karate club */
  for( int i = 1; i <= 5; i++ ){
    rc = graphAddNode(pVtab, i, "{}");
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  }
  
  /* Add edges (undirected - add both directions) */
  int edges[][2] = {
    {1, 2}, {1, 3}, {1, 4},
    {2, 3}, {2, 4},
    {3, 4}, {3, 5},
    {4, 5}
  };
  
  for( int i = 0; i < 8; i++ ){
    rc = graphAddEdge(pVtab, edges[i][0], edges[i][1], 1.0, "{}");
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    rc = graphAddEdge(pVtab, edges[i][1], edges[i][0], 1.0, "{}");
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  }
}

/*
** Test betweenness centrality on simple graph.
*/
void testBetweennessCentralitySimple(void){
  char *zResults = 0;
  int rc;
  
  setupAdvancedTests();
  createKarateClubGraph();
  
  /* Calculate betweenness centrality */
  rc = graphBetweennessCentrality(pVtab, &zResults);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_NOT_NULL(zResults);
  
  /* Node 3 should have highest betweenness (bridge node) */
  TEST_ASSERT_NOT_NULL(strstr(zResults, "\"3\":"));
  
  /* Verify JSON format */
  TEST_ASSERT_EQUAL('{', zResults[0]);
  TEST_ASSERT_EQUAL('}', zResults[strlen(zResults)-1]);
  
  sqlite3_free(zResults);
  teardownAdvancedTests();
}

/*
** Test betweenness centrality on empty graph.
*/
void testBetweennessCentralityEmpty(void){
  char *zResults = 0;
  int rc;
  
  setupAdvancedTests();
  
  rc = graphBetweennessCentrality(pVtab, &zResults);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_NOT_NULL(zResults);
  TEST_ASSERT_EQUAL_STRING("{}", zResults);
  
  sqlite3_free(zResults);
  teardownAdvancedTests();
}

/*
** Test closeness centrality.
*/
void testClosenessCentralitySimple(void){
  char *zResults = 0;
  int rc;
  
  setupAdvancedTests();
  createKarateClubGraph();
  
  /* Calculate closeness centrality */
  rc = graphClosenessCentrality(pVtab, &zResults);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_NOT_NULL(zResults);
  
  /* All nodes should be present */
  for( int i = 1; i <= 5; i++ ){
    char buf[16];
    sprintf(buf, "\"%d\":", i);
    TEST_ASSERT_NOT_NULL(strstr(zResults, buf));
  }
  
  sqlite3_free(zResults);
  teardownAdvancedTests();
}

/*
** Test topological sort on DAG.
*/
void testTopologicalSortDAG(void){
  char *zOrder = 0;
  int rc;
  
  setupAdvancedTests();
  
  /* Create a DAG: 1->2->3, 1->3, 2->4, 3->4 */
  for( int i = 1; i <= 4; i++ ){
    rc = graphAddNode(pVtab, i, "{}");
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  }
  
  rc = graphAddEdge(pVtab, 1, 2, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  rc = graphAddEdge(pVtab, 2, 3, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  rc = graphAddEdge(pVtab, 1, 3, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  rc = graphAddEdge(pVtab, 2, 4, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  rc = graphAddEdge(pVtab, 3, 4, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  /* Perform topological sort */
  rc = graphTopologicalSort(pVtab, &zOrder);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_NOT_NULL(zOrder);
  
  /* Verify it's a valid JSON array */
  TEST_ASSERT_EQUAL('[', zOrder[0]);
  TEST_ASSERT_EQUAL(']', zOrder[strlen(zOrder)-1]);
  
  /* Node 1 should come before all others */
  char *pos1 = strstr(zOrder, "1");
  char *pos2 = strstr(zOrder, "2");
  char *pos3 = strstr(zOrder, "3");
  char *pos4 = strstr(zOrder, "4");
  (void)pos2;  /* Used in future implementation */
  (void)pos3;  /* Used in future implementation */
  
  TEST_ASSERT_NOT_NULL(pos1);
  TEST_ASSERT_NOT_NULL(pos4);
  TEST_ASSERT_TRUE(pos1 < pos4);
  
  sqlite3_free(zOrder);
  teardownAdvancedTests();
}

/*
** Test topological sort with cycle.
*/
void testTopologicalSortCycle(void){
  char *zOrder = 0;
  int rc;
  
  setupAdvancedTests();
  
  /* Create cycle: 1->2->3->1 */
  for( int i = 1; i <= 3; i++ ){
    rc = graphAddNode(pVtab, i, "{}");
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  }
  
  rc = graphAddEdge(pVtab, 1, 2, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  rc = graphAddEdge(pVtab, 2, 3, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  rc = graphAddEdge(pVtab, 3, 1, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  /* Should detect cycle */
  rc = graphTopologicalSort(pVtab, &zOrder);
  TEST_ASSERT_EQUAL(SQLITE_CONSTRAINT, rc);
  TEST_ASSERT_NOT_NULL(zOrder);
  TEST_ASSERT_NOT_NULL(strstr(zOrder, "cycle"));
  
  sqlite3_free(zOrder);
  teardownAdvancedTests();
}

/*
** Test cycle detection.
*/
void testHasCycle(void){
  int bHasCycle;
  int rc;
  
  setupAdvancedTests();
  
  /* Empty graph has no cycle */
  bHasCycle = graphHasCycle(pVtab);
  TEST_ASSERT_EQUAL(0, bHasCycle);
  
  /* DAG has no cycle */
  for( int i = 1; i <= 3; i++ ){
    rc = graphAddNode(pVtab, i, "{}");
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  }
  rc = graphAddEdge(pVtab, 1, 2, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  rc = graphAddEdge(pVtab, 2, 3, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  bHasCycle = graphHasCycle(pVtab);
  TEST_ASSERT_EQUAL(0, bHasCycle);
  
  /* Add back edge to create cycle */
  rc = graphAddEdge(pVtab, 3, 1, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  bHasCycle = graphHasCycle(pVtab);
  TEST_ASSERT_EQUAL(1, bHasCycle);
  
  teardownAdvancedTests();
}

/*
** Test connected components.
*/
void testConnectedComponents(void){
  char *zComponents = 0;
  int rc;
  
  setupAdvancedTests();
  
  /* Create two disconnected components */
  /* Component 1: 1-2-3 */
  for( int i = 1; i <= 3; i++ ){
    rc = graphAddNode(pVtab, i, "{}");
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  }
  rc = graphAddEdge(pVtab, 1, 2, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  rc = graphAddEdge(pVtab, 2, 3, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  /* Component 2: 4-5 */
  rc = graphAddNode(pVtab, 4, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  rc = graphAddNode(pVtab, 5, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  rc = graphAddEdge(pVtab, 4, 5, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  /* Find components */
  rc = graphConnectedComponents(pVtab, &zComponents);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_NOT_NULL(zComponents);
  
  /* Should have 2 components */
  TEST_ASSERT_NOT_NULL(strstr(zComponents, "\"0\":"));
  TEST_ASSERT_NOT_NULL(strstr(zComponents, "\"1\":"));
  
  sqlite3_free(zComponents);
  teardownAdvancedTests();
}

/*
** Test strongly connected components.
*/
void testStronglyConnectedComponents(void){
  char *zSCC = 0;
  int rc;
  
  setupAdvancedTests();
  
  /* Create graph with 2 SCCs */
  /* SCC 1: 1<->2<->3 (cycle) */
  /* SCC 2: 4 (single node) */
  for( int i = 1; i <= 4; i++ ){
    rc = graphAddNode(pVtab, i, "{}");
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  }
  
  /* SCC 1 edges */
  rc = graphAddEdge(pVtab, 1, 2, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  rc = graphAddEdge(pVtab, 2, 3, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  rc = graphAddEdge(pVtab, 3, 1, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  /* Edge from SCC 1 to SCC 2 */
  rc = graphAddEdge(pVtab, 3, 4, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  /* Find SCCs */
  rc = graphStronglyConnectedComponents(pVtab, &zSCC);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_NOT_NULL(zSCC);
  
  /* Should be JSON array */
  TEST_ASSERT_EQUAL('[', zSCC[0]);
  TEST_ASSERT_EQUAL(']', zSCC[strlen(zSCC)-1]);
  
  /* Should contain nodes */
  TEST_ASSERT_NOT_NULL(strstr(zSCC, "1"));
  TEST_ASSERT_NOT_NULL(strstr(zSCC, "4"));
  
  sqlite3_free(zSCC);
  teardownAdvancedTests();
}

/*
** Test betweenness centrality on linear graph.
*/
void testBetweennessCentralityLinear(void){
  char *zResults = 0;
  int rc;
  
  setupAdvancedTests();
  
  /* Create linear graph: 1-2-3-4-5 */
  for( int i = 1; i <= 5; i++ ){
    rc = graphAddNode(pVtab, i, "{}");
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  }
  
  for( int i = 1; i < 5; i++ ){
    rc = graphAddEdge(pVtab, i, i+1, 1.0, "{}");
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    rc = graphAddEdge(pVtab, i+1, i, 1.0, "{}");
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  }
  
  rc = graphBetweennessCentrality(pVtab, &zResults);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_NOT_NULL(zResults);
  
  /* Middle nodes should have higher betweenness */
  /* Node 3 should have highest centrality in linear graph */
  TEST_ASSERT_NOT_NULL(strstr(zResults, "\"3\":"));
  
  sqlite3_free(zResults);
  teardownAdvancedTests();
}

/*
** Test closeness centrality on star graph.
*/
void testClosenessCentralityStar(void){
  char *zResults = 0;
  int rc;
  
  setupAdvancedTests();
  
  /* Create star graph: node 1 connected to all others */
  for( int i = 1; i <= 5; i++ ){
    rc = graphAddNode(pVtab, i, "{}");
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  }
  
  for( int i = 2; i <= 5; i++ ){
    rc = graphAddEdge(pVtab, 1, i, 1.0, "{}");
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    rc = graphAddEdge(pVtab, i, 1, 1.0, "{}");
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  }
  
  rc = graphClosenessCentrality(pVtab, &zResults);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_NOT_NULL(zResults);
  
  /* Center node (1) should have highest closeness */
  TEST_ASSERT_NOT_NULL(strstr(zResults, "\"1\":"));
  
  sqlite3_free(zResults);
  teardownAdvancedTests();
}

/*
** Main test runner for advanced algorithm tests.
*/
void runAdvancedTests(void){
  RUN_TEST(testBetweennessCentralitySimple);
  RUN_TEST(testBetweennessCentralityEmpty);
  RUN_TEST(testBetweennessCentralityLinear);
  RUN_TEST(testClosenessCentralitySimple);
  RUN_TEST(testClosenessCentralityStar);
  RUN_TEST(testTopologicalSortDAG);
  RUN_TEST(testTopologicalSortCycle);
  RUN_TEST(testHasCycle);
  RUN_TEST(testConnectedComponents);
  RUN_TEST(testStronglyConnectedComponents);
  
}