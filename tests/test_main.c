/*
** SQLite Graph Database Extension - Unity Test Runner
**
** This file implements the main test runner using Unity framework.
** Follows SQLite testing patterns with comprehensive coverage.
**
** Test categories: Unit tests, integration tests, performance tests
** Coverage target: >=90% line coverage, >=85% branch coverage
*/

#define SQLITE_CORE
#include "unity.h"
#include "sqlite3.h"


/* Forward declaration of our extension init function */
int sqlite3_graph_init(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi);

/* Test database for all tests */
sqlite3 *g_pTestDb = 0;

/*
** Set up test database.
** Creates in-memory database and loads graph extension.
** Called before each test to ensure clean state.
*/
sqlite3 *setupTestDb(void){
  sqlite3 *pDb;
  int rc;
  
  /* Create in-memory database */
  rc = sqlite3_open(":memory:", &pDb);
  if( rc!=SQLITE_OK ){
    return 0;
  }
  
  /* Enable extension loading */
  rc = sqlite3_db_config(pDb, SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION, 1, 0);
  if( rc!=SQLITE_OK ){
    sqlite3_close(pDb);
    return 0;
  }
  
  /* Initialize graph extension */
  rc = sqlite3_graph_init(pDb, NULL, NULL);
  if( rc!=SQLITE_OK ){
    sqlite3_close(pDb);
    return 0;
  }
  
  return pDb;
}

/*
** Clean up test database.
** Closes database connection and frees resources.
** Called after each test to prevent resource leaks.
*/
void cleanupTestDb(sqlite3 *pDb){
  if( pDb ){
    /* Finalize any remaining prepared statements */
    sqlite3_stmt *pStmt;
    while( (pStmt = sqlite3_next_stmt(pDb, NULL)) != NULL ){
      sqlite3_finalize(pStmt);
    }
    
    /* Use sqlite3_close_v2 to handle prepared statements properly */
    int rc = sqlite3_close_v2(pDb);
    if( rc != SQLITE_OK ){
      /* Force close even if there are outstanding prepared statements */
      sqlite3_close(pDb);
    }
  }
}

/*
** Unity framework setup function.
** Called before each individual test case.
*/
void setUp(void){
  g_pTestDb = setupTestDb();
  TEST_ASSERT_NOT_NULL(g_pTestDb);
}

/*
** Unity framework teardown function.
** Called after each individual test case.
*/
void tearDown(void){
  cleanupTestDb(g_pTestDb);
  g_pTestDb = 0;
}

/*
** Forward declarations for test functions.
** Organized by test category for clear structure.
*/

/* Storage function tests */
void testGraphAddNodeSuccess(void);
void testGraphAddNodeDuplicateId(void);
void testGraphAddNodeOOM(void);
void testGraphRemoveNodeSuccess(void);
void testGraphRemoveNodeNotFound(void);
void testGraphAddEdgeSuccess(void);
void testGraphAddEdgeInvalidNodes(void);

/* Virtual table tests */
void testGraphVirtualTableCreate(void);
void testGraphVirtualTableIteration(void);

/* Integration tests */
void testGraphBasicWorkflow(void){
  int rc;
  sqlite3_stmt *pStmt;
  
  /* Create virtual table */
  rc = sqlite3_prepare_v2(g_pTestDb, 
    "CREATE VIRTUAL TABLE advanced_graph USING graph()", -1, &pStmt, 0);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  rc = sqlite3_step(pStmt);
  TEST_ASSERT_EQUAL(SQLITE_DONE, rc);
  sqlite3_finalize(pStmt);

  /* Add 100 nodes */
  for (int i = 1; i <= 100; i++) {
    char zSql[128];
    sqlite3_snprintf(sizeof(zSql), zSql, "INSERT INTO advanced_graph_nodes (id, properties) VALUES (%d, '{}')", i);
    rc = sqlite3_exec(g_pTestDb, zSql, 0, 0, 0);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  }
  
  /* Test basic SQL functions by querying backing tables */
  rc = sqlite3_prepare_v2(g_pTestDb, "SELECT count(*) FROM advanced_graph_nodes", -1, &pStmt, 0);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  rc = sqlite3_step(pStmt);
  TEST_ASSERT_EQUAL(SQLITE_ROW, rc);
  
  int nNodes = sqlite3_column_int(pStmt, 0);
  TEST_ASSERT_EQUAL(100, nNodes);
  
  sqlite3_finalize(pStmt);
}

/* Traversal tests - declared in test_traversal.c */
extern void runTraversalTests(void);

/* Algorithm tests - declared in test_algorithms.c */
extern void runAlgorithmTests(void);

/* Advanced algorithm tests - declared in test_advanced.c */
extern void runAdvancedTests(void);

/* Schema tests - declared in test_schema.c */
extern void runSchemaTests(void);

/* Cypher parser tests - declared in test_cypher_parser.c */
extern void runCypherParserTests(void);

/* Cypher planner tests - declared in test_cypher_planner.c */
extern void runCypherPlannerTests(void);

/* Cypher executor tests - declared in test_cypher_executor.c */
extern void runCypherExecutorTests(void);

/*
** Main test runner.
** Executes all test cases and reports results.
** Returns 0 on success, non-zero on test failures.
*/
int main(void){
  UNITY_BEGIN();
  
  /* Storage function tests */
  RUN_TEST(testGraphAddNodeSuccess);
  RUN_TEST(testGraphAddNodeDuplicateId);
  RUN_TEST(testGraphRemoveNodeSuccess);
  RUN_TEST(testGraphRemoveNodeNotFound);
  RUN_TEST(testGraphAddEdgeSuccess);
  RUN_TEST(testGraphAddEdgeInvalidNodes);
  
  /* Virtual table tests */
  RUN_TEST(testGraphVirtualTableCreate);
  RUN_TEST(testGraphVirtualTableIteration);
  
  /* Integration tests */
  RUN_TEST(testGraphBasicWorkflow);
  
  /* Traversal algorithm tests */
  runTraversalTests();
  
  /* Graph algorithm tests */
  runAlgorithmTests();
  
  /* Advanced algorithm tests */
  runAdvancedTests();
  
  /* Schema and indexing tests */
  runSchemaTests();
  
  /* Cypher parser tests */
  runCypherParserTests();
  
  /* Cypher planner tests */
  runCypherPlannerTests();
  
  /* Cypher executor tests */
  runCypherExecutorTests();
  
  return UNITY_END();
}