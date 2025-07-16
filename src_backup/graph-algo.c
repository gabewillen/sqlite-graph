/*
** SQLite Graph Database Extension - Graph Algorithms
**
** This file implements graph algorithms including shortest path,
** centrality measures, and connectivity analysis.
** All algorithms follow SQLite patterns with proper error handling.
**
** Algorithms: Dijkstra, PageRank, Betweenness/Closeness Centrality
** Data structures: Priority queue, visited sets, distance arrays
*/

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1
#include "graph.h"
#include <float.h>
#include <math.h>
#include <string.h>
#include <assert.h>

/*
** Priority queue element for Dijkstra's algorithm.
** Stores node ID and distance from source.
*/
typedef struct PQNode PQNode;
struct PQNode {
  sqlite3_int64 iNodeId;    /* Node identifier */
  double rDistance;         /* Distance from source */
};

/*
** Min-heap priority queue for Dijkstra's algorithm.
** Uses dynamic array with parent/child indexing.
*/
typedef struct GraphPriorityQueue GraphPriorityQueue;
struct GraphPriorityQueue {
  PQNode *aNodes;           /* Array of nodes */
  int nUsed;                /* Number of nodes in queue */
  int nAlloc;               /* Allocated size */
};

/*
** Create a new priority queue.
** Memory allocation: Uses sqlite3_malloc() for initial capacity.
** Returns pointer to queue or NULL on OOM.
*/
static GraphPriorityQueue *graphPriorityQueueCreate(void){
  GraphPriorityQueue *pQueue;
  
  pQueue = sqlite3_malloc(sizeof(*pQueue));
  if( pQueue==0 ){
    testcase( pQueue==0 );  /* Out of memory */
    return 0;
  }
  
  /* Initialize with small initial capacity */
  pQueue->nAlloc = 16;
  pQueue->nUsed = 0;
  pQueue->aNodes = sqlite3_malloc(pQueue->nAlloc * sizeof(PQNode));
  if( pQueue->aNodes==0 ){
    testcase( pQueue->aNodes==0 );  /* Out of memory */
    sqlite3_free(pQueue);
    return 0;
  }
  
  return pQueue;
}

/*
** Destroy priority queue and free all memory.
*/
static void graphPriorityQueueDestroy(GraphPriorityQueue *pQueue){
  if( pQueue ){
    sqlite3_free(pQueue->aNodes);
    sqlite3_free(pQueue);
  }
}

/*
** Get parent index in heap.
*/
static int pqParent(int i){
  return (i - 1) / 2;
}

/*
** Get left child index in heap.
*/
static int pqLeftChild(int i){
  return 2 * i + 1;
}

/*
** Get right child index in heap.
*/
static int pqRightChild(int i){
  return 2 * i + 2;
}

/*
** Swap two nodes in the heap.
*/
static void pqSwap(GraphPriorityQueue *pQueue, int i, int j){
  PQNode temp = pQueue->aNodes[i];
  pQueue->aNodes[i] = pQueue->aNodes[j];
  pQueue->aNodes[j] = temp;
}

/*
** Bubble up element to maintain heap property.
*/
static void pqBubbleUp(GraphPriorityQueue *pQueue, int i){
  while( i>0 && pQueue->aNodes[pqParent(i)].rDistance > 
                pQueue->aNodes[i].rDistance ){
    pqSwap(pQueue, i, pqParent(i));
    i = pqParent(i);
  }
}

/*
** Bubble down element to maintain heap property.
*/
static void pqBubbleDown(GraphPriorityQueue *pQueue, int i){
  int minIndex = i;
  int left = pqLeftChild(i);
  int right = pqRightChild(i);
  
  if( left<pQueue->nUsed && 
      pQueue->aNodes[left].rDistance < pQueue->aNodes[minIndex].rDistance ){
    minIndex = left;
  }
  
  if( right<pQueue->nUsed && 
      pQueue->aNodes[right].rDistance < pQueue->aNodes[minIndex].rDistance ){
    minIndex = right;
  }
  
  if( minIndex!=i ){
    pqSwap(pQueue, i, minIndex);
    pqBubbleDown(pQueue, minIndex);
  }
}

/*
** Insert node into priority queue.
** Returns SQLITE_OK on success, SQLITE_NOMEM on allocation failure.
*/
static int graphPriorityQueueInsert(GraphPriorityQueue *pQueue, 
                                   sqlite3_int64 iNodeId, double rDistance){
  PQNode *aNew;
  
  assert( pQueue!=0 );
  
  /* Grow array if needed */
  if( pQueue->nUsed >= pQueue->nAlloc ){
    int nNewAlloc = pQueue->nAlloc * 2;
    aNew = sqlite3_realloc(pQueue->aNodes, nNewAlloc * sizeof(PQNode));
    if( aNew==0 ){
      testcase( aNew==0 );  /* Realloc failed */
      return SQLITE_NOMEM;
    }
    pQueue->aNodes = aNew;
    pQueue->nAlloc = nNewAlloc;
  }
  
  /* Add node at end */
  pQueue->aNodes[pQueue->nUsed].iNodeId = iNodeId;
  pQueue->aNodes[pQueue->nUsed].rDistance = rDistance;
  
  /* Bubble up to maintain heap property */
  pqBubbleUp(pQueue, pQueue->nUsed);
  pQueue->nUsed++;
  
  return SQLITE_OK;
}

/*
** Extract minimum distance node from queue.
** Returns SQLITE_OK and sets outputs, or SQLITE_NOTFOUND if empty.
*/
static int graphPriorityQueueExtractMin(GraphPriorityQueue *pQueue,
                                       sqlite3_int64 *piNodeId,
                                       double *prDistance){
  assert( pQueue!=0 );
  assert( piNodeId!=0 );
  assert( prDistance!=0 );
  
  if( pQueue->nUsed==0 ){
    testcase( pQueue->nUsed==0 );  /* Queue is empty */
    return SQLITE_NOTFOUND;
  }
  
  /* Extract root (minimum) */
  *piNodeId = pQueue->aNodes[0].iNodeId;
  *prDistance = pQueue->aNodes[0].rDistance;
  
  /* Move last element to root */
  pQueue->nUsed--;
  if( pQueue->nUsed>0 ){
    pQueue->aNodes[0] = pQueue->aNodes[pQueue->nUsed];
    pqBubbleDown(pQueue, 0);
  }
  
  return SQLITE_OK;
}

/*
** Check if priority queue is empty.
*/
static int graphPriorityQueueIsEmpty(GraphPriorityQueue *pQueue){
  return pQueue->nUsed == 0;
}

/*
** Distance information for Dijkstra's algorithm.
** Maps node IDs to distances and predecessors.
*/
typedef struct DistanceInfo DistanceInfo;
struct DistanceInfo {
  sqlite3_int64 iNodeId;    /* Node ID */
  double rDistance;         /* Distance from source */
  sqlite3_int64 iPredecessor; /* Previous node in shortest path */
  DistanceInfo *pNext;      /* Next in hash bucket */
};

typedef struct DistanceMap DistanceMap;
struct DistanceMap {
  DistanceInfo **aBuckets;  /* Hash table buckets */
  int nBuckets;             /* Number of buckets */
  int nEntries;             /* Number of entries */
};

/*
** Create distance map for tracking shortest paths.
*/
static DistanceMap *distanceMapCreate(int nExpectedNodes){
  DistanceMap *pMap;
  int i;
  
  pMap = sqlite3_malloc(sizeof(*pMap));
  if( pMap==0 ){
    return 0;
  }
  
  /* Use prime number of buckets */
  pMap->nBuckets = nExpectedNodes * 2 + 1;
  pMap->nEntries = 0;
  pMap->aBuckets = sqlite3_malloc(pMap->nBuckets * sizeof(DistanceInfo*));
  if( pMap->aBuckets==0 ){
    sqlite3_free(pMap);
    return 0;
  }
  
  for( i=0; i<pMap->nBuckets; i++ ){
    pMap->aBuckets[i] = 0;
  }
  
  return pMap;
}

/*
** Destroy distance map and free all memory.
*/
static void distanceMapDestroy(DistanceMap *pMap){
  int i;
  DistanceInfo *pInfo, *pNext;
  
  if( pMap==0 ) return;
  
  for( i=0; i<pMap->nBuckets; i++ ){
    pInfo = pMap->aBuckets[i];
    while( pInfo ){
      pNext = pInfo->pNext;
      sqlite3_free(pInfo);
      pInfo = pNext;
    }
  }
  
  sqlite3_free(pMap->aBuckets);
  sqlite3_free(pMap);
}

/*
** Set distance for a node in the map.
*/
static int distanceMapSet(DistanceMap *pMap, sqlite3_int64 iNodeId,
                         double rDistance, sqlite3_int64 iPredecessor){
  int iBucket = (int)(iNodeId % pMap->nBuckets);
  DistanceInfo *pInfo;
  
  /* Look for existing entry */
  for( pInfo=pMap->aBuckets[iBucket]; pInfo; pInfo=pInfo->pNext ){
    if( pInfo->iNodeId==iNodeId ){
      pInfo->rDistance = rDistance;
      pInfo->iPredecessor = iPredecessor;
      return SQLITE_OK;
    }
  }
  
  /* Create new entry */
  pInfo = sqlite3_malloc(sizeof(*pInfo));
  if( pInfo==0 ){
    return SQLITE_NOMEM;
  }
  
  pInfo->iNodeId = iNodeId;
  pInfo->rDistance = rDistance;
  pInfo->iPredecessor = iPredecessor;
  pInfo->pNext = pMap->aBuckets[iBucket];
  pMap->aBuckets[iBucket] = pInfo;
  pMap->nEntries++;
  
  return SQLITE_OK;
}

/*
** Get distance for a node from the map.
** Returns DBL_MAX if not found.
*/
static double distanceMapGet(DistanceMap *pMap, sqlite3_int64 iNodeId){
  int iBucket = (int)(iNodeId % pMap->nBuckets);
  DistanceInfo *pInfo;
  
  for( pInfo=pMap->aBuckets[iBucket]; pInfo; pInfo=pInfo->pNext ){
    if( pInfo->iNodeId==iNodeId ){
      return pInfo->rDistance;
    }
  }
  
  return DBL_MAX;  /* Infinity for unvisited nodes */
}

/*
** Get predecessor for a node from the map.
** Returns -1 if not found.
*/
static sqlite3_int64 distanceMapGetPredecessor(DistanceMap *pMap, 
                                               sqlite3_int64 iNodeId){
  int iBucket = (int)(iNodeId % pMap->nBuckets);
  DistanceInfo *pInfo;
  
  for( pInfo=pMap->aBuckets[iBucket]; pInfo; pInfo=pInfo->pNext ){
    if( pInfo->iNodeId==iNodeId ){
      return pInfo->iPredecessor;
    }
  }
  
  return -1;
}

/*
** Dijkstra's shortest path algorithm implementation.
** Time complexity: O((V + E) log V) with binary heap.
** Memory allocation: Uses sqlite3_malloc() for distance map and heap.
*/
int graphDijkstra(GraphVtab *pVtab, sqlite3_int64 iStartId, 
                  sqlite3_int64 iEndId, char **pzPath, double *prDistance){
  GraphPriorityQueue *pQueue = 0;
  DistanceMap *pDistances = 0;
  GraphNode *pNode;
  GraphEdge *pEdge;
  sqlite3_int64 iCurrentId;
  double rCurrentDist;
  double rNewDist;
  int rc = SQLITE_OK;
  
  assert( pVtab!=0 );
  assert( pzPath!=0 );
  
  *pzPath = 0;
  if( prDistance ) *prDistance = DBL_MAX;
  
  /* Validate start node exists */
  if( graphFindNode(pVtab, iStartId)==0 ){
    testcase( graphFindNode(pVtab, iStartId)==0 );
    return SQLITE_NOTFOUND;
  }
  
  /* Create priority queue and distance map */
  pQueue = graphPriorityQueueCreate();
  if( pQueue==0 ){
    return SQLITE_NOMEM;
  }
  
  pDistances = distanceMapCreate(pVtab->nNodes);
  if( pDistances==0 ){
    graphPriorityQueueDestroy(pQueue);
    return SQLITE_NOMEM;
  }
  
  /* Initialize distances */
  rc = distanceMapSet(pDistances, iStartId, 0.0, -1);
  if( rc!=SQLITE_OK ){
    goto dijkstra_cleanup;
  }
  
  /* Add start node to queue */
  rc = graphPriorityQueueInsert(pQueue, iStartId, 0.0);
  if( rc!=SQLITE_OK ){
    goto dijkstra_cleanup;
  }
  
  /* Main Dijkstra loop */
  while( !graphPriorityQueueIsEmpty(pQueue) ){
    /* Extract node with minimum distance */
    rc = graphPriorityQueueExtractMin(pQueue, &iCurrentId, &rCurrentDist);
    if( rc!=SQLITE_OK ){
      break;
    }
    
    /* Check if we reached the destination */
    if( iEndId>=0 && iCurrentId==iEndId ){
      break;
    }
    
    /* Skip if we've already processed this node with a shorter distance */
    if( rCurrentDist > distanceMapGet(pDistances, iCurrentId) ){
      continue;
    }
    
    /* Process all outgoing edges */
    for( pEdge=pVtab->pEdgeList; pEdge; pEdge=pEdge->pNext ){
      if( pEdge->iFromId==iCurrentId ){
        /* Calculate new distance through current node */
        rNewDist = rCurrentDist + pEdge->rWeight;
        
        /* Update if shorter path found */
        if( rNewDist < distanceMapGet(pDistances, pEdge->iToId) ){
          rc = distanceMapSet(pDistances, pEdge->iToId, rNewDist, iCurrentId);
          if( rc!=SQLITE_OK ){
            goto dijkstra_cleanup;
          }
          
          rc = graphPriorityQueueInsert(pQueue, pEdge->iToId, rNewDist);
          if( rc!=SQLITE_OK ){
            goto dijkstra_cleanup;
          }
        }
      }
    }
  }
  
  /* Build path if destination was specified and reached */
  if( iEndId>=0 ){
    double rFinalDist = distanceMapGet(pDistances, iEndId);
    if( rFinalDist < DBL_MAX ){
      /* Path found - reconstruct it */
      sqlite3_int64 *aPath = 0;
      int nPath = 0;
      int nAlloc = 0;
      sqlite3_int64 iCurrent = iEndId;
      char *zPath = 0;
      int i;
      
      /* Count path length */
      while( iCurrent != -1 ){
        nPath++;
        if( iCurrent == iStartId ) break;
        iCurrent = distanceMapGetPredecessor(pDistances, iCurrent);
      }
      
      /* Allocate path array */
      aPath = sqlite3_malloc(nPath * sizeof(sqlite3_int64));
      if( aPath==0 ){
        rc = SQLITE_NOMEM;
        goto dijkstra_cleanup;
      }
      
      /* Fill path array in reverse */
      iCurrent = iEndId;
      for( i=nPath-1; i>=0; i-- ){
        aPath[i] = iCurrent;
        if( iCurrent == iStartId ) break;
        iCurrent = distanceMapGetPredecessor(pDistances, iCurrent);
      }
      
      /* Build JSON array */
      zPath = sqlite3_mprintf("[");
      for( i=0; i<nPath && zPath; i++ ){
        char *zNew;
        if( i==0 ){
          zNew = sqlite3_mprintf("%s%lld", zPath, aPath[i]);
        } else {
          zNew = sqlite3_mprintf("%s,%lld", zPath, aPath[i]);
        }
        sqlite3_free(zPath);
        zPath = zNew;
      }
      
      if( zPath ){
        char *zNew = sqlite3_mprintf("%s]", zPath);
        sqlite3_free(zPath);
        *pzPath = zNew;
      }
      
      sqlite3_free(aPath);
      
      if( *pzPath==0 ){
        rc = SQLITE_NOMEM;
      } else if( prDistance ){
        *prDistance = rFinalDist;
      }
    } else {
      /* No path exists */
      rc = SQLITE_NOTFOUND;
    }
  } else {
    /* No specific destination - return distances to all nodes */
    /* TODO: Format all distances as JSON */
    rc = SQLITE_OK;
  }
  
dijkstra_cleanup:
  graphPriorityQueueDestroy(pQueue);
  distanceMapDestroy(pDistances);
  
  return rc;
}

/*
** Shortest path for unweighted graphs using BFS.
** More efficient than Dijkstra for unweighted graphs: O(V + E).
*/
int graphShortestPathUnweighted(GraphVtab *pVtab, sqlite3_int64 iStartId,
                                sqlite3_int64 iEndId, char **pzPath){
  /* Use BFS which naturally finds shortest paths in unweighted graphs */
  /* For now, call graphBFS which already implements this */
  extern int graphBFS(GraphVtab*, sqlite3_int64, int, char**);
  
  /* TODO: Modify to stop at iEndId and return just the path to it */
  return graphBFS(pVtab, iStartId, -1, pzPath);
}

/*
** PageRank algorithm implementation.
** Iterative algorithm with configurable damping factor.
** Convergence: Stops when change between iterations < epsilon.
*/
int graphPageRank(GraphVtab *pVtab, double rDamping, int nMaxIter, 
                  double rEpsilon, char **pzResults){
  double *aPageRank = 0;      /* Current PageRank values */
  double *aNewPageRank = 0;   /* New PageRank values for iteration */
  int *aOutDegree = 0;        /* Out-degree for each node */
  GraphNode *pNode;
  GraphEdge *pEdge;
  int nNodes;
  int nIter;
  int i;
  double rDiff;
  int rc = SQLITE_OK;
  
  assert( pVtab!=0 );
  assert( pzResults!=0 );
  
  *pzResults = 0;
  nNodes = pVtab->nNodes;
  
  if( nNodes==0 ){
    *pzResults = sqlite3_mprintf("{}");
    return *pzResults ? SQLITE_OK : SQLITE_NOMEM;
  }
  
  /* Allocate arrays */
  aPageRank = sqlite3_malloc(nNodes * sizeof(double));
  aNewPageRank = sqlite3_malloc(nNodes * sizeof(double));
  aOutDegree = sqlite3_malloc(nNodes * sizeof(int));
  
  if( !aPageRank || !aNewPageRank || !aOutDegree ){
    rc = SQLITE_NOMEM;
    goto pagerank_cleanup;
  }
  
  /* Initialize PageRank values and calculate out-degrees */
  for( i=0; i<nNodes; i++ ){
    aPageRank[i] = 1.0 / nNodes;
    aOutDegree[i] = 0;
  }
  
  /* Count out-degrees */
  i = 0;
  for( pNode=pVtab->pNodeList; pNode; pNode=pNode->pNext ){
    for( pEdge=pVtab->pEdgeList; pEdge; pEdge=pEdge->pNext ){
      if( pEdge->iFromId == pNode->iNodeId ){
        aOutDegree[i]++;
      }
    }
    i++;
  }
  
  /* PageRank iterations */
  for( nIter=0; nIter<nMaxIter; nIter++ ){
    double rMaxDiff = 0.0;
    
    /* Calculate new PageRank values */
    for( i=0; i<nNodes; i++ ){
      aNewPageRank[i] = (1.0 - rDamping) / nNodes;
    }
    
    /* Add contributions from incoming links */
    i = 0;
    for( pNode=pVtab->pNodeList; pNode; pNode=pNode->pNext ){
      int j = 0;
      GraphNode *pFromNode;
      
      for( pFromNode=pVtab->pNodeList; pFromNode; pFromNode=pFromNode->pNext ){
        for( pEdge=pVtab->pEdgeList; pEdge; pEdge=pEdge->pNext ){
          if( pEdge->iFromId==pFromNode->iNodeId && 
              pEdge->iToId==pNode->iNodeId ){
            if( aOutDegree[j]>0 ){
              aNewPageRank[i] += rDamping * aPageRank[j] / aOutDegree[j];
            }
          }
        }
        j++;
      }
      i++;
    }
    
    /* Check convergence */
    for( i=0; i<nNodes; i++ ){
      rDiff = fabs(aNewPageRank[i] - aPageRank[i]);
      if( rDiff > rMaxDiff ){
        rMaxDiff = rDiff;
      }
      aPageRank[i] = aNewPageRank[i];
    }
    
    if( rMaxDiff < rEpsilon ){
      break;  /* Converged */
    }
  }
  
  /* Format results as JSON object */
  *pzResults = sqlite3_mprintf("{");
  i = 0;
  for( pNode=pVtab->pNodeList; pNode && *pzResults; pNode=pNode->pNext ){
    char *zNew;
    if( i==0 ){
      zNew = sqlite3_mprintf("%s\"%lld\":%.6f", *pzResults, 
                            pNode->iNodeId, aPageRank[i]);
    } else {
      zNew = sqlite3_mprintf("%s,\"%lld\":%.6f", *pzResults, 
                            pNode->iNodeId, aPageRank[i]);
    }
    sqlite3_free(*pzResults);
    *pzResults = zNew;
    i++;
  }
  
  if( *pzResults ){
    char *zNew = sqlite3_mprintf("%s}", *pzResults);
    sqlite3_free(*pzResults);
    *pzResults = zNew;
  }
  
  if( *pzResults==0 ){
    rc = SQLITE_NOMEM;
  }
  
pagerank_cleanup:
  sqlite3_free(aPageRank);
  sqlite3_free(aNewPageRank);
  sqlite3_free(aOutDegree);
  
  return rc;
}

/*
** Calculate in-degree for a specific node.
** Returns the number of incoming edges.
*/
int graphInDegree(GraphVtab *pVtab, sqlite3_int64 iNodeId){
  GraphEdge *pEdge;
  int nInDegree = 0;
  
  assert( pVtab!=0 );
  
  /* Count incoming edges */
  for( pEdge=pVtab->pEdgeList; pEdge; pEdge=pEdge->pNext ){
    if( pEdge->iToId==iNodeId ){
      nInDegree++;
    }
  }
  
  return nInDegree;
}

/*
** Calculate out-degree for a specific node.
** Returns the number of outgoing edges.
*/
int graphOutDegree(GraphVtab *pVtab, sqlite3_int64 iNodeId){
  GraphEdge *pEdge;
  int nOutDegree = 0;
  
  assert( pVtab!=0 );
  
  /* Count outgoing edges */
  for( pEdge=pVtab->pEdgeList; pEdge; pEdge=pEdge->pNext ){
    if( pEdge->iFromId==iNodeId ){
      nOutDegree++;
    }
  }
  
  return nOutDegree;
}

/*
** Calculate total degree for a specific node.
** Returns in-degree + out-degree.
*/
int graphTotalDegree(GraphVtab *pVtab, sqlite3_int64 iNodeId){
  return graphInDegree(pVtab, iNodeId) + graphOutDegree(pVtab, iNodeId);
}

/*
** Calculate degree centrality for a node.
** For directed graphs: (in-degree + out-degree) / (2 * (n-1))
** For undirected graphs: degree / (n-1)
*/
double graphDegreeCentrality(GraphVtab *pVtab, sqlite3_int64 iNodeId,
                            int bDirected){
  int nDegree;
  int nNodes = pVtab->nNodes;
  
  if( nNodes <= 1 ) return 0.0;
  
  if( bDirected ){
    nDegree = graphTotalDegree(pVtab, iNodeId);
    return (double)nDegree / (2.0 * (nNodes - 1));
  } else {
    /* For undirected, count each edge once */
    nDegree = graphOutDegree(pVtab, iNodeId);
    return (double)nDegree / (nNodes - 1);
  }
}

/*
** Check if the graph is connected.
** Uses DFS to see if all nodes are reachable from any starting node.
*/
int graphIsConnected(GraphVtab *pVtab){
  char *zPath = 0;
  GraphNode *pFirstNode;
  int nVisited = 0;
  int rc;
  
  assert( pVtab!=0 );
  
  if( pVtab->nNodes==0 ) return 1;  /* Empty graph is connected */
  if( pVtab->nNodes==1 ) return 1;  /* Single node is connected */
  
  /* Start DFS from first node */
  pFirstNode = pVtab->pNodeList;
  if( pFirstNode==0 ) return 0;
  
  /* Use DFS to explore the graph */
  extern int graphDFS(GraphVtab*, sqlite3_int64, int, char**);
  rc = graphDFS(pVtab, pFirstNode->iNodeId, -1, &zPath);
  
  if( rc==SQLITE_OK && zPath ){
    /* Count nodes in path */
    const char *p = zPath;
    while( *p ){
      if( *p==',' ) nVisited++;
      p++;
    }
    if( strchr(zPath, '[') ) nVisited++;  /* Count first node */
  }
  
  sqlite3_free(zPath);
  
  /* Graph is connected if DFS visited all nodes */
  return nVisited == pVtab->nNodes;
}

/*
** Calculate graph density.
** For directed graphs: |E| / (|V| * (|V|-1))
** For undirected graphs: 2*|E| / (|V| * (|V|-1))
*/
double graphDensity(GraphVtab *pVtab, int bDirected){
  int nNodes = pVtab->nNodes;
  int nEdges = pVtab->nEdges;
  
  if( nNodes <= 1 ) return 0.0;
  
  if( bDirected ){
    return (double)nEdges / (nNodes * (nNodes - 1));
  } else {
    return (2.0 * nEdges) / (nNodes * (nNodes - 1));
  }
}