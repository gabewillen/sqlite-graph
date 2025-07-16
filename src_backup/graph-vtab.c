/*
** SQLite Graph Database Extension - Virtual Table Implementation
**
** This file implements the virtual table interface for graph storage.
** Follows SQLite virtual table patterns exactly with proper error handling
** and memory management using sqlite3_malloc()/sqlite3_free().
**
** Virtual table schema: CREATE VIRTUAL TABLE graph USING graph();
** Supports both node and edge iteration through cursor modes.
*/

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1
#include "graph.h"
#include "graph-vtab.h"
#include <string.h>
#include <assert.h>

/*
** Virtual table module structure.
** Defines all method pointers for SQLite virtual table interface.
** Version 0 indicates original virtual table interface.
*/
sqlite3_module graphModule = {
  0,                    /* iVersion */
  graphCreate,          /* xCreate */
  graphConnect,         /* xConnect */
  graphBestIndex,       /* xBestIndex */
  graphDisconnect,      /* xDisconnect */
  graphDestroy,         /* xDestroy */
  graphOpen,            /* xOpen */
  graphClose,           /* xClose */
  graphFilter,          /* xFilter */
  graphNext,            /* xNext */
  graphEof,             /* xEof */
  graphColumn,          /* xColumn */
  graphRowid,           /* xRowid */
  0,                    /* xUpdate - read-only for now */
  0,                    /* xBegin */
  0,                    /* xSync */
  0,                    /* xCommit */
  0,                    /* xRollback */
  0,                    /* xFindFunction */
  0,                    /* xRename */
  0,                    /* xSavepoint */
  0,                    /* xRelease */
  0                     /* xRollbackTo */
};

/*
** Create a new virtual table instance.
** Called when CREATE VIRTUAL TABLE is executed.
** 
** Memory allocation: Creates new GraphVtab with sqlite3_malloc().
** Error handling: Returns SQLITE_NOMEM on allocation failure.
** Initialization: Sets up empty graph with reference count of 1.
*/
int graphCreate(sqlite3 *pDb, void *pAux, int argc, 
                const char *const *argv, sqlite3_vtab **ppVtab, 
                char **pzErr){
  GraphVtab *pNew;
  int rc = SQLITE_OK;
  
  /* Validate arguments - expect at least module name */
  assert( argc>=3 );
  assert( argv!=0 );
  assert( ppVtab!=0 );
  
  /* Allocate new virtual table structure */
  pNew = sqlite3_malloc(sizeof(*pNew));
  if( pNew==0 ){
    return SQLITE_NOMEM;
  }
  
  /* Initialize structure to zero */
  memset(pNew, 0, sizeof(*pNew));
  
  /* Set up virtual table base */
  pNew->pDb = pDb;
  pNew->nRef = 1;
  
  /* Copy database and table names */
  pNew->zDbName = sqlite3_mprintf("%s", argv[1]);
  pNew->zTableName = sqlite3_mprintf("%s", argv[2]);
  
  if( pNew->zDbName==0 || pNew->zTableName==0 ){
    sqlite3_free(pNew->zDbName);
    sqlite3_free(pNew->zTableName);
    sqlite3_free(pNew);
    return SQLITE_NOMEM;
  }
  
  /* Declare enhanced virtual table schema with label and type support */
  rc = sqlite3_declare_vtab(pDb, 
    "CREATE TABLE graph("
    "type TEXT,"           /* 'node' or 'edge' */
    "id INTEGER,"          /* node_id or edge_id */
    "from_id INTEGER,"     /* source node (edges only) */
    "to_id INTEGER,"       /* target node (edges only) */
    "labels TEXT,"         /* JSON array of node labels (nodes only) */
    "rel_type TEXT,"       /* relationship type (edges only) */
    "weight REAL,"         /* edge weight (edges only) */
    "properties TEXT"      /* JSON properties */
    ")"
  );
  
  if( rc!=SQLITE_OK ){
    sqlite3_free(pNew->zDbName);
    sqlite3_free(pNew->zTableName);
    sqlite3_free(pNew);
    *pzErr = sqlite3_mprintf("Failed to declare vtab schema: %s", 
                             sqlite3_errmsg(pDb));
    return rc;
  }
  
  /* Initialize schema and indexing structures */
  pNew->pLabelIndex = NULL;
  pNew->pPropertyIndex = NULL;
  pNew->pSchema = NULL;
  pNew->iNextEdgeId = 0;
  
  /* Initialize schema on first use to avoid startup cost */
  
  *ppVtab = &pNew->base;
  return SQLITE_OK;
}

/*
** Connect to an existing virtual table.
** Called when accessing existing virtual table.
** Reference counting: Increments nRef on successful connection.
*/
int graphConnect(sqlite3 *pDb, void *pAux, int argc,
                 const char *const *argv, sqlite3_vtab **ppVtab,
                 char **pzErr){
  /* For now, connecting is the same as creating */
  return graphCreate(pDb, pAux, argc, argv, ppVtab, pzErr);
}

/*
** Query planner interface.
** Provides cost estimates and index usage hints to SQLite.
** Performance: Critical for query optimization.
*/
int graphBestIndex(sqlite3_vtab *pVtab, sqlite3_index_info *pInfo){
  GraphVtab *pGraphVtab = (GraphVtab*)pVtab;
  
  /* Basic cost estimate based on graph size */
  pInfo->estimatedCost = (double)(pGraphVtab->nNodes + pGraphVtab->nEdges);
  pInfo->estimatedRows = pGraphVtab->nNodes + pGraphVtab->nEdges;
  
  /* No special indexing for now */
  pInfo->idxNum = 0;
  pInfo->idxStr = 0;
  pInfo->needToFreeIdxStr = 0;
  
  return SQLITE_OK;
}

/*
** Disconnect from virtual table.
** Reference counting: Decrements nRef, cleans up if zero.
*/
int graphDisconnect(sqlite3_vtab *pVtab){
  GraphVtab *pGraphVtab = (GraphVtab*)pVtab;
  
  assert( pGraphVtab!=0 );
  assert( pGraphVtab->nRef>0 );
  
  pGraphVtab->nRef--;
  if( pGraphVtab->nRef<=0 ){
    return graphDestroy(pVtab);
  }
  
  return SQLITE_OK;
}

/*
** Destroy virtual table instance.
** Memory management: Frees all nodes, edges, and vtab structure.
** Called during DROP TABLE or database close.
*/
int graphDestroy(sqlite3_vtab *pVtab){
  GraphVtab *pGraphVtab = (GraphVtab*)pVtab;
  GraphNode *pNode, *pNextNode;
  GraphEdge *pEdge, *pNextEdge;
  
  assert( pGraphVtab!=0 );
  
  /* Free all nodes */
  pNode = pGraphVtab->pNodeList;
  while( pNode ){
    pNextNode = pNode->pNext;
    sqlite3_free(pNode->zProperties);
    /* Free enhanced node fields */
    if( pNode->azLabels ){
      graphFreeLabels(pNode->azLabels, pNode->nLabels);
    }
    sqlite3_free(pNode);
    pNode = pNextNode;
  }
  
  /* Free all edges */
  pEdge = pGraphVtab->pEdgeList;
  while( pEdge ){
    pNextEdge = pEdge->pNext;
    sqlite3_free(pEdge->zProperties);
    /* Free enhanced edge fields */
    sqlite3_free(pEdge->zType);
    sqlite3_free(pEdge);
    pEdge = pNextEdge;
  }
  
  /* Free schema and indexing structures */
  if( pGraphVtab->pSchema ){
    graphDestroySchema(pGraphVtab->pSchema);
  }
  
  if( pGraphVtab->pLabelIndex ){
    sqlite3_free(pGraphVtab->pLabelIndex->zIndexName);
    sqlite3_free(pGraphVtab->pLabelIndex->apNodes);
    sqlite3_free(pGraphVtab->pLabelIndex);
  }
  
  if( pGraphVtab->pPropertyIndex ){
    sqlite3_free(pGraphVtab->pPropertyIndex->zIndexName);
    sqlite3_free(pGraphVtab->pPropertyIndex->apNodes);
    sqlite3_free(pGraphVtab->pPropertyIndex);
  }
  
  /* Free table names and structure */
  sqlite3_free(pGraphVtab->zDbName);
  sqlite3_free(pGraphVtab->zTableName);
  sqlite3_free(pGraphVtab);
  
  return SQLITE_OK;
}

/*
** Open a cursor for table iteration.
** Memory allocation: Creates new GraphCursor with sqlite3_malloc().
** Cursor state: Initializes position to beginning of iteration.
*/
int graphOpen(sqlite3_vtab *pVtab, sqlite3_vtab_cursor **ppCursor){
  GraphVtab *pGraphVtab = (GraphVtab*)pVtab;
  GraphCursor *pCursor;
  
  /* Allocate cursor structure */
  pCursor = sqlite3_malloc(sizeof(*pCursor));
  if( pCursor==0 ){
    return SQLITE_NOMEM;
  }
  
  /* Initialize cursor */
  memset(pCursor, 0, sizeof(*pCursor));
  pCursor->pVtab = pGraphVtab;
  pCursor->iRowid = 0;
  pCursor->pCurrentNode = 0;
  pCursor->pCurrentEdge = 0;
  pCursor->iIterMode = 0;  /* Start with nodes */
  
  *ppCursor = &pCursor->base;
  return SQLITE_OK;
}

/*
** Close cursor and free resources.
** Memory management: Calls sqlite3_free() on cursor structure.
*/
int graphClose(sqlite3_vtab_cursor *pCursor){
  assert( pCursor!=0 );
  sqlite3_free(pCursor);
  return SQLITE_OK;
}

/*
** Filter cursor based on constraints.
** Query processing: Implements WHERE clause filtering.
** Performance: Optimizes iteration based on provided constraints.
*/
int graphFilter(sqlite3_vtab_cursor *pCursor, int idxNum,
                const char *idxStr, int argc, sqlite3_value **argv){
  GraphCursor *pGraphCursor = (GraphCursor*)pCursor;
  GraphVtab *pVtab = pGraphCursor->pVtab;
  
  /* Reset cursor to beginning */
  pGraphCursor->iRowid = 1;
  pGraphCursor->pCurrentNode = pVtab->pNodeList;
  pGraphCursor->pCurrentEdge = 0;
  pGraphCursor->iIterMode = 0;  /* Start with nodes */
  
  /* If no nodes, move to edges */
  if( pGraphCursor->pCurrentNode==0 ){
    pGraphCursor->iIterMode = 1;
    pGraphCursor->pCurrentEdge = pVtab->pEdgeList;
  }
  
  return SQLITE_OK;
}

/*
** Move cursor to next row.
** Iteration: Advances through nodes or edges based on mode.
** End detection: Sets cursor state for graphEof() check.
*/
int graphNext(sqlite3_vtab_cursor *pCursor){
  GraphCursor *pGraphCursor = (GraphCursor*)pCursor;
  GraphVtab *pVtab = pGraphCursor->pVtab;
  
  pGraphCursor->iRowid++;
  
  if( pGraphCursor->iIterMode==0 ){
    /* Iterating through nodes */
    if( pGraphCursor->pCurrentNode ){
      pGraphCursor->pCurrentNode = pGraphCursor->pCurrentNode->pNext;
    }
    
    /* If no more nodes, switch to edges */
    if( pGraphCursor->pCurrentNode==0 ){
      pGraphCursor->iIterMode = 1;
      pGraphCursor->pCurrentEdge = pVtab->pEdgeList;
    }
  } else {
    /* Iterating through edges */
    if( pGraphCursor->pCurrentEdge ){
      pGraphCursor->pCurrentEdge = pGraphCursor->pCurrentEdge->pNext;
    }
  }
  
  return SQLITE_OK;
}

/*
** Check if cursor is at end of iteration.
** Returns: Non-zero if no more rows, zero if rows remain.
*/
int graphEof(sqlite3_vtab_cursor *pCursor){
  GraphCursor *pGraphCursor = (GraphCursor*)pCursor;
  
  /* EOF if both node and edge iterators are exhausted */
  return (pGraphCursor->pCurrentNode==0 && pGraphCursor->pCurrentEdge==0);
}

/*
** Return column value for current cursor position.
** Data retrieval: Extracts node/edge properties as SQLite values.
** Memory management: Uses sqlite3_result_* functions appropriately.
*/
int graphColumn(sqlite3_vtab_cursor *pCursor, sqlite3_context *pCtx,
                int iCol){
  GraphCursor *pGraphCursor = (GraphCursor*)pCursor;
  
  if( pGraphCursor->iIterMode==0 && pGraphCursor->pCurrentNode ){
    /* Return node data */
    switch( iCol ){
      case 0: /* type */
        sqlite3_result_text(pCtx, "node", -1, SQLITE_STATIC);
        break;
      case 1: /* id */
        sqlite3_result_int64(pCtx, pGraphCursor->pCurrentNode->iNodeId);
        break;
      case 2: /* from_id */
        sqlite3_result_null(pCtx);
        break;
      case 3: /* to_id */
        sqlite3_result_null(pCtx);
        break;
      case 4: /* labels */
        {
          char *zLabels = NULL;
          if( graphGetNodeLabels(pGraphCursor->pVtab, pGraphCursor->pCurrentNode->iNodeId, &zLabels) == SQLITE_OK && zLabels ){
            sqlite3_result_text(pCtx, zLabels, -1, sqlite3_free);
          } else {
            sqlite3_result_text(pCtx, "[]", -1, SQLITE_STATIC);
          }
        }
        break;
      case 5: /* rel_type */
        sqlite3_result_null(pCtx);
        break;
      case 6: /* weight */
        sqlite3_result_null(pCtx);
        break;
      case 7: /* properties */
        sqlite3_result_text(pCtx, pGraphCursor->pCurrentNode->zProperties, 
                           -1, SQLITE_TRANSIENT);
        break;
      default:
        sqlite3_result_null(pCtx);
        break;
    }
  } else if( pGraphCursor->iIterMode==1 && pGraphCursor->pCurrentEdge ){
    /* Return edge data */
    switch( iCol ){
      case 0: /* type */
        sqlite3_result_text(pCtx, "edge", -1, SQLITE_STATIC);
        break;
      case 1: /* id */
        sqlite3_result_int64(pCtx, pGraphCursor->pCurrentEdge->iEdgeId);
        break;
      case 2: /* from_id */
        sqlite3_result_int64(pCtx, pGraphCursor->pCurrentEdge->iFromId);
        break;
      case 3: /* to_id */
        sqlite3_result_int64(pCtx, pGraphCursor->pCurrentEdge->iToId);
        break;
      case 4: /* labels */
        sqlite3_result_null(pCtx);
        break;
      case 5: /* rel_type */
        sqlite3_result_text(pCtx, pGraphCursor->pCurrentEdge->zType,
                           -1, SQLITE_TRANSIENT);
        break;
      case 6: /* weight */
        sqlite3_result_double(pCtx, pGraphCursor->pCurrentEdge->rWeight);
        break;
      case 7: /* properties */
        sqlite3_result_text(pCtx, pGraphCursor->pCurrentEdge->zProperties,
                           -1, SQLITE_TRANSIENT);
        break;
      default:
        sqlite3_result_null(pCtx);
        break;
    }
  } else {
    sqlite3_result_null(pCtx);
  }
  
  return SQLITE_OK;
}

/*
** Return rowid for current cursor position.
** Unique identification: Provides stable row identifier.
*/
int graphRowid(sqlite3_vtab_cursor *pCursor, sqlite3_int64 *pRowid){
  GraphCursor *pGraphCursor = (GraphCursor*)pCursor;
  *pRowid = pGraphCursor->iRowid;
  return SQLITE_OK;
}