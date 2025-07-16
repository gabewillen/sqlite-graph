/*
** SQLite Graph Database Extension - Advanced Graph Algorithms
**
** This file implements advanced graph algorithms including betweenness
** centrality, closeness centrality, topological sort, and strongly
** connected components.
**
** Algorithms follow SQLite patterns with proper memory management.
*/

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1
#include "graph.h"
#include <float.h>
#include <string.h>
#include <stdlib.h>

/*
** Stack structure for path reconstruction in Brandes' algorithm.
** Each element tracks a node and its predecessors.
*/
typedef struct BrandesStack BrandesStack;
struct BrandesStack {
  sqlite3_int64 iNodeId;
  BrandesStack *pNext;
};

/*
** Predecessor list for Brandes' algorithm.
** Tracks all predecessors of a node in shortest paths.
*/
typedef struct PredecessorList PredecessorList;
struct PredecessorList {
  sqlite3_int64 iPredId;
  PredecessorList *pNext;
};

/*
** Brandes algorithm state for a single source node.
** Tracks distances, predecessors, and dependency values.
*/
typedef struct BrandesState BrandesState;
struct BrandesState {
  sqlite3_int64 iSourceId;
  double *aDistance;              /* Distance from source to each node */
  int *aSigma;                    /* Number of shortest paths */
  double *aDelta;                 /* Dependency values */
  PredecessorList **aPred;        /* Predecessors for each node */
  int nNodes;                     /* Number of nodes */
};

/*
** Node index mapping for Brandes' algorithm.
** Maps node IDs to array indices for O(1) access.
*/
typedef struct NodeIndexMap NodeIndexMap;
struct NodeIndexMap {
  sqlite3_int64 *aNodeIds;        /* Array of node IDs */
  int nNodes;                     /* Number of nodes */
};

/*
** Create node index mapping for efficient array access.
** Maps GraphNode IDs to 0-based array indices.
*/
static NodeIndexMap *createNodeIndexMap(GraphVtab *pVtab){
  NodeIndexMap *pMap;
  GraphNode *pNode;
  int i = 0;
  
  pMap = sqlite3_malloc(sizeof(*pMap));
  if( pMap==0 ) return 0;
  
  pMap->nNodes = pVtab->nNodes;
  pMap->aNodeIds = sqlite3_malloc(sizeof(sqlite3_int64) * pMap->nNodes);
  if( pMap->aNodeIds==0 ){
    sqlite3_free(pMap);
    return 0;
  }
  
  /* Build mapping */
  for( pNode = pVtab->pNodeList; pNode; pNode = pNode->pNext ){
    pMap->aNodeIds[i++] = pNode->iNodeId;
  }
  
  return pMap;
}

/*
** Get array index for a node ID.
** Returns -1 if not found.
*/
static int getNodeIndex(NodeIndexMap *pMap, sqlite3_int64 iNodeId){
  int i;
  for( i = 0; i < pMap->nNodes; i++ ){
    if( pMap->aNodeIds[i] == iNodeId ) return i;
  }
  return -1;
}

/*
** Free node index mapping.
*/
static void freeNodeIndexMap(NodeIndexMap *pMap){
  if( pMap ){
    sqlite3_free(pMap->aNodeIds);
    sqlite3_free(pMap);
  }
}

/*
** Initialize Brandes algorithm state for a source node.
*/
static BrandesState *brandesInitState(GraphVtab *pVtab, 
                                     sqlite3_int64 iSourceId,
                                     NodeIndexMap *pMap){
  BrandesState *pState;
  int nNodes = pVtab->nNodes;
  int i;
  
  pState = sqlite3_malloc(sizeof(*pState));
  if( pState==0 ) return 0;
  memset(pState, 0, sizeof(*pState));
  
  pState->iSourceId = iSourceId;
  pState->nNodes = nNodes;
  
  /* Allocate arrays */
  pState->aDistance = sqlite3_malloc(sizeof(double) * nNodes);
  pState->aSigma = sqlite3_malloc(sizeof(int) * nNodes);
  pState->aDelta = sqlite3_malloc(sizeof(double) * nNodes);
  pState->aPred = sqlite3_malloc(sizeof(PredecessorList*) * nNodes);
  
  if( pState->aDistance==0 || pState->aSigma==0 || 
      pState->aDelta==0 || pState->aPred==0 ){
    sqlite3_free(pState->aDistance);
    sqlite3_free(pState->aSigma);
    sqlite3_free(pState->aDelta);
    sqlite3_free(pState->aPred);
    sqlite3_free(pState);
    return 0;
  }
  
  /* Initialize arrays */
  for( i = 0; i < nNodes; i++ ){
    pState->aDistance[i] = DBL_MAX;
    pState->aSigma[i] = 0;
    pState->aDelta[i] = 0.0;
    pState->aPred[i] = 0;
  }
  
  /* Source node initialization */
  i = getNodeIndex(pMap, iSourceId);
  if( i >= 0 ){
    pState->aDistance[i] = 0.0;
    pState->aSigma[i] = 1;
  }
  
  return pState;
}

/*
** Free Brandes algorithm state.
*/
static void brandesFreeState(BrandesState *pState){
  int i;
  if( pState ){
    /* Free predecessor lists */
    if( pState->aPred ){
      for( i = 0; i < pState->nNodes; i++ ){
        PredecessorList *pPred = pState->aPred[i];
        while( pPred ){
          PredecessorList *pNext = pPred->pNext;
          sqlite3_free(pPred);
          pPred = pNext;
        }
      }
    }
    sqlite3_free(pState->aDistance);
    sqlite3_free(pState->aSigma);
    sqlite3_free(pState->aDelta);
    sqlite3_free(pState->aPred);
    sqlite3_free(pState);
  }
}

/*
** Add predecessor to node's predecessor list.
*/
static int brandesAddPredecessor(BrandesState *pState, int iNodeIdx, 
                                sqlite3_int64 iPredId){
  PredecessorList *pPred;
  
  pPred = sqlite3_malloc(sizeof(*pPred));
  if( pPred==0 ) return SQLITE_NOMEM;
  
  pPred->iPredId = iPredId;
  pPred->pNext = pState->aPred[iNodeIdx];
  pState->aPred[iNodeIdx] = pPred;
  
  return SQLITE_OK;
}

/*
** Queue structure for BFS in Brandes' algorithm.
*/
typedef struct BrandesQueue BrandesQueue;
struct BrandesQueue {
  int *aItems;
  int nFront;
  int nRear;
  int nCapacity;
};

/*
** Create queue for Brandes BFS.
*/
static BrandesQueue *brandesQueueCreate(int nCapacity){
  BrandesQueue *pQueue;
  
  pQueue = sqlite3_malloc(sizeof(*pQueue));
  if( pQueue==0 ) return 0;
  
  pQueue->aItems = sqlite3_malloc(sizeof(int) * nCapacity);
  if( pQueue->aItems==0 ){
    sqlite3_free(pQueue);
    return 0;
  }
  
  pQueue->nFront = 0;
  pQueue->nRear = 0;
  pQueue->nCapacity = nCapacity;
  
  return pQueue;
}

/*
** Enqueue item in Brandes queue.
*/
static void brandesQueuePush(BrandesQueue *pQueue, int iItem){
  pQueue->aItems[pQueue->nRear] = iItem;
  pQueue->nRear = (pQueue->nRear + 1) % pQueue->nCapacity;
}

/*
** Dequeue item from Brandes queue.
*/
static int brandesQueuePop(BrandesQueue *pQueue){
  int iItem = pQueue->aItems[pQueue->nFront];
  pQueue->nFront = (pQueue->nFront + 1) % pQueue->nCapacity;
  return iItem;
}

/*
** Check if queue is empty.
*/
static int brandesQueueIsEmpty(BrandesQueue *pQueue){
  return pQueue->nFront == pQueue->nRear;
}

/*
** Free Brandes queue.
*/
static void brandesQueueFree(BrandesQueue *pQueue){
  if( pQueue ){
    sqlite3_free(pQueue->aItems);
    sqlite3_free(pQueue);
  }
}

/*
** Calculate betweenness centrality using Brandes' algorithm.
** Returns SQLITE_OK and sets *pzResults to JSON object with scores.
** Algorithm: O(V*E) for unweighted graphs, O(V*E + V²log V) for weighted.
*/
int graphBetweennessCentrality(GraphVtab *pVtab, char **pzResults){
  NodeIndexMap *pMap = 0;
  double *aCentrality = 0;
  GraphNode *pNode;
  char *zResults = 0;
  int rc = SQLITE_OK;
  int nNodes = pVtab->nNodes;
  int i;
  
  /* Empty graph case */
  if( nNodes == 0 ){
    *pzResults = sqlite3_mprintf("{}");
    return *pzResults ? SQLITE_OK : SQLITE_NOMEM;
  }
  
  /* Create node index mapping */
  pMap = createNodeIndexMap(pVtab);
  if( pMap==0 ){
    return SQLITE_NOMEM;
  }
  
  /* Allocate centrality array */
  aCentrality = sqlite3_malloc(sizeof(double) * nNodes);
  if( aCentrality==0 ){
    freeNodeIndexMap(pMap);
    return SQLITE_NOMEM;
  }
  memset(aCentrality, 0, sizeof(double) * nNodes);
  
  /* Run Brandes' algorithm from each node */
  for( pNode = pVtab->pNodeList; pNode; pNode = pNode->pNext ){
    BrandesState *pState;
    BrandesQueue *pQueue;
    BrandesStack *pStack = 0;
    GraphEdge *pEdge;
    int iSrcIdx;
    
    /* Initialize state for this source */
    pState = brandesInitState(pVtab, pNode->iNodeId, pMap);
    if( pState==0 ){
      rc = SQLITE_NOMEM;
      goto betweenness_cleanup;
    }
    
    /* Create queue for BFS */
    pQueue = brandesQueueCreate(nNodes + 1);
    if( pQueue==0 ){
      brandesFreeState(pState);
      rc = SQLITE_NOMEM;
      goto betweenness_cleanup;
    }
    
    /* Get source index */
    iSrcIdx = getNodeIndex(pMap, pNode->iNodeId);
    if( iSrcIdx < 0 ){
      brandesQueueFree(pQueue);
      brandesFreeState(pState);
      continue;
    }
    
    /* BFS phase - compute shortest paths */
    brandesQueuePush(pQueue, iSrcIdx);
    
    while( !brandesQueueIsEmpty(pQueue) ){
      int iCurIdx = brandesQueuePop(pQueue);
      sqlite3_int64 iCurId = pMap->aNodeIds[iCurIdx];
      
      /* Push to stack for later processing */
      BrandesStack *pStackNode = sqlite3_malloc(sizeof(*pStackNode));
      if( pStackNode ){
        pStackNode->iNodeId = iCurId;
        pStackNode->pNext = pStack;
        pStack = pStackNode;
      }
      
      /* Process all edges from current node */
      for( pEdge = pVtab->pEdgeList; pEdge; pEdge = pEdge->pNext ){
        int iNextIdx;
        if( pEdge->iFromId != iCurId ) continue;
        
        iNextIdx = getNodeIndex(pMap, pEdge->iToId);
        if( iNextIdx < 0 ) continue;
        
        /* First time reaching this node */
        if( pState->aDistance[iNextIdx] == DBL_MAX ){
          pState->aDistance[iNextIdx] = pState->aDistance[iCurIdx] + 1;
          brandesQueuePush(pQueue, iNextIdx);
        }
        
        /* On shortest path */
        if( pState->aDistance[iNextIdx] == pState->aDistance[iCurIdx] + 1 ){
          pState->aSigma[iNextIdx] += pState->aSigma[iCurIdx];
          brandesAddPredecessor(pState, iNextIdx, iCurId);
        }
      }
    }
    
    /* Accumulation phase - compute dependencies */
    while( pStack ){
      BrandesStack *pNext = pStack->pNext;
      int iNodeIdx = getNodeIndex(pMap, pStack->iNodeId);
      PredecessorList *pPred;
      
      if( iNodeIdx >= 0 ){
        /* Process all predecessors */
        for( pPred = pState->aPred[iNodeIdx]; pPred; pPred = pPred->pNext ){
          int iPredIdx = getNodeIndex(pMap, pPred->iPredId);
          if( iPredIdx >= 0 && pState->aSigma[iNodeIdx] > 0 ){
            double factor = (double)pState->aSigma[iPredIdx] / 
                           pState->aSigma[iNodeIdx];
            pState->aDelta[iPredIdx] += factor * (1.0 + pState->aDelta[iNodeIdx]);
          }
        }
        
        /* Update centrality (excluding source) */
        if( iNodeIdx != iSrcIdx ){
          aCentrality[iNodeIdx] += pState->aDelta[iNodeIdx];
        }
      }
      
      sqlite3_free(pStack);
      pStack = pNext;
    }
    
    brandesQueueFree(pQueue);
    brandesFreeState(pState);
  }
  
  /* Build JSON result */
  zResults = sqlite3_mprintf("{");
  for( i = 0; i < nNodes && zResults; i++ ){
    char *zOld = zResults;
    zResults = sqlite3_mprintf("%s%s\"%lld\":%.6f", zOld,
                              i > 0 ? "," : "",
                              pMap->aNodeIds[i],
                              aCentrality[i] / 2.0);  /* Undirected normalization */
    sqlite3_free(zOld);
  }
  
  if( zResults ){
    char *zOld = zResults;
    zResults = sqlite3_mprintf("%s}", zOld);
    sqlite3_free(zOld);
  }
  
  if( zResults==0 ){
    rc = SQLITE_NOMEM;
  } else {
    *pzResults = zResults;
  }
  
betweenness_cleanup:
  sqlite3_free(aCentrality);
  freeNodeIndexMap(pMap);
  return rc;
}

/*
** Calculate closeness centrality for all nodes.
** Returns SQLITE_OK and sets *pzResults to JSON object with scores.
** Closeness = (n-1) / sum of distances to all other nodes
*/
int graphClosenessCentrality(GraphVtab *pVtab, char **pzResults){
  NodeIndexMap *pMap = 0;
  double *aCentrality = 0;
  GraphNode *pNode;
  char *zResults = 0;
  int rc = SQLITE_OK;
  int nNodes = pVtab->nNodes;
  int i;
  
  /* Empty graph case */
  if( nNodes == 0 ){
    *pzResults = sqlite3_mprintf("{}");
    return *pzResults ? SQLITE_OK : SQLITE_NOMEM;
  }
  
  /* Create node index mapping */
  pMap = createNodeIndexMap(pVtab);
  if( pMap==0 ){
    return SQLITE_NOMEM;
  }
  
  /* Allocate centrality array */
  aCentrality = sqlite3_malloc(sizeof(double) * nNodes);
  if( aCentrality==0 ){
    freeNodeIndexMap(pMap);
    return SQLITE_NOMEM;
  }
  
  /* Calculate closeness for each node */
  for( i = 0; i < nNodes; i++ ){
    char *zPath = 0;
    double rTotalDistance = 0.0;
    int nReachable = 0;
    
    /* Use Dijkstra to find all distances from this node */
    extern int graphDijkstra(GraphVtab*, sqlite3_int64, sqlite3_int64, 
                           char**, double*);
    rc = graphDijkstra(pVtab, pMap->aNodeIds[i], -1, &zPath, NULL);
    
    if( rc == SQLITE_OK && zPath ){
      /* Parse distance results */
      /* Format: {"node_id": distance, ...} */
      const char *p = zPath;
      while( *p ){
        if( *p == ':' ){
          p++;
          while( *p == ' ' ) p++;
          double dist = 0.0;
          char *endptr;
          dist = strtod(p, &endptr);
          if( endptr > p && dist < DBL_MAX ){
            rTotalDistance += dist;
            nReachable++;
          }
          p = endptr;
        } else {
          p++;
        }
      }
    }
    
    sqlite3_free(zPath);
    
    /* Calculate closeness centrality */
    if( nReachable > 0 && rTotalDistance > 0 ){
      aCentrality[i] = (double)(nReachable) / rTotalDistance;
    } else {
      aCentrality[i] = 0.0;
    }
  }
  
  /* Build JSON result */
  zResults = sqlite3_mprintf("{");
  for( i = 0; i < nNodes && zResults; i++ ){
    char *zOld = zResults;
    zResults = sqlite3_mprintf("%s%s\"%lld\":%.6f", zOld,
                              i > 0 ? "," : "",
                              pMap->aNodeIds[i],
                              aCentrality[i]);
    sqlite3_free(zOld);
  }
  
  if( zResults ){
    char *zOld = zResults;
    zResults = sqlite3_mprintf("%s}", zOld);
    sqlite3_free(zOld);
  }
  
  if( zResults==0 ){
    rc = SQLITE_NOMEM;
  } else {
    *pzResults = zResults;
  }
  
  sqlite3_free(aCentrality);
  freeNodeIndexMap(pMap);
  return rc;
}

/*
** Topological sort using Kahn's algorithm.
** Returns SQLITE_OK and sets *pzOrder to JSON array of node IDs.
** Returns SQLITE_CONSTRAINT if graph has cycles.
*/
int graphTopologicalSort(GraphVtab *pVtab, char **pzOrder){
  int *aInDegree = 0;
  int *aQueue = 0;
  char *zOrder = 0;
  NodeIndexMap *pMap = 0;
  GraphEdge *pEdge;
  int nProcessed = 0;
  int nFront = 0, nRear = 0;
  int rc = SQLITE_OK;
  int i;
  
  /* Empty graph case */
  if( pVtab->nNodes == 0 ){
    *pzOrder = sqlite3_mprintf("[]");
    return *pzOrder ? SQLITE_OK : SQLITE_NOMEM;
  }
  
  /* Create node index mapping */
  pMap = createNodeIndexMap(pVtab);
  if( pMap==0 ){
    return SQLITE_NOMEM;
  }
  
  /* Allocate arrays */
  aInDegree = sqlite3_malloc(sizeof(int) * pVtab->nNodes);
  aQueue = sqlite3_malloc(sizeof(int) * pVtab->nNodes);
  
  if( aInDegree==0 || aQueue==0 ){
    rc = SQLITE_NOMEM;
    goto topo_cleanup;
  }
  
  /* Calculate in-degrees */
  memset(aInDegree, 0, sizeof(int) * pVtab->nNodes);
  for( pEdge = pVtab->pEdgeList; pEdge; pEdge = pEdge->pNext ){
    int iToIdx = getNodeIndex(pMap, pEdge->iToId);
    if( iToIdx >= 0 ){
      aInDegree[iToIdx]++;
    }
  }
  
  /* Queue nodes with in-degree 0 */
  for( i = 0; i < pVtab->nNodes; i++ ){
    if( aInDegree[i] == 0 ){
      aQueue[nRear++] = i;
    }
  }
  
  /* Process queue */
  zOrder = sqlite3_mprintf("[");
  while( nFront < nRear && zOrder ){
    int iCurIdx = aQueue[nFront++];
    sqlite3_int64 iCurId = pMap->aNodeIds[iCurIdx];
    
    /* Add to result */
    char *zOld = zOrder;
    zOrder = sqlite3_mprintf("%s%s%lld", zOld,
                            nProcessed > 0 ? "," : "",
                            iCurId);
    sqlite3_free(zOld);
    nProcessed++;
    
    /* Update neighbors */
    for( pEdge = pVtab->pEdgeList; pEdge; pEdge = pEdge->pNext ){
      if( pEdge->iFromId == iCurId ){
        int iToIdx = getNodeIndex(pMap, pEdge->iToId);
        if( iToIdx >= 0 ){
          aInDegree[iToIdx]--;
          if( aInDegree[iToIdx] == 0 ){
            aQueue[nRear++] = iToIdx;
          }
        }
      }
    }
  }
  
  /* Check for cycles */
  if( nProcessed < pVtab->nNodes ){
    sqlite3_free(zOrder);
    rc = SQLITE_CONSTRAINT;
    *pzOrder = sqlite3_mprintf("Graph contains cycles");
  } else if( zOrder ){
    char *zOld = zOrder;
    zOrder = sqlite3_mprintf("%s]", zOld);
    sqlite3_free(zOld);
    *pzOrder = zOrder;
    if( zOrder==0 ) rc = SQLITE_NOMEM;
  } else {
    rc = SQLITE_NOMEM;
  }
  
topo_cleanup:
  sqlite3_free(aInDegree);
  sqlite3_free(aQueue);
  freeNodeIndexMap(pMap);
  return rc;
}

/*
** Detect cycles in directed graph using DFS.
** Returns 1 if cycle exists, 0 otherwise.
*/
int graphHasCycle(GraphVtab *pVtab){
  NodeIndexMap *pMap;
  int *aColor;  /* 0=white, 1=gray, 2=black */
  GraphNode *pNode;
  int hasCycle = 0;
  
  if( pVtab->nNodes == 0 ) return 0;
  
  pMap = createNodeIndexMap(pVtab);
  if( pMap==0 ) return 0;
  
  aColor = sqlite3_malloc(sizeof(int) * pVtab->nNodes);
  if( aColor==0 ){
    freeNodeIndexMap(pMap);
    return 0;
  }
  memset(aColor, 0, sizeof(int) * pVtab->nNodes);
  
  /* Helper function for DFS */
  int dfsVisit(int iNodeIdx){
    GraphEdge *pEdge;
    sqlite3_int64 iNodeId = pMap->aNodeIds[iNodeIdx];
    
    aColor[iNodeIdx] = 1;  /* Gray */
    
    for( pEdge = pVtab->pEdgeList; pEdge; pEdge = pEdge->pNext ){
      if( pEdge->iFromId == iNodeId ){
        int iToIdx = getNodeIndex(pMap, pEdge->iToId);
        if( iToIdx >= 0 ){
          if( aColor[iToIdx] == 1 ){
            /* Back edge - cycle detected */
            return 1;
          }
          if( aColor[iToIdx] == 0 ){
            if( dfsVisit(iToIdx) ) return 1;
          }
        }
      }
    }
    
    aColor[iNodeIdx] = 2;  /* Black */
    return 0;
  }
  
  /* Run DFS from each unvisited node */
  for( int i = 0; i < pVtab->nNodes && !hasCycle; i++ ){
    if( aColor[i] == 0 ){
      hasCycle = dfsVisit(i);
    }
  }
  
  sqlite3_free(aColor);
  freeNodeIndexMap(pMap);
  return hasCycle;
}

/*
** Find connected components using DFS.
** Returns SQLITE_OK and sets *pzComponents to JSON object.
** Format: {"component_id": [node_ids...], ...}
*/
int graphConnectedComponents(GraphVtab *pVtab, char **pzComponents){
  NodeIndexMap *pMap = 0;
  int *aComponent = 0;
  char *zComponents = 0;
  int nComponents = 0;
  int rc = SQLITE_OK;
  int i;
  
  /* Empty graph case */
  if( pVtab->nNodes == 0 ){
    *pzComponents = sqlite3_mprintf("{}");
    return *pzComponents ? SQLITE_OK : SQLITE_NOMEM;
  }
  
  pMap = createNodeIndexMap(pVtab);
  if( pMap==0 ) return SQLITE_NOMEM;
  
  aComponent = sqlite3_malloc(sizeof(int) * pVtab->nNodes);
  if( aComponent==0 ){
    freeNodeIndexMap(pMap);
    return SQLITE_NOMEM;
  }
  
  /* Initialize all nodes as unvisited (-1) */
  for( i = 0; i < pVtab->nNodes; i++ ){
    aComponent[i] = -1;
  }
  
  /* Find components using DFS */
  for( i = 0; i < pVtab->nNodes; i++ ){
    if( aComponent[i] == -1 ){
      /* Start new component */
      int *aStack = sqlite3_malloc(sizeof(int) * pVtab->nNodes);
      int nStack = 0;
      
      if( aStack==0 ){
        rc = SQLITE_NOMEM;
        goto comp_cleanup;
      }
      
      /* DFS to mark component */
      aStack[nStack++] = i;
      aComponent[i] = nComponents;
      
      while( nStack > 0 ){
        int iCurIdx = aStack[--nStack];
        sqlite3_int64 iCurId = pMap->aNodeIds[iCurIdx];
        GraphEdge *pEdge;
        
        /* Process all edges (treating as undirected) */
        for( pEdge = pVtab->pEdgeList; pEdge; pEdge = pEdge->pNext ){
          int iNextIdx = -1;
          
          if( pEdge->iFromId == iCurId ){
            iNextIdx = getNodeIndex(pMap, pEdge->iToId);
          } else if( pEdge->iToId == iCurId ){
            iNextIdx = getNodeIndex(pMap, pEdge->iFromId);
          }
          
          if( iNextIdx >= 0 && aComponent[iNextIdx] == -1 ){
            aComponent[iNextIdx] = nComponents;
            aStack[nStack++] = iNextIdx;
          }
        }
      }
      
      sqlite3_free(aStack);
      nComponents++;
    }
  }
  
  /* Build JSON result */
  zComponents = sqlite3_mprintf("{");
  for( int comp = 0; comp < nComponents && zComponents; comp++ ){
    char *zOld = zComponents;
    zComponents = sqlite3_mprintf("%s%s\"%d\":[", zOld,
                                 comp > 0 ? "," : "", comp);
    sqlite3_free(zOld);
    
    int first = 1;
    for( i = 0; i < pVtab->nNodes && zComponents; i++ ){
      if( aComponent[i] == comp ){
        zOld = zComponents;
        zComponents = sqlite3_mprintf("%s%s%lld", zOld,
                                     first ? "" : ",",
                                     pMap->aNodeIds[i]);
        sqlite3_free(zOld);
        first = 0;
      }
    }
    
    if( zComponents ){
      zOld = zComponents;
      zComponents = sqlite3_mprintf("%s]", zOld);
      sqlite3_free(zOld);
    }
  }
  
  if( zComponents ){
    char *zOld = zComponents;
    zComponents = sqlite3_mprintf("%s}", zOld);
    sqlite3_free(zOld);
  }
  
  if( zComponents==0 ){
    rc = SQLITE_NOMEM;
  } else {
    *pzComponents = zComponents;
  }
  
comp_cleanup:
  sqlite3_free(aComponent);
  freeNodeIndexMap(pMap);
  return rc;
}

/*
** Stack entry for Tarjan's algorithm.
*/
typedef struct TarjanNode TarjanNode;
struct TarjanNode {
  int iIndex;
  TarjanNode *pNext;
};

/*
** Tarjan's algorithm state.
*/
typedef struct TarjanState TarjanState;
struct TarjanState {
  int *aIndex;      /* DFS index for each node */
  int *aLowLink;    /* Lowest index reachable */
  int *aOnStack;    /* Is node on stack? */
  TarjanNode *pStack;  /* Stack of nodes */
  int nIndex;       /* Current DFS index */
  int nSCC;         /* Number of SCCs found */
  char **azSCC;     /* Array of SCC strings */
  NodeIndexMap *pMap;
};

/*
** Tarjan's strongconnect function.
*/
static void tarjanStrongConnect(GraphVtab *pVtab, TarjanState *pState, 
                               int iNodeIdx){
  GraphEdge *pEdge;
  TarjanNode *pStackNode;
  sqlite3_int64 iNodeId = pState->pMap->aNodeIds[iNodeIdx];
  
  /* Set index and lowlink */
  pState->aIndex[iNodeIdx] = pState->nIndex;
  pState->aLowLink[iNodeIdx] = pState->nIndex;
  pState->nIndex++;
  
  /* Push to stack */
  pStackNode = sqlite3_malloc(sizeof(*pStackNode));
  if( pStackNode ){
    pStackNode->iIndex = iNodeIdx;
    pStackNode->pNext = pState->pStack;
    pState->pStack = pStackNode;
    pState->aOnStack[iNodeIdx] = 1;
  }
  
  /* Process edges */
  for( pEdge = pVtab->pEdgeList; pEdge; pEdge = pEdge->pNext ){
    if( pEdge->iFromId == iNodeId ){
      int iToIdx = getNodeIndex(pState->pMap, pEdge->iToId);
      if( iToIdx >= 0 ){
        if( pState->aIndex[iToIdx] == -1 ){
          /* Not visited */
          tarjanStrongConnect(pVtab, pState, iToIdx);
          if( pState->aLowLink[iToIdx] < pState->aLowLink[iNodeIdx] ){
            pState->aLowLink[iNodeIdx] = pState->aLowLink[iToIdx];
          }
        } else if( pState->aOnStack[iToIdx] ){
          /* On stack - part of current SCC */
          if( pState->aIndex[iToIdx] < pState->aLowLink[iNodeIdx] ){
            pState->aLowLink[iNodeIdx] = pState->aIndex[iToIdx];
          }
        }
      }
    }
  }
  
  /* Root of SCC? */
  if( pState->aLowLink[iNodeIdx] == pState->aIndex[iNodeIdx] ){
    char *zSCC = sqlite3_mprintf("[");
    int first = 1;
    
    /* Pop SCC from stack */
    while( pState->pStack ){
      TarjanNode *pNext = pState->pStack->pNext;
      int iIdx = pState->pStack->iIndex;
      
      if( zSCC ){
        char *zOld = zSCC;
        zSCC = sqlite3_mprintf("%s%s%lld", zOld,
                              first ? "" : ",",
                              pState->pMap->aNodeIds[iIdx]);
        sqlite3_free(zOld);
        first = 0;
      }
      
      pState->aOnStack[iIdx] = 0;
      sqlite3_free(pState->pStack);
      pState->pStack = pNext;
      
      if( iIdx == iNodeIdx ) break;
    }
    
    if( zSCC ){
      char *zOld = zSCC;
      zSCC = sqlite3_mprintf("%s]", zOld);
      sqlite3_free(zOld);
      
      /* Store SCC */
      char **azNew = sqlite3_realloc(pState->azSCC, 
                                    sizeof(char*) * (pState->nSCC + 1));
      if( azNew ){
        pState->azSCC = azNew;
        pState->azSCC[pState->nSCC++] = zSCC;
      } else {
        sqlite3_free(zSCC);
      }
    }
  }
}

/*
** Find strongly connected components using Tarjan's algorithm.
** Returns SQLITE_OK and sets *pzSCC to JSON array of components.
*/
int graphStronglyConnectedComponents(GraphVtab *pVtab, char **pzSCC){
  TarjanState state;
  char *zResult = 0;
  int rc = SQLITE_OK;
  int i;
  
  /* Empty graph case */
  if( pVtab->nNodes == 0 ){
    *pzSCC = sqlite3_mprintf("[]");
    return *pzSCC ? SQLITE_OK : SQLITE_NOMEM;
  }
  
  /* Initialize state */
  memset(&state, 0, sizeof(state));
  state.pMap = createNodeIndexMap(pVtab);
  if( state.pMap==0 ) return SQLITE_NOMEM;
  
  state.aIndex = sqlite3_malloc(sizeof(int) * pVtab->nNodes);
  state.aLowLink = sqlite3_malloc(sizeof(int) * pVtab->nNodes);
  state.aOnStack = sqlite3_malloc(sizeof(int) * pVtab->nNodes);
  
  if( !state.aIndex || !state.aLowLink || !state.aOnStack ){
    rc = SQLITE_NOMEM;
    goto scc_cleanup;
  }
  
  /* Initialize arrays */
  for( i = 0; i < pVtab->nNodes; i++ ){
    state.aIndex[i] = -1;
    state.aLowLink[i] = -1;
    state.aOnStack[i] = 0;
  }
  
  /* Run Tarjan's algorithm */
  for( i = 0; i < pVtab->nNodes; i++ ){
    if( state.aIndex[i] == -1 ){
      tarjanStrongConnect(pVtab, &state, i);
    }
  }
  
  /* Build result */
  zResult = sqlite3_mprintf("[");
  for( i = 0; i < state.nSCC && zResult; i++ ){
    char *zOld = zResult;
    zResult = sqlite3_mprintf("%s%s%s", zOld,
                             i > 0 ? "," : "",
                             state.azSCC[i]);
    sqlite3_free(zOld);
  }
  
  if( zResult ){
    char *zOld = zResult;
    zResult = sqlite3_mprintf("%s]", zOld);
    sqlite3_free(zOld);
  }
  
  if( zResult==0 ){
    rc = SQLITE_NOMEM;
  } else {
    *pzSCC = zResult;
  }
  
scc_cleanup:
  /* Free SCCs */
  for( i = 0; i < state.nSCC; i++ ){
    sqlite3_free(state.azSCC[i]);
  }
  sqlite3_free(state.azSCC);
  
  /* Free remaining stack */
  while( state.pStack ){
    TarjanNode *pNext = state.pStack->pNext;
    sqlite3_free(state.pStack);
    state.pStack = pNext;
  }
  
  sqlite3_free(state.aIndex);
  sqlite3_free(state.aLowLink);
  sqlite3_free(state.aOnStack);
  freeNodeIndexMap(state.pMap);
  
  return rc;
}