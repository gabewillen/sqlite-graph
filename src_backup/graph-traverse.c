/*
** SQLite Graph Database Extension - Graph Traversal
**
** This file implements graph traversal algorithms including DFS and BFS.
** Supports cycle detection, depth limits, and path tracking.
** All functions follow SQLite patterns with proper memory management.
**
** Traversal modes: Depth-first search, Breadth-first search
** Features: Visited tracking, path reconstruction, cycle detection
*/

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1
#include "graph.h"
#include <assert.h>

/*
** Visited set structure for tracking nodes during traversal.
** Uses dynamic array for O(1) insertion and O(n) lookup.
** SQLite-style memory management with sqlite3_malloc/free.
*/
typedef struct GraphVisitedSet GraphVisitedSet;
struct GraphVisitedSet {
  sqlite3_int64 *aNodes;    /* Array of visited node IDs */
  int nUsed;                /* Number of nodes currently in set */
  int nAlloc;               /* Allocated size of array */
};

/* Forward declarations for internal functions */
static int graphDFSRecursive(GraphVtab *pVtab, sqlite3_int64 iNodeId, 
                            int nMaxDepth, int nCurrentDepth,
                            GraphVisitedSet *pVisited, char **pzPath);

/*
** Queue structure for BFS implementation.
** Dynamic array with head/tail pointers for efficient FIFO operations.
*/
typedef struct GraphQueue GraphQueue;
struct GraphQueue {
  sqlite3_int64 *aNodes;    /* Array of node IDs */
  int nUsed;                /* Number of nodes currently queued */
  int nAlloc;               /* Allocated size of array */
  int iHead;                /* Index of first node in queue */
};

/*
** Create a new visited set for tracking traversal state.
** Memory allocation: Uses sqlite3_malloc() for initial capacity.
** Returns pointer to set or NULL on OOM.
*/
static GraphVisitedSet *graphVisitedSetCreate(void){
  GraphVisitedSet *pSet;
  
  pSet = sqlite3_malloc(sizeof(*pSet));
  if( pSet==0 ){
    testcase( pSet==0 );  /* Out of memory */
    return 0;
  }
  
  /* Initialize with small initial capacity */
  pSet->nAlloc = 16;
  pSet->nUsed = 0;
  pSet->aNodes = sqlite3_malloc(pSet->nAlloc * sizeof(sqlite3_int64));
  if( pSet->aNodes==0 ){
    testcase( pSet->aNodes==0 );  /* Out of memory */
    sqlite3_free(pSet);
    return 0;
  }
  
  return pSet;
}

/*
** Add a node to the visited set.
** Returns SQLITE_OK on success, SQLITE_NOMEM on allocation failure.
** Grows array as needed using sqlite3_realloc().
*/
static int graphVisitedSetAdd(GraphVisitedSet *pSet, sqlite3_int64 iNodeId){
  sqlite3_int64 *aNew;
  
  assert( pSet!=0 );
  
  /* Grow array if needed */
  if( pSet->nUsed>=pSet->nAlloc ){
    int nNewAlloc = pSet->nAlloc * 2;
    aNew = sqlite3_realloc(pSet->aNodes, nNewAlloc * sizeof(sqlite3_int64));
    if( aNew==0 ){
      testcase( aNew==0 );  /* Realloc failed */
      return SQLITE_NOMEM;
    }
    pSet->aNodes = aNew;
    pSet->nAlloc = nNewAlloc;
  }
  
  /* Add node to set */
  pSet->aNodes[pSet->nUsed++] = iNodeId;
  return SQLITE_OK;
}

/*
** Check if a node is in the visited set.
** Returns non-zero if node is visited, zero otherwise.
** Time complexity: O(n) linear search.
*/
static int graphVisitedSetContains(GraphVisitedSet *pSet, 
                                   sqlite3_int64 iNodeId){
  int i;
  
  assert( pSet!=0 );
  
  for( i=0; i<pSet->nUsed; i++ ){
    if( pSet->aNodes[i]==iNodeId ){
      testcase( pSet->aNodes[i]==iNodeId );  /* Node found in set */
      return 1;
    }
  }
  
  testcase( i==pSet->nUsed );  /* Node not found in set */
  return 0;
}

/*
** Destroy visited set and free all memory.
** Memory management: Calls sqlite3_free() on all allocated memory.
*/
static void graphVisitedSetDestroy(GraphVisitedSet *pSet){
  if( pSet ){
    sqlite3_free(pSet->aNodes);
    sqlite3_free(pSet);
  }
}

/*
** Depth-first search with cycle detection.
** Recursive implementation with configurable depth limits.
** Memory allocation: Uses sqlite3_malloc() for visited set and path.
** Returns: SQLITE_OK on success, error codes on failure.
*/
int graphDFS(GraphVtab *pVtab, sqlite3_int64 iStartId, int nMaxDepth,
             char **pzPath){
  GraphVisitedSet *pVisited = 0;
  GraphNode *pStartNode;
  int rc = SQLITE_OK;
  
  assert( pVtab!=0 );
  assert( pzPath!=0 );
  
  *pzPath = 0;
  
  /* Validate start node exists */
  pStartNode = graphFindNode(pVtab, iStartId);
  if( pStartNode==0 ){
    testcase( pStartNode==0 );  /* Start node not found */
    return SQLITE_NOTFOUND;
  }
  
  /* Create visited set */
  pVisited = graphVisitedSetCreate();
  if( pVisited==0 ){
    return SQLITE_NOMEM;
  }
  
  /* Perform DFS traversal */
  rc = graphDFSRecursive(pVtab, iStartId, nMaxDepth, 0, pVisited, pzPath);
  
  /* Close JSON array */
  if( rc==SQLITE_OK && *pzPath ){
    char *zNewPath = sqlite3_mprintf("%s]", *pzPath);
    sqlite3_free(*pzPath);
    *pzPath = zNewPath;
    if( *pzPath==0 ){
      testcase( *pzPath==0 );  /* Final path allocation failed */
      rc = SQLITE_NOMEM;
    }
  }
  
  /* Cleanup */
  graphVisitedSetDestroy(pVisited);
  
  return rc;
}

/*
** Recursive DFS implementation.
** Explores graph depth-first with cycle detection and depth limiting.
** Path tracking: Builds JSON array of visited node IDs.
*/
static int graphDFSRecursive(GraphVtab *pVtab, sqlite3_int64 iNodeId, 
                            int nMaxDepth, int nCurrentDepth,
                            GraphVisitedSet *pVisited, char **pzPath){
  GraphEdge *pEdge;
  char *zNewPath = 0;
  int rc;
  
  assert( pVtab!=0 );
  assert( pVisited!=0 );
  assert( pzPath!=0 );
  
  /* Check depth limit */
  if( nMaxDepth>=0 && nCurrentDepth>=nMaxDepth ){
    testcase( nCurrentDepth>=nMaxDepth );  /* Depth limit reached */
    return SQLITE_OK;
  }
  
  /* Check for cycle */
  if( graphVisitedSetContains(pVisited, iNodeId) ){
    testcase( graphVisitedSetContains(pVisited, iNodeId) );  /* Cycle detected */
    return SQLITE_OK;
  }
  
  /* Mark node as visited */
  rc = graphVisitedSetAdd(pVisited, iNodeId);
  if( rc!=SQLITE_OK ){
    return rc;
  }
  
  /* Add node to path */
  if( *pzPath==0 ){
    *pzPath = sqlite3_mprintf("[%lld", iNodeId);
  } else {
    zNewPath = sqlite3_mprintf("%s,%lld", *pzPath, iNodeId);
    sqlite3_free(*pzPath);
    *pzPath = zNewPath;
  }
  
  if( *pzPath==0 ){
    testcase( *pzPath==0 );  /* Path allocation failed */
    return SQLITE_NOMEM;
  }
  
  /* Recursively visit adjacent nodes */
  pEdge = pVtab->pEdgeList;
  while( pEdge ){
    if( pEdge->iFromId==iNodeId ){
      rc = graphDFSRecursive(pVtab, pEdge->iToId, nMaxDepth, 
                            nCurrentDepth + 1, pVisited, pzPath);
      if( rc!=SQLITE_OK ){
        return rc;
      }
    }
    pEdge = pEdge->pNext;
  }
  
  return SQLITE_OK;
}

/*
** Create a new queue for BFS traversal.
** Memory allocation: Uses sqlite3_malloc() for initial capacity.
** Returns pointer to queue or NULL on OOM.
*/
static GraphQueue *graphQueueCreate(void){
  GraphQueue *pQueue;
  
  pQueue = sqlite3_malloc(sizeof(*pQueue));
  if( pQueue==0 ){
    testcase( pQueue==0 );  /* Out of memory */
    return 0;
  }
  
  /* Initialize with small initial capacity */
  pQueue->nAlloc = 16;
  pQueue->nUsed = 0;
  pQueue->iHead = 0;
  pQueue->aNodes = sqlite3_malloc(pQueue->nAlloc * sizeof(sqlite3_int64));
  if( pQueue->aNodes==0 ){
    testcase( pQueue->aNodes==0 );  /* Out of memory */
    sqlite3_free(pQueue);
    return 0;
  }
  
  return pQueue;
}

/*
** Enqueue a node ID to the queue.
** Returns SQLITE_OK on success, SQLITE_NOMEM on allocation failure.
** Grows array as needed using sqlite3_realloc().
*/
static int graphQueueEnqueue(GraphQueue *pQueue, sqlite3_int64 iNodeId){
  sqlite3_int64 *aNew;
  int iTail;
  
  assert( pQueue!=0 );
  
  /* Check if queue is full and needs to grow */
  if( pQueue->nUsed>=pQueue->nAlloc ){
    int nNewAlloc = pQueue->nAlloc * 2;
    int i, j;
    
    /* Allocate new array */
    aNew = sqlite3_malloc(nNewAlloc * sizeof(sqlite3_int64));
    if( aNew==0 ){
      testcase( aNew==0 );  /* Allocation failed */
      return SQLITE_NOMEM;
    }
    
    /* Copy elements from old array to new array, unwrapping circular buffer */
    j = 0;
    for( i=pQueue->iHead; j<pQueue->nUsed; i=(i+1)%pQueue->nAlloc ){
      aNew[j++] = pQueue->aNodes[i];
    }
    
    /* Replace old array with new array */
    sqlite3_free(pQueue->aNodes);
    pQueue->aNodes = aNew;
    pQueue->nAlloc = nNewAlloc;
    pQueue->iHead = 0;
  }
  
  /* Add node to tail of queue */
  iTail = (pQueue->iHead + pQueue->nUsed) % pQueue->nAlloc;
  pQueue->aNodes[iTail] = iNodeId;
  pQueue->nUsed++;
  
  return SQLITE_OK;
}

/*
** Dequeue a node ID from the queue.
** Returns SQLITE_OK and sets *piNodeId, or SQLITE_NOTFOUND if empty.
*/
static int graphQueueDequeue(GraphQueue *pQueue, sqlite3_int64 *piNodeId){
  assert( pQueue!=0 );
  assert( piNodeId!=0 );
  
  if( pQueue->nUsed==0 ){
    testcase( pQueue->nUsed==0 );  /* Queue is empty */
    return SQLITE_NOTFOUND;
  }
  
  /* Remove node from head of queue */
  *piNodeId = pQueue->aNodes[pQueue->iHead];
  pQueue->iHead = (pQueue->iHead + 1) % pQueue->nAlloc;
  pQueue->nUsed--;
  
  return SQLITE_OK;
}

/*
** Destroy queue and free all memory.
** Memory management: Calls sqlite3_free() on all allocated memory.
*/
static void graphQueueDestroy(GraphQueue *pQueue){
  if( pQueue ){
    sqlite3_free(pQueue->aNodes);
    sqlite3_free(pQueue);
  }
}

/*
** Structure to track depth information for BFS.
** Maps node IDs to their depth level from the start node.
*/
typedef struct GraphDepthInfo GraphDepthInfo;
struct GraphDepthInfo {
  sqlite3_int64 iNodeId;    /* Node ID */
  int nDepth;               /* Depth from start node */
  GraphDepthInfo *pNext;    /* Next in linked list */
};

/*
** Breadth-first search with queue implementation.
** Level-order traversal with optional depth limiting.
** Queue: Implemented as dynamic array with sqlite3_malloc().
*/
int graphBFS(GraphVtab *pVtab, sqlite3_int64 iStartId, int nMaxDepth,
             char **pzPath){
  GraphQueue *pQueue = 0;
  GraphVisitedSet *pVisited = 0;
  GraphDepthInfo *pDepthList = 0;
  GraphDepthInfo *pDepthInfo;
  GraphNode *pStartNode;
  GraphEdge *pEdge;
  sqlite3_int64 iCurrentId;
  int nCurrentDepth;
  char *zNewPath = 0;
  int rc = SQLITE_OK;
  
  assert( pVtab!=0 );
  assert( pzPath!=0 );
  
  *pzPath = 0;
  
  /* Validate start node exists */
  pStartNode = graphFindNode(pVtab, iStartId);
  if( pStartNode==0 ){
    testcase( pStartNode==0 );  /* Start node not found */
    return SQLITE_NOTFOUND;
  }
  
  /* Create queue and visited set */
  pQueue = graphQueueCreate();
  if( pQueue==0 ){
    return SQLITE_NOMEM;
  }
  
  pVisited = graphVisitedSetCreate();
  if( pVisited==0 ){
    graphQueueDestroy(pQueue);
    return SQLITE_NOMEM;
  }
  
  /* Initialize with start node */
  rc = graphQueueEnqueue(pQueue, iStartId);
  if( rc!=SQLITE_OK ){
    goto bfs_cleanup;
  }
  
  rc = graphVisitedSetAdd(pVisited, iStartId);
  if( rc!=SQLITE_OK ){
    goto bfs_cleanup;
  }
  
  /* Add start node depth info */
  pDepthInfo = sqlite3_malloc(sizeof(*pDepthInfo));
  if( pDepthInfo==0 ){
    rc = SQLITE_NOMEM;
    goto bfs_cleanup;
  }
  pDepthInfo->iNodeId = iStartId;
  pDepthInfo->nDepth = 0;
  pDepthInfo->pNext = pDepthList;
  pDepthList = pDepthInfo;
  
  /* Start JSON array */
  *pzPath = sqlite3_mprintf("[%lld", iStartId);
  if( *pzPath==0 ){
    rc = SQLITE_NOMEM;
    goto bfs_cleanup;
  }
  
  /* BFS main loop */
  while( graphQueueDequeue(pQueue, &iCurrentId)==SQLITE_OK ){
    /* Find depth of current node */
    nCurrentDepth = 0;
    for( pDepthInfo=pDepthList; pDepthInfo; pDepthInfo=pDepthInfo->pNext ){
      if( pDepthInfo->iNodeId==iCurrentId ){
        nCurrentDepth = pDepthInfo->nDepth;
        break;
      }
    }
    
    /* Check depth limit */
    if( nMaxDepth>=0 && nCurrentDepth>=nMaxDepth ){
      testcase( nCurrentDepth>=nMaxDepth );  /* Depth limit reached */
      continue;
    }
    
    /* Process all adjacent nodes */
    pEdge = pVtab->pEdgeList;
    while( pEdge ){
      if( pEdge->iFromId==iCurrentId ){
        /* Check if already visited */
        if( !graphVisitedSetContains(pVisited, pEdge->iToId) ){
          /* Mark as visited */
          rc = graphVisitedSetAdd(pVisited, pEdge->iToId);
          if( rc!=SQLITE_OK ){
            goto bfs_cleanup;
          }
          
          /* Add to queue */
          rc = graphQueueEnqueue(pQueue, pEdge->iToId);
          if( rc!=SQLITE_OK ){
            goto bfs_cleanup;
          }
          
          /* Add depth info */
          pDepthInfo = sqlite3_malloc(sizeof(*pDepthInfo));
          if( pDepthInfo==0 ){
            rc = SQLITE_NOMEM;
            goto bfs_cleanup;
          }
          pDepthInfo->iNodeId = pEdge->iToId;
          pDepthInfo->nDepth = nCurrentDepth + 1;
          pDepthInfo->pNext = pDepthList;
          pDepthList = pDepthInfo;
          
          /* Add to path */
          zNewPath = sqlite3_mprintf("%s,%lld", *pzPath, pEdge->iToId);
          sqlite3_free(*pzPath);
          *pzPath = zNewPath;
          if( *pzPath==0 ){
            rc = SQLITE_NOMEM;
            goto bfs_cleanup;
          }
        }
      }
      pEdge = pEdge->pNext;
    }
  }
  
  /* Close JSON array */
  if( rc==SQLITE_OK && *pzPath ){
    zNewPath = sqlite3_mprintf("%s]", *pzPath);
    sqlite3_free(*pzPath);
    *pzPath = zNewPath;
    if( *pzPath==0 ){
      testcase( *pzPath==0 );  /* Final path allocation failed */
      rc = SQLITE_NOMEM;
    }
  }
  
bfs_cleanup:
  /* Free depth info list */
  while( pDepthList ){
    pDepthInfo = pDepthList;
    pDepthList = pDepthList->pNext;
    sqlite3_free(pDepthInfo);
  }
  
  /* Clean up resources */
  graphQueueDestroy(pQueue);
  graphVisitedSetDestroy(pVisited);
  
  return rc;
}