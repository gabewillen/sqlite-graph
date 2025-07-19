/*
** SQLite Graph Database Extension - Hardened Virtual Table Implementation
**
** This file implements the virtual table interface with hardened memory management.
** Uses RAII-style helpers and consistent sqlite3_malloc/sqlite3_free patterns
** to eliminate double-frees and memory leaks.
*/

#include "sqlite3ext.h"
#ifndef SQLITE_CORE
extern const sqlite3_api_routines *sqlite3_api;
#endif
#include "graph.h"
#include "graph-vtab.h"
#include "graph-memory.h"
#include <string.h>
#include <assert.h>
#include <stdio.h>

/*
** Forward declarations for virtual table methods
*/
static int graphCreate(sqlite3*, void*, int, const char*const*, sqlite3_vtab**, char**);
static int graphConnect(sqlite3*, void*, int, const char*const*, sqlite3_vtab**, char**);
static int graphBestIndex(sqlite3_vtab*, sqlite3_index_info*);
static int graphDisconnect(sqlite3_vtab*);
static int graphDestroy(sqlite3_vtab*);
static int graphOpen(sqlite3_vtab*, sqlite3_vtab_cursor**);
static int graphClose(sqlite3_vtab_cursor*);
static int graphFilter(sqlite3_vtab_cursor*, int, const char*, int, sqlite3_value**);
static int graphNext(sqlite3_vtab_cursor*);
static int graphEof(sqlite3_vtab_cursor*);
static int graphColumn(sqlite3_vtab_cursor*, sqlite3_context*, int);
static int graphRowid(sqlite3_vtab_cursor*, sqlite3_int64*);
static int graphUpdate(sqlite3_vtab*, int, sqlite3_value**, sqlite3_int64*);

/*
** Virtual table module structure with hardened destructor callbacks
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
  graphUpdate,          /* xUpdate */
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
** Create a new virtual table instance with hardened memory management
*/
static int graphCreate(sqlite3 *pDb, void *pAux, int argc, 
                       const char *const *argv, sqlite3_vtab **ppVtab, 
                       char **pzErr) {
  GRAPH_MEMORY_GUARD_BEGIN(mem_ctx);
  GraphVtab *pNew;
  int rc = SQLITE_OK;
  char *zSql;
  
  /* Suppress unused parameter warnings */
  (void)pAux;
  
  /* Validate arguments */
  assert(argc >= 3);
  assert(argv != 0);
  assert(ppVtab != 0);
  
  /* Allocate new virtual table structure using hardened allocation */
  pNew = graph_malloc_safe(&mem_ctx, sizeof(*pNew));
  if (!pNew) {
    GRAPH_MEMORY_GUARD_RETURN(mem_ctx, SQLITE_NOMEM);
  }
  
  /* Initialize structure to zero */
  memset(pNew, 0, sizeof(*pNew));
  
  /* Set up virtual table base */
  pNew->pDb = pDb;
  pNew->nRef = 1;
  
  /* Copy database and table names using safe allocation */
  pNew->zDbName = graph_mprintf_safe(&mem_ctx, "%s", argv[1]);
  pNew->zTableName = graph_mprintf_safe(&mem_ctx, "%s", argv[2]);
  
  if (!pNew->zDbName || !pNew->zTableName) {
    GRAPH_MEMORY_GUARD_RETURN(mem_ctx, SQLITE_NOMEM);
  }
  
  /* Create schema declaration */
  zSql = graph_mprintf_safe(&mem_ctx, 
    "CREATE TABLE x("
    "id INTEGER PRIMARY KEY,"
    "type TEXT,"
    "from_id INTEGER,"
    "to_id INTEGER,"
    "weight REAL,"
    "properties TEXT"
    ")");
  
  if (!zSql) {
    GRAPH_MEMORY_GUARD_RETURN(mem_ctx, SQLITE_NOMEM);
  }
  
  rc = sqlite3_declare_vtab(pDb, zSql);
  if (rc != SQLITE_OK) {
    GRAPH_MEMORY_GUARD_RETURN(mem_ctx, rc);
  }
  
  /* Create backing tables for nodes and edges */
  zSql = graph_mprintf_safe(&mem_ctx,
    "CREATE TABLE IF NOT EXISTS %s_nodes("
    "id INTEGER PRIMARY KEY,"
    "properties TEXT"
    ")", pNew->zTableName);
  
  if (!zSql) {
    GRAPH_MEMORY_GUARD_RETURN(mem_ctx, SQLITE_NOMEM);
  }
  
  rc = sqlite3_exec(pDb, zSql, 0, 0, pzErr);
  if (rc != SQLITE_OK) {
    GRAPH_MEMORY_GUARD_RETURN(mem_ctx, rc);
  }
  
  zSql = graph_mprintf_safe(&mem_ctx,
    "CREATE TABLE IF NOT EXISTS %s_edges("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "from_id INTEGER,"
    "to_id INTEGER,"
    "weight REAL DEFAULT 1.0,"
    "properties TEXT,"
    "FOREIGN KEY(from_id) REFERENCES %s_nodes(id),"
    "FOREIGN KEY(to_id) REFERENCES %s_nodes(id)"
    ")", pNew->zTableName, pNew->zTableName, pNew->zTableName);
  
  if (!zSql) {
    GRAPH_MEMORY_GUARD_RETURN(mem_ctx, SQLITE_NOMEM);
  }
  
  rc = sqlite3_exec(pDb, zSql, 0, 0, pzErr);
  if (rc != SQLITE_OK) {
    GRAPH_MEMORY_GUARD_RETURN(mem_ctx, rc);
  }
  
  /* Remove auto-cleanup for successful allocations - they're now owned by the vtab */
  mem_ctx.is_active = 0;
  
  *ppVtab = &pNew->base;
  setGlobalGraph(pNew);
  
  return SQLITE_OK;
}

/*
** Connect to existing virtual table - same as create for this implementation
*/
static int graphConnect(sqlite3 *pDb, void *pAux, int argc,
                        const char *const *argv, sqlite3_vtab **ppVtab,
                        char **pzErr) {
  return graphCreate(pDb, pAux, argc, argv, ppVtab, pzErr);
}

/*
** Provide the query planner with information about the virtual table
*/
static int graphBestIndex(sqlite3_vtab *tab, sqlite3_index_info *pIdxInfo) {
  /* Suppress unused parameter warnings */
  (void)tab;
  
  pIdxInfo->estimatedCost = 1000000.0; /* High cost encourages proper indexing */
  return SQLITE_OK;
}

/*
** Disconnect from virtual table with proper cleanup
*/
static int graphDisconnect(sqlite3_vtab *pVtab) {
  return graph_vtab_destroy_safe((GraphVtab*)pVtab);
}

/*
** Destroy virtual table with hardened cleanup
*/
static int graphDestroy(sqlite3_vtab *pVtab) {
  GraphVtab *pGraph = (GraphVtab*)pVtab;
  GRAPH_MEMORY_GUARD_BEGIN(mem_ctx);
  char *zSql;
  
  if (!pGraph) return SQLITE_OK;
  
  /* Drop backing tables */
  if (pGraph->zTableName && pGraph->pDb) {
    zSql = graph_mprintf_safe(&mem_ctx, "DROP TABLE IF EXISTS %s_nodes", pGraph->zTableName);
    if (zSql) {
      sqlite3_exec(pGraph->pDb, zSql, 0, 0, 0);
    }
    
    zSql = graph_mprintf_safe(&mem_ctx, "DROP TABLE IF EXISTS %s_edges", pGraph->zTableName);
    if (zSql) {
      sqlite3_exec(pGraph->pDb, zSql, 0, 0, 0);
    }
  }
  
  GRAPH_MEMORY_GUARD_END(mem_ctx);
  
  /* Clear global reference if this is the global graph */
  if (getGlobalGraph() == pGraph) {
    setGlobalGraph(NULL);
  }
  
  return graph_vtab_destroy_safe(pGraph);
}

/*
** Open a cursor for table scanning with hardened memory management
*/
static int graphOpen(sqlite3_vtab *pVtab, sqlite3_vtab_cursor **ppCursor) {
  GRAPH_MEMORY_GUARD_BEGIN(mem_ctx);
  GraphCursor *pCur;
  
  pCur = graph_cursor_create(&mem_ctx, (GraphVtab*)pVtab);
  if (!pCur) {
    GRAPH_MEMORY_GUARD_RETURN(mem_ctx, SQLITE_NOMEM);
  }
  
  /* Remove auto-cleanup - cursor is now owned by SQLite */
  mem_ctx.is_active = 0;
  
  *ppCursor = &pCur->base;
  return SQLITE_OK;
}

/*
** Close cursor with proper cleanup
*/
static int graphClose(sqlite3_vtab_cursor *cur) {
  graph_cursor_destroy((GraphCursor*)cur);
  return SQLITE_OK;
}

/*
** Filter cursor for query with hardened SQL preparation
*/
static int graphFilter(sqlite3_vtab_cursor *pVtabCursor, int idxNum,
                       const char *idxStr, int argc, sqlite3_value **argv) {
  GraphCursor *pCur = (GraphCursor*)pVtabCursor;
  GraphVtab *pVtab = pCur->pVtab;
  GRAPH_MEMORY_GUARD_BEGIN(mem_ctx);
  char *zSql;
  int rc = SQLITE_OK;
  
  /* Suppress unused parameter warnings */
  (void)idxNum;
  (void)idxStr;
  (void)argc;
  (void)argv;
  
  /* Finalize any existing statements */
  if (pCur->pNodeStmt) {
    sqlite3_finalize(pCur->pNodeStmt);
    pCur->pNodeStmt = NULL;
  }
  if (pCur->pEdgeStmt) {
    sqlite3_finalize(pCur->pEdgeStmt);
    pCur->pEdgeStmt = NULL;
  }
  
  /* Prepare node query */
  zSql = graph_mprintf_safe(&mem_ctx, 
    "SELECT id, 'node' as type, NULL as from_id, NULL as to_id, "
    "NULL as weight, properties FROM %s_nodes ORDER BY id", 
    pVtab->zTableName);
  
  if (!zSql) {
    GRAPH_MEMORY_GUARD_RETURN(mem_ctx, SQLITE_NOMEM);
  }
  
  rc = sqlite3_prepare_v2(pVtab->pDb, zSql, -1, &pCur->pNodeStmt, 0);
  if (rc != SQLITE_OK) {
    GRAPH_MEMORY_GUARD_RETURN(mem_ctx, rc);
  }
  
  /* Prepare edge query */
  zSql = graph_mprintf_safe(&mem_ctx,
    "SELECT id, 'edge' as type, from_id, to_id, weight, properties "
    "FROM %s_edges ORDER BY id", 
    pVtab->zTableName);
  
  if (!zSql) {
    GRAPH_MEMORY_GUARD_RETURN(mem_ctx, SQLITE_NOMEM);
  }
  
  rc = sqlite3_prepare_v2(pVtab->pDb, zSql, -1, &pCur->pEdgeStmt, 0);
  if (rc != SQLITE_OK) {
    GRAPH_MEMORY_GUARD_RETURN(mem_ctx, rc);
  }
  
  /* Start with nodes */
  pCur->iIterMode = 0;
  pCur->iRowid = 1;
  
  GRAPH_MEMORY_GUARD_END(mem_ctx);
  
  return graphNext(pVtabCursor);
}

/*
** Move cursor to next row
*/
static int graphNext(sqlite3_vtab_cursor *cur) {
  GraphCursor *pCur = (GraphCursor*)cur;
  int rc;
  
  if (pCur->iIterMode == 0) {
    /* Node iteration */
    if (pCur->pNodeStmt) {
      rc = sqlite3_step(pCur->pNodeStmt);
      if (rc == SQLITE_ROW) {
        pCur->iRowid++;
        return SQLITE_OK;
      } else if (rc == SQLITE_DONE) {
        /* Switch to edge iteration */
        pCur->iIterMode = 1;
        return graphNext(cur);
      } else {
        return rc;
      }
    }
  } else {
    /* Edge iteration */
    if (pCur->pEdgeStmt) {
      rc = sqlite3_step(pCur->pEdgeStmt);
      if (rc == SQLITE_ROW) {
        pCur->iRowid++;
        return SQLITE_OK;
      } else {
        return rc;
      }
    }
  }
  
  return SQLITE_OK;
}

/*
** Check if cursor is at end
*/
static int graphEof(sqlite3_vtab_cursor *cur) {
  GraphCursor *pCur = (GraphCursor*)cur;
  
  if (pCur->iIterMode == 0 && pCur->pNodeStmt) {
    return 0; /* Still in node iteration */
  } else if (pCur->iIterMode == 1 && pCur->pEdgeStmt) {
    /* Check if edge statement has more rows */
    return sqlite3_data_count(pCur->pEdgeStmt) == 0;
  }
  
  return 1; /* End of iteration */
}

/*
** Return column data for current cursor position
*/
static int graphColumn(sqlite3_vtab_cursor *cur, sqlite3_context *ctx, int i) {
  GraphCursor *pCur = (GraphCursor*)cur;
  sqlite3_stmt *pStmt = (pCur->iIterMode == 0) ? pCur->pNodeStmt : pCur->pEdgeStmt;
  
  if (!pStmt) {
    sqlite3_result_null(ctx);
    return SQLITE_OK;
  }
  
  /* Column mapping: id, type, from_id, to_id, weight, properties */
  switch (i) {
    case 0: /* id */
      sqlite3_result_int64(ctx, sqlite3_column_int64(pStmt, 0));
      break;
    case 1: /* type */
      sqlite3_result_text(ctx, (const char*)sqlite3_column_text(pStmt, 1), -1, SQLITE_TRANSIENT);
      break;
    case 2: /* from_id */
      if (sqlite3_column_type(pStmt, 2) != SQLITE_NULL) {
        sqlite3_result_int64(ctx, sqlite3_column_int64(pStmt, 2));
      } else {
        sqlite3_result_null(ctx);
      }
      break;
    case 3: /* to_id */
      if (sqlite3_column_type(pStmt, 3) != SQLITE_NULL) {
        sqlite3_result_int64(ctx, sqlite3_column_int64(pStmt, 3));
      } else {
        sqlite3_result_null(ctx);
      }
      break;
    case 4: /* weight */
      if (sqlite3_column_type(pStmt, 4) != SQLITE_NULL) {
        sqlite3_result_double(ctx, sqlite3_column_double(pStmt, 4));
      } else {
        sqlite3_result_null(ctx);
      }
      break;
    case 5: /* properties */
      if (sqlite3_column_type(pStmt, 5) != SQLITE_NULL) {
        sqlite3_result_text(ctx, (const char*)sqlite3_column_text(pStmt, 5), -1, SQLITE_TRANSIENT);
      } else {
        sqlite3_result_null(ctx);
      }
      break;
    default:
      sqlite3_result_null(ctx);
      break;
  }
  
  return SQLITE_OK;
}

/*
** Return rowid for current cursor position
*/
static int graphRowid(sqlite3_vtab_cursor *cur, sqlite3_int64 *pRowid) {
  GraphCursor *pCur = (GraphCursor*)cur;
  *pRowid = pCur->iRowid;
  return SQLITE_OK;
}

/*
** Handle INSERT/UPDATE/DELETE operations with hardened memory management
*/
static int graphUpdate(sqlite3_vtab *pVTab, int argc, sqlite3_value **argv,
                       sqlite3_int64 *pRowid) {
  GraphVtab *pGraph = (GraphVtab*)pVTab;
  GRAPH_MEMORY_GUARD_BEGIN(mem_ctx);
  char *zSql;
  int rc = SQLITE_OK;
  
  if (argc == 1) {
    /* DELETE operation */
    sqlite3_int64 rowid = sqlite3_value_int64(argv[0]);
    
    /* Try to delete from both nodes and edges tables */
    zSql = graph_mprintf_safe(&mem_ctx, "DELETE FROM %s_nodes WHERE id = ?", pGraph->zTableName);
    if (zSql) {
      sqlite3_stmt *pStmt;
      rc = sqlite3_prepare_v2(pGraph->pDb, zSql, -1, &pStmt, 0);
      if (rc == SQLITE_OK) {
        sqlite3_bind_int64(pStmt, 1, rowid);
        rc = sqlite3_step(pStmt);
        sqlite3_finalize(pStmt);
        if (rc == SQLITE_DONE) rc = SQLITE_OK;
      }
    }
    
    if (rc == SQLITE_OK) {
      zSql = graph_mprintf_safe(&mem_ctx, "DELETE FROM %s_edges WHERE id = ?", pGraph->zTableName);
      if (zSql) {
        sqlite3_stmt *pStmt;
        rc = sqlite3_prepare_v2(pGraph->pDb, zSql, -1, &pStmt, 0);
        if (rc == SQLITE_OK) {
          sqlite3_bind_int64(pStmt, 1, rowid);
          rc = sqlite3_step(pStmt);
          sqlite3_finalize(pStmt);
          if (rc == SQLITE_DONE) rc = SQLITE_OK;
        }
      }
    }
  } else if (argc > 1) {
    /* INSERT or UPDATE operation */
    const char *type = (const char*)sqlite3_value_text(argv[2]); /* Column 1: type */
    
    if (type && strcmp(type, "node") == 0) {
      /* Node operation */
      sqlite3_int64 id = sqlite3_value_int64(argv[1]); /* Column 0: id */
      const char *properties = (const char*)sqlite3_value_text(argv[6]); /* Column 5: properties */
      
      zSql = graph_mprintf_safe(&mem_ctx, 
        "INSERT OR REPLACE INTO %s_nodes(id, properties) VALUES(?, ?)", 
        pGraph->zTableName);
      
      if (zSql) {
        sqlite3_stmt *pStmt;
        rc = sqlite3_prepare_v2(pGraph->pDb, zSql, -1, &pStmt, 0);
        if (rc == SQLITE_OK) {
          sqlite3_bind_int64(pStmt, 1, id);
          sqlite3_bind_text(pStmt, 2, properties, -1, SQLITE_TRANSIENT);
          rc = sqlite3_step(pStmt);
          sqlite3_finalize(pStmt);
          if (rc == SQLITE_DONE) {
            rc = SQLITE_OK;
            *pRowid = id;
          }
        }
      }
    } else if (type && strcmp(type, "edge") == 0) {
      /* Edge operation */
      sqlite3_int64 from_id = sqlite3_value_int64(argv[3]); /* Column 2: from_id */
      sqlite3_int64 to_id = sqlite3_value_int64(argv[4]); /* Column 3: to_id */
      double weight = sqlite3_value_double(argv[5]); /* Column 4: weight */
      const char *properties = (const char*)sqlite3_value_text(argv[6]); /* Column 5: properties */
      
      zSql = graph_mprintf_safe(&mem_ctx,
        "INSERT INTO %s_edges(from_id, to_id, weight, properties) VALUES(?, ?, ?, ?)",
        pGraph->zTableName);
      
      if (zSql) {
        sqlite3_stmt *pStmt;
        rc = sqlite3_prepare_v2(pGraph->pDb, zSql, -1, &pStmt, 0);
        if (rc == SQLITE_OK) {
          sqlite3_bind_int64(pStmt, 1, from_id);
          sqlite3_bind_int64(pStmt, 2, to_id);
          sqlite3_bind_double(pStmt, 3, weight);
          sqlite3_bind_text(pStmt, 4, properties, -1, SQLITE_TRANSIENT);
          rc = sqlite3_step(pStmt);
          sqlite3_finalize(pStmt);
          if (rc == SQLITE_DONE) {
            rc = SQLITE_OK;
            *pRowid = sqlite3_last_insert_rowid(pGraph->pDb);
          }
        }
      }
    }
  }
  
  GRAPH_MEMORY_GUARD_END(mem_ctx);
  return rc;
}
