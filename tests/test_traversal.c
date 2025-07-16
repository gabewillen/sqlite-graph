/*
** SQLite Graph Database Extension - Traversal Tests
**
** Comprehensive tests for DFS and BFS traversal algorithms.
** Tests include simple paths, cycles, depth limits, and edge cases.
*/

#define SQLITE_CORE
#include "unity.h"
#include <string.h>
#include "sqlite3.h"
#include "../include/graph.h"
#include "../include/graph-vtab.h"

/* Test database pointer */
static sqlite3 *pDb = 0;
static GraphVtab *pVtab = 0;


/*
** Setup test database and graph virtual table.
** Reuses the global test database instead of creating a new one.
*/
void setupTraversalTests(void){
  /* Use the global test database from Unity setUp */
  extern sqlite3 *g_pTestDb;
  pDb = g_pTestDb;
  
  /* TODO: Create graph virtual table instance */
  /* For now, allocate vtab structure manually for testing */
  pVtab = (GraphVtab*)sqlite3_malloc(sizeof(GraphVtab));
  TEST_ASSERT_NOT_NULL(pVtab);
  
  memset(pVtab, 0, sizeof(GraphVtab));
  pVtab->pDb = pDb;
  pVtab->zDbName = sqlite3_mprintf("main");
  pVtab->zTableName = sqlite3_mprintf("test_graph");
}

/*
** Cleanup test database.
** Does not close the database since we're reusing the global test database.
*/
void teardownTraversalTests(void){
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
** Helper function to create a simple linear graph: 1 -> 2 -> 3 -> 4
*/
static void createLinearGraph(void){
  int rc;
  
  /* Add nodes */
  rc = graphAddNode(pVtab, 1, "{\"name\":\"Node1\"}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  rc = graphAddNode(pVtab, 2, "{\"name\":\"Node2\"}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  rc = graphAddNode(pVtab, 3, "{\"name\":\"Node3\"}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  rc = graphAddNode(pVtab, 4, "{\"name\":\"Node4\"}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  /* Add edges */
  rc = graphAddEdge(pVtab, 1, 2, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  rc = graphAddEdge(pVtab, 2, 3, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  rc = graphAddEdge(pVtab, 3, 4, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
}

/*
** Helper function to create a cyclic graph: 1 -> 2 -> 3 -> 1
*/
static void createCyclicGraph(void){
  int rc;
  
  /* Add nodes */
  rc = graphAddNode(pVtab, 1, "{\"name\":\"Node1\"}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  rc = graphAddNode(pVtab, 2, "{\"name\":\"Node2\"}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  rc = graphAddNode(pVtab, 3, "{\"name\":\"Node3\"}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  /* Add edges forming a cycle */
  rc = graphAddEdge(pVtab, 1, 2, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  rc = graphAddEdge(pVtab, 2, 3, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  rc = graphAddEdge(pVtab, 3, 1, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
}

/*
** Helper function to create a tree-like graph:
**      1
**     / \
**    2   3
**   / \   \
**  4   5   6
*/
static void createTreeGraph(void){
  int rc;
  
  /* Add nodes */
  for( int i=1; i<=6; i++ ){
    char *zProp = sqlite3_mprintf("{\"name\":\"Node%d\"}", i);
    rc = graphAddNode(pVtab, i, zProp);
    sqlite3_free(zProp);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  }
  
  /* Add edges */
  rc = graphAddEdge(pVtab, 1, 2, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  rc = graphAddEdge(pVtab, 1, 3, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  rc = graphAddEdge(pVtab, 2, 4, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  rc = graphAddEdge(pVtab, 2, 5, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  rc = graphAddEdge(pVtab, 3, 6, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
}

/*
** Test DFS on simple linear graph.
*/
void testGraphDFSLinear(void){
  char *zPath = 0;
  int rc;
  
  setupTraversalTests();
  createLinearGraph();
  
  /* DFS from node 1 */
  rc = graphDFS(pVtab, 1, -1, &zPath);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_NOT_NULL(zPath);
  TEST_ASSERT_EQUAL_STRING("[1,2,3,4]", zPath);
  
  sqlite3_free(zPath);
  teardownTraversalTests();
}

/*
** Test DFS with non-existent start node.
*/
void testGraphDFSNotFound(void){
  char *zPath = 0;
  int rc;
  
  setupTraversalTests();
  createLinearGraph();
  
  /* DFS from non-existent node */
  rc = graphDFS(pVtab, 99, -1, &zPath);
  TEST_ASSERT_EQUAL(SQLITE_NOTFOUND, rc);
  TEST_ASSERT_NULL(zPath);
  
  teardownTraversalTests();
}

/*
** Test DFS with depth limit.
*/
void testGraphDFSDepthLimit(void){
  setupTraversalTests();
  char *zPath = 0;
  int rc;
  
  createLinearGraph();
  
  /* DFS with depth limit 2 */
  rc = graphDFS(pVtab, 1, 2, &zPath);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_NOT_NULL(zPath);
  TEST_ASSERT_EQUAL_STRING("[1,2]", zPath);
  
  sqlite3_free(zPath);
  teardownTraversalTests();
}

/*
** Test DFS on cyclic graph (cycle detection).
*/
void testGraphDFSCycleDetection(void){
  setupTraversalTests();
  char *zPath = 0;
  int rc;
  
  createCyclicGraph();
  
  /* DFS should handle cycle gracefully */
  rc = graphDFS(pVtab, 1, -1, &zPath);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_NOT_NULL(zPath);
  /* Should visit each node exactly once */
  TEST_ASSERT_EQUAL_STRING("[1,2,3]", zPath);
  
  sqlite3_free(zPath);
  teardownTraversalTests();
}

/*
** Test DFS on tree graph.
*/
void testGraphDFSTree(void){
  setupTraversalTests();
  char *zPath = 0;
  int rc;
  
  createTreeGraph();
  
  /* DFS from root */
  rc = graphDFS(pVtab, 1, -1, &zPath);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_NOT_NULL(zPath);
  /* DFS should explore left subtree first */
  TEST_ASSERT_EQUAL_STRING("[1,2,4,5,3,6]", zPath);
  
  sqlite3_free(zPath);
  teardownTraversalTests();
}

/*
** Test BFS on simple linear graph.
*/
void testGraphBFSLinear(void){
  setupTraversalTests();
  char *zPath = 0;
  int rc;
  
  createLinearGraph();
  
  /* BFS from node 1 */
  rc = graphBFS(pVtab, 1, -1, &zPath);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_NOT_NULL(zPath);
  TEST_ASSERT_EQUAL_STRING("[1,2,3,4]", zPath);
  
  sqlite3_free(zPath);
  teardownTraversalTests();
}

/*
** Test BFS with non-existent start node.
*/
void testGraphBFSNotFound(void){
  setupTraversalTests();
  char *zPath = 0;
  int rc;
  
  createLinearGraph();
  
  /* BFS from non-existent node */
  rc = graphBFS(pVtab, 99, -1, &zPath);
  TEST_ASSERT_EQUAL(SQLITE_NOTFOUND, rc);
  TEST_ASSERT_NULL(zPath);
  
  teardownTraversalTests();
}

/*
** Test BFS with depth limit.
*/
void testGraphBFSDepthLimit(void){
  setupTraversalTests();
  char *zPath = 0;
  int rc;
  
  createLinearGraph();
  
  /* BFS with depth limit 2 */
  rc = graphBFS(pVtab, 1, 2, &zPath);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_NOT_NULL(zPath);
  TEST_ASSERT_EQUAL_STRING("[1,2]", zPath);
  
  sqlite3_free(zPath);
  teardownTraversalTests();
}

/*
** Test BFS on cyclic graph.
*/
void testGraphBFSCycleDetection(void){
  setupTraversalTests();
  char *zPath = 0;
  int rc;
  
  createCyclicGraph();
  
  /* BFS should handle cycle gracefully */
  rc = graphBFS(pVtab, 1, -1, &zPath);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_NOT_NULL(zPath);
  /* Should visit each node exactly once */
  TEST_ASSERT_EQUAL_STRING("[1,2,3]", zPath);
  
  sqlite3_free(zPath);
  teardownTraversalTests();
}

/*
** Test BFS on tree graph.
*/
void testGraphBFSTree(void){
  setupTraversalTests();
  char *zPath = 0;
  int rc;
  
  createTreeGraph();
  
  /* BFS from root */
  rc = graphBFS(pVtab, 1, -1, &zPath);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_NOT_NULL(zPath);
  /* BFS should explore level by level */
  TEST_ASSERT_EQUAL_STRING("[1,2,3,4,5,6]", zPath);
  
  sqlite3_free(zPath);
  teardownTraversalTests();
}

/*
** Test DFS on empty graph.
*/
void testGraphDFSEmpty(void){
  setupTraversalTests();
  char *zPath = 0;
  int rc;
  
  /* No nodes added */
  
  /* DFS should fail with NOTFOUND */
  rc = graphDFS(pVtab, 1, -1, &zPath);
  TEST_ASSERT_EQUAL(SQLITE_NOTFOUND, rc);
  TEST_ASSERT_NULL(zPath);
  
  teardownTraversalTests();
}

/*
** Test BFS on empty graph.
*/
void testGraphBFSEmpty(void){
  setupTraversalTests();
  char *zPath = 0;
  int rc;
  
  /* No nodes added */
  
  /* BFS should fail with NOTFOUND */
  rc = graphBFS(pVtab, 1, -1, &zPath);
  TEST_ASSERT_EQUAL(SQLITE_NOTFOUND, rc);
  TEST_ASSERT_NULL(zPath);
  
  teardownTraversalTests();
}

/*
** Test DFS on single node.
*/
void testGraphDFSSingleNode(void){
  setupTraversalTests();
  char *zPath = 0;
  int rc;
  
  
  /* Add single node */
  rc = graphAddNode(pVtab, 42, "{\"name\":\"lonely\"}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  /* DFS should return just that node */
  rc = graphDFS(pVtab, 42, -1, &zPath);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_NOT_NULL(zPath);
  TEST_ASSERT_EQUAL_STRING("[42]", zPath);
  
  sqlite3_free(zPath);
  teardownTraversalTests();
}

/*
** Test BFS on single node.
*/
void testGraphBFSSingleNode(void){
  setupTraversalTests();
  char *zPath = 0;
  int rc;
  
  
  /* Add single node */
  rc = graphAddNode(pVtab, 42, "{\"name\":\"lonely\"}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  /* BFS should return just that node */
  rc = graphBFS(pVtab, 42, -1, &zPath);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_NOT_NULL(zPath);
  TEST_ASSERT_EQUAL_STRING("[42]", zPath);
  
  sqlite3_free(zPath);
  teardownTraversalTests();
}

/*
** Test DFS on disconnected graph.
*/
void testGraphDFSDisconnected(void){
  setupTraversalTests();
  char *zPath = 0;
  int rc;
  
  
  /* Create two disconnected components */
  rc = graphAddNode(pVtab, 1, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  rc = graphAddNode(pVtab, 2, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  rc = graphAddEdge(pVtab, 1, 2, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  rc = graphAddNode(pVtab, 3, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  rc = graphAddNode(pVtab, 4, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  rc = graphAddEdge(pVtab, 3, 4, 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  /* DFS from node 1 should only reach nodes 1 and 2 */
  rc = graphDFS(pVtab, 1, -1, &zPath);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_NOT_NULL(zPath);
  TEST_ASSERT_EQUAL_STRING("[1,2]", zPath);
  
  sqlite3_free(zPath);
  teardownTraversalTests();
}

/*
** Main test runner for traversal tests.
*/
void runTraversalTests(void){
  RUN_TEST(testGraphDFSLinear);
  RUN_TEST(testGraphDFSNotFound);
  RUN_TEST(testGraphDFSDepthLimit);
  RUN_TEST(testGraphDFSCycleDetection);
  RUN_TEST(testGraphDFSTree);
  RUN_TEST(testGraphBFSLinear);
  RUN_TEST(testGraphBFSNotFound);
  RUN_TEST(testGraphBFSDepthLimit);
  RUN_TEST(testGraphBFSCycleDetection);
  RUN_TEST(testGraphBFSTree);
  RUN_TEST(testGraphDFSEmpty);
  RUN_TEST(testGraphBFSEmpty);
  RUN_TEST(testGraphDFSSingleNode);
  RUN_TEST(testGraphBFSSingleNode);
  RUN_TEST(testGraphDFSDisconnected);
}