/*
** SQLite Graph Database Extension - Execution Context Implementation
**
** This file implements the execution context and value management for
** the Cypher execution engine. The context manages variable bindings,
** memory allocation, and execution state during query execution.
**
** Features:
** - Variable binding and lookup
** - Memory management for execution
** - Value creation and manipulation
** - Result row management
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
** Create a new execution context.
** Returns NULL on allocation failure.
*/
ExecutionContext *executionContextCreate(sqlite3 *pDb, GraphVtab *pGraph) {
  ExecutionContext *pContext;
  
  pContext = sqlite3_malloc(sizeof(ExecutionContext));
  if( !pContext ) return NULL;
  
  memset(pContext, 0, sizeof(ExecutionContext));
  pContext->pDb = pDb;
  pContext->pGraph = pGraph;
  
  return pContext;
}

/*
** Destroy an execution context and free all associated memory.
** Safe to call with NULL pointer.
*/
void executionContextDestroy(ExecutionContext *pContext) {
  int i;
  
  if( !pContext ) return;
  
  /* Free variable bindings */
  for( i = 0; i < pContext->nVariables; i++ ) {
    sqlite3_free(pContext->azVariables[i]);
    cypherValueDestroy(&pContext->aBindings[i]);
  }
  sqlite3_free(pContext->azVariables);
  sqlite3_free(pContext->aBindings);
  
  /* Free allocated memory blocks */
  for( i = 0; i < pContext->nAllocated; i++ ) {
    sqlite3_free(pContext->apAllocated[i]);
  }
  sqlite3_free(pContext->apAllocated);
  
  sqlite3_free(pContext->zErrorMsg);
  sqlite3_free(pContext);
}

/*
** Bind a variable to a value in the execution context.
** Returns SQLITE_OK on success, error code on failure.
*/
int executionContextBind(ExecutionContext *pContext, const char *zVar, CypherValue *pValue) {
  char **azNew;
  CypherValue *aNew;
  int i;
  
  if( !pContext || !zVar || !pValue ) return SQLITE_MISUSE;
  
  /* Check if variable already exists */
  for( i = 0; i < pContext->nVariables; i++ ) {
    if( strcmp(pContext->azVariables[i], zVar) == 0 ) {
      /* Update existing binding */
      cypherValueDestroy(&pContext->aBindings[i]);
      pContext->aBindings[i] = *cypherValueCopy(pValue);
      return SQLITE_OK;
    }
  }
  
  /* Resize arrays if needed */
  if( pContext->nVariables >= pContext->nVariablesAlloc ) {
    int nNew = pContext->nVariablesAlloc ? pContext->nVariablesAlloc * 2 : 8;
    
    azNew = sqlite3_realloc(pContext->azVariables, nNew * sizeof(char*));
    if( !azNew ) return SQLITE_NOMEM;
    pContext->azVariables = azNew;
    
    aNew = sqlite3_realloc(pContext->aBindings, nNew * sizeof(CypherValue));
    if( !aNew ) return SQLITE_NOMEM;
    pContext->aBindings = aNew;
    
    pContext->nVariablesAlloc = nNew;
  }
  
  /* Add new binding */
  pContext->azVariables[pContext->nVariables] = sqlite3_mprintf("%s", zVar);
  if( !pContext->azVariables[pContext->nVariables] ) return SQLITE_NOMEM;
  
  pContext->aBindings[pContext->nVariables] = *cypherValueCopy(pValue);
  pContext->nVariables++;
  
  return SQLITE_OK;
}

/*
** Get the value of a variable from the execution context.
** Returns NULL if variable not found.
*/
CypherValue *executionContextGet(ExecutionContext *pContext, const char *zVar) {
  int i;
  
  if( !pContext || !zVar ) return NULL;
  
  for( i = 0; i < pContext->nVariables; i++ ) {
    if( strcmp(pContext->azVariables[i], zVar) == 0 ) {
      return &pContext->aBindings[i];
    }
  }
  
  return NULL;
}

/*
** Create a new Cypher value.
** Returns NULL on allocation failure.
*/
CypherValue *cypherValueCreate(CypherValueType type) {
  CypherValue *pValue;
  
  pValue = sqlite3_malloc(sizeof(CypherValue));
  if( !pValue ) return NULL;
  
  memset(pValue, 0, sizeof(CypherValue));
  pValue->type = type;
  
  return pValue;
}

/*
** Destroy a Cypher value and free associated memory.
** Safe to call with NULL pointer.
*/
void cypherValueDestroy(CypherValue *pValue) {
  int i;
  
  if( !pValue ) return;
  
  switch( pValue->type ) {
    case CYPHER_VALUE_STRING:
      sqlite3_free(pValue->u.zString);
      break;
      
    case CYPHER_VALUE_LIST:
      for( i = 0; i < pValue->u.list.nValues; i++ ) {
        cypherValueDestroy(&pValue->u.list.apValues[i]);
      }
      sqlite3_free(pValue->u.list.apValues);
      break;
      
    case CYPHER_VALUE_MAP:
      for( i = 0; i < pValue->u.map.nPairs; i++ ) {
        sqlite3_free(pValue->u.map.azKeys[i]);
        cypherValueDestroy(&pValue->u.map.apValues[i]);
      }
      sqlite3_free(pValue->u.map.azKeys);
      sqlite3_free(pValue->u.map.apValues);
      break;
      
    default:
      /* No additional cleanup needed for other types */
      break;
  }
  
  /* Don't free pValue itself if it's a stack value */
}

/*
** Copy a Cypher value.
** Returns NULL on allocation failure.
*/
CypherValue *cypherValueCopy(CypherValue *pValue) {
  CypherValue *pCopy;
  int i;
  
  if( !pValue ) return NULL;
  
  pCopy = cypherValueCreate(pValue->type);
  if( !pCopy ) return NULL;
  
  switch( pValue->type ) {
    case CYPHER_VALUE_NULL:
      /* Nothing to copy */
      break;
      
    case CYPHER_VALUE_BOOLEAN:
      pCopy->u.bBoolean = pValue->u.bBoolean;
      break;
      
    case CYPHER_VALUE_INTEGER:
      pCopy->u.iInteger = pValue->u.iInteger;
      break;
      
    case CYPHER_VALUE_FLOAT:
      pCopy->u.rFloat = pValue->u.rFloat;
      break;
      
    case CYPHER_VALUE_STRING:
      if( pValue->u.zString ) {
        pCopy->u.zString = sqlite3_mprintf("%s", pValue->u.zString);
        if( !pCopy->u.zString ) {
          cypherValueDestroy(pCopy);
          sqlite3_free(pCopy);
          return NULL;
        }
      }
      break;
      
    case CYPHER_VALUE_NODE:
      pCopy->u.iNodeId = pValue->u.iNodeId;
      break;
      
    case CYPHER_VALUE_RELATIONSHIP:
      pCopy->u.iRelId = pValue->u.iRelId;
      break;
      
    case CYPHER_VALUE_LIST:
      if( pValue->u.list.nValues > 0 ) {
        pCopy->u.list.apValues = sqlite3_malloc(pValue->u.list.nValues * sizeof(CypherValue));
        if( !pCopy->u.list.apValues ) {
          cypherValueDestroy(pCopy);
          sqlite3_free(pCopy);
          return NULL;
        }
        
        pCopy->u.list.nValues = pValue->u.list.nValues;
        for( i = 0; i < pValue->u.list.nValues; i++ ) {
          CypherValue *pElementCopy = cypherValueCopy(&pValue->u.list.apValues[i]);
          if( !pElementCopy ) {
            cypherValueDestroy(pCopy);
            sqlite3_free(pCopy);
            return NULL;
          }
          pCopy->u.list.apValues[i] = *pElementCopy;
          sqlite3_free(pElementCopy);
        }
      }
      break;
      
    default:
      /* Unsupported type for copying */
      cypherValueDestroy(pCopy);
      sqlite3_free(pCopy);
      return NULL;
  }
  
  return pCopy;
}

/*
** Set string value.
** Makes a copy of the string using sqlite3_malloc().
*/
int cypherValueSetString(CypherValue *pValue, const char *zString) {
  if( !pValue ) return SQLITE_MISUSE;
  
  /* Free existing string if any */
  if( pValue->type == CYPHER_VALUE_STRING && pValue->u.zString ) {
    sqlite3_free(pValue->u.zString);
  }
  
  pValue->type = CYPHER_VALUE_STRING;
  
  if( zString ) {
    pValue->u.zString = sqlite3_mprintf("%s", zString);
    if( !pValue->u.zString ) return SQLITE_NOMEM;
  } else {
    pValue->u.zString = NULL;
  }
  
  return SQLITE_OK;
}

/*
** Get string representation of a Cypher value.
** Caller must sqlite3_free() the returned string.
*/
char *cypherValueToString(CypherValue *pValue) {
  char *zResult;
  
  if( !pValue ) return sqlite3_mprintf("(null)");
  
  switch( pValue->type ) {
    case CYPHER_VALUE_NULL:
      return sqlite3_mprintf("null");
      
    case CYPHER_VALUE_BOOLEAN:
      return sqlite3_mprintf(pValue->u.bBoolean ? "true" : "false");
      
    case CYPHER_VALUE_INTEGER:
      return sqlite3_mprintf("%lld", pValue->u.iInteger);
      
    case CYPHER_VALUE_FLOAT:
      return sqlite3_mprintf("%.6g", pValue->u.rFloat);
      
    case CYPHER_VALUE_STRING:
      return sqlite3_mprintf("\"%s\"", pValue->u.zString ? pValue->u.zString : "");
      
    case CYPHER_VALUE_NODE:
      return sqlite3_mprintf("Node(%lld)", pValue->u.iNodeId);
      
    case CYPHER_VALUE_RELATIONSHIP:
      return sqlite3_mprintf("Relationship(%lld)", pValue->u.iRelId);
      
    case CYPHER_VALUE_LIST:
      /* Simplified list representation */
      return sqlite3_mprintf("[List with %d elements]", pValue->u.list.nValues);
      
    case CYPHER_VALUE_MAP:
      /* Simplified map representation */
      return sqlite3_mprintf("{Map with %d pairs}", pValue->u.map.nPairs);
      
    default:
      return sqlite3_mprintf("Unknown(%d)", pValue->type);
  }
}

/*
** Get string representation of value type.
** Returns static string, do not free.
*/
const char *cypherValueTypeName(CypherValueType type) {
  switch( type ) {
    case CYPHER_VALUE_NULL:         return "NULL";
    case CYPHER_VALUE_BOOLEAN:      return "BOOLEAN";
    case CYPHER_VALUE_INTEGER:      return "INTEGER";
    case CYPHER_VALUE_FLOAT:        return "FLOAT";
    case CYPHER_VALUE_STRING:       return "STRING";
    case CYPHER_VALUE_NODE:         return "NODE";
    case CYPHER_VALUE_RELATIONSHIP: return "RELATIONSHIP";
    case CYPHER_VALUE_PATH:         return "PATH";
    case CYPHER_VALUE_LIST:         return "LIST";
    case CYPHER_VALUE_MAP:          return "MAP";
    default:                        return "UNKNOWN";
  }
}

/*
** Initialize a CypherValue to NULL.
*/
void cypherValueInit(CypherValue *pValue) {
  if( pValue ) {
    memset(pValue, 0, sizeof(CypherValue));
    pValue->type = CYPHER_VALUE_NULL;
  }
}

/*
** Set a CypherValue to NULL.
*/
void cypherValueSetNull(CypherValue *pValue) {
  if( pValue ) {
    cypherValueDestroy(pValue);
    pValue->type = CYPHER_VALUE_NULL;
    memset(&pValue->u, 0, sizeof(pValue->u));
  }
}

/*
** Set a CypherValue to an integer.
*/
void cypherValueSetInteger(CypherValue *pValue, sqlite3_int64 iValue) {
  if( pValue ) {
    cypherValueDestroy(pValue);
    pValue->type = CYPHER_VALUE_INTEGER;
    pValue->u.iInteger = iValue;
  }
}

/*
** Set a CypherValue to a float.
*/
void cypherValueSetFloat(CypherValue *pValue, double rValue) {
  if( pValue ) {
    cypherValueDestroy(pValue);
    pValue->type = CYPHER_VALUE_FLOAT;
    pValue->u.rFloat = rValue;
  }
}

/*
** Create a new result row.
** Returns NULL on allocation failure.
*/
CypherResult *cypherResultCreate(void) {
  CypherResult *pResult;
  
  pResult = sqlite3_malloc(sizeof(CypherResult));
  if( !pResult ) return NULL;
  
  memset(pResult, 0, sizeof(CypherResult));
  return pResult;
}

/*
** Destroy a result row and free all associated memory.
** Safe to call with NULL pointer.
*/
void cypherResultDestroy(CypherResult *pResult) {
  int i;
  
  if( !pResult ) return;
  
  /* Free column names */
  for( i = 0; i < pResult->nColumns; i++ ) {
    sqlite3_free(pResult->azColumnNames[i]);
    cypherValueDestroy(&pResult->aValues[i]);
  }
  sqlite3_free(pResult->azColumnNames);
  sqlite3_free(pResult->aValues);
  sqlite3_free(pResult);
}

/*
** Add a column to a result row.
** Returns SQLITE_OK on success, error code on failure.
*/
int cypherResultAddColumn(CypherResult *pResult, const char *zName, CypherValue *pValue) {
  char **azNew;
  CypherValue *aNew;
  
  if( !pResult || !zName || !pValue ) return SQLITE_MISUSE;
  
  /* Resize arrays if needed */
  if( pResult->nColumns >= pResult->nColumnsAlloc ) {
    int nNew = pResult->nColumnsAlloc ? pResult->nColumnsAlloc * 2 : 4;
    
    azNew = sqlite3_realloc(pResult->azColumnNames, nNew * sizeof(char*));
    if( !azNew ) return SQLITE_NOMEM;
    pResult->azColumnNames = azNew;
    
    aNew = sqlite3_realloc(pResult->aValues, nNew * sizeof(CypherValue));
    if( !aNew ) return SQLITE_NOMEM;
    pResult->aValues = aNew;
    
    pResult->nColumnsAlloc = nNew;
  }
  
  /* Add column */
  pResult->azColumnNames[pResult->nColumns] = sqlite3_mprintf("%s", zName);
  if( !pResult->azColumnNames[pResult->nColumns] ) return SQLITE_NOMEM;
  
  pResult->aValues[pResult->nColumns] = *cypherValueCopy(pValue);
  pResult->nColumns++;
  
  return SQLITE_OK;
}

/*
** Get JSON representation of a result row.
** Caller must sqlite3_free() the returned string.
*/
char *cypherResultToJson(CypherResult *pResult) {
  char *zResult, *zValue;
  int i, nResult = 0, nAlloc = 256;
  
  if( !pResult ) return sqlite3_mprintf("null");
  
  zResult = sqlite3_malloc(nAlloc);
  if( !zResult ) return NULL;
  
  /* Start JSON object */
  nResult = snprintf(zResult, nAlloc, "{");
  
  for( i = 0; i < pResult->nColumns; i++ ) {
    zValue = cypherValueToString(&pResult->aValues[i]);
    if( !zValue ) {
      sqlite3_free(zResult);
      return NULL;
    }
    
    /* Calculate space needed */
    int nNeeded = snprintf(NULL, 0, "%s\"%s\":%s", 
                          i > 0 ? "," : "", 
                          pResult->azColumnNames[i], 
                          zValue);
    
    /* Resize if needed */
    if( nResult + nNeeded + 2 >= nAlloc ) {
      nAlloc = (nResult + nNeeded + 256) * 2;
      char *zNew = sqlite3_realloc(zResult, nAlloc);
      if( !zNew ) {
        sqlite3_free(zResult);
        sqlite3_free(zValue);
        return NULL;
      }
      zResult = zNew;
    }
    
    /* Add column to JSON */
    nResult += snprintf(zResult + nResult, nAlloc - nResult,
                       "%s\"%s\":%s", 
                       i > 0 ? "," : "", 
                       pResult->azColumnNames[i], 
                       zValue);
    
    sqlite3_free(zValue);
  }
  
  /* Close JSON object */
  if( nResult + 2 < nAlloc ) {
    zResult[nResult++] = '}';
    zResult[nResult] = '\0';
  }
  
  return zResult;
}