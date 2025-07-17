/*
** SQLite Graph Database Extension - Cypher Planner Tests
**
** Comprehensive tests for the Cypher query planner functionality.
** These tests verify logical plan compilation, physical plan optimization,
** cost estimation, and SQL function interfaces.
*/

#define SQLITE_CORE
#include "unity.h"
#include "sqlite3.h"
#include "cypher-planner.h"
#include <string.h>
#include <stdlib.h>

/*
** Test logical plan node creation and management.
*/
void testLogicalPlanNodeBasic(void) {
  LogicalPlanNode *pNode, *pChild1, *pChild2;
  int rc;
  
  /* Test node creation */
  pNode = logicalPlanNodeCreate(LOGICAL_LABEL_SCAN);
  TEST_ASSERT_NOT_NULL(pNode);
  TEST_ASSERT_EQUAL(LOGICAL_LABEL_SCAN, pNode->type);
  TEST_ASSERT_EQUAL(0, pNode->nChildren);
  TEST_ASSERT_NULL(pNode->zAlias);
  
  /* Test setting properties */
  rc = logicalPlanNodeSetAlias(pNode, "n");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_EQUAL_STRING("n", pNode->zAlias);
  
  rc = logicalPlanNodeSetLabel(pNode, "Person");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_EQUAL_STRING("Person", pNode->zLabel);
  
  /* Test adding children */
  pChild1 = logicalPlanNodeCreate(LOGICAL_FILTER);
  pChild2 = logicalPlanNodeCreate(LOGICAL_PROJECTION);
  TEST_ASSERT_NOT_NULL(pChild1);
  TEST_ASSERT_NOT_NULL(pChild2);
  
  rc = logicalPlanNodeAddChild(pNode, pChild1);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_EQUAL(1, pNode->nChildren);
  TEST_ASSERT_EQUAL(pChild1, pNode->apChildren[0]);
  TEST_ASSERT_EQUAL(pNode, pChild1->pParent);
  
  rc = logicalPlanNodeAddChild(pNode, pChild2);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_EQUAL(2, pNode->nChildren);
  TEST_ASSERT_EQUAL(pChild2, pNode->apChildren[1]);
  
  logicalPlanNodeDestroy(pNode);
}

/*
** Test logical plan node type names.
*/
void testLogicalPlanNodeTypeNames(void) {
  TEST_ASSERT_EQUAL_STRING("NODE_SCAN", logicalPlanNodeTypeName(LOGICAL_NODE_SCAN));
  TEST_ASSERT_EQUAL_STRING("LABEL_SCAN", logicalPlanNodeTypeName(LOGICAL_LABEL_SCAN));
  TEST_ASSERT_EQUAL_STRING("FILTER", logicalPlanNodeTypeName(LOGICAL_FILTER));
  TEST_ASSERT_EQUAL_STRING("PROJECTION", logicalPlanNodeTypeName(LOGICAL_PROJECTION));
  TEST_ASSERT_EQUAL_STRING("HASH_JOIN", logicalPlanNodeTypeName(LOGICAL_HASH_JOIN));
  TEST_ASSERT_EQUAL_STRING("UNKNOWN", logicalPlanNodeTypeName((LogicalPlanNodeType)999));
}

/*
** Test cost estimation for logical plan nodes.
*/
void testLogicalPlanCostEstimation(void) {
  LogicalPlanNode *pScan, *pFilter, *pJoin;
  PlanContext context;
  double rCost;
  sqlite3_int64 iRows;
  
  memset(&context, 0, sizeof(context));
  
  /* Test scan costs */
  pScan = logicalPlanNodeCreate(LOGICAL_NODE_SCAN);
  rCost = logicalPlanEstimateCost(pScan, &context);
  TEST_ASSERT_TRUE(rCost > 100.0); /* Full scan should be expensive */
  
  iRows = logicalPlanEstimateRows(pScan, &context);
  TEST_ASSERT_TRUE(iRows > 1000); /* Should estimate reasonable graph size */
  
  /* Test filter costs */
  pFilter = logicalPlanNodeCreate(LOGICAL_FILTER);
  logicalPlanNodeAddChild(pFilter, pScan);
  
  rCost = logicalPlanEstimateCost(pFilter, &context);
  TEST_ASSERT_TRUE(rCost > pScan->rEstimatedCost); /* Should include child cost */
  
  iRows = logicalPlanEstimateRows(pFilter, &context);
  TEST_ASSERT_TRUE(iRows < pScan->iEstimatedRows); /* Filter should reduce rows */
  
  /* Test join costs */
  pJoin = logicalPlanNodeCreate(LOGICAL_HASH_JOIN);
  LogicalPlanNode *pScan2 = logicalPlanNodeCreate(LOGICAL_LABEL_SCAN);
  logicalPlanNodeAddChild(pJoin, pFilter);
  logicalPlanNodeAddChild(pJoin, pScan2);
  
  rCost = logicalPlanEstimateCost(pJoin, &context);
  TEST_ASSERT_TRUE(rCost > pFilter->rEstimatedCost); /* Join should be expensive */
  
  logicalPlanNodeDestroy(pJoin);
}

/*
** Test logical plan string representation.
*/
void testLogicalPlanToString(void) {
  LogicalPlanNode *pNode;
  char *zString;
  
  /* Test simple node */
  pNode = logicalPlanNodeCreate(LOGICAL_LABEL_SCAN);
  logicalPlanNodeSetAlias(pNode, "n");
  logicalPlanNodeSetLabel(pNode, "Person");
  
  zString = logicalPlanToString(pNode);
  TEST_ASSERT_NOT_NULL(zString);
  TEST_ASSERT_NOT_NULL(strstr(zString, "LABEL_SCAN"));
  TEST_ASSERT_NOT_NULL(strstr(zString, "n"));
  TEST_ASSERT_NOT_NULL(strstr(zString, "cost="));
  TEST_ASSERT_NOT_NULL(strstr(zString, "rows="));
  
  sqlite3_free(zString);
  logicalPlanNodeDestroy(pNode);
}

/*
** Test physical plan node creation and management.
*/
void testPhysicalPlanNodeBasic(void) {
  PhysicalPlanNode *pNode, *pChild;
  int rc;
  
  /* Test node creation */
  pNode = physicalPlanNodeCreate(PHYSICAL_LABEL_INDEX_SCAN);
  TEST_ASSERT_NOT_NULL(pNode);
  TEST_ASSERT_EQUAL(PHYSICAL_LABEL_INDEX_SCAN, pNode->type);
  TEST_ASSERT_EQUAL(0, pNode->nChildren);
  TEST_ASSERT_NULL(pNode->zAlias);
  
  /* Test adding child */
  pChild = physicalPlanNodeCreate(PHYSICAL_FILTER);
  TEST_ASSERT_NOT_NULL(pChild);
  
  rc = physicalPlanNodeAddChild(pNode, pChild);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_EQUAL(1, pNode->nChildren);
  TEST_ASSERT_EQUAL(pChild, pNode->apChildren[0]);
  
  physicalPlanNodeDestroy(pNode);
}

/*
** Test physical operator type names.
*/
void testPhysicalOperatorTypeNames(void) {
  TEST_ASSERT_EQUAL_STRING("AllNodesScan", physicalOperatorTypeName(PHYSICAL_ALL_NODES_SCAN));
  TEST_ASSERT_EQUAL_STRING("LabelIndexScan", physicalOperatorTypeName(PHYSICAL_LABEL_INDEX_SCAN));
  TEST_ASSERT_EQUAL_STRING("PropertyIndexScan", physicalOperatorTypeName(PHYSICAL_PROPERTY_INDEX_SCAN));
  TEST_ASSERT_EQUAL_STRING("HashJoin", physicalOperatorTypeName(PHYSICAL_HASH_JOIN));
  TEST_ASSERT_EQUAL_STRING("Filter", physicalOperatorTypeName(PHYSICAL_FILTER));
  TEST_ASSERT_EQUAL_STRING("Unknown", physicalOperatorTypeName((PhysicalOperatorType)999));
}

/*
** Test logical to physical plan conversion.
*/
void testLogicalToPhysicalConversion(void) {
  LogicalPlanNode *pLogical;
  PhysicalPlanNode *pPhysical;
  PlanContext context;
  
  memset(&context, 0, sizeof(context));
  context.bUseIndexes = 1;
  context.rIndexCostFactor = 0.1;
  
  /* Create logical plan */
  pLogical = logicalPlanNodeCreate(LOGICAL_LABEL_SCAN);
  logicalPlanNodeSetAlias(pLogical, "n");
  logicalPlanNodeSetLabel(pLogical, "Person");
  
  /* Convert to physical plan */
  pPhysical = logicalPlanToPhysical(pLogical, &context);
  TEST_ASSERT_NOT_NULL(pPhysical);
  TEST_ASSERT_EQUAL(PHYSICAL_LABEL_INDEX_SCAN, pPhysical->type);
  TEST_ASSERT_EQUAL_STRING("Person", pPhysical->zLabel);
  
  physicalPlanNodeDestroy(pPhysical);
  logicalPlanNodeDestroy(pLogical);
}

/*
** Test physical plan string representation.
*/
void testPhysicalPlanToString(void) {
  PhysicalPlanNode *pNode;
  char *zString;
  
  /* Test physical node with properties */
  pNode = physicalPlanNodeCreate(PHYSICAL_PROPERTY_INDEX_SCAN);
  pNode->zAlias = sqlite3_mprintf("n");
  pNode->zProperty = sqlite3_mprintf("age");
  pNode->zValue = sqlite3_mprintf("30");
  pNode->rCost = 5.5;
  pNode->iRows = 100;
  
  zString = physicalPlanToString(pNode);
  TEST_ASSERT_NOT_NULL(zString);
  TEST_ASSERT_NOT_NULL(strstr(zString, "PropertyIndexScan"));
  TEST_ASSERT_NOT_NULL(strstr(zString, "n"));
  TEST_ASSERT_NOT_NULL(strstr(zString, "age"));
  TEST_ASSERT_NOT_NULL(strstr(zString, "30"));
  TEST_ASSERT_NOT_NULL(strstr(zString, "5.5"));
  TEST_ASSERT_NOT_NULL(strstr(zString, "100"));
  
  sqlite3_free(zString);
  physicalPlanNodeDestroy(pNode);
}

/*
** Test planner creation and destruction.
*/
void testCypherPlannerBasic(void) {
  CypherPlanner *pPlanner;
  sqlite3 *pDb = NULL;
  
  /* Test planner creation */
  pPlanner = cypherPlannerCreate(pDb, NULL);
  TEST_ASSERT_NOT_NULL(pPlanner);
  TEST_ASSERT_EQUAL(pDb, pPlanner->pDb);
  TEST_ASSERT_NOT_NULL(pPlanner->pContext);
  TEST_ASSERT_NULL(pPlanner->pLogicalPlan);
  TEST_ASSERT_NULL(pPlanner->pPhysicalPlan);
  
  /* Test default optimization settings */
  TEST_ASSERT_TRUE(pPlanner->pContext->bUseIndexes);
  TEST_ASSERT_TRUE(pPlanner->pContext->bReorderJoins);
  TEST_ASSERT_EQUAL_DOUBLE(0.1, pPlanner->pContext->rIndexCostFactor);
  
  /* Test destruction */
  cypherPlannerDestroy(pPlanner);
  
  /* Test destruction with NULL */
  cypherPlannerDestroy(NULL); /* Should not crash */
}

/*
** Test AST compilation to logical plan.
*/
void testCypherPlannerCompile(void) {
  CypherPlanner *pPlanner;
  CypherAst *pAst;
  int rc;
  
  pPlanner = cypherPlannerCreate(NULL, NULL);
  TEST_ASSERT_NOT_NULL(pPlanner);
  
  /* Create test AST (simple node pattern) */
  pAst = cypherAstCreate(CYPHER_AST_NODE_PATTERN, 1, 1);
  TEST_ASSERT_NOT_NULL(pAst);
  
  CypherAst *pId = cypherAstCreateIdentifier("n", 1, 2);
  cypherAstAddChild(pAst, pId);
  
  /* Test compilation */
  rc = cypherPlannerCompile(pPlanner, pAst);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_NOT_NULL(pPlanner->pLogicalPlan);
  TEST_ASSERT_EQUAL(LOGICAL_NODE_SCAN, pPlanner->pLogicalPlan->type);
  
  cypherAstDestroy(pAst);
  cypherPlannerDestroy(pPlanner);
}

/*
** Test planner optimization.
*/
void testCypherPlannerOptimize(void) {
  CypherPlanner *pPlanner;
  LogicalPlanNode *pLogical;
  PhysicalPlanNode *pPhysical;
  int rc;
  
  pPlanner = cypherPlannerCreate(NULL, NULL);
  TEST_ASSERT_NOT_NULL(pPlanner);
  
  /* Create simple logical plan */
  pLogical = logicalPlanNodeCreate(LOGICAL_LABEL_SCAN);
  logicalPlanNodeSetAlias(pLogical, "n");
  logicalPlanNodeSetLabel(pLogical, "Person");
  pPlanner->pLogicalPlan = pLogical;
  
  /* Test optimization */
  rc = cypherPlannerOptimize(pPlanner);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  pPhysical = cypherPlannerGetPlan(pPlanner);
  TEST_ASSERT_NOT_NULL(pPhysical);
  TEST_ASSERT_EQUAL(PHYSICAL_LABEL_INDEX_SCAN, pPhysical->type);
  
  cypherPlannerDestroy(pPlanner);
}

/*
** Test join optimization.
*/
void testLogicalPlanOptimizeJoins(void) {
  LogicalPlanNode *pJoin, *pLeft, *pRight;
  PlanContext context;
  int rc;
  
  memset(&context, 0, sizeof(context));
  
  /* Create join with unbalanced children */
  pJoin = logicalPlanNodeCreate(LOGICAL_HASH_JOIN);
  pLeft = logicalPlanNodeCreate(LOGICAL_NODE_SCAN);
  pRight = logicalPlanNodeCreate(LOGICAL_LABEL_SCAN);
  
  /* Set up so right side is smaller */
  pLeft->iEstimatedRows = 10000;
  pRight->iEstimatedRows = 100;
  
  logicalPlanNodeAddChild(pJoin, pLeft);
  logicalPlanNodeAddChild(pJoin, pRight);
  
  /* Verify initial order */
  TEST_ASSERT_EQUAL(pLeft, pJoin->apChildren[0]);
  TEST_ASSERT_EQUAL(pRight, pJoin->apChildren[1]);
  
  /* Optimize joins */
  rc = logicalPlanOptimizeJoins(pJoin, &context);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  /* Should swap to put smaller table first */
  TEST_ASSERT_EQUAL(pRight, pJoin->apChildren[0]);
  TEST_ASSERT_EQUAL(pLeft, pJoin->apChildren[1]);
  
  logicalPlanNodeDestroy(pJoin);
}

/*
** Test creating test logical plan.
*/
void testCypherCreateTestLogicalPlan(void) {
  LogicalPlanNode *pPlan;
  char *zString;
  
  pPlan = cypherCreateTestLogicalPlan();
  TEST_ASSERT_NOT_NULL(pPlan);
  TEST_ASSERT_EQUAL(LOGICAL_PROJECTION, pPlan->type);
  TEST_ASSERT_TRUE(pPlan->nChildren > 0);
  
  /* Verify structure */
  TEST_ASSERT_EQUAL(LOGICAL_PROPERTY_FILTER, pPlan->apChildren[0]->type);
  TEST_ASSERT_EQUAL(LOGICAL_LABEL_SCAN, pPlan->apChildren[0]->apChildren[0]->type);
  
  /* Test string representation */
  zString = logicalPlanToString(pPlan);
  TEST_ASSERT_NOT_NULL(zString);
  TEST_ASSERT_NOT_NULL(strstr(zString, "PROJECTION"));
  TEST_ASSERT_NOT_NULL(strstr(zString, "PROPERTY_FILTER"));
  TEST_ASSERT_NOT_NULL(strstr(zString, "LABEL_SCAN"));
  
  sqlite3_free(zString);
  logicalPlanNodeDestroy(pPlan);
}

/*
** Test creating test physical plan.
*/
void testCypherCreateTestPhysicalPlan(void) {
  PhysicalPlanNode *pPlan;
  char *zString;
  
  pPlan = cypherCreateTestPhysicalPlan();
  TEST_ASSERT_NOT_NULL(pPlan);
  TEST_ASSERT_EQUAL(PHYSICAL_PROJECTION, pPlan->type);
  TEST_ASSERT_TRUE(pPlan->nChildren > 0);
  
  /* Verify structure */
  TEST_ASSERT_EQUAL(PHYSICAL_FILTER, pPlan->apChildren[0]->type);
  TEST_ASSERT_EQUAL(PHYSICAL_LABEL_INDEX_SCAN, pPlan->apChildren[0]->apChildren[0]->type);
  
  /* Test string representation */
  zString = physicalPlanToString(pPlan);
  TEST_ASSERT_NOT_NULL(zString);
  TEST_ASSERT_NOT_NULL(strstr(zString, "Projection"));
  TEST_ASSERT_NOT_NULL(strstr(zString, "Filter"));
  TEST_ASSERT_NOT_NULL(strstr(zString, "LabelIndexScan"));
  
  sqlite3_free(zString);
  physicalPlanNodeDestroy(pPlan);
}

/*
** Test planner error handling.
*/
void testCypherPlannerErrors(void) {
  CypherPlanner *pPlanner;
  int rc;
  
  pPlanner = cypherPlannerCreate(NULL, NULL);
  TEST_ASSERT_NOT_NULL(pPlanner);
  
  /* Test compilation with NULL AST */
  rc = cypherPlannerCompile(pPlanner, NULL);
  TEST_ASSERT_NOT_EQUAL(SQLITE_OK, rc);
  
  /* Test optimization without logical plan */
  rc = cypherPlannerOptimize(pPlanner);
  TEST_ASSERT_NOT_EQUAL(SQLITE_OK, rc);
  
  /* Test getting plan when none exists */
  TEST_ASSERT_NULL(cypherPlannerGetPlan(pPlanner));
  
  cypherPlannerDestroy(pPlanner);
}

/*
** Main test runner for Cypher planner tests.
*/
void runCypherPlannerTests(void) {
  RUN_TEST(testLogicalPlanNodeBasic);
  RUN_TEST(testLogicalPlanNodeTypeNames);
  RUN_TEST(testLogicalPlanCostEstimation);
  RUN_TEST(testLogicalPlanToString);
  RUN_TEST(testPhysicalPlanNodeBasic);
  RUN_TEST(testPhysicalOperatorTypeNames);
  RUN_TEST(testLogicalToPhysicalConversion);
  RUN_TEST(testPhysicalPlanToString);
  RUN_TEST(testCypherPlannerBasic);
  RUN_TEST(testCypherPlannerCompile);
  RUN_TEST(testCypherPlannerOptimize);
  RUN_TEST(testLogicalPlanOptimizeJoins);
  RUN_TEST(testCypherCreateTestLogicalPlan);
  RUN_TEST(testCypherCreateTestPhysicalPlan);
  RUN_TEST(testCypherPlannerErrors);
}
