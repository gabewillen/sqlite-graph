/*
** SQLite Graph Database Extension - Cypher Executor Implementation
**
** This file implements the main Cypher query executor that coordinates
** the execution of physical plans using the Volcano iterator model.
** The executor manages the execution pipeline from plan to results.
**
** Features:
** - Physical plan execution coordination
** - Iterator lifecycle management
** - Result collection and formatting
** - Error handling and cleanup
**
** Memory allocation: All functions use sqlite3_malloc()/sqlite3_free()
** Error handling: Functions return SQLite error codes
*/

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1
#include "cypher-executor.h"
#include <string.h>
#include <assert.h>
#include <stdio.h>

/*
** Create a new Cypher executor.
** Returns NULL on allocation failure.
*/
CypherExecutor *cypherExecutorCreate(sqlite3 *pDb, GraphVtab *pGraph) {
  CypherExecutor *pExecutor;
  
  pExecutor = sqlite3_malloc(sizeof(CypherExecutor));
  if( !pExecutor ) return NULL;
  
  memset(pExecutor, 0, sizeof(CypherExecutor));
  pExecutor->pDb = pDb;
  pExecutor->pGraph = pGraph;
  
  /* Create execution context */
  pExecutor->pContext = executionContextCreate(pDb, pGraph);
  if( !pExecutor->pContext ) {
    sqlite3_free(pExecutor);
    return NULL;
  }
  
  return pExecutor;
}

/*
** Destroy a Cypher executor and free all associated memory.
** Safe to call with NULL pointer.
*/
void cypherExecutorDestroy(CypherExecutor *pExecutor) {
  if( !pExecutor ) return;
  
  cypherIteratorDestroy(pExecutor->pRootIterator);
  executionContextDestroy(pExecutor->pContext);
  sqlite3_free(pExecutor->zErrorMsg);
  sqlite3_free(pExecutor);
}

/*
** Recursively create iterators from a physical plan tree.
** Returns the root iterator, or NULL on error.
*/
static CypherIterator *createIteratorTree(PhysicalPlanNode *pPlan, ExecutionContext *pContext) {
  CypherIterator *pIterator;
  CypherIterator *pChild;
  int i, rc;
  
  if( !pPlan ) return NULL;
  
  /* Create iterator for this plan node */
  pIterator = cypherIteratorCreate(pPlan, pContext);
  if( !pIterator ) return NULL;
  
  /* Create child iterators */
  if( pPlan->nChildren > 0 ) {
    pIterator->apChildren = sqlite3_malloc(pPlan->nChildren * sizeof(CypherIterator*));
    if( !pIterator->apChildren ) {
      cypherIteratorDestroy(pIterator);
      return NULL;
    }
    
    for( i = 0; i < pPlan->nChildren; i++ ) {
      pChild = createIteratorTree(pPlan->apChildren[i], pContext);
      if( !pChild ) {
        /* Clean up partial iterator tree */
        for( int j = 0; j < i; j++ ) {
          cypherIteratorDestroy(pIterator->apChildren[j]);
        }
        cypherIteratorDestroy(pIterator);
        return NULL;
      }
      pIterator->apChildren[i] = pChild;
      pIterator->nChildren++;
    }
  }
  
  return pIterator;
}

/*
** Prepare an executor with a physical execution plan.
** Returns SQLITE_OK on success, error code on failure.
*/
int cypherExecutorPrepare(CypherExecutor *pExecutor, PhysicalPlanNode *pPlan) {
  if( !pExecutor || !pPlan ) return SQLITE_MISUSE;
  
  /* Clean up any previous iterator */
  cypherIteratorDestroy(pExecutor->pRootIterator);
  pExecutor->pRootIterator = NULL;
  
  /* Store plan reference */
  pExecutor->pPlan = pPlan;
  
  /* Create iterator tree from physical plan */
  pExecutor->pRootIterator = createIteratorTree(pPlan, pExecutor->pContext);
  if( !pExecutor->pRootIterator ) {
    pExecutor->zErrorMsg = sqlite3_mprintf("Failed to create iterator tree");
    return SQLITE_ERROR;
  }
  
  return SQLITE_OK;
}

/*
** Execute the prepared query and collect all results.
** Returns SQLITE_OK on success, error code on failure.
** Results are returned as a JSON array string.
*/
int cypherExecutorExecute(CypherExecutor *pExecutor, char **pzResults) {
  CypherIterator *pRoot;
  CypherResult *pResult;
  char *zResultArray = NULL;
  char *zRowJson;
  int nResults = 0;
  int nAllocated = 256;
  int nUsed = 0;
  int rc;
  
  if( !pExecutor || !pzResults ) return SQLITE_MISUSE;
  if( !pExecutor->pRootIterator ) return SQLITE_ERROR;
  
  *pzResults = NULL;
  pRoot = pExecutor->pRootIterator;
  
  /* Allocate result buffer */
  zResultArray = sqlite3_malloc(nAllocated);
  if( !zResultArray ) return SQLITE_NOMEM;
  
  /* Start JSON array */
  nUsed = snprintf(zResultArray, nAllocated, "[");
  
  /* Open root iterator */
  rc = pRoot->xOpen(pRoot);
  if( rc != SQLITE_OK ) {
    pExecutor->zErrorMsg = sqlite3_mprintf("Failed to open root iterator");
    sqlite3_free(zResultArray);
    return rc;
  }
  
  /* Iterate through results */
  while( 1 ) {
    pResult = cypherResultCreate();
    if( !pResult ) {
      rc = SQLITE_NOMEM;
      break;
    }
    
    /* Get next result row */
    rc = pRoot->xNext(pRoot, pResult);
    if( rc == SQLITE_DONE ) {
      cypherResultDestroy(pResult);
      rc = SQLITE_OK;
      break;
    } else if( rc != SQLITE_OK ) {
      cypherResultDestroy(pResult);
      pExecutor->zErrorMsg = sqlite3_mprintf("Iterator error: %d", rc);
      break;
    }
    
    /* Convert result to JSON */
    zRowJson = cypherResultToJson(pResult);
    cypherResultDestroy(pResult);
    
    if( !zRowJson ) {
      rc = SQLITE_NOMEM;
      break;
    }
    
    /* Calculate space needed */
    int nRowLen = strlen(zRowJson);
    int nNeeded = nUsed + (nResults > 0 ? 1 : 0) + nRowLen;
    
    /* Resize buffer if needed */
    if( nNeeded + 2 >= nAllocated ) {
      nAllocated = (nNeeded + 256) * 2;
      char *zNew = sqlite3_realloc(zResultArray, nAllocated);
      if( !zNew ) {
        sqlite3_free(zRowJson);
        rc = SQLITE_NOMEM;
        break;
      }
      zResultArray = zNew;
    }
    
    /* Add result to array */
    if( nResults > 0 ) {
      zResultArray[nUsed++] = ',';
    }
    memcpy(zResultArray + nUsed, zRowJson, nRowLen);
    nUsed += nRowLen;
    
    sqlite3_free(zRowJson);
    nResults++;
    
    /* Sanity check to prevent infinite loops */
    if( nResults > 10000 ) {
      pExecutor->zErrorMsg = sqlite3_mprintf("Result limit exceeded (10000 rows)");
      rc = SQLITE_ERROR;
      break;
    }
  }
  
  /* Close iterator */
  pRoot->xClose(pRoot);
  
  if( rc == SQLITE_OK ) {
    /* Close JSON array */
    if( nUsed + 2 < nAllocated ) {
      zResultArray[nUsed++] = ']';
      zResultArray[nUsed] = '\0';
    }
    
    *pzResults = zResultArray;
  } else {
    sqlite3_free(zResultArray);
  }
  
  return rc;
}

/*
** Get error message from executor.
** Returns NULL if no error occurred.
*/
const char *cypherExecutorGetError(CypherExecutor *pExecutor) {
  return pExecutor ? pExecutor->zErrorMsg : NULL;
}

/*
** Create a test execution context for demonstration.
** Returns context with sample graph data loaded.
*/
ExecutionContext *cypherCreateTestExecutionContext(sqlite3 *pDb) {
  ExecutionContext *pContext;
  
  /* For now, just create an empty context */
  /* TODO: Load sample data when graph storage is available */
  pContext = executionContextCreate(pDb, NULL);
  
  return pContext;
}

/*
** Execute a simple test query for demonstration.
** Returns JSON results, caller must sqlite3_free().
*/
char *cypherExecuteTestQuery(sqlite3 *pDb, const char *zQuery) {
  CypherParser *pParser = NULL;
  CypherPlanner *pPlanner = NULL;
  CypherExecutor *pExecutor = NULL;
  CypherAst *pAst;
  PhysicalPlanNode *pPlan;
  char *zResults = NULL;
  int rc;
  
  if( !zQuery ) return NULL;
  
  /* Parse query */
  pParser = cypherParserCreate(pDb);
  if( !pParser ) goto cleanup;
  
  rc = cypherParserParse(pParser, zQuery, -1);
  if( rc != SQLITE_OK ) goto cleanup;
  
  pAst = cypherParserGetAst(pParser);
  if( !pAst ) goto cleanup;
  
  /* Plan query */
  pPlanner = cypherPlannerCreate(pDb, NULL);
  if( !pPlanner ) goto cleanup;
  
  rc = cypherPlannerCompile(pPlanner, pAst);
  if( rc != SQLITE_OK ) goto cleanup;
  
  rc = cypherPlannerOptimize(pPlanner);
  if( rc != SQLITE_OK ) goto cleanup;
  
  pPlan = cypherPlannerGetPlan(pPlanner);
  if( !pPlan ) goto cleanup;
  
  /* Execute query */
  pExecutor = cypherExecutorCreate(pDb, NULL);
  if( !pExecutor ) goto cleanup;
  
  rc = cypherExecutorPrepare(pExecutor, pPlan);
  if( rc != SQLITE_OK ) goto cleanup;
  
  rc = cypherExecutorExecute(pExecutor, &zResults);
  if( rc != SQLITE_OK ) {
    const char *zError = cypherExecutorGetError(pExecutor);
    zResults = sqlite3_mprintf("ERROR: %s", zError ? zError : "Unknown execution error");
  }
  
cleanup:
  cypherExecutorDestroy(pExecutor);
  cypherPlannerDestroy(pPlanner);
  cypherParserDestroy(pParser);
  
  if( !zResults ) {
    zResults = sqlite3_mprintf("ERROR: Query execution failed");
  }
  
  return zResults;
}