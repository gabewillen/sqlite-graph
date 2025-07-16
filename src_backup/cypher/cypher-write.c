/*
** SQLite Graph Database Extension - Cypher Write Operations Implementation
**
** This file implements the core write operations for Cypher: CREATE, MERGE,
** SET, DELETE, and DETACH DELETE. Includes transaction management and
** rollback support using SQLite transaction semantics.
**
** Memory allocation: All functions use sqlite3_malloc()/sqlite3_free()
** Error handling: Functions return SQLite error codes (SQLITE_OK, etc.)
** Transaction safety: All operations integrate with SQLite transactions
*/

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include "cypher-write.h"
#include "graph-vtab.h"

/*
** Write context management functions.
*/

/*
** Create a new write context for mutation operations.
** Returns NULL on allocation failure.
*/
CypherWriteContext *cypherWriteContextCreate(sqlite3 *pDb, GraphVtab *pGraph,
                                           ExecutionContext *pExecContext) {
    CypherWriteContext *pCtx;
    
    if (!pDb || !pGraph || !pExecContext) {
        return NULL;
    }
    
    pCtx = (CypherWriteContext*)sqlite3_malloc(sizeof(CypherWriteContext));
    if (!pCtx) {
        return NULL;
    }
    
    memset(pCtx, 0, sizeof(CypherWriteContext));
    pCtx->pDb = pDb;
    pCtx->pGraph = pGraph;
    pCtx->pExecContext = pExecContext;
    pCtx->bAutoCommit = 1;  /* Default to auto-commit */
    pCtx->iNextNodeId = 1;  /* Start node IDs at 1 */
    pCtx->iNextRelId = 1;   /* Start relationship IDs at 1 */
    
    return pCtx;
}

/*
** Destroy a write context and free all associated memory.
** Automatically rolls back any uncommitted operations.
*/
void cypherWriteContextDestroy(CypherWriteContext *pCtx) {
    CypherWriteOp *pOp, *pNext;
    
    if (!pCtx) return;
    
    /* Rollback any uncommitted operations */
    if (pCtx->bInTransaction) {
        cypherWriteContextRollback(pCtx);
    }
    
    /* Free operation list */
    for (pOp = pCtx->pOperations; pOp; pOp = pNext) {
        pNext = pOp->pNext;
        cypherWriteOpDestroy(pOp);
    }
    
    /* Free error message */
    if (pCtx->zErrorMsg) {
        sqlite3_free(pCtx->zErrorMsg);
    }
    
    sqlite3_free(pCtx);
}

/*
** Begin a write transaction in the context.
** Returns SQLITE_OK on success, error code on failure.
*/
int cypherWriteContextBegin(CypherWriteContext *pCtx) {
    int rc;
    
    if (!pCtx) return SQLITE_MISUSE;
    if (pCtx->bInTransaction) return SQLITE_OK;  /* Already in transaction */
    
    /* Begin SQLite transaction */
    rc = sqlite3_exec(pCtx->pDb, "BEGIN", 0, 0, 0);
    if (rc != SQLITE_OK) {
        return rc;
    }
    
    pCtx->bInTransaction = 1;
    pCtx->bAutoCommit = 0;
    
    return SQLITE_OK;
}

/*
** Commit all operations in the write context.
** Returns SQLITE_OK on success, error code on failure.
*/
int cypherWriteContextCommit(CypherWriteContext *pCtx) {
    int rc;
    
    if (!pCtx) return SQLITE_MISUSE;
    if (!pCtx->bInTransaction) return SQLITE_OK;  /* Nothing to commit */
    
    /* First execute all pending write operations */
    rc = cypherExecuteOperations(pCtx);
    if (rc != SQLITE_OK) {
        /* Execute failed, rollback SQLite transaction */
        sqlite3_exec(pCtx->pDb, "ROLLBACK", 0, 0, 0);
        cypherRollbackOperations(pCtx);
        pCtx->bInTransaction = 0;
        pCtx->bAutoCommit = 1;
        return rc;
    }
    
    /* Commit SQLite transaction */
    rc = sqlite3_exec(pCtx->pDb, "COMMIT", 0, 0, 0);
    if (rc != SQLITE_OK) {
        /* SQLite commit failed, rollback our operations */
        sqlite3_exec(pCtx->pDb, "ROLLBACK", 0, 0, 0);
        cypherRollbackOperations(pCtx);
        pCtx->bInTransaction = 0;
        pCtx->bAutoCommit = 1;
        return rc;
    }
    
    /* Success - clear operation log and reset state */
    pCtx->bInTransaction = 0;
    pCtx->bAutoCommit = 1;
    
    return SQLITE_OK;
}

/*
** Rollback all operations in the write context.
** Returns SQLITE_OK on success, error code on failure.
*/
int cypherWriteContextRollback(CypherWriteContext *pCtx) {
    int rc;
    
    if (!pCtx) return SQLITE_MISUSE;
    if (!pCtx->bInTransaction) return SQLITE_OK;  /* Nothing to rollback */
    
    /* First rollback our write operations */
    cypherRollbackOperations(pCtx);
    
    /* Then rollback SQLite transaction */
    rc = sqlite3_exec(pCtx->pDb, "ROLLBACK", 0, 0, 0);
    
    pCtx->bInTransaction = 0;
    pCtx->bAutoCommit = 1;
    
    return rc;
}

/*
** Add a write operation to the transaction log.
** Returns SQLITE_OK on success, SQLITE_NOMEM on allocation failure.
*/
int cypherWriteContextAddOperation(CypherWriteContext *pCtx, CypherWriteOp *pOp) {
    if (!pCtx || !pOp) return SQLITE_MISUSE;
    
    /* Add to end of operation list */
    if (pCtx->pLastOp) {
        pCtx->pLastOp->pNext = pOp;
    } else {
        pCtx->pOperations = pOp;
    }
    pCtx->pLastOp = pOp;
    pCtx->nOperations++;
    
    return SQLITE_OK;
}

/*
** Get the next available node ID from the context.
** Updates the context's next ID counter.
*/
sqlite3_int64 cypherWriteContextNextNodeId(CypherWriteContext *pCtx) {
    if (!pCtx) return -1;
    return pCtx->iNextNodeId++;
}

/*
** Get the next available relationship ID from the context.
** Updates the context's next ID counter.
*/
sqlite3_int64 cypherWriteContextNextRelId(CypherWriteContext *pCtx) {
    if (!pCtx) return -1;
    return pCtx->iNextRelId++;
}

/*
** CREATE operation functions.
*/

/*
** Execute a CREATE node operation.
** Returns SQLITE_OK on success, error code on failure.
*/
int cypherCreateNode(CypherWriteContext *pCtx, CreateNodeOp *pOp) {
    CypherWriteOp *pWriteOp;
    char *zLabelsJson = NULL;
    char *zPropsJson = NULL;
    int rc = SQLITE_OK;
    int i;
    
    if (!pCtx || !pOp) return SQLITE_MISUSE;
    
    /* Get next node ID */
    pOp->iCreatedNodeId = cypherWriteContextNextNodeId(pCtx);
    if (pOp->iCreatedNodeId < 0) {
        return SQLITE_ERROR;
    }
    
    /* Build labels JSON array */
    if (pOp->nLabels > 0) {
        char *zLabels = sqlite3_malloc(1024);  /* Simple fixed buffer for now */
        if (!zLabels) return SQLITE_NOMEM;
        
        strcpy(zLabels, "[");
        for (i = 0; i < pOp->nLabels; i++) {
            if (i > 0) strcat(zLabels, ",");
            strcat(zLabels, "\"");
            strcat(zLabels, pOp->azLabels[i]);
            strcat(zLabels, "\"");
        }
        strcat(zLabels, "]");
        zLabelsJson = zLabels;
    } else {
        zLabelsJson = sqlite3_mprintf("[]");
    }
    
    /* Build properties JSON object */
    if (pOp->nProperties > 0) {
        char *zProps = sqlite3_malloc(2048);  /* Simple fixed buffer for now */
        if (!zProps) {
            sqlite3_free(zLabelsJson);
            return SQLITE_NOMEM;
        }
        
        strcpy(zProps, "{");
        for (i = 0; i < pOp->nProperties; i++) {
            if (i > 0) strcat(zProps, ",");
            strcat(zProps, "\"");
            strcat(zProps, pOp->azPropNames[i]);
            strcat(zProps, "\":");
            
            /* Simple value serialization */
            if (pOp->apPropValues[i]->type == CYPHER_VALUE_STRING) {
                strcat(zProps, "\"");
                strcat(zProps, pOp->apPropValues[i]->u.zString);
                strcat(zProps, "\"");
            } else if (pOp->apPropValues[i]->type == CYPHER_VALUE_INTEGER) {
                char zNum[32];
                sprintf(zNum, "%lld", pOp->apPropValues[i]->u.iInteger);
                strcat(zProps, zNum);
            } else if (pOp->apPropValues[i]->type == CYPHER_VALUE_FLOAT) {
                char zNum[32];
                sprintf(zNum, "%g", pOp->apPropValues[i]->u.rFloat);
                strcat(zProps, zNum);
            } else {
                strcat(zProps, "null");
            }
        }
        strcat(zProps, "}");
        zPropsJson = zProps;
    } else {
        zPropsJson = sqlite3_mprintf("{}");
    }
    
    /* Create write operation record */
    pWriteOp = cypherWriteOpCreate(CYPHER_WRITE_CREATE_NODE);
    if (!pWriteOp) {
        sqlite3_free(zLabelsJson);
        sqlite3_free(zPropsJson);
        return SQLITE_NOMEM;
    }
    
    pWriteOp->iNodeId = pOp->iCreatedNodeId;
    pWriteOp->zNewLabels = zLabelsJson;
    
    /* Add to operation log */
    rc = cypherWriteContextAddOperation(pCtx, pWriteOp);
    if (rc != SQLITE_OK) {
        cypherWriteOpDestroy(pWriteOp);
        sqlite3_free(zLabelsJson);
        sqlite3_free(zPropsJson);
        return rc;
    }
    
    /* TODO: Actually add node to graph storage here */
    /* For now, just simulate success */
    
    /* Bind variable in execution context */
    if (pOp->zVariable) {
        CypherValue nodeValue;
        nodeValue.type = CYPHER_VALUE_NODE;
        nodeValue.u.iNodeId = pOp->iCreatedNodeId;
        /* TODO: Bind variable in execution context */
    }
    
    sqlite3_free(zPropsJson);
    return SQLITE_OK;
}

/*
** Execute a CREATE relationship operation.
** Returns SQLITE_OK on success, error code on failure.
*/
int cypherCreateRelationship(CypherWriteContext *pCtx, CreateRelOp *pOp) {
    CypherWriteOp *pWriteOp;
    char *zPropsJson = NULL;
    int rc = SQLITE_OK;
    int i;
    
    if (!pCtx || !pOp) return SQLITE_MISUSE;
    
    /* Validate that source and target nodes exist */
    rc = cypherValidateNodeExists(pCtx, pOp->iFromNodeId);
    if (rc != SQLITE_OK) {
        return rc;
    }
    
    rc = cypherValidateNodeExists(pCtx, pOp->iToNodeId);
    if (rc != SQLITE_OK) {
        return rc;
    }
    
    /* Get next relationship ID */
    pOp->iCreatedRelId = cypherWriteContextNextRelId(pCtx);
    if (pOp->iCreatedRelId < 0) {
        return SQLITE_ERROR;
    }
    
    /* Build properties JSON object */
    if (pOp->nProperties > 0) {
        char *zProps = sqlite3_malloc(2048);  /* Simple fixed buffer for now */
        if (!zProps) return SQLITE_NOMEM;
        
        strcpy(zProps, "{");
        for (i = 0; i < pOp->nProperties; i++) {
            if (i > 0) strcat(zProps, ",");
            strcat(zProps, "\"");
            strcat(zProps, pOp->azPropNames[i]);
            strcat(zProps, "\":");
            
            /* Simple value serialization */
            if (pOp->apPropValues[i]->type == CYPHER_VALUE_STRING) {
                strcat(zProps, "\"");
                strcat(zProps, pOp->apPropValues[i]->u.zString);
                strcat(zProps, "\"");
            } else if (pOp->apPropValues[i]->type == CYPHER_VALUE_INTEGER) {
                char zNum[32];
                sprintf(zNum, "%lld", pOp->apPropValues[i]->u.iInteger);
                strcat(zProps, zNum);
            } else {
                strcat(zProps, "null");
            }
        }
        strcat(zProps, "}");
        zPropsJson = zProps;
    } else {
        zPropsJson = sqlite3_mprintf("{}");
    }
    
    /* Create write operation record */
    pWriteOp = cypherWriteOpCreate(CYPHER_WRITE_CREATE_RELATIONSHIP);
    if (!pWriteOp) {
        sqlite3_free(zPropsJson);
        return SQLITE_NOMEM;
    }
    
    pWriteOp->iRelId = pOp->iCreatedRelId;
    pWriteOp->iFromId = pOp->iFromNodeId;
    pWriteOp->iToId = pOp->iToNodeId;
    pWriteOp->zRelType = sqlite3_mprintf("%s", pOp->zRelType ? pOp->zRelType : "");
    
    /* Add to operation log */
    rc = cypherWriteContextAddOperation(pCtx, pWriteOp);
    if (rc != SQLITE_OK) {
        cypherWriteOpDestroy(pWriteOp);
        sqlite3_free(zPropsJson);
        return rc;
    }
    
    /* TODO: Actually add relationship to graph storage here */
    /* For now, just simulate success */
    
    /* Bind variable in execution context */
    if (pOp->zRelVar) {
        CypherValue relValue;
        relValue.type = CYPHER_VALUE_RELATIONSHIP;
        relValue.u.iRelId = pOp->iCreatedRelId;
        /* TODO: Bind variable in execution context */
    }
    
    sqlite3_free(zPropsJson);
    return SQLITE_OK;
}

/*
** Utility functions for write operations.
*/

/*
** Validate that a node exists before creating relationships.
** Returns SQLITE_OK if node exists, SQLITE_ERROR if not found.
*/
int cypherValidateNodeExists(CypherWriteContext *pCtx, sqlite3_int64 iNodeId) {
    /* TODO: Check if node exists in graph storage */
    /* For now, assume all positive node IDs exist */
    return (iNodeId > 0) ? SQLITE_OK : SQLITE_ERROR;
}

/*
** Check if a node matches the given labels and properties.
** Returns 1 if match, 0 if no match, -1 on error.
*/
int cypherNodeMatches(CypherWriteContext *pCtx, sqlite3_int64 iNodeId,
                     char **azLabels, int nLabels,
                     char **azProps, CypherValue **apValues, int nProps) {
    /* TODO: Implement actual node matching logic */
    /* For now, return no match */
    return 0;
}

/*
** Find a node that matches the given criteria.
** Returns node ID if found, 0 if not found, -1 on error.
*/
sqlite3_int64 cypherFindMatchingNode(CypherWriteContext *pCtx,
                                    char **azLabels, int nLabels,
                                    char **azProps, CypherValue **apValues, int nProps) {
    /* TODO: Implement actual node search logic */
    /* For now, return not found */
    return 0;
}

/*
** Get all relationships connected to a node (for DETACH DELETE).
** Returns JSON array of relationship IDs, caller must sqlite3_free().
*/
char *cypherGetNodeRelationships(CypherWriteContext *pCtx, sqlite3_int64 iNodeId) {
    /* TODO: Implement actual relationship lookup from graph storage */
    /* For now, return empty array - would be replaced with actual graph traversal */
    return sqlite3_mprintf("[]");
}

/*
** Transaction management implementation.
*/

/*
** Execute all pending write operations and commit them.
** Returns SQLITE_OK on success, error code on failure.
*/
int cypherExecuteOperations(CypherWriteContext *pCtx) {
    CypherWriteOp *pOp;
    int rc = SQLITE_OK;
    
    if (!pCtx) return SQLITE_MISUSE;
    
    /* Execute each operation in sequence */
    for (pOp = pCtx->pOperations; pOp; pOp = pOp->pNext) {
        switch (pOp->type) {
            case CYPHER_WRITE_CREATE_NODE:
                /* TODO: Execute actual node creation in graph storage */
                /* For now, just log success */
                break;
                
            case CYPHER_WRITE_CREATE_RELATIONSHIP:
                /* TODO: Execute actual relationship creation in graph storage */
                /* For now, just log success */
                break;
                
            case CYPHER_WRITE_MERGE_NODE:
                /* TODO: Execute actual node merge in graph storage */
                /* For now, just log success */
                break;
                
            case CYPHER_WRITE_SET_PROPERTY:
                /* TODO: Execute actual property update in graph storage */
                /* For now, just log success */
                break;
                
            case CYPHER_WRITE_SET_LABEL:
                /* TODO: Execute actual label update in graph storage */
                /* For now, just log success */
                break;
                
            case CYPHER_WRITE_DELETE_NODE:
            case CYPHER_WRITE_DETACH_DELETE_NODE:
                /* TODO: Execute actual node deletion in graph storage */
                /* For now, just log success */
                break;
                
            case CYPHER_WRITE_DELETE_RELATIONSHIP:
                /* TODO: Execute actual relationship deletion in graph storage */
                /* For now, just log success */
                break;
                
            default:
                /* Unknown operation type */
                return SQLITE_ERROR;
        }
    }
    
    return rc;
}

/*
** Rollback all pending write operations.
** Returns SQLITE_OK on success, error code on failure.
*/
int cypherRollbackOperations(CypherWriteContext *pCtx) {
    CypherWriteOp *pOp;
    int rc = SQLITE_OK;
    
    if (!pCtx) return SQLITE_MISUSE;
    
    /* Rollback operations in reverse order */
    /* TODO: Implement actual rollback logic using stored old values */
    /* For now, just clear the operation log */
    
    for (pOp = pCtx->pOperations; pOp; pOp = pOp->pNext) {
        switch (pOp->type) {
            case CYPHER_WRITE_SET_PROPERTY:
                /* Restore old property value if available */
                if (pOp->pOldValue) {
                    /* TODO: Restore property to old value in graph storage */
                }
                break;
                
            case CYPHER_WRITE_SET_LABEL:
                /* Restore old labels if available */
                if (pOp->zOldLabels) {
                    /* TODO: Restore labels to old state in graph storage */
                }
                break;
                
            case CYPHER_WRITE_CREATE_NODE:
            case CYPHER_WRITE_MERGE_NODE:
                /* Remove created nodes */
                /* TODO: Remove node from graph storage */
                break;
                
            case CYPHER_WRITE_CREATE_RELATIONSHIP:
                /* Remove created relationships */
                /* TODO: Remove relationship from graph storage */
                break;
                
            case CYPHER_WRITE_DELETE_NODE:
            case CYPHER_WRITE_DETACH_DELETE_NODE:
            case CYPHER_WRITE_DELETE_RELATIONSHIP:
                /* Restore deleted items */
                /* TODO: Restore deleted items from backup data */
                break;
        }
    }
    
    return rc;
}

/*
** Write operation memory management.
*/

/*
** Create a write operation record.
** Returns NULL on allocation failure.
*/
CypherWriteOp *cypherWriteOpCreate(CypherWriteOpType type) {
    CypherWriteOp *pOp = (CypherWriteOp*)sqlite3_malloc(sizeof(CypherWriteOp));
    if (!pOp) return NULL;
    
    memset(pOp, 0, sizeof(CypherWriteOp));
    pOp->type = type;
    
    return pOp;
}

/*
** Destroy a write operation and free all associated memory.
** Safe to call with NULL pointer.
*/
void cypherWriteOpDestroy(CypherWriteOp *pOp) {
    if (!pOp) return;
    
    sqlite3_free(pOp->zProperty);
    sqlite3_free(pOp->zLabel);
    sqlite3_free(pOp->zRelType);
    sqlite3_free(pOp->zOldLabels);
    sqlite3_free(pOp->zNewLabels);
    
    if (pOp->pOldValue) {
        cypherValueDestroy(pOp->pOldValue);
        sqlite3_free(pOp->pOldValue);
    }
    if (pOp->pNewValue) {
        cypherValueDestroy(pOp->pNewValue);
        sqlite3_free(pOp->pNewValue);
    }
    
    sqlite3_free(pOp);
}

/*
** Create operation structures.
*/

/*
** Create a CREATE node operation structure.
** Returns NULL on allocation failure.
*/
CreateNodeOp *cypherCreateNodeOpCreate(void) {
    CreateNodeOp *pOp = (CreateNodeOp*)sqlite3_malloc(sizeof(CreateNodeOp));
    if (!pOp) return NULL;
    
    memset(pOp, 0, sizeof(CreateNodeOp));
    return pOp;
}

/*
** Destroy a CREATE node operation.
** Safe to call with NULL pointer.
*/
void cypherCreateNodeOpDestroy(CreateNodeOp *pOp) {
    int i;
    
    if (!pOp) return;
    
    sqlite3_free(pOp->zVariable);
    
    for (i = 0; i < pOp->nLabels; i++) {
        sqlite3_free(pOp->azLabels[i]);
    }
    sqlite3_free(pOp->azLabels);
    
    for (i = 0; i < pOp->nProperties; i++) {
        sqlite3_free(pOp->azPropNames[i]);
        if (pOp->apPropValues[i]) {
            cypherValueDestroy(pOp->apPropValues[i]);
            sqlite3_free(pOp->apPropValues[i]);
        }
    }
    sqlite3_free(pOp->azPropNames);
    sqlite3_free(pOp->apPropValues);
    
    sqlite3_free(pOp);
}

/*
** Create a CREATE relationship operation structure.
** Returns NULL on allocation failure.
*/
CreateRelOp *cypherCreateRelOpCreate(void) {
    CreateRelOp *pOp = (CreateRelOp*)sqlite3_malloc(sizeof(CreateRelOp));
    if (!pOp) return NULL;
    
    memset(pOp, 0, sizeof(CreateRelOp));
    return pOp;
}

/*
** Destroy a CREATE relationship operation.
** Safe to call with NULL pointer.
*/
void cypherCreateRelOpDestroy(CreateRelOp *pOp) {
    int i;
    
    if (!pOp) return;
    
    sqlite3_free(pOp->zFromVar);
    sqlite3_free(pOp->zToVar);
    sqlite3_free(pOp->zRelVar);
    sqlite3_free(pOp->zRelType);
    
    for (i = 0; i < pOp->nProperties; i++) {
        sqlite3_free(pOp->azPropNames[i]);
        if (pOp->apPropValues[i]) {
            cypherValueDestroy(pOp->apPropValues[i]);
            sqlite3_free(pOp->apPropValues[i]);
        }
    }
    sqlite3_free(pOp->azPropNames);
    sqlite3_free(pOp->apPropValues);
    
    sqlite3_free(pOp);
}

/*
** MERGE operation functions.
*/

/*
** Execute a MERGE node operation.
** Returns SQLITE_OK on success, error code on failure.
*/
int cypherMergeNode(CypherWriteContext *pCtx, MergeNodeOp *pOp) {
    CypherWriteOp *pWriteOp;
    sqlite3_int64 iFoundNodeId;
    char *zLabelsJson = NULL;
    char *zPropsJson = NULL;
    int rc = SQLITE_OK;
    int i;
    
    if (!pCtx || !pOp) return SQLITE_MISUSE;
    
    /* First, try to find an existing node that matches the criteria */
    iFoundNodeId = cypherFindMatchingNode(pCtx, 
                                         pOp->azLabels, pOp->nLabels,
                                         pOp->azMatchProps, pOp->apMatchValues, pOp->nMatchProps);
    
    if (iFoundNodeId > 0) {
        /* Node found - execute ON MATCH clause */
        pOp->iNodeId = iFoundNodeId;
        pOp->bWasCreated = 0;
        
        /* Apply ON MATCH property updates */
        for (i = 0; i < pOp->nOnMatchProps; i++) {
            SetPropertyOp setOp;
            memset(&setOp, 0, sizeof(SetPropertyOp));
            setOp.zVariable = pOp->zVariable;
            setOp.zProperty = pOp->azOnMatchProps[i];
            setOp.pValue = pOp->apOnMatchValues[i];
            setOp.iNodeId = iFoundNodeId;
            
            rc = cypherSetProperty(pCtx, &setOp);
            if (rc != SQLITE_OK) {
                return rc;
            }
        }
        
        /* Create operation record for MERGE match */
        pWriteOp = cypherWriteOpCreate(CYPHER_WRITE_MERGE_NODE);
        if (!pWriteOp) return SQLITE_NOMEM;
        
        pWriteOp->iNodeId = iFoundNodeId;
        pWriteOp->zProperty = sqlite3_mprintf("MATCH");
        
        rc = cypherWriteContextAddOperation(pCtx, pWriteOp);
        if (rc != SQLITE_OK) {
            cypherWriteOpDestroy(pWriteOp);
            return rc;
        }
        
    } else {
        /* Node not found - create new node with ON CREATE properties */
        pOp->iNodeId = cypherWriteContextNextNodeId(pCtx);
        pOp->bWasCreated = 1;
        
        /* Build labels JSON array */
        if (pOp->nLabels > 0) {
            char *zLabels = sqlite3_malloc(1024);
            if (!zLabels) return SQLITE_NOMEM;
            
            strcpy(zLabels, "[");
            for (i = 0; i < pOp->nLabels; i++) {
                if (i > 0) strcat(zLabels, ",");
                strcat(zLabels, "\"");
                strcat(zLabels, pOp->azLabels[i]);
                strcat(zLabels, "\"");
            }
            strcat(zLabels, "]");
            zLabelsJson = zLabels;
        } else {
            zLabelsJson = sqlite3_mprintf("[]");
        }
        
        /* Build combined properties JSON (match props + on create props) */
        char *zProps = sqlite3_malloc(4096);
        if (!zProps) {
            sqlite3_free(zLabelsJson);
            return SQLITE_NOMEM;
        }
        
        strcpy(zProps, "{");
        int propCount = 0;
        
        /* Add match properties */
        for (i = 0; i < pOp->nMatchProps; i++) {
            if (propCount > 0) strcat(zProps, ",");
            strcat(zProps, "\"");
            strcat(zProps, pOp->azMatchProps[i]);
            strcat(zProps, "\":");
            
            /* Simple value serialization */
            if (pOp->apMatchValues[i]->type == CYPHER_VALUE_STRING) {
                strcat(zProps, "\"");
                strcat(zProps, pOp->apMatchValues[i]->u.zString);
                strcat(zProps, "\"");
            } else if (pOp->apMatchValues[i]->type == CYPHER_VALUE_INTEGER) {
                char zNum[32];
                sprintf(zNum, "%lld", pOp->apMatchValues[i]->u.iInteger);
                strcat(zProps, zNum);
            } else {
                strcat(zProps, "null");
            }
            propCount++;
        }
        
        /* Add ON CREATE properties */
        for (i = 0; i < pOp->nOnCreateProps; i++) {
            if (propCount > 0) strcat(zProps, ",");
            strcat(zProps, "\"");
            strcat(zProps, pOp->azOnCreateProps[i]);
            strcat(zProps, "\":");
            
            /* Simple value serialization */
            if (pOp->apOnCreateValues[i]->type == CYPHER_VALUE_STRING) {
                strcat(zProps, "\"");
                strcat(zProps, pOp->apOnCreateValues[i]->u.zString);
                strcat(zProps, "\"");
            } else if (pOp->apOnCreateValues[i]->type == CYPHER_VALUE_INTEGER) {
                char zNum[32];
                sprintf(zNum, "%lld", pOp->apOnCreateValues[i]->u.iInteger);
                strcat(zProps, zNum);
            } else {
                strcat(zProps, "null");
            }
            propCount++;
        }
        
        strcat(zProps, "}");
        zPropsJson = zProps;
        
        /* Create write operation record for MERGE create */
        pWriteOp = cypherWriteOpCreate(CYPHER_WRITE_MERGE_NODE);
        if (!pWriteOp) {
            sqlite3_free(zLabelsJson);
            sqlite3_free(zPropsJson);
            return SQLITE_NOMEM;
        }
        
        pWriteOp->iNodeId = pOp->iNodeId;
        pWriteOp->zNewLabels = zLabelsJson;
        pWriteOp->zProperty = sqlite3_mprintf("CREATE");
        
        /* Add to operation log */
        rc = cypherWriteContextAddOperation(pCtx, pWriteOp);
        if (rc != SQLITE_OK) {
            cypherWriteOpDestroy(pWriteOp);
            sqlite3_free(zLabelsJson);
            sqlite3_free(zPropsJson);
            return rc;
        }
        
        /* TODO: Actually create node in graph storage here */
        
        sqlite3_free(zPropsJson);
    }
    
    /* Bind variable in execution context */
    if (pOp->zVariable) {
        CypherValue nodeValue;
        nodeValue.type = CYPHER_VALUE_NODE;
        nodeValue.u.iNodeId = pOp->iNodeId;
        /* TODO: Bind variable in execution context */
    }
    
    return SQLITE_OK;
}

/*
** Create a MERGE node iterator.
** Returns NULL on allocation failure.
*/
CypherWriteIterator *cypherMergeNodeIteratorCreate(CypherWriteContext *pCtx, MergeNodeOp *pOp) {
    CypherWriteIterator *pIterator;
    
    if (!pCtx || !pOp) return NULL;
    
    pIterator = (CypherWriteIterator*)sqlite3_malloc(sizeof(CypherWriteIterator));
    if (!pIterator) return NULL;
    
    memset(pIterator, 0, sizeof(CypherWriteIterator));
    pIterator->pWriteCtx = pCtx;
    pIterator->pOperationData = pOp;
    
    /* Set up base iterator interface */
    pIterator->base.xOpen = NULL;  /* TODO: Implement iterator functions */
    pIterator->base.xNext = NULL;
    pIterator->base.xClose = NULL;
    pIterator->base.xDestroy = NULL;
    
    return pIterator;
}

MergeNodeOp *cypherMergeNodeOpCreate(void) {
    MergeNodeOp *pOp = (MergeNodeOp*)sqlite3_malloc(sizeof(MergeNodeOp));
    if (!pOp) return NULL;
    memset(pOp, 0, sizeof(MergeNodeOp));
    return pOp;
}

void cypherMergeNodeOpDestroy(MergeNodeOp *pOp) {
    int i;
    
    if (!pOp) return;
    
    sqlite3_free(pOp->zVariable);
    
    /* Free labels */
    for (i = 0; i < pOp->nLabels; i++) {
        sqlite3_free(pOp->azLabels[i]);
    }
    sqlite3_free(pOp->azLabels);
    
    /* Free match properties */
    for (i = 0; i < pOp->nMatchProps; i++) {
        sqlite3_free(pOp->azMatchProps[i]);
        if (pOp->apMatchValues[i]) {
            cypherValueDestroy(pOp->apMatchValues[i]);
            sqlite3_free(pOp->apMatchValues[i]);
        }
    }
    sqlite3_free(pOp->azMatchProps);
    sqlite3_free(pOp->apMatchValues);
    
    /* Free ON CREATE properties */
    for (i = 0; i < pOp->nOnCreateProps; i++) {
        sqlite3_free(pOp->azOnCreateProps[i]);
        if (pOp->apOnCreateValues[i]) {
            cypherValueDestroy(pOp->apOnCreateValues[i]);
            sqlite3_free(pOp->apOnCreateValues[i]);
        }
    }
    sqlite3_free(pOp->azOnCreateProps);
    sqlite3_free(pOp->apOnCreateValues);
    
    /* Free ON MATCH properties */
    for (i = 0; i < pOp->nOnMatchProps; i++) {
        sqlite3_free(pOp->azOnMatchProps[i]);
        if (pOp->apOnMatchValues[i]) {
            cypherValueDestroy(pOp->apOnMatchValues[i]);
            sqlite3_free(pOp->apOnMatchValues[i]);
        }
    }
    sqlite3_free(pOp->azOnMatchProps);
    sqlite3_free(pOp->apOnMatchValues);
    
    sqlite3_free(pOp);
}

/*
** SET operation functions.
*/

/*
** Execute a SET property operation.
** Returns SQLITE_OK on success, error code on failure.
*/
int cypherSetProperty(CypherWriteContext *pCtx, SetPropertyOp *pOp) {
    CypherWriteOp *pWriteOp;
    int rc = SQLITE_OK;
    
    if (!pCtx || !pOp) return SQLITE_MISUSE;
    
    /* Validate that the target node exists */
    rc = cypherValidateNodeExists(pCtx, pOp->iNodeId);
    if (rc != SQLITE_OK) {
        return rc;
    }
    
    /* Create write operation record */
    pWriteOp = cypherWriteOpCreate(CYPHER_WRITE_SET_PROPERTY);
    if (!pWriteOp) return SQLITE_NOMEM;
    
    pWriteOp->iNodeId = pOp->iNodeId;
    pWriteOp->zProperty = sqlite3_mprintf("%s", pOp->zProperty);
    
    /* TODO: Get old value for rollback support */
    pWriteOp->pOldValue = NULL;
    
    /* Set new value */
    pWriteOp->pNewValue = (CypherValue*)sqlite3_malloc(sizeof(CypherValue));
    if (!pWriteOp->pNewValue) {
        cypherWriteOpDestroy(pWriteOp);
        return SQLITE_NOMEM;
    }
    
    memcpy(pWriteOp->pNewValue, pOp->pValue, sizeof(CypherValue));
    
    /* If it's a string value, make a copy */
    if (pOp->pValue->type == CYPHER_VALUE_STRING && pOp->pValue->u.zString) {
        pWriteOp->pNewValue->u.zString = sqlite3_mprintf("%s", pOp->pValue->u.zString);
        if (!pWriteOp->pNewValue->u.zString) {
            cypherWriteOpDestroy(pWriteOp);
            return SQLITE_NOMEM;
        }
    }
    
    /* Add to operation log */
    rc = cypherWriteContextAddOperation(pCtx, pWriteOp);
    if (rc != SQLITE_OK) {
        cypherWriteOpDestroy(pWriteOp);
        return rc;
    }
    
    /* TODO: Actually update property in graph storage here */
    
    return SQLITE_OK;
}

/*
** Execute a SET label operation.
** Returns SQLITE_OK on success, error code on failure.
*/
int cypherSetLabel(CypherWriteContext *pCtx, SetLabelOp *pOp) {
    CypherWriteOp *pWriteOp;
    char *zLabelsJson = NULL;
    int rc = SQLITE_OK;
    int i;
    
    if (!pCtx || !pOp) return SQLITE_MISUSE;
    
    /* Validate that the target node exists */
    rc = cypherValidateNodeExists(pCtx, pOp->iNodeId);
    if (rc != SQLITE_OK) {
        return rc;
    }
    
    /* Build labels JSON array */
    if (pOp->nLabels > 0) {
        char *zLabels = sqlite3_malloc(1024);
        if (!zLabels) return SQLITE_NOMEM;
        
        strcpy(zLabels, "[");
        for (i = 0; i < pOp->nLabels; i++) {
            if (i > 0) strcat(zLabels, ",");
            strcat(zLabels, "\"");
            strcat(zLabels, pOp->azLabels[i]);
            strcat(zLabels, "\"");
        }
        strcat(zLabels, "]");
        zLabelsJson = zLabels;
    } else {
        zLabelsJson = sqlite3_mprintf("[]");
    }
    
    /* Create write operation record */
    pWriteOp = cypherWriteOpCreate(CYPHER_WRITE_SET_LABEL);
    if (!pWriteOp) {
        sqlite3_free(zLabelsJson);
        return SQLITE_NOMEM;
    }
    
    pWriteOp->iNodeId = pOp->iNodeId;
    pWriteOp->zNewLabels = zLabelsJson;
    
    /* TODO: Get old labels for rollback support */
    pWriteOp->zOldLabels = NULL;
    
    /* Add to operation log */
    rc = cypherWriteContextAddOperation(pCtx, pWriteOp);
    if (rc != SQLITE_OK) {
        cypherWriteOpDestroy(pWriteOp);
        return rc;
    }
    
    /* TODO: Actually update labels in graph storage here */
    
    return SQLITE_OK;
}

/*
** Create a SET property iterator.
** Returns NULL on allocation failure.
*/
CypherWriteIterator *cypherSetPropertyIteratorCreate(CypherWriteContext *pCtx, SetPropertyOp *pOp) {
    CypherWriteIterator *pIterator;
    
    if (!pCtx || !pOp) return NULL;
    
    pIterator = (CypherWriteIterator*)sqlite3_malloc(sizeof(CypherWriteIterator));
    if (!pIterator) return NULL;
    
    memset(pIterator, 0, sizeof(CypherWriteIterator));
    pIterator->pWriteCtx = pCtx;
    pIterator->pOperationData = pOp;
    
    return pIterator;
}

/*
** Create a SET label iterator.
** Returns NULL on allocation failure.
*/
CypherWriteIterator *cypherSetLabelIteratorCreate(CypherWriteContext *pCtx, SetLabelOp *pOp) {
    CypherWriteIterator *pIterator;
    
    if (!pCtx || !pOp) return NULL;
    
    pIterator = (CypherWriteIterator*)sqlite3_malloc(sizeof(CypherWriteIterator));
    if (!pIterator) return NULL;
    
    memset(pIterator, 0, sizeof(CypherWriteIterator));
    pIterator->pWriteCtx = pCtx;
    pIterator->pOperationData = pOp;
    
    return pIterator;
}

SetPropertyOp *cypherSetPropertyOpCreate(void) {
    SetPropertyOp *pOp = (SetPropertyOp*)sqlite3_malloc(sizeof(SetPropertyOp));
    if (!pOp) return NULL;
    memset(pOp, 0, sizeof(SetPropertyOp));
    return pOp;
}

void cypherSetPropertyOpDestroy(SetPropertyOp *pOp) {
    if (!pOp) return;
    
    sqlite3_free(pOp->zVariable);
    sqlite3_free(pOp->zProperty);
    
    if (pOp->pValue) {
        cypherValueDestroy(pOp->pValue);
        sqlite3_free(pOp->pValue);
    }
    
    sqlite3_free(pOp);
}

SetLabelOp *cypherSetLabelOpCreate(void) {
    SetLabelOp *pOp = (SetLabelOp*)sqlite3_malloc(sizeof(SetLabelOp));
    if (!pOp) return NULL;
    memset(pOp, 0, sizeof(SetLabelOp));
    return pOp;
}

void cypherSetLabelOpDestroy(SetLabelOp *pOp) {
    int i;
    
    if (!pOp) return;
    
    sqlite3_free(pOp->zVariable);
    
    for (i = 0; i < pOp->nLabels; i++) {
        sqlite3_free(pOp->azLabels[i]);
    }
    sqlite3_free(pOp->azLabels);
    
    sqlite3_free(pOp);
}

/*
** DELETE operation functions.
*/

/*
** Execute a DELETE operation.
** Returns SQLITE_OK on success, error code on failure.
*/
int cypherDelete(CypherWriteContext *pCtx, DeleteOp *pOp) {
    CypherWriteOp *pWriteOp;
    char *zRelationships = NULL;
    int rc = SQLITE_OK;
    
    if (!pCtx || !pOp) return SQLITE_MISUSE;
    
    if (pOp->bIsNode) {
        /* Deleting a node */
        
        /* Validate that the node exists */
        rc = cypherValidateNodeExists(pCtx, pOp->iNodeId);
        if (rc != SQLITE_OK) {
            return rc;
        }
        
        if (pOp->bDetach) {
            /* DETACH DELETE - first delete all connected relationships */
            zRelationships = cypherGetNodeRelationships(pCtx, pOp->iNodeId);
            if (!zRelationships) {
                return SQLITE_NOMEM;
            }
            
            /* TODO: Parse relationship IDs and delete each one */
            /* For now, just log that we would delete relationships */
            
            /* Create write operation record for DETACH DELETE */
            pWriteOp = cypherWriteOpCreate(CYPHER_WRITE_DETACH_DELETE_NODE);
        } else {
            /* Regular DELETE - check if node has relationships */
            zRelationships = cypherGetNodeRelationships(pCtx, pOp->iNodeId);
            if (zRelationships && strlen(zRelationships) > 2) {  /* More than just "[]" */
                sqlite3_free(zRelationships);
                /* Cannot delete node with relationships without DETACH */
                return SQLITE_ERROR;
            }
            
            /* Create write operation record for DELETE */
            pWriteOp = cypherWriteOpCreate(CYPHER_WRITE_DELETE_NODE);
        }
        
        if (!pWriteOp) {
            sqlite3_free(zRelationships);
            return SQLITE_NOMEM;
        }
        
        pWriteOp->iNodeId = pOp->iNodeId;
        
        /* TODO: Store old node data for rollback */
        
    } else {
        /* Deleting a relationship */
        
        /* Create write operation record for relationship DELETE */
        pWriteOp = cypherWriteOpCreate(CYPHER_WRITE_DELETE_RELATIONSHIP);
        if (!pWriteOp) return SQLITE_NOMEM;
        
        pWriteOp->iRelId = pOp->iRelId;
        
        /* TODO: Store old relationship data for rollback */
    }
    
    /* Add to operation log */
    rc = cypherWriteContextAddOperation(pCtx, pWriteOp);
    if (rc != SQLITE_OK) {
        cypherWriteOpDestroy(pWriteOp);
        sqlite3_free(zRelationships);
        return rc;
    }
    
    /* TODO: Actually delete from graph storage here */
    
    sqlite3_free(zRelationships);
    return SQLITE_OK;
}

/*
** Create a DELETE iterator.
** Returns NULL on allocation failure.
*/
CypherWriteIterator *cypherDeleteIteratorCreate(CypherWriteContext *pCtx, DeleteOp *pOp) {
    CypherWriteIterator *pIterator;
    
    if (!pCtx || !pOp) return NULL;
    
    pIterator = (CypherWriteIterator*)sqlite3_malloc(sizeof(CypherWriteIterator));
    if (!pIterator) return NULL;
    
    memset(pIterator, 0, sizeof(CypherWriteIterator));
    pIterator->pWriteCtx = pCtx;
    pIterator->pOperationData = pOp;
    
    return pIterator;
}

DeleteOp *cypherDeleteOpCreate(void) {
    DeleteOp *pOp = (DeleteOp*)sqlite3_malloc(sizeof(DeleteOp));
    if (!pOp) return NULL;
    memset(pOp, 0, sizeof(DeleteOp));
    return pOp;
}

void cypherDeleteOpDestroy(DeleteOp *pOp) {
    if (!pOp) return;
    
    sqlite3_free(pOp->zVariable);
    sqlite3_free(pOp);
}