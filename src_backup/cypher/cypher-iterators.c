/*
** SQLite Graph Database Extension - Iterator Implementations
**
** This file implements the core iterators for the Cypher execution engine
** using the Volcano iterator model. Each iterator implements the standard
** open/next/close interface for streaming query execution.
**
** Features:
** - AllNodesScan iterator for full table scans
** - LabelIndexScan iterator for label-based filtering
** - PropertyIndexScan iterator for property-based filtering
** - Filter iterator for predicate evaluation
** - Projection iterator for column selection
**
** Memory allocation: All functions use sqlite3_malloc()/sqlite3_free()
** Error handling: Functions return SQLite error codes
*/

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1
#include "cypher-executor.h"
#include <string.h>
#include <assert.h>

/*
** Helper function to extract a property value from a node's JSON properties.
** Returns a malloc'd string that caller must free, or NULL if not found.
*/
static char *graphGetNodeProperty(GraphNode *pNode, const char *zProperty) {
  if( !pNode || !pNode->zProperties || !zProperty ) return NULL;
  
  /* Simple property extraction - look for "property":"value" pattern */
  char *zSearch = sqlite3_mprintf("\"%s\":", zProperty);
  if( !zSearch ) return NULL;
  
  char *zPos = strstr(pNode->zProperties, zSearch);
  sqlite3_free(zSearch);
  
  if( !zPos ) return NULL;
  
  /* Skip past the property name and colon */
  zPos += strlen(zProperty) + 3;
  
  /* Skip whitespace */
  while( *zPos == ' ' || *zPos == '\t' ) zPos++;
  
  /* Handle quoted strings */
  if( *zPos == '"' ) {
    zPos++; /* Skip opening quote */
    char *zEnd = strchr(zPos, '"');
    if( zEnd ) {
      int nLen = zEnd - zPos;
      char *zResult = sqlite3_malloc(nLen + 1);
      if( zResult ) {
        memcpy(zResult, zPos, nLen);
        zResult[nLen] = '\0';
      }
      return zResult;
    }
  }
  
  return NULL;
}

/*
** Base iterator functions.
*/

/*
** Create an iterator from a physical plan node.
** Returns NULL on allocation failure or unsupported operator.
*/
CypherIterator *cypherIteratorCreate(PhysicalPlanNode *pPlan, ExecutionContext *pContext) {
  if( !pPlan || !pContext ) return NULL;
  
  switch( pPlan->type ) {
    case PHYSICAL_ALL_NODES_SCAN:
      return cypherAllNodesScanCreate(pPlan, pContext);
      
    case PHYSICAL_LABEL_INDEX_SCAN:
      return cypherLabelIndexScanCreate(pPlan, pContext);
      
    case PHYSICAL_PROPERTY_INDEX_SCAN:
      return cypherPropertyIndexScanCreate(pPlan, pContext);
      
    case PHYSICAL_FILTER:
      return cypherFilterCreate(pPlan, pContext);
      
    case PHYSICAL_PROJECTION:
      return cypherProjectionCreate(pPlan, pContext);
      
    case PHYSICAL_SORT:
      return cypherSortCreate(pPlan, pContext);
      
    case PHYSICAL_LIMIT:
      return cypherLimitCreate(pPlan, pContext);
      
    default:
      /* Unsupported operator type */
      return NULL;
  }
}

/*
** Destroy an iterator and free all associated memory.
** Safe to call with NULL pointer.
*/
void cypherIteratorDestroy(CypherIterator *pIterator) {
  int i;
  
  if( !pIterator ) return;
  
  /* Close iterator if still open */
  if( pIterator->bOpened ) {
    pIterator->xClose(pIterator);
  }
  
  /* Destroy child iterators */
  for( i = 0; i < pIterator->nChildren; i++ ) {
    cypherIteratorDestroy(pIterator->apChildren[i]);
  }
  sqlite3_free(pIterator->apChildren);
  
  /* Call iterator-specific destroy function */
  if( pIterator->xDestroy ) {
    pIterator->xDestroy(pIterator);
  }
  
  sqlite3_free(pIterator);
}

/*
** AllNodesScan iterator implementation.
** Scans all nodes in the graph sequentially.
*/

typedef struct AllNodesScanData {
  GraphNode *pCurrent;          /* Current node being scanned */
  GraphNode *pNext;             /* Next node to scan */
} AllNodesScanData;

static int allNodesScanOpen(CypherIterator *pIterator) {
  AllNodesScanData *pData = (AllNodesScanData*)pIterator->pIterData;
  GraphVtab *pGraph = pIterator->pContext->pGraph;
  
  if( !pGraph ) return SQLITE_ERROR;
  
  /* Start from the first node */
  pData->pCurrent = NULL;
  pData->pNext = pGraph->pNodeList;
  pIterator->bOpened = 1;
  pIterator->bEof = (pData->pNext == NULL);
  
  return SQLITE_OK;
}

static int allNodesScanNext(CypherIterator *pIterator, CypherResult *pResult) {
  AllNodesScanData *pData = (AllNodesScanData*)pIterator->pIterData;
  PhysicalPlanNode *pPlan = pIterator->pPlan;
  CypherValue nodeValue;
  int rc;
  
  if( pIterator->bEof || !pData->pNext ) {
    return SQLITE_DONE;
  }
  
  /* Get current node */
  pData->pCurrent = pData->pNext;
  pData->pNext = pData->pNext->pNext;
  
  /* Create node value */
  memset(&nodeValue, 0, sizeof(nodeValue));
  nodeValue.type = CYPHER_VALUE_NODE;
  nodeValue.u.iNodeId = pData->pCurrent->iNodeId;
  
  /* Add to result with alias */
  rc = cypherResultAddColumn(pResult, pPlan->zAlias ? pPlan->zAlias : "node", &nodeValue);
  if( rc != SQLITE_OK ) return rc;
  
  /* Update EOF status */
  pIterator->bEof = (pData->pNext == NULL);
  pIterator->nRowsProduced++;
  
  return SQLITE_OK;
}

static int allNodesScanClose(CypherIterator *pIterator) {
  pIterator->bOpened = 0;
  return SQLITE_OK;
}

static void allNodesScanDestroy(CypherIterator *pIterator) {
  sqlite3_free(pIterator->pIterData);
}

CypherIterator *cypherAllNodesScanCreate(PhysicalPlanNode *pPlan, ExecutionContext *pContext) {
  CypherIterator *pIterator;
  AllNodesScanData *pData;
  
  pIterator = sqlite3_malloc(sizeof(CypherIterator));
  if( !pIterator ) return NULL;
  
  pData = sqlite3_malloc(sizeof(AllNodesScanData));
  if( !pData ) {
    sqlite3_free(pIterator);
    return NULL;
  }
  
  memset(pIterator, 0, sizeof(CypherIterator));
  memset(pData, 0, sizeof(AllNodesScanData));
  
  /* Set up iterator */
  pIterator->xOpen = allNodesScanOpen;
  pIterator->xNext = allNodesScanNext;
  pIterator->xClose = allNodesScanClose;
  pIterator->xDestroy = allNodesScanDestroy;
  pIterator->pContext = pContext;
  pIterator->pPlan = pPlan;
  pIterator->pIterData = pData;
  
  return pIterator;
}

/*
** LabelIndexScan iterator implementation.
** Scans nodes with a specific label using the label index.
*/

typedef struct LabelIndexScanData {
  GraphNode *pCurrent;          /* Current node being scanned */
  GraphNode *pNext;             /* Next node to scan */
  const char *zLabel;           /* Label to filter by */
  int iLabelHash;               /* Hash of the label */
} LabelIndexScanData;

static int labelIndexScanOpen(CypherIterator *pIterator) {
  LabelIndexScanData *pData = (LabelIndexScanData*)pIterator->pIterData;
  PhysicalPlanNode *pPlan = pIterator->pPlan;
  GraphVtab *pGraph = pIterator->pContext->pGraph;
  
  if( !pGraph || !pPlan->zLabel ) return SQLITE_ERROR;
  
  pData->zLabel = pPlan->zLabel;
  pData->iLabelHash = graphHashString(pData->zLabel);
  
  /* Use label index if available */
  if( pGraph->pLabelIndex ) {
    /* Find nodes with this label using existing function */
    pData->pNext = graphFindNodesByLabel(pGraph, pData->zLabel);
  } else {
    /* Fall back to full scan with filtering */
    pData->pNext = pGraph->pNodeList;
  }
  
  pIterator->bOpened = 1;
  pIterator->bEof = (pData->pNext == NULL);
  
  return SQLITE_OK;
}

static int labelIndexScanNext(CypherIterator *pIterator, CypherResult *pResult) {
  LabelIndexScanData *pData = (LabelIndexScanData*)pIterator->pIterData;
  PhysicalPlanNode *pPlan = pIterator->pPlan;
  CypherValue nodeValue;
  int rc;
  
  if( pIterator->bEof ) return SQLITE_DONE;
  
  /* Find next node with matching label */
  while( pData->pNext ) {
    GraphNode *pNode = pData->pNext;
    pData->pNext = pData->pNext->pLabelNext ? pData->pNext->pLabelNext : pData->pNext->pNext;
    
    /* Check if node has the required label */
    if( graphNodeHasLabel(pIterator->pContext->pGraph, pNode->iNodeId, pData->zLabel) ) {
      pData->pCurrent = pNode;
      
      /* Create node value */
      memset(&nodeValue, 0, sizeof(nodeValue));
      nodeValue.type = CYPHER_VALUE_NODE;
      nodeValue.u.iNodeId = pNode->iNodeId;
      
      /* Add to result */
      rc = cypherResultAddColumn(pResult, pPlan->zAlias ? pPlan->zAlias : "node", &nodeValue);
      if( rc != SQLITE_OK ) return rc;
      
      pIterator->nRowsProduced++;
      return SQLITE_OK;
    }
  }
  
  /* No more matching nodes */
  pIterator->bEof = 1;
  return SQLITE_DONE;
}

static int labelIndexScanClose(CypherIterator *pIterator) {
  pIterator->bOpened = 0;
  return SQLITE_OK;
}

static void labelIndexScanDestroy(CypherIterator *pIterator) {
  sqlite3_free(pIterator->pIterData);
}

CypherIterator *cypherLabelIndexScanCreate(PhysicalPlanNode *pPlan, ExecutionContext *pContext) {
  CypherIterator *pIterator;
  LabelIndexScanData *pData;
  
  pIterator = sqlite3_malloc(sizeof(CypherIterator));
  if( !pIterator ) return NULL;
  
  pData = sqlite3_malloc(sizeof(LabelIndexScanData));
  if( !pData ) {
    sqlite3_free(pIterator);
    return NULL;
  }
  
  memset(pIterator, 0, sizeof(CypherIterator));
  memset(pData, 0, sizeof(LabelIndexScanData));
  
  /* Set up iterator */
  pIterator->xOpen = labelIndexScanOpen;
  pIterator->xNext = labelIndexScanNext;
  pIterator->xClose = labelIndexScanClose;
  pIterator->xDestroy = labelIndexScanDestroy;
  pIterator->pContext = pContext;
  pIterator->pPlan = pPlan;
  pIterator->pIterData = pData;
  
  return pIterator;
}

/*
** PropertyIndexScan iterator implementation.
** Scans nodes/relationships with specific property values using property indexes.
*/

typedef struct PropertyIndexScanData {
  GraphNode *pCurrent;          /* Current node being scanned */
  GraphNode *pNext;             /* Next node to scan */
  const char *zProperty;        /* Property to filter by */
  const char *zValue;           /* Value to match */
} PropertyIndexScanData;

static int propertyIndexScanOpen(CypherIterator *pIterator) {
  PropertyIndexScanData *pData = (PropertyIndexScanData*)pIterator->pIterData;
  PhysicalPlanNode *pPlan = pIterator->pPlan;
  GraphVtab *pGraph = pIterator->pContext->pGraph;
  
  if( !pGraph || !pPlan->zProperty ) return SQLITE_ERROR;
  
  pData->zProperty = pPlan->zProperty;
  pData->zValue = pPlan->zValue;
  
  /* For now, fall back to full scan with property filtering */
  /* TODO: Implement actual property index lookup */
  pData->pNext = pGraph->pNodeList;
  
  pIterator->bOpened = 1;
  pIterator->bEof = (pData->pNext == NULL);
  
  return SQLITE_OK;
}

static int propertyIndexScanNext(CypherIterator *pIterator, CypherResult *pResult) {
  PropertyIndexScanData *pData = (PropertyIndexScanData*)pIterator->pIterData;
  PhysicalPlanNode *pPlan = pIterator->pPlan;
  CypherValue nodeValue;
  int rc;
  
  if( pIterator->bEof ) return SQLITE_DONE;
  
  /* Find next node with matching property */
  while( pData->pNext ) {
    GraphNode *pNode = pData->pNext;
    pData->pNext = pData->pNext->pNext;
    
    /* Check if node has the required property value */
    char *zNodeValue = graphGetNodeProperty(pNode, pData->zProperty);
    if( zNodeValue && pData->zValue && strcmp(zNodeValue, pData->zValue) == 0 ) {
      pData->pCurrent = pNode;
      
      /* Create node value */
      memset(&nodeValue, 0, sizeof(nodeValue));
      nodeValue.type = CYPHER_VALUE_NODE;
      nodeValue.u.iNodeId = pNode->iNodeId;
      
      /* Add to result */
      rc = cypherResultAddColumn(pResult, pPlan->zAlias ? pPlan->zAlias : "node", &nodeValue);
      sqlite3_free(zNodeValue);
      if( rc != SQLITE_OK ) return rc;
      
      pIterator->nRowsProduced++;
      return SQLITE_OK;
    }
    sqlite3_free(zNodeValue);
  }
  
  /* No more matching nodes */
  pIterator->bEof = 1;
  return SQLITE_DONE;
}

static int propertyIndexScanClose(CypherIterator *pIterator) {
  pIterator->bOpened = 0;
  return SQLITE_OK;
}

static void propertyIndexScanDestroy(CypherIterator *pIterator) {
  sqlite3_free(pIterator->pIterData);
}

CypherIterator *cypherPropertyIndexScanCreate(PhysicalPlanNode *pPlan, ExecutionContext *pContext) {
  CypherIterator *pIterator;
  PropertyIndexScanData *pData;
  
  pIterator = sqlite3_malloc(sizeof(CypherIterator));
  if( !pIterator ) return NULL;
  
  pData = sqlite3_malloc(sizeof(PropertyIndexScanData));
  if( !pData ) {
    sqlite3_free(pIterator);
    return NULL;
  }
  
  memset(pIterator, 0, sizeof(CypherIterator));
  memset(pData, 0, sizeof(PropertyIndexScanData));
  
  /* Set up iterator */
  pIterator->xOpen = propertyIndexScanOpen;
  pIterator->xNext = propertyIndexScanNext;
  pIterator->xClose = propertyIndexScanClose;
  pIterator->xDestroy = propertyIndexScanDestroy;
  pIterator->pContext = pContext;
  pIterator->pPlan = pPlan;
  pIterator->pIterData = pData;
  
  return pIterator;
}

/*
** Stub implementations for other iterators.
** These will be implemented as needed.
*/

CypherIterator *cypherFilterCreate(PhysicalPlanNode *pPlan, ExecutionContext *pContext) {
  /* TODO: Implement filter iterator */
  return NULL;
}

CypherIterator *cypherProjectionCreate(PhysicalPlanNode *pPlan, ExecutionContext *pContext) {
  /* TODO: Implement projection iterator */
  return NULL;
}

CypherIterator *cypherSortCreate(PhysicalPlanNode *pPlan, ExecutionContext *pContext) {
  /* TODO: Implement sort iterator */
  return NULL;
}

CypherIterator *cypherLimitCreate(PhysicalPlanNode *pPlan, ExecutionContext *pContext) {
  /* TODO: Implement limit iterator */
  return NULL;
}