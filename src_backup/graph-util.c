/*
** SQLite Graph Database Extension - Utility Functions
**
** This file implements core storage functions for graph nodes and edges.
** All functions follow SQLite patterns with proper error handling and
** memory management using sqlite3_malloc()/sqlite3_free().
**
** Node operations: Add, remove, get, update nodes with JSON properties
** Edge operations: Add, remove, get edges with weights and properties
*/

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1
#include "graph.h"
#include <string.h>
#include <assert.h>

/*
** Add a node to the graph. Return SQLITE_OK on success.
** Memory allocation: Uses sqlite3_malloc() for internal structures.
** Error conditions: Returns SQLITE_NOMEM on OOM, SQLITE_CONSTRAINT on 
**                   duplicate ID.
** I/O errors: Returns SQLITE_IOERR on database write failure.
*/
int graphAddNode(GraphVtab *pVtab, sqlite3_int64 iNodeId, 
                 const char *zProperties){
  GraphNode *pNode, *pNew;
  char *zProps;
  
  assert( pVtab!=0 );
  assert( zProperties!=0 );
  
  /* Check for duplicate node ID */
  pNode = graphFindNode(pVtab, iNodeId);
  if( pNode!=0 ){
    testcase( pNode!=0 );  /* Duplicate node ID */
    return SQLITE_CONSTRAINT;
  }
  
  /* Allocate new node structure */
  pNew = sqlite3_malloc(sizeof(*pNew));
  if( pNew==0 ){
    testcase( pNew==0 );  /* Out of memory */
    return SQLITE_NOMEM;
  }
  
  /* Copy properties string */
  zProps = sqlite3_mprintf("%s", zProperties);
  if( zProps==0 ){
    sqlite3_free(pNew);
    return SQLITE_NOMEM;
  }
  
  /* Initialize node */
  memset(pNew, 0, sizeof(*pNew));
  pNew->iNodeId = iNodeId;
  pNew->zProperties = zProps;
  
  /* Add to front of linked list */
  pNew->pNext = pVtab->pNodeList;
  pVtab->pNodeList = pNew;
  pVtab->nNodes++;
  
  return SQLITE_OK;
}

/*
** Remove a node from the graph along with all connected edges.
** Return SQLITE_OK on success, SQLITE_NOTFOUND if node doesn't exist.
** Memory management: Frees all associated memory with sqlite3_free().
*/
int graphRemoveNode(GraphVtab *pVtab, sqlite3_int64 iNodeId){
  GraphNode *pNode, *pPrevNode;
  GraphEdge *pEdge, *pPrevEdge, *pNextEdge;
  
  assert( pVtab!=0 );
  
  /* Find and remove node from list */
  pPrevNode = 0;
  pNode = pVtab->pNodeList;
  while( pNode && pNode->iNodeId!=iNodeId ){
    pPrevNode = pNode;
    pNode = pNode->pNext;
  }
  
  if( pNode==0 ){
    testcase( pNode==0 );  /* Node not found */
    return SQLITE_NOTFOUND;
  }
  
  /* Remove from linked list */
  if( pPrevNode ){
    pPrevNode->pNext = pNode->pNext;
  } else {
    pVtab->pNodeList = pNode->pNext;
  }
  
  /* Free node memory */
  sqlite3_free(pNode->zProperties);
  sqlite3_free(pNode);
  pVtab->nNodes--;
  
  /* Remove all edges connected to this node */
  pPrevEdge = 0;
  pEdge = pVtab->pEdgeList;
  while( pEdge ){
    pNextEdge = pEdge->pNext;
    
    if( pEdge->iFromId==iNodeId || pEdge->iToId==iNodeId ){
      /* Remove this edge */
      if( pPrevEdge ){
        pPrevEdge->pNext = pEdge->pNext;
      } else {
        pVtab->pEdgeList = pEdge->pNext;
      }
      
      sqlite3_free(pEdge->zProperties);
      sqlite3_free(pEdge);
      pVtab->nEdges--;
    } else {
      pPrevEdge = pEdge;
    }
    
    pEdge = pNextEdge;
  }
  
  return SQLITE_OK;
}

/*
** Retrieve node properties. Caller must sqlite3_free() the returned string.
** Returns SQLITE_OK and sets *pzProperties to allocated string.
** Returns SQLITE_NOTFOUND if node doesn't exist.
** Memory allocation: Allocates new string with sqlite3_malloc().
*/
int graphGetNode(GraphVtab *pVtab, sqlite3_int64 iNodeId, 
                 char **pzProperties){
  GraphNode *pNode;
  
  assert( pVtab!=0 );
  assert( pzProperties!=0 );
  
  pNode = graphFindNode(pVtab, iNodeId);
  if( pNode==0 ){
    *pzProperties = 0;
    return SQLITE_NOTFOUND;
  }
  
  /* Copy properties string for caller */
  *pzProperties = sqlite3_mprintf("%s", pNode->zProperties);
  if( *pzProperties==0 ){
    return SQLITE_NOMEM;
  }
  
  return SQLITE_OK;
}

/*
** Add edge between two nodes. Return SQLITE_OK on success.
** Both nodes must exist before adding edge.
** Error conditions: SQLITE_CONSTRAINT if nodes don't exist.
*/
int graphAddEdge(GraphVtab *pVtab, sqlite3_int64 iFromId, 
                 sqlite3_int64 iToId, double rWeight, 
                 const char *zProperties){
  GraphEdge *pEdge, *pNew;
  char *zProps;
  
  assert( pVtab!=0 );
  assert( zProperties!=0 );
  
  /* Verify both nodes exist */
  if( graphFindNode(pVtab, iFromId)==0 ){
    return SQLITE_CONSTRAINT;
  }
  if( graphFindNode(pVtab, iToId)==0 ){
    return SQLITE_CONSTRAINT;
  }
  
  /* Check for duplicate edge */
  pEdge = graphFindEdge(pVtab, iFromId, iToId);
  if( pEdge!=0 ){
    return SQLITE_CONSTRAINT;
  }
  
  /* Allocate new edge structure */
  pNew = sqlite3_malloc(sizeof(*pNew));
  if( pNew==0 ){
    return SQLITE_NOMEM;
  }
  
  /* Copy properties string */
  zProps = sqlite3_mprintf("%s", zProperties);
  if( zProps==0 ){
    sqlite3_free(pNew);
    return SQLITE_NOMEM;
  }
  
  /* Initialize edge */
  memset(pNew, 0, sizeof(*pNew));
  pNew->iFromId = iFromId;
  pNew->iToId = iToId;
  pNew->rWeight = rWeight;
  pNew->zProperties = zProps;
  
  /* Add to front of linked list */
  pNew->pNext = pVtab->pEdgeList;
  pVtab->pEdgeList = pNew;
  pVtab->nEdges++;
  
  return SQLITE_OK;
}

/*
** Remove specific edge between two nodes.
** Returns SQLITE_OK on success, SQLITE_NOTFOUND if edge doesn't exist.
*/
int graphRemoveEdge(GraphVtab *pVtab, sqlite3_int64 iFromId, 
                    sqlite3_int64 iToId){
  GraphEdge *pEdge, *pPrevEdge;
  
  assert( pVtab!=0 );
  
  /* Find edge in list */
  pPrevEdge = 0;
  pEdge = pVtab->pEdgeList;
  while( pEdge && (pEdge->iFromId!=iFromId || pEdge->iToId!=iToId) ){
    pPrevEdge = pEdge;
    pEdge = pEdge->pNext;
  }
  
  if( pEdge==0 ){
    return SQLITE_NOTFOUND;
  }
  
  /* Remove from linked list */
  if( pPrevEdge ){
    pPrevEdge->pNext = pEdge->pNext;
  } else {
    pVtab->pEdgeList = pEdge->pNext;
  }
  
  /* Free edge memory */
  sqlite3_free(pEdge->zProperties);
  sqlite3_free(pEdge);
  pVtab->nEdges--;
  
  return SQLITE_OK;
}

/*
** Utility functions for graph properties.
** These provide O(1) access to cached counts.
*/
int graphCountNodes(GraphVtab *pVtab){
  assert( pVtab!=0 );
  return pVtab->nNodes;
}

int graphCountEdges(GraphVtab *pVtab){
  assert( pVtab!=0 );
  return pVtab->nEdges;
}

/*
** Find a node by ID. Returns pointer to node or NULL if not found.
** Internal function for efficient node lookup.
** Time complexity: O(n) linear search through linked list.
*/
GraphNode *graphFindNode(GraphVtab *pVtab, sqlite3_int64 iNodeId){
  GraphNode *pNode;
  
  assert( pVtab!=0 );
  
  pNode = pVtab->pNodeList;
  while( pNode && pNode->iNodeId!=iNodeId ){
    pNode = pNode->pNext;
  }
  
  return pNode;
}

/*
** Update existing node properties.
** Returns SQLITE_OK on success, SQLITE_NOTFOUND if node doesn't exist.
** Memory management: Frees old properties, allocates new ones.
*/
int graphUpdateNode(GraphVtab *pVtab, sqlite3_int64 iNodeId, 
                    const char *zProperties){
  GraphNode *pNode;
  char *zProps;
  
  assert( pVtab!=0 );
  assert( zProperties!=0 );
  
  /* Find the node */
  pNode = graphFindNode(pVtab, iNodeId);
  if( pNode==0 ){
    return SQLITE_NOTFOUND;
  }
  
  /* Allocate new properties string */
  zProps = sqlite3_mprintf("%s", zProperties);
  if( zProps==0 ){
    return SQLITE_NOMEM;
  }
  
  /* Replace old properties */
  sqlite3_free(pNode->zProperties);
  pNode->zProperties = zProps;
  
  return SQLITE_OK;
}

/*
** Retrieve edge properties and weight.
** Caller must sqlite3_free() the returned properties string.
** Returns SQLITE_OK and sets outputs, SQLITE_NOTFOUND if edge doesn't exist.
*/
int graphGetEdge(GraphVtab *pVtab, sqlite3_int64 iFromId, 
                 sqlite3_int64 iToId, double *prWeight, 
                 char **pzProperties){
  GraphEdge *pEdge;
  
  assert( pVtab!=0 );
  assert( prWeight!=0 );
  assert( pzProperties!=0 );
  
  pEdge = graphFindEdge(pVtab, iFromId, iToId);
  if( pEdge==0 ){
    *prWeight = 0.0;
    *pzProperties = 0;
    return SQLITE_NOTFOUND;
  }
  
  /* Copy weight and properties for caller */
  *prWeight = pEdge->rWeight;
  *pzProperties = sqlite3_mprintf("%s", pEdge->zProperties);
  if( *pzProperties==0 ){
    return SQLITE_NOMEM;
  }
  
  return SQLITE_OK;
}

/*
** Find an edge by source and target IDs.
** Returns pointer to edge or NULL if not found.
** Time complexity: O(n) linear search through linked list.
*/
GraphEdge *graphFindEdge(GraphVtab *pVtab, sqlite3_int64 iFromId, 
                         sqlite3_int64 iToId){
  GraphEdge *pEdge;
  
  assert( pVtab!=0 );
  
  pEdge = pVtab->pEdgeList;
  while( pEdge && (pEdge->iFromId!=iFromId || pEdge->iToId!=iToId) ){
    pEdge = pEdge->pNext;
  }
  
  return pEdge;
}