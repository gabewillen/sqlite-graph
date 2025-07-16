/*
** SQLite Graph Database Extension - CREATE Node Integration Test
**
** This test verifies the ACTUAL cypherCreateNode implementation
** from cypher-write.c, not a stub or mock.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* SQLite function implementations for testing */
void *sqlite3_malloc(int n) { return malloc(n); }
void sqlite3_free(void *p) { free(p); }
int sqlite3_realloc_fn(void *p, int n) { return (int)(size_t)realloc(p, n); }
void *sqlite3_realloc(void *p, int n) { return realloc(p, n); }

char *sqlite3_mprintf(const char *zFormat, ...) {
    va_list ap;
    char *result = malloc(1024);
    if (result) {
        va_start(ap, zFormat);
        vsnprintf(result, 1024, zFormat, ap);
        va_end(ap);
    }
    return result;
}

char *sqlite3_vmprintf(const char *zFormat, va_list ap) {
    char *result = malloc(1024);
    if (result) {
        vsnprintf(result, 1024, zFormat, ap);
    }
    return result;
}

/* Constants */
#define SQLITE_OK 0
#define SQLITE_ERROR 1
#define SQLITE_NOMEM 7
#define SQLITE_MISUSE 21
#define SQLITE_FORMAT 24
#define SQLITE_RANGE 25
#define SQLITE_NOTFOUND 12
#define SQLITE_FULL 13
#define SQLITE_CANTOPEN 14
#define SQLITE_EMPTY 16
#define SQLITE_TOOBIG 18
#define SQLITE_MISMATCH 20

typedef struct sqlite3 sqlite3;
typedef struct sqlite3_stmt sqlite3_stmt;
typedef long long sqlite3_int64;

/* Include the actual implementation files */
#define SQLITE_CORE 1

/* Mock structures needed by the implementation */
typedef struct GraphVtab {
    sqlite3 *pDb;
} GraphVtab;

/* Include cypher value types */
typedef enum {
    CYPHER_VALUE_NULL = 0,
    CYPHER_VALUE_BOOLEAN,
    CYPHER_VALUE_INTEGER,
    CYPHER_VALUE_FLOAT,
    CYPHER_VALUE_STRING,
    CYPHER_VALUE_NODE,
    CYPHER_VALUE_RELATIONSHIP,
    CYPHER_VALUE_PATH,
    CYPHER_VALUE_LIST,
    CYPHER_VALUE_MAP
} CypherValueType;

typedef struct CypherValue {
    CypherValueType type;
    union {
        int bBoolean;
        sqlite3_int64 iInteger;
        double rFloat;
        char *zString;
        sqlite3_int64 iNodeId;
        sqlite3_int64 iRelId;
        struct {
            struct CypherValue *apValues;
            int nValues;
        } list;
        struct {
            char **azKeys;
            struct CypherValue *apValues;
            int nPairs;
        } map;
    } u;
} CypherValue;

/* Execution context */
typedef struct ExecutionContext {
    sqlite3 *pDb;
    GraphVtab *pGraph;
    char **azVariables;
    CypherValue *aBindings;
    int nVariables;
    int nVariablesAlloc;
    void **apAllocated;
    int nAllocated;
    int nAllocatedAlloc;
    char *zErrorMsg;
} ExecutionContext;

/* Write operation types */
typedef enum {
    CYPHER_WRITE_CREATE_NODE = 1,
    CYPHER_WRITE_CREATE_RELATIONSHIP,
    CYPHER_WRITE_MERGE_NODE,
    CYPHER_WRITE_MERGE_RELATIONSHIP,
    CYPHER_WRITE_SET_PROPERTY,
    CYPHER_WRITE_SET_LABEL,
    CYPHER_WRITE_REMOVE_PROPERTY,
    CYPHER_WRITE_REMOVE_LABEL,
    CYPHER_WRITE_DELETE_NODE,
    CYPHER_WRITE_DELETE_RELATIONSHIP,
    CYPHER_WRITE_DETACH_DELETE_NODE
} CypherWriteOpType;

/* Write operation structure */
typedef struct CypherWriteOp CypherWriteOp;
struct CypherWriteOp {
    CypherWriteOpType type;
    sqlite3_int64 iNodeId;
    sqlite3_int64 iFromId;
    sqlite3_int64 iToId;
    sqlite3_int64 iRelId;
    char *zProperty;
    char *zLabel;
    char *zRelType;
    CypherValue *pOldValue;
    CypherValue *pNewValue;
    char *zOldLabels;
    char *zNewLabels;
    CypherWriteOp *pNext;
};

/* Write context */
typedef struct CypherWriteContext {
    sqlite3 *pDb;
    GraphVtab *pGraph;
    ExecutionContext *pExecContext;
    CypherWriteOp *pOperations;
    CypherWriteOp *pLastOp;
    int nOperations;
    int bInTransaction;
    int bAutoCommit;
    char *zErrorMsg;
    sqlite3_int64 iNextNodeId;
    sqlite3_int64 iNextRelId;
} CypherWriteContext;

/* Create node operation */
typedef struct CreateNodeOp {
    char *zVariable;
    char **azLabels;
    int nLabels;
    char **azPropNames;
    CypherValue **apPropValues;
    int nProperties;
    sqlite3_int64 iCreatedNodeId;
} CreateNodeOp;

/* Mock implementations of required functions */
ExecutionContext *executionContextCreate(sqlite3 *pDb, GraphVtab *pGraph) {
    ExecutionContext *pCtx = (ExecutionContext*)sqlite3_malloc(sizeof(ExecutionContext));
    if (pCtx) {
        memset(pCtx, 0, sizeof(ExecutionContext));
        pCtx->pDb = pDb;
        pCtx->pGraph = pGraph;
    }
    return pCtx;
}

void executionContextDestroy(ExecutionContext *pCtx) {
    if (pCtx) {
        for (int i = 0; i < pCtx->nVariables; i++) {
            sqlite3_free(pCtx->azVariables[i]);
        }
        sqlite3_free(pCtx->azVariables);
        sqlite3_free(pCtx->aBindings);
        sqlite3_free(pCtx);
    }
}

int executionContextBind(ExecutionContext *pCtx, const char *zVar, CypherValue *pValue) {
    if (!pCtx || !zVar || !pValue) return SQLITE_MISUSE;
    
    /* Simple implementation for testing */
    if (pCtx->nVariables >= pCtx->nVariablesAlloc) {
        int nNew = pCtx->nVariablesAlloc ? pCtx->nVariablesAlloc * 2 : 8;
        char **azNew = sqlite3_realloc(pCtx->azVariables, nNew * sizeof(char*));
        if (!azNew) return SQLITE_NOMEM;
        pCtx->azVariables = azNew;
        
        CypherValue *aNew = sqlite3_realloc(pCtx->aBindings, nNew * sizeof(CypherValue));
        if (!aNew) return SQLITE_NOMEM;
        pCtx->aBindings = aNew;
        pCtx->nVariablesAlloc = nNew;
    }
    
    pCtx->azVariables[pCtx->nVariables] = sqlite3_mprintf("%s", zVar);
    pCtx->aBindings[pCtx->nVariables] = *pValue;
    pCtx->nVariables++;
    return SQLITE_OK;
}

CypherValue *executionContextGet(ExecutionContext *pCtx, const char *zVar) {
    if (!pCtx || !zVar) return NULL;
    for (int i = 0; i < pCtx->nVariables; i++) {
        if (strcmp(pCtx->azVariables[i], zVar) == 0) {
            return &pCtx->aBindings[i];
        }
    }
    return NULL;
}

/* Cypher value functions */
void cypherValueInit(CypherValue *pValue) {
    if (pValue) {
        memset(pValue, 0, sizeof(CypherValue));
        pValue->type = CYPHER_VALUE_NULL;
    }
}

void cypherValueDestroy(CypherValue *pValue) {
    if (!pValue) return;
    if (pValue->type == CYPHER_VALUE_STRING && pValue->u.zString) {
        sqlite3_free(pValue->u.zString);
    }
}

void cypherValueSetNode(CypherValue *pValue, sqlite3_int64 iNodeId) {
    if (pValue) {
        cypherValueDestroy(pValue);
        pValue->type = CYPHER_VALUE_NODE;
        pValue->u.iNodeId = iNodeId;
    }
}

/* Storage mock - tracks what would be stored */
static struct {
    sqlite3_int64 iNodeId;
    char *zLabels;
    char *zProperties;
    int bStored;
} g_lastStoredNode = {0, NULL, NULL, 0};

int cypherStorageAddNode(GraphVtab *pGraph, sqlite3_int64 iNodeId, 
                        const char **azLabels, int nLabels, 
                        const char *zProperties) {
    /* Mock implementation that records what would be stored */
    g_lastStoredNode.iNodeId = iNodeId;
    if (g_lastStoredNode.zLabels) sqlite3_free(g_lastStoredNode.zLabels);
    if (g_lastStoredNode.zProperties) sqlite3_free(g_lastStoredNode.zProperties);
    
    /* Build labels JSON for verification */
    if (nLabels > 0) {
        char *zLabels = sqlite3_malloc(256);
        strcpy(zLabels, "[");
        for (int i = 0; i < nLabels; i++) {
            if (i > 0) strcat(zLabels, ",");
            strcat(zLabels, "\"");
            strcat(zLabels, azLabels[i]);
            strcat(zLabels, "\"");
        }
        strcat(zLabels, "]");
        g_lastStoredNode.zLabels = zLabels;
    } else {
        g_lastStoredNode.zLabels = sqlite3_mprintf("[]");
    }
    
    g_lastStoredNode.zProperties = sqlite3_mprintf("%s", zProperties ? zProperties : "{}");
    g_lastStoredNode.bStored = 1;
    return SQLITE_OK;
}

/* Write operation management */
CypherWriteOp *cypherWriteOpCreate(CypherWriteOpType type) {
    CypherWriteOp *pOp = (CypherWriteOp*)sqlite3_malloc(sizeof(CypherWriteOp));
    if (pOp) {
        memset(pOp, 0, sizeof(CypherWriteOp));
        pOp->type = type;
    }
    return pOp;
}

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

/* Write context functions */
CypherWriteContext *cypherWriteContextCreate(sqlite3 *pDb, GraphVtab *pGraph,
                                           ExecutionContext *pExecContext) {
    CypherWriteContext *pCtx = (CypherWriteContext*)sqlite3_malloc(sizeof(CypherWriteContext));
    if (pCtx) {
        memset(pCtx, 0, sizeof(CypherWriteContext));
        pCtx->pDb = pDb;
        pCtx->pGraph = pGraph;
        pCtx->pExecContext = pExecContext;
        pCtx->bAutoCommit = 1;
        pCtx->iNextNodeId = 1;
        pCtx->iNextRelId = 1;
    }
    return pCtx;
}

void cypherWriteContextDestroy(CypherWriteContext *pCtx) {
    if (!pCtx) return;
    CypherWriteOp *pOp = pCtx->pOperations;
    while (pOp) {
        CypherWriteOp *pNext = pOp->pNext;
        cypherWriteOpDestroy(pOp);
        pOp = pNext;
    }
    sqlite3_free(pCtx->zErrorMsg);
    sqlite3_free(pCtx);
}

sqlite3_int64 cypherWriteContextNextNodeId(CypherWriteContext *pCtx) {
    if (!pCtx) return -1;
    return pCtx->iNextNodeId++;
}

int cypherWriteContextAddOperation(CypherWriteContext *pCtx, CypherWriteOp *pOp) {
    if (!pCtx || !pOp) return SQLITE_MISUSE;
    if (pCtx->pLastOp) {
        pCtx->pLastOp->pNext = pOp;
    } else {
        pCtx->pOperations = pOp;
    }
    pCtx->pLastOp = pOp;
    pCtx->nOperations++;
    return SQLITE_OK;
}

/* Create node operation functions */
CreateNodeOp *cypherCreateNodeOpCreate(void) {
    CreateNodeOp *pOp = (CreateNodeOp*)sqlite3_malloc(sizeof(CreateNodeOp));
    if (pOp) {
        memset(pOp, 0, sizeof(CreateNodeOp));
    }
    return pOp;
}

void cypherCreateNodeOpDestroy(CreateNodeOp *pOp) {
    if (!pOp) return;
    sqlite3_free(pOp->zVariable);
    for (int i = 0; i < pOp->nLabels; i++) {
        sqlite3_free(pOp->azLabels[i]);
    }
    sqlite3_free(pOp->azLabels);
    for (int i = 0; i < pOp->nProperties; i++) {
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

CypherValue *cypherValueCreate(CypherValueType type) {
    CypherValue *pValue = (CypherValue*)sqlite3_malloc(sizeof(CypherValue));
    if (pValue) {
        memset(pValue, 0, sizeof(CypherValue));
        pValue->type = type;
    }
    return pValue;
}

int cypherValueSetString(CypherValue *pValue, const char *zString) {
    if (!pValue) return SQLITE_MISUSE;
    if (pValue->type == CYPHER_VALUE_STRING && pValue->u.zString) {
        sqlite3_free(pValue->u.zString);
    }
    pValue->type = CYPHER_VALUE_STRING;
    pValue->u.zString = zString ? sqlite3_mprintf("%s", zString) : NULL;
    return SQLITE_OK;
}

void cypherValueSetInteger(CypherValue *pValue, sqlite3_int64 iValue) {
    if (pValue) {
        cypherValueDestroy(pValue);
        pValue->type = CYPHER_VALUE_INTEGER;
        pValue->u.iInteger = iValue;
    }
}

void cypherValueSetFloat(CypherValue *pValue, double rValue) {
    if (pValue) {
        cypherValueDestroy(pValue);
        pValue->type = CYPHER_VALUE_FLOAT;
        pValue->u.rFloat = rValue;
    }
}

/* Include the ACTUAL implementation */
#include "../src/cypher/cypher-write.c"

/* Test functions */
static int g_tests_run = 0;
static int g_tests_passed = 0;

#define TEST_ASSERT(cond, msg) do { \
    g_tests_run++; \
    if (cond) { \
        g_tests_passed++; \
        printf("  PASS: %s\n", msg); \
    } else { \
        printf("  FAIL: %s (line %d)\n", msg, __LINE__); \
    } \
} while(0)

void test_create_node_basic(void) {
    printf("\nTest: Basic CREATE node\n");
    
    GraphVtab graph = {0};
    ExecutionContext *execCtx = executionContextCreate(NULL, &graph);
    CypherWriteContext *writeCtx = cypherWriteContextCreate(NULL, &graph, execCtx);
    
    CreateNodeOp *op = cypherCreateNodeOpCreate();
    op->zVariable = sqlite3_mprintf("n");
    
    /* Clear storage mock */
    g_lastStoredNode.bStored = 0;
    
    int rc = cypherCreateNode(writeCtx, op);
    
    TEST_ASSERT(rc == SQLITE_OK, "cypherCreateNode returns SQLITE_OK");
    TEST_ASSERT(op->iCreatedNodeId == 1, "Node ID assigned correctly");
    TEST_ASSERT(g_lastStoredNode.bStored == 1, "Storage function called");
    TEST_ASSERT(g_lastStoredNode.iNodeId == 1, "Correct node ID stored");
    TEST_ASSERT(strcmp(g_lastStoredNode.zLabels, "[]") == 0, "Empty labels stored");
    TEST_ASSERT(strcmp(g_lastStoredNode.zProperties, "{}") == 0, "Empty properties stored");
    
    /* Check variable binding */
    CypherValue *boundValue = executionContextGet(execCtx, "n");
    TEST_ASSERT(boundValue != NULL, "Variable bound in execution context");
    TEST_ASSERT(boundValue->type == CYPHER_VALUE_NODE, "Bound value is NODE type");
    TEST_ASSERT(boundValue->u.iNodeId == 1, "Bound node ID is correct");
    
    /* Check operation recorded */
    TEST_ASSERT(writeCtx->nOperations == 1, "Operation recorded");
    TEST_ASSERT(writeCtx->pOperations != NULL, "Operation list not empty");
    TEST_ASSERT(writeCtx->pOperations->type == CYPHER_WRITE_CREATE_NODE, "Correct operation type");
    
    cypherCreateNodeOpDestroy(op);
    cypherWriteContextDestroy(writeCtx);
    executionContextDestroy(execCtx);
}

void test_create_node_with_labels(void) {
    printf("\nTest: CREATE node with labels\n");
    
    GraphVtab graph = {0};
    ExecutionContext *execCtx = executionContextCreate(NULL, &graph);
    CypherWriteContext *writeCtx = cypherWriteContextCreate(NULL, &graph, execCtx);
    
    CreateNodeOp *op = cypherCreateNodeOpCreate();
    op->zVariable = sqlite3_mprintf("person");
    op->nLabels = 2;
    op->azLabels = sqlite3_malloc(2 * sizeof(char*));
    op->azLabels[0] = sqlite3_mprintf("Person");
    op->azLabels[1] = sqlite3_mprintf("Actor");
    
    g_lastStoredNode.bStored = 0;
    
    int rc = cypherCreateNode(writeCtx, op);
    
    TEST_ASSERT(rc == SQLITE_OK, "cypherCreateNode returns SQLITE_OK");
    TEST_ASSERT(g_lastStoredNode.bStored == 1, "Storage function called");
    TEST_ASSERT(strcmp(g_lastStoredNode.zLabels, "[\"Person\",\"Actor\"]") == 0, "Labels stored correctly");
    
    cypherCreateNodeOpDestroy(op);
    cypherWriteContextDestroy(writeCtx);
    executionContextDestroy(execCtx);
}

void test_create_node_with_properties(void) {
    printf("\nTest: CREATE node with properties\n");
    
    GraphVtab graph = {0};
    ExecutionContext *execCtx = executionContextCreate(NULL, &graph);
    CypherWriteContext *writeCtx = cypherWriteContextCreate(NULL, &graph, execCtx);
    
    CreateNodeOp *op = cypherCreateNodeOpCreate();
    op->zVariable = sqlite3_mprintf("user");
    op->nLabels = 1;
    op->azLabels = sqlite3_malloc(sizeof(char*));
    op->azLabels[0] = sqlite3_mprintf("User");
    
    op->nProperties = 3;
    op->azPropNames = sqlite3_malloc(3 * sizeof(char*));
    op->apPropValues = sqlite3_malloc(3 * sizeof(CypherValue*));
    
    op->azPropNames[0] = sqlite3_mprintf("name");
    op->apPropValues[0] = cypherValueCreate(CYPHER_VALUE_STRING);
    cypherValueSetString(op->apPropValues[0], "Alice");
    
    op->azPropNames[1] = sqlite3_mprintf("age");
    op->apPropValues[1] = cypherValueCreate(CYPHER_VALUE_INTEGER);
    cypherValueSetInteger(op->apPropValues[1], 30);
    
    op->azPropNames[2] = sqlite3_mprintf("score");
    op->apPropValues[2] = cypherValueCreate(CYPHER_VALUE_FLOAT);
    cypherValueSetFloat(op->apPropValues[2], 95.5);
    
    g_lastStoredNode.bStored = 0;
    
    int rc = cypherCreateNode(writeCtx, op);
    
    TEST_ASSERT(rc == SQLITE_OK, "cypherCreateNode returns SQLITE_OK");
    TEST_ASSERT(g_lastStoredNode.bStored == 1, "Storage function called");
    
    /* Verify properties JSON */
    TEST_ASSERT(strstr(g_lastStoredNode.zProperties, "\"name\":\"Alice\"") != NULL, "String property stored");
    TEST_ASSERT(strstr(g_lastStoredNode.zProperties, "\"age\":30") != NULL, "Integer property stored");
    TEST_ASSERT(strstr(g_lastStoredNode.zProperties, "\"score\":95.5") != NULL, "Float property stored");
    
    cypherCreateNodeOpDestroy(op);
    cypherWriteContextDestroy(writeCtx);
    executionContextDestroy(execCtx);
}

void test_create_node_error_cases(void) {
    printf("\nTest: CREATE node error cases\n");
    
    GraphVtab graph = {0};
    ExecutionContext *execCtx = executionContextCreate(NULL, &graph);
    CypherWriteContext *writeCtx = cypherWriteContextCreate(NULL, &graph, execCtx);
    CreateNodeOp *op = cypherCreateNodeOpCreate();
    
    /* Test NULL context */
    int rc = cypherCreateNode(NULL, op);
    TEST_ASSERT(rc == SQLITE_MISUSE, "NULL context returns SQLITE_MISUSE");
    
    /* Test NULL operation */
    rc = cypherCreateNode(writeCtx, NULL);
    TEST_ASSERT(rc == SQLITE_MISUSE, "NULL operation returns SQLITE_MISUSE");
    
    cypherCreateNodeOpDestroy(op);
    cypherWriteContextDestroy(writeCtx);
    executionContextDestroy(execCtx);
}

void test_create_multiple_nodes(void) {
    printf("\nTest: CREATE multiple nodes\n");
    
    GraphVtab graph = {0};
    ExecutionContext *execCtx = executionContextCreate(NULL, &graph);
    CypherWriteContext *writeCtx = cypherWriteContextCreate(NULL, &graph, execCtx);
    
    /* Create first node */
    CreateNodeOp *op1 = cypherCreateNodeOpCreate();
    op1->zVariable = sqlite3_mprintf("n1");
    int rc1 = cypherCreateNode(writeCtx, op1);
    sqlite3_int64 id1 = op1->iCreatedNodeId;
    
    /* Create second node */
    CreateNodeOp *op2 = cypherCreateNodeOpCreate();
    op2->zVariable = sqlite3_mprintf("n2");
    int rc2 = cypherCreateNode(writeCtx, op2);
    sqlite3_int64 id2 = op2->iCreatedNodeId;
    
    TEST_ASSERT(rc1 == SQLITE_OK && rc2 == SQLITE_OK, "Both nodes created successfully");
    TEST_ASSERT(id1 == 1 && id2 == 2, "Sequential node IDs assigned");
    TEST_ASSERT(writeCtx->nOperations == 2, "Two operations recorded");
    
    cypherCreateNodeOpDestroy(op1);
    cypherCreateNodeOpDestroy(op2);
    cypherWriteContextDestroy(writeCtx);
    executionContextDestroy(execCtx);
}

int main(void) {
    printf("=== SQLite Graph CREATE Node Integration Test ===\n");
    printf("Testing ACTUAL cypherCreateNode implementation\n");
    
    test_create_node_basic();
    test_create_node_with_labels();
    test_create_node_with_properties();
    test_create_node_error_cases();
    test_create_multiple_nodes();
    
    printf("\n=== Test Summary ===\n");
    printf("Total tests: %d\n", g_tests_run);
    printf("Passed: %d\n", g_tests_passed);
    printf("Failed: %d\n", g_tests_run - g_tests_passed);
    
    if (g_tests_passed == g_tests_run) {
        printf("\nSUCCESS: All tests passed!\n");
        return 0;
    } else {
        printf("\nFAILURE: %d tests failed!\n", g_tests_run - g_tests_passed);
        return 1;
    }
}