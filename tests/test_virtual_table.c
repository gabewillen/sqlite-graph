/*
** SQLite Graph Database Extension - Virtual Table Tests
**
** Tests virtual table creation, iteration, and SQL integration.
** Validates sqlite3_vtab interface implementation.
*/

#define SQLITE_CORE
#include "unity.h"
#include "sqlite3.h"

extern sqlite3 *g_pTestDb;

void testGraphVirtualTableCreate(void){
  int rc;
  sqlite3_stmt *pStmt;
  
  rc = sqlite3_prepare_v2(g_pTestDb, 
    "CREATE VIRTUAL TABLE test_graph USING graph()", -1, &pStmt, 0);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  rc = sqlite3_step(pStmt);
  TEST_ASSERT_EQUAL(SQLITE_DONE, rc);
  
  sqlite3_finalize(pStmt);
}

void testGraphVirtualTableIteration(void){
  /* Create table first */
  testGraphVirtualTableCreate();
  
  /* Test empty table iteration */
  int rc;
  sqlite3_stmt *pStmt;
  
  rc = sqlite3_prepare_v2(g_pTestDb, "SELECT * FROM test_graph", -1, &pStmt, 0);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  /* Should have no rows initially */
  rc = sqlite3_step(pStmt);
  TEST_ASSERT_EQUAL(SQLITE_DONE, rc);
  
  sqlite3_finalize(pStmt);
}