/*
** SQLite Graph Database Extension - Schema Management Tests
**
** Comprehensive tests for label support, relationship types, indexing,
** and schema discovery functionality. These tests verify the Phase 5
** implementation of enhanced graph schema capabilities.
*/

#define SQLITE_CORE
#include "unity.h"
#include "sqlite3.h"
#include "../include/graph.h"
#include "../include/graph-vtab.h"
#include <string.h>
#include <stdlib.h>

/* Test database pointer */
static sqlite3 *pDb = 0;
static GraphVtab *pVtab = 0;


/*
** Setup test environment for schema tests.
*/
void setupSchemaTests(void) {
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
  pVtab->zTableName = sqlite3_mprintf("schema_test_graph");
  pVtab->nRef = 1;
}

/*
** Cleanup test environment.
*/
void teardownSchemaTests(void) {
  if( pVtab ) {
    char *zSql = sqlite3_mprintf("DROP TABLE IF EXISTS %s_nodes; DROP TABLE IF EXISTS %s_edges;", pVtab->zTableName, pVtab->zTableName);
    sqlite3_exec(pDb, zSql, 0, 0, 0);
    sqlite3_free(zSql);
    
    /* Free schema structures */
    if( pVtab->pSchema ) {
      graphDestroySchema(pVtab->pSchema);
    }
    
    if( pVtab->pLabelIndex ) {
      sqlite3_free(pVtab->pLabelIndex->zIndexName);
      sqlite3_free(pVtab->pLabelIndex->apNodes);
      sqlite3_free(pVtab->pLabelIndex);
    }
    
    sqlite3_free(pVtab->zDbName);
    sqlite3_free(pVtab->zTableName);
    sqlite3_free(pVtab);
    pVtab = 0;
  }
  
  if( pDb ) {
    sqlite3_close(pDb);
    pDb = 0;
  }
}

/*
** Test basic schema initialization.
*/
void testSchemaInitialization(void) {
  int rc;
  
  setupSchemaTests();
  
  /* Schema should be NULL initially */
  TEST_ASSERT_NULL(pVtab->pSchema);
  
  /* Initialize schema */
  rc = graphInitSchema(pVtab);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_NOT_NULL(pVtab->pSchema);
  
  /* Should have empty arrays */
  TEST_ASSERT_EQUAL(0, pVtab->pSchema->nNodeLabels);
  TEST_ASSERT_EQUAL(0, pVtab->pSchema->nRelTypes);
  TEST_ASSERT_NOT_NULL(pVtab->pSchema->azNodeLabels);
  TEST_ASSERT_NOT_NULL(pVtab->pSchema->azRelTypes);
  
  teardownSchemaTests();
}

/*
** Test label registration and retrieval.
*/
void testLabelRegistration(void) {
  int rc;
  
  setupSchemaTests();
  rc = graphInitSchema(pVtab);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  /* Register some labels */
  rc = graphRegisterLabel(pVtab->pSchema, "Person");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_EQUAL(1, pVtab->pSchema->nNodeLabels);
  
  rc = graphRegisterLabel(pVtab->pSchema, "Company");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_EQUAL(2, pVtab->pSchema->nNodeLabels);
  
  /* Test duplicate registration */
  rc = graphRegisterLabel(pVtab->pSchema, "Person");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_EQUAL(2, pVtab->pSchema->nNodeLabels); /* Should not increase */
  
  /* Verify labels */
  TEST_ASSERT_EQUAL_STRING("Person", pVtab->pSchema->azNodeLabels[0]);
  TEST_ASSERT_EQUAL_STRING("Company", pVtab->pSchema->azNodeLabels[1]);
  
  teardownSchemaTests();
}

/*
** Test relationship type registration.
*/
void testRelationshipTypeRegistration(void) {
  int rc;
  
  setupSchemaTests();
  rc = graphInitSchema(pVtab);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  /* Register relationship types */
  rc = graphRegisterRelationshipType(pVtab->pSchema, "KNOWS");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_EQUAL(1, pVtab->pSchema->nRelTypes);
  
  rc = graphRegisterRelationshipType(pVtab->pSchema, "WORKS_AT");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_EQUAL(2, pVtab->pSchema->nRelTypes);
  
  /* Test duplicate registration */
  rc = graphRegisterRelationshipType(pVtab->pSchema, "KNOWS");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_EQUAL(2, pVtab->pSchema->nRelTypes); /* Should not increase */
  
  /* Verify types */
  TEST_ASSERT_EQUAL_STRING("KNOWS", pVtab->pSchema->azRelTypes[0]);
  TEST_ASSERT_EQUAL_STRING("WORKS_AT", pVtab->pSchema->azRelTypes[1]);
  
  teardownSchemaTests();
}

/*
** Test adding nodes with labels.
*/
void testAddNodeWithLabels(void) {
  int rc;
  const char *labels[] = {"Person", "Employee"};
  setupSchemaTests();
  
  
  /* Add node with labels */
  rc = graphAddNodeWithLabels(pVtab, 1, labels, 2, "{\"name\": \"Alice\"}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_EQUAL(1, graphCountNodes(pVtab));
  
  /* Verify labels were registered in schema */
  TEST_ASSERT_NOT_NULL(pVtab->pSchema);
  TEST_ASSERT_EQUAL(2, pVtab->pSchema->nNodeLabels);
  
  /* Verify node has labels */
  GraphNode *pNode = graphFindNode(pVtab, 1);
  TEST_ASSERT_NOT_NULL(pNode);
  TEST_ASSERT_EQUAL(2, pNode->nLabels);
  TEST_ASSERT_EQUAL_STRING("Person", pNode->azLabels[0]);
  TEST_ASSERT_EQUAL_STRING("Employee", pNode->azLabels[1]);
  
}

/*
** Test adding edges with relationship types.
*/
void testAddEdgeWithType(void) {
  int rc;
  const char *labels[] = {"Person"};
  setupSchemaTests();
  
  
  /* Add two nodes first */
  rc = graphAddNodeWithLabels(pVtab, 1, labels, 1, "{\"name\": \"Alice\"}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  rc = graphAddNodeWithLabels(pVtab, 2, labels, 1, "{\"name\": \"Bob\"}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  /* Add edge with relationship type */
  rc = graphAddEdgeWithType(pVtab, 1, 2, "KNOWS", 1.0, "{\"since\": 2020}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_EQUAL(1, graphCountEdges(pVtab));
  
  /* Verify relationship type was registered */
  TEST_ASSERT_NOT_NULL(pVtab->pSchema);
  TEST_ASSERT_EQUAL(1, pVtab->pSchema->nRelTypes);
  TEST_ASSERT_EQUAL_STRING("KNOWS", pVtab->pSchema->azRelTypes[0]);
  
  /* Verify edge has type */
  /* No direct access to pEdgeList */
  char *zType = NULL;
  double rWeight;
  rc = graphGetEdge(pVtab, 1, 2, &rWeight, &zType);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_NOT_NULL(zType);
  TEST_ASSERT_EQUAL_STRING("KNOWS", zType);
  sqlite3_free(zType);
  
}

/*
** Test label indexing functionality.
*/
void testLabelIndexing(void) {
  int rc;
  const char *personLabels[] = {"Person"};
  setupSchemaTests();
  const char *companyLabels[] = {"Company"};
  
  
  /* Add some nodes with different labels */
  rc = graphAddNodeWithLabels(pVtab, 1, personLabels, 1, "{\"name\": \"Alice\"}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  rc = graphAddNodeWithLabels(pVtab, 2, personLabels, 1, "{\"name\": \"Bob\"}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  rc = graphAddNodeWithLabels(pVtab, 3, companyLabels, 1, "{\"name\": \"ACME\"}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  /* Create label index */
  rc = graphCreateLabelIndex(pVtab, NULL);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_NOT_NULL(pVtab->pLabelIndex);
  
  /* Test finding nodes by label */
  GraphNode *pPersonNode = graphFindNodesByLabel(pVtab, "Person");
  TEST_ASSERT_NOT_NULL(pPersonNode);
  TEST_ASSERT_TRUE(pPersonNode->iNodeId == 1 || pPersonNode->iNodeId == 2);
  
  GraphNode *pCompanyNode = graphFindNodesByLabel(pVtab, "Company");
  TEST_ASSERT_NOT_NULL(pCompanyNode);
  TEST_ASSERT_EQUAL(3, pCompanyNode->iNodeId);
  
  /* Test non-existent label */
  GraphNode *pNonExistent = graphFindNodesByLabel(pVtab, "NonExistent");
  TEST_ASSERT_NULL(pNonExistent);
  
}

/*
** Test label management operations.
*/
void testLabelManagement(void) {
  int rc;
  const char *initialLabels[] = {"Person"};
  setupSchemaTests();
  char *zLabels = NULL;
  
  
  /* Add node with initial label */
  rc = graphAddNodeWithLabels(pVtab, 1, initialLabels, 1, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  /* Add another label */
  rc = graphAddNodeLabel(pVtab, 1, "Employee");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  /* Check labels */
  rc = graphGetNodeLabels(pVtab, 1, &zLabels);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_NOT_NULL(zLabels);
  TEST_ASSERT_NOT_NULL(strstr(zLabels, "Person"));
  TEST_ASSERT_NOT_NULL(strstr(zLabels, "Employee"));
  sqlite3_free(zLabels);
  
  /* Test hasLabel */
  TEST_ASSERT_EQUAL(1, graphNodeHasLabel(pVtab, 1, "Person"));
  TEST_ASSERT_EQUAL(1, graphNodeHasLabel(pVtab, 1, "Employee"));
  TEST_ASSERT_EQUAL(0, graphNodeHasLabel(pVtab, 1, "Manager"));
  
  /* Remove a label */
  rc = graphRemoveNodeLabel(pVtab, 1, "Person");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_EQUAL(0, graphNodeHasLabel(pVtab, 1, "Person"));
  TEST_ASSERT_EQUAL(1, graphNodeHasLabel(pVtab, 1, "Employee"));
  
}

/*
** Test schema discovery functionality.
*/
void testSchemaDiscovery(void) {
  int rc;
  const char *personLabels[] = {"Person", "Employee"};
  setupSchemaTests();
  const char *companyLabels[] = {"Company"};
  char *zSchemaInfo = NULL;
  
  
  /* Add some nodes and edges */
  rc = graphAddNodeWithLabels(pVtab, 1, personLabels, 2, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  rc = graphAddNodeWithLabels(pVtab, 2, companyLabels, 1, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  rc = graphAddEdgeWithType(pVtab, 1, 2, "WORKS_AT", 1.0, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  /* Test schema discovery */
  rc = graphDiscoverSchema(pVtab);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  /* Verify discovered schema */
  TEST_ASSERT_EQUAL(3, pVtab->pSchema->nNodeLabels); /* Person, Employee, Company */
  TEST_ASSERT_EQUAL(1, pVtab->pSchema->nRelTypes);   /* WORKS_AT */
  
  /* Test schema info JSON */
  rc = graphGetSchemaInfo(pVtab, &zSchemaInfo);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_NOT_NULL(zSchemaInfo);
  
  /* Check JSON contains expected elements */
  TEST_ASSERT_NOT_NULL(strstr(zSchemaInfo, "node_labels"));
  TEST_ASSERT_NOT_NULL(strstr(zSchemaInfo, "relationship_types"));
  TEST_ASSERT_NOT_NULL(strstr(zSchemaInfo, "Person"));
  TEST_ASSERT_NOT_NULL(strstr(zSchemaInfo, "WORKS_AT"));
  TEST_ASSERT_NOT_NULL(strstr(zSchemaInfo, "node_count"));
  
  sqlite3_free(zSchemaInfo);
}

/*
** Test validation functions.
*/
void testValidation(void) {
  int rc;
  const char *validLabels[] = {"Person", "Employee"};
  setupSchemaTests();
  const char *invalidLabels[] = {"Person", "Person"}; /* Duplicate */
  const char *emptyLabel[] = {""};
  
  rc = graphInitSchema(pVtab);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  /* Test valid labels */
  rc = graphValidateNodeLabels(pVtab, validLabels, 2);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  /* Test invalid labels (duplicates) */
  rc = graphValidateNodeLabels(pVtab, invalidLabels, 2);
  TEST_ASSERT_EQUAL(SQLITE_CONSTRAINT, rc);
  
  /* Test empty label */
  rc = graphValidateNodeLabels(pVtab, emptyLabel, 1);
  TEST_ASSERT_EQUAL(SQLITE_CONSTRAINT, rc);
  
  /* Test valid relationship type */
  rc = graphValidateRelationshipType(pVtab, "KNOWS");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  /* Test NULL relationship type (should be valid) */
  rc = graphValidateRelationshipType(pVtab, NULL);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  /* Test empty relationship type */
  rc = graphValidateRelationshipType(pVtab, "");
  TEST_ASSERT_EQUAL(SQLITE_CONSTRAINT, rc);
  
}

/*
** Test index rebuilding.
*/
void testIndexRebuilding(void) {
  int rc;
  const char *labels[] = {"Person"};
  setupSchemaTests();
  
  
  /* Add nodes */
  rc = graphAddNodeWithLabels(pVtab, 1, labels, 1, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  rc = graphAddNodeWithLabels(pVtab, 2, labels, 1, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  /* Create index */
  rc = graphCreateLabelIndex(pVtab, NULL);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  /* Verify index works */
  GraphNode *pNode = graphFindNodesByLabel(pVtab, "Person");
  TEST_ASSERT_NOT_NULL(pNode);
  
  /* Add more nodes */
  rc = graphAddNodeWithLabels(pVtab, 3, labels, 1, "{}");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  /* Rebuild index */
  rc = graphRebuildIndexes(pVtab);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  /* Verify index still works */
  pNode = graphFindNodesByLabel(pVtab, "Person");
  TEST_ASSERT_NOT_NULL(pNode);
  
}

/*
** Main test runner for schema tests.
*/
void runSchemaTests(void) {
  RUN_TEST(testSchemaInitialization);
  RUN_TEST(testLabelRegistration);
  RUN_TEST(testRelationshipTypeRegistration);
  RUN_TEST(testAddNodeWithLabels);
  RUN_TEST(testAddEdgeWithType);
  RUN_TEST(testLabelIndexing);
  RUN_TEST(testLabelManagement);
  RUN_TEST(testSchemaDiscovery);
  RUN_TEST(testValidation);
  RUN_TEST(testIndexRebuilding);
}