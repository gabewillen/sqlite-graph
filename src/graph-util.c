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

#ifdef SQLITE_CORE
#include "sqlite3.h"
#else
#include "sqlite3ext.h"
extern const sqlite3_api_routines *sqlite3_api;
/* SQLITE_EXTENSION_INIT1 - removed to prevent multiple definition */
#endif
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
  char *zSql;
  int rc;

  zSql = sqlite3_mprintf("INSERT INTO %s_nodes(id, properties) VALUES(%lld, %Q)", pVtab->zTableName, iNodeId, zProperties);
  rc = sqlite3_exec(pVtab->pDb, zSql, 0, 0, 0);
  sqlite3_free(zSql);

  return rc;
}

int graphRemoveNode(GraphVtab *pVtab, sqlite3_int64 iNodeId){
  char *zSql;
  int rc;

  zSql = sqlite3_mprintf("DELETE FROM %s_nodes WHERE id = %lld", pVtab->zTableName, iNodeId);
  rc = sqlite3_exec(pVtab->pDb, zSql, 0, 0, 0);
  sqlite3_free(zSql);
  if( rc!=SQLITE_OK ) return rc;

  zSql = sqlite3_mprintf("DELETE FROM %s_edges WHERE from_id = %lld OR to_id = %lld", pVtab->zTableName, iNodeId, iNodeId);
  rc = sqlite3_exec(pVtab->pDb, zSql, 0, 0, 0);
  sqlite3_free(zSql);

  return rc;
}

int graphGetNode(GraphVtab *pVtab, sqlite3_int64 iNodeId, 
                 char **pzProperties){
  char *zSql;
  sqlite3_stmt *pStmt;
  int rc;

  zSql = sqlite3_mprintf("SELECT properties FROM %s_nodes WHERE id = %lld", pVtab->zTableName, iNodeId);
  rc = sqlite3_prepare_v2(pVtab->pDb, zSql, -1, &pStmt, 0);
  sqlite3_free(zSql);
  if( rc!=SQLITE_OK ) return rc;

  rc = sqlite3_step(pStmt);
  if( rc==SQLITE_ROW ){
    *pzProperties = sqlite3_mprintf("%s", sqlite3_column_text(pStmt, 0));
    rc = SQLITE_OK;
  } else {
    *pzProperties = 0;
    rc = SQLITE_NOTFOUND;
  }
  sqlite3_finalize(pStmt);
  return rc;
}

int graphAddEdge(GraphVtab *pVtab, sqlite3_int64 iFromId, 
                 sqlite3_int64 iToId, double rWeight, 
                 const char *zProperties){
  char *zSql;
  int rc;

  zSql = sqlite3_mprintf("INSERT INTO %s_edges(from_id, to_id, weight, properties) VALUES(%lld, %lld, %f, %Q)", pVtab->zTableName, iFromId, iToId, rWeight, zProperties);
  rc = sqlite3_exec(pVtab->pDb, zSql, 0, 0, 0);
  sqlite3_free(zSql);

  return rc;
}

int graphRemoveEdge(GraphVtab *pVtab, sqlite3_int64 iFromId, 
                    sqlite3_int64 iToId){
  char *zSql;
  int rc;

  zSql = sqlite3_mprintf("DELETE FROM %s_edges WHERE from_id = %lld AND to_id = %lld", pVtab->zTableName, iFromId, iToId);
  rc = sqlite3_exec(pVtab->pDb, zSql, 0, 0, 0);
  sqlite3_free(zSql);

  return rc;
}

int graphUpdateNode(GraphVtab *pVtab, sqlite3_int64 iNodeId, 
                    const char *zProperties){
  char *zSql;
  int rc;

  zSql = sqlite3_mprintf("UPDATE %s_nodes SET properties = %Q WHERE id = %lld", pVtab->zTableName, zProperties, iNodeId);
  rc = sqlite3_exec(pVtab->pDb, zSql, 0, 0, 0);
  sqlite3_free(zSql);

  return rc;
}

int graphGetEdge(GraphVtab *pVtab, sqlite3_int64 iFromId, 
                 sqlite3_int64 iToId, double *prWeight, 
                 char **pzProperties){
  char *zSql;
  sqlite3_stmt *pStmt;
  int rc;

  zSql = sqlite3_mprintf("SELECT weight, properties FROM %s_edges WHERE from_id = %lld AND to_id = %lld", pVtab->zTableName, iFromId, iToId);
  rc = sqlite3_prepare_v2(pVtab->pDb, zSql, -1, &pStmt, 0);
  sqlite3_free(zSql);
  if( rc!=SQLITE_OK ) return rc;

  rc = sqlite3_step(pStmt);
  if( rc==SQLITE_ROW ){
    *prWeight = sqlite3_column_double(pStmt, 0);
    *pzProperties = sqlite3_mprintf("%s", sqlite3_column_text(pStmt, 1));
    rc = SQLITE_OK;
  } else {
    *prWeight = 0.0;
    *pzProperties = 0;
    rc = SQLITE_NOTFOUND;
  }
  sqlite3_finalize(pStmt);
  return rc;
}

int graphCountNodes(GraphVtab *pVtab){
  char *zSql;
  sqlite3_stmt *pStmt;
  int rc;
  int nNodes = 0;

  zSql = sqlite3_mprintf("SELECT count(*) FROM %s_nodes", pVtab->zTableName);
  rc = sqlite3_prepare_v2(pVtab->pDb, zSql, -1, &pStmt, 0);
  sqlite3_free(zSql);
  if( rc==SQLITE_OK && sqlite3_step(pStmt)==SQLITE_ROW ){
    nNodes = sqlite3_column_int(pStmt, 0);
  }
  sqlite3_finalize(pStmt);
  return nNodes;
}

int graphCountEdges(GraphVtab *pVtab){
  char *zSql;
  sqlite3_stmt *pStmt;
  int rc;
  int nEdges = 0;

  zSql = sqlite3_mprintf("SELECT count(*) FROM %s_edges", pVtab->zTableName);
  rc = sqlite3_prepare_v2(pVtab->pDb, zSql, -1, &pStmt, 0);
  sqlite3_free(zSql);
  if( rc==SQLITE_OK && sqlite3_step(pStmt)==SQLITE_ROW ){
    nEdges = sqlite3_column_int(pStmt, 0);
  }
  sqlite3_finalize(pStmt);
  return nEdges;
}

GraphNode *graphFindNode(GraphVtab *pVtab, sqlite3_int64 iNodeId){
  char *zSql;
  sqlite3_stmt *pStmt;
  int rc;
  GraphNode *pNode = 0;

  zSql = sqlite3_mprintf("SELECT id, properties FROM %s_nodes WHERE id = %lld", pVtab->zTableName, iNodeId);
  rc = sqlite3_prepare_v2(pVtab->pDb, zSql, -1, &pStmt, 0);
  sqlite3_free(zSql);
  if( rc!=SQLITE_OK ) return 0;

  rc = sqlite3_step(pStmt);
  if( rc==SQLITE_ROW ){
    pNode = sqlite3_malloc(sizeof(GraphNode));
    if( pNode ){
      pNode->iNodeId = sqlite3_column_int64(pStmt, 0);
      pNode->zProperties = sqlite3_mprintf("%s", sqlite3_column_text(pStmt, 1));
    }
  }
  sqlite3_finalize(pStmt);
  return pNode;
}

GraphEdge *graphFindEdge(GraphVtab *pVtab, sqlite3_int64 iFromId, 
                         sqlite3_int64 iToId){
  char *zSql;
  sqlite3_stmt *pStmt;
  int rc;
  GraphEdge *pEdge = 0;

  zSql = sqlite3_mprintf("SELECT id, from_id, to_id, weight, properties FROM %s_edges WHERE from_id = %lld AND to_id = %lld", pVtab->zTableName, iFromId, iToId);
  rc = sqlite3_prepare_v2(pVtab->pDb, zSql, -1, &pStmt, 0);
  sqlite3_free(zSql);
  if( rc!=SQLITE_OK ) return 0;

  rc = sqlite3_step(pStmt);
  if( rc==SQLITE_ROW ){
    pEdge = sqlite3_malloc(sizeof(GraphEdge));
    if( pEdge ){
      pEdge->iEdgeId = sqlite3_column_int64(pStmt, 0);
      pEdge->iFromId = sqlite3_column_int64(pStmt, 1);
      pEdge->iToId = sqlite3_column_int64(pStmt, 2);
      pEdge->rWeight = sqlite3_column_double(pStmt, 3);
      pEdge->zProperties = sqlite3_mprintf("%s", sqlite3_column_text(pStmt, 4));
    }
  }
  sqlite3_finalize(pStmt);
  return pEdge;
}