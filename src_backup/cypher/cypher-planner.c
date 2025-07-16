/*
** SQLite Graph Database Extension - Cypher Query Planner
**
** This file implements the main query planner that compiles Cypher ASTs
** into optimized logical and physical execution plans. The planner handles
** pattern recognition, cost estimation, and operator selection.
**
** Features:
** - AST to logical plan compilation
** - Pattern optimization and rewriting
** - Cost-based physical plan generation
** - Index utilization planning
** - Join ordering optimization
**
** Memory allocation: All functions use sqlite3_malloc()/sqlite3_free()
** Error handling: Functions return SQLite error codes
*/

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1
#include "cypher-planner.h"
#include <string.h>
#include <assert.h>

/*
** Create a new Cypher query planner.
** Returns NULL on allocation failure.
*/
CypherPlanner *cypherPlannerCreate(sqlite3 *pDb, GraphVtab *pGraph) {
  CypherPlanner *pPlanner;
  
  pPlanner = sqlite3_malloc(sizeof(CypherPlanner));
  if( !pPlanner ) return NULL;
  
  memset(pPlanner, 0, sizeof(CypherPlanner));
  pPlanner->pDb = pDb;
  
  /* Create planning context */
  pPlanner->pContext = sqlite3_malloc(sizeof(PlanContext));
  if( !pPlanner->pContext ) {
    sqlite3_free(pPlanner);
    return NULL;
  }
  
  memset(pPlanner->pContext, 0, sizeof(PlanContext));
  pPlanner->pContext->pDb = pDb;
  pPlanner->pContext->pGraph = pGraph;
  
  /* Set default optimization settings */
  pPlanner->pContext->bUseIndexes = 1;
  pPlanner->pContext->bReorderJoins = 1;
  pPlanner->pContext->rIndexCostFactor = 0.1;
  
  return pPlanner;
}

/*
** Destroy a Cypher planner and free all associated memory.
** Safe to call with NULL pointer.
*/
void cypherPlannerDestroy(CypherPlanner *pPlanner) {
  int i;
  
  if( !pPlanner ) return;
  
  /* Free context */
  if( pPlanner->pContext ) {
    /* Free variable arrays */
    for( i = 0; i < pPlanner->pContext->nVariables; i++ ) {
      sqlite3_free(pPlanner->pContext->azVariables[i]);
    }
    sqlite3_free(pPlanner->pContext->azVariables);
    sqlite3_free(pPlanner->pContext->apVarNodes);
    
    /* Free index arrays */
    for( i = 0; i < pPlanner->pContext->nLabelIndexes; i++ ) {
      sqlite3_free(pPlanner->pContext->azLabelIndexes[i]);
    }
    sqlite3_free(pPlanner->pContext->azLabelIndexes);
    
    for( i = 0; i < pPlanner->pContext->nPropertyIndexes; i++ ) {
      sqlite3_free(pPlanner->pContext->azPropertyIndexes[i]);
    }
    sqlite3_free(pPlanner->pContext->azPropertyIndexes);
    
    sqlite3_free(pPlanner->pContext->zErrorMsg);
    sqlite3_free(pPlanner->pContext);
  }
  
  /* Free plans */
  logicalPlanNodeDestroy(pPlanner->pLogicalPlan);
  physicalPlanNodeDestroy(pPlanner->pPhysicalPlan);
  
  sqlite3_free(pPlanner->zErrorMsg);
  sqlite3_free(pPlanner);
}

/*
** Add a variable to the planning context.
** Returns SQLITE_OK on success, SQLITE_NOMEM on allocation failure.
*/
static int planContextAddVariable(PlanContext *pContext, const char *zVar, LogicalPlanNode *pNode) {
  char **azNew;
  LogicalPlanNode **apNew;
  
  if( !pContext || !zVar ) return SQLITE_MISUSE;
  
  /* Resize arrays if needed */
  if( pContext->nVariables >= pContext->nVariablesAlloc ) {
    int nNew = pContext->nVariablesAlloc ? pContext->nVariablesAlloc * 2 : 8;
    
    azNew = sqlite3_realloc(pContext->azVariables, nNew * sizeof(char*));
    if( !azNew ) return SQLITE_NOMEM;
    pContext->azVariables = azNew;
    
    apNew = sqlite3_realloc(pContext->apVarNodes, nNew * sizeof(LogicalPlanNode*));
    if( !apNew ) return SQLITE_NOMEM;
    pContext->apVarNodes = apNew;
    
    pContext->nVariablesAlloc = nNew;
  }
  
  /* Add variable */
  pContext->azVariables[pContext->nVariables] = sqlite3_mprintf("%s", zVar);
  pContext->apVarNodes[pContext->nVariables] = pNode;
  
  if( !pContext->azVariables[pContext->nVariables] ) {
    return SQLITE_NOMEM;
  }
  
  pContext->nVariables++;
  return SQLITE_OK;
}

/*
** Find a variable in the planning context.
** Returns the associated logical plan node, or NULL if not found.
*/
static LogicalPlanNode *planContextFindVariable(PlanContext *pContext, const char *zVar) {
  int i;
  
  if( !pContext || !zVar ) return NULL;
  
  for( i = 0; i < pContext->nVariables; i++ ) {
    if( strcmp(pContext->azVariables[i], zVar) == 0 ) {
      return pContext->apVarNodes[i];
    }
  }
  
  return NULL;
}

/*
** Compile a Cypher AST node into a logical plan node.
** Returns the compiled logical plan node, or NULL on error.
*/
static LogicalPlanNode *compileAstNode(CypherAst *pAst, PlanContext *pContext) {
  LogicalPlanNode *pLogical = NULL;
  LogicalPlanNode *pChild;
  const char *zAlias;
  int i, rc;
  
  if( !pAst ) return NULL;
  
  switch( pAst->type ) {
    case CYPHER_AST_QUERY:
    case CYPHER_AST_SINGLE_QUERY:
      /* Compile children and combine them */
      if( pAst->nChildren > 0 ) {
        pLogical = compileAstNode(pAst->apChildren[0], pContext);
        
        for( i = 1; i < pAst->nChildren; i++ ) {
          pChild = compileAstNode(pAst->apChildren[i], pContext);
          if( pChild && pLogical ) {
            /* Create a join or sequence node */
            LogicalPlanNode *pJoin = logicalPlanNodeCreate(LOGICAL_HASH_JOIN);
            if( pJoin ) {
              logicalPlanNodeAddChild(pJoin, pLogical);
              logicalPlanNodeAddChild(pJoin, pChild);
              pLogical = pJoin;
            }
          }
        }
      }
      break;
      
    case CYPHER_AST_MATCH:
      /* Compile MATCH clause */
      if( pAst->nChildren > 0 ) {
        pLogical = compileAstNode(pAst->apChildren[0], pContext);
      }
      break;
      
    case CYPHER_AST_NODE_PATTERN:
      /* Node pattern becomes a scan operation */
      if( pAst->nChildren > 0 && cypherAstIsType(pAst->apChildren[0], CYPHER_AST_IDENTIFIER) ) {
        zAlias = cypherAstGetValue(pAst->apChildren[0]);
        
        /* Check if this is a labeled node */
        if( pAst->nChildren > 1 && cypherAstIsType(pAst->apChildren[1], CYPHER_AST_LABELS) ) {
          /* Label scan */
          pLogical = logicalPlanNodeCreate(LOGICAL_LABEL_SCAN);
          if( pLogical ) {
            logicalPlanNodeSetAlias(pLogical, zAlias);
            
            /* Get first label */
            if( pAst->apChildren[1]->nChildren > 0 ) {
              const char *zLabel = cypherAstGetValue(pAst->apChildren[1]->apChildren[0]);
              logicalPlanNodeSetLabel(pLogical, zLabel);
            }
            
            /* Add variable to context */
            planContextAddVariable(pContext, zAlias, pLogical);
          }
        } else {
          /* Full node scan */
          pLogical = logicalPlanNodeCreate(LOGICAL_NODE_SCAN);
          if( pLogical ) {
            logicalPlanNodeSetAlias(pLogical, zAlias);
            planContextAddVariable(pContext, zAlias, pLogical);
          }
        }
      }
      break;
      
    case CYPHER_AST_WHERE:
      /* WHERE clause becomes a filter */
      if( pAst->nChildren > 0 ) {
        CypherAst *pExpr = pAst->apChildren[0];
        
        if( cypherAstIsType(pExpr, CYPHER_AST_BINARY_OP) && 
            strcmp(cypherAstGetValue(pExpr), "=") == 0 &&
            pExpr->nChildren >= 2 ) {
          
          /* Property filter: n.prop = value */
          if( cypherAstIsType(pExpr->apChildren[0], CYPHER_AST_PROPERTY) ) {
            pLogical = logicalPlanNodeCreate(LOGICAL_PROPERTY_FILTER);
            if( pLogical ) {
              CypherAst *pProp = pExpr->apChildren[0];
              if( pProp->nChildren >= 2 ) {
                const char *zVar = cypherAstGetValue(pProp->apChildren[0]);
                const char *zProp = cypherAstGetValue(pProp->apChildren[1]);
                const char *zValue = cypherAstGetValue(pExpr->apChildren[1]);
                
                logicalPlanNodeSetAlias(pLogical, zVar);
                logicalPlanNodeSetProperty(pLogical, zProp);
                logicalPlanNodeSetValue(pLogical, zValue);
              }
            }
          }
        }
        
        if( !pLogical ) {
          /* Generic filter */
          pLogical = logicalPlanNodeCreate(LOGICAL_FILTER);
        }
      }
      break;
      
    case CYPHER_AST_RETURN:
      /* RETURN clause becomes a projection */
      pLogical = logicalPlanNodeCreate(LOGICAL_PROJECTION);
      if( pLogical && pAst->nChildren > 0 ) {
        /* Process projection list */
        CypherAst *pProjList = pAst->apChildren[0];
        if( cypherAstIsType(pProjList, CYPHER_AST_PROJECTION_LIST) && 
            pProjList->nChildren > 0 ) {
          CypherAst *pItem = pProjList->apChildren[0];
          if( cypherAstIsType(pItem, CYPHER_AST_PROJECTION_ITEM) && 
              pItem->nChildren > 0 ) {
            CypherAst *pExpr = pItem->apChildren[0];
            
            if( cypherAstIsType(pExpr, CYPHER_AST_IDENTIFIER) ) {
              logicalPlanNodeSetAlias(pLogical, cypherAstGetValue(pExpr));
            } else if( cypherAstIsType(pExpr, CYPHER_AST_PROPERTY) && 
                       pExpr->nChildren >= 2 ) {
              const char *zVar = cypherAstGetValue(pExpr->apChildren[0]);
              const char *zProp = cypherAstGetValue(pExpr->apChildren[1]);
              
              logicalPlanNodeSetAlias(pLogical, zVar);
              logicalPlanNodeSetProperty(pLogical, zProp);
            }
          }
        }
      }
      break;
      
    default:
      /* Unsupported AST node type */
      pContext->zErrorMsg = sqlite3_mprintf("Unsupported AST node type: %d", pAst->type);
      pContext->nErrors++;
      break;
  }
  
  return pLogical;
}

/*
** Compile an AST into a logical plan.
** Returns SQLITE_OK on success, error code on failure.
*/
int cypherPlannerCompile(CypherPlanner *pPlanner, CypherAst *pAst) {
  LogicalPlanNode *pRoot;
  
  if( !pPlanner || !pAst ) return SQLITE_MISUSE;
  
  /* Clean up any previous plan */
  logicalPlanNodeDestroy(pPlanner->pLogicalPlan);
  pPlanner->pLogicalPlan = NULL;
  physicalPlanNodeDestroy(pPlanner->pPhysicalPlan);
  pPlanner->pPhysicalPlan = NULL;
  
  sqlite3_free(pPlanner->zErrorMsg);
  pPlanner->zErrorMsg = NULL;
  
  /* Reset context */
  pPlanner->pContext->pAst = pAst;
  pPlanner->pContext->nErrors = 0;
  sqlite3_free(pPlanner->pContext->zErrorMsg);
  pPlanner->pContext->zErrorMsg = NULL;
  
  /* Compile AST to logical plan */
  pRoot = compileAstNode(pAst, pPlanner->pContext);
  if( !pRoot ) {
    if( pPlanner->pContext->zErrorMsg ) {
      pPlanner->zErrorMsg = sqlite3_mprintf("Compilation failed: %s", 
                                           pPlanner->pContext->zErrorMsg);
    } else {
      pPlanner->zErrorMsg = sqlite3_mprintf("Failed to compile AST to logical plan");
    }
    return SQLITE_ERROR;
  }
  
  pPlanner->pLogicalPlan = pRoot;
  
  /* Estimate costs */
  logicalPlanEstimateCost(pRoot, pPlanner->pContext);
  logicalPlanEstimateRows(pRoot, pPlanner->pContext);
  
  return SQLITE_OK;
}

/*
** Optimize the logical plan and generate physical plan.
** Returns SQLITE_OK on success, error code on failure.
*/
int cypherPlannerOptimize(CypherPlanner *pPlanner) {
  PhysicalPlanNode *pPhysical;
  
  if( !pPlanner || !pPlanner->pLogicalPlan ) return SQLITE_MISUSE;
  
  /* Clean up any previous physical plan */
  physicalPlanNodeDestroy(pPlanner->pPhysicalPlan);
  pPlanner->pPhysicalPlan = NULL;
  
  /* TODO: Implement join reordering optimization */
  if( pPlanner->pContext->bReorderJoins ) {
    logicalPlanOptimizeJoins(pPlanner->pLogicalPlan, pPlanner->pContext);
  }
  
  /* Convert logical plan to physical plan */
  pPhysical = logicalPlanToPhysical(pPlanner->pLogicalPlan, pPlanner->pContext);
  if( !pPhysical ) {
    pPlanner->zErrorMsg = sqlite3_mprintf("Failed to generate physical plan");
    return SQLITE_ERROR;
  }
  
  pPlanner->pPhysicalPlan = pPhysical;
  return SQLITE_OK;
}

/*
** Get the final physical execution plan.
** Returns NULL if planning failed or not yet completed.
*/
PhysicalPlanNode *cypherPlannerGetPlan(CypherPlanner *pPlanner) {
  return pPlanner ? pPlanner->pPhysicalPlan : NULL;
}

/*
** Get error message from planner.
** Returns NULL if no error occurred.
*/
const char *cypherPlannerGetError(CypherPlanner *pPlanner) {
  return pPlanner ? pPlanner->zErrorMsg : NULL;
}

/*
** Optimize join ordering using simple heuristics.
** This is a simplified version - full implementation would use dynamic programming.
*/
int logicalPlanOptimizeJoins(LogicalPlanNode *pNode, PlanContext *pContext) {
  int i;
  
  if( !pNode ) return SQLITE_OK;
  
  /* Recursively optimize children first */
  for( i = 0; i < pNode->nChildren; i++ ) {
    logicalPlanOptimizeJoins(pNode->apChildren[i], pContext);
  }
  
  /* For join nodes, prefer smaller tables on the left */
  if( pNode->type == LOGICAL_HASH_JOIN || 
      pNode->type == LOGICAL_NESTED_LOOP_JOIN ) {
    if( pNode->nChildren >= 2 ) {
      LogicalPlanNode *pLeft = pNode->apChildren[0];
      LogicalPlanNode *pRight = pNode->apChildren[1];
      
      /* Swap if right side is smaller */
      if( pRight->iEstimatedRows < pLeft->iEstimatedRows ) {
        pNode->apChildren[0] = pRight;
        pNode->apChildren[1] = pLeft;
      }
    }
  }
  
  return SQLITE_OK;
}