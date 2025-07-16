#include "cypher.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Helper function to allocate memory and handle errors
static void *cypher_malloc(size_t size) {
    void *ptr = sqlite3_malloc(size);
    if (!ptr) {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

// Helper function to reallocate memory and handle errors
static void *cypher_realloc(void *ptr, size_t size) {
    void *new_ptr = sqlite3_realloc(ptr, size);
    if (!new_ptr) {
        fprintf(stderr, "Memory reallocation failed.\n");
        exit(EXIT_FAILURE);
    }
    return new_ptr;
}

// Helper function to free memory
static void cypher_free(void *ptr) {
    sqlite3_free(ptr);
}

// AST Node creation functions
CypherAst *cypherAstCreate(CypherAstNodeType type, int iLine, int iColumn) {
    CypherAst *pNode = cypher_malloc(sizeof(CypherAst));
    pNode->type = type;
    pNode->zValue = NULL;
    pNode->apChildren = NULL;
    pNode->nChildren = 0;
    pNode->nChildrenAlloc = 0;
    pNode->iLine = iLine;
    pNode->iColumn = iColumn;
    pNode->iFlags = 0;
    return pNode;
}

CypherAst *cypherAstCreateIdentifier(const char *zName, int iLine, int iColumn) {
    CypherAst *pNode = cypherAstCreate(CYPHER_AST_IDENTIFIER, iLine, iColumn);
    if (pNode) {
        pNode->zValue = cypher_malloc(strlen(zName) + 1);
        strcpy(pNode->zValue, zName);
    }
    return pNode;
}

CypherAst *cypherAstCreateLiteral(const char *zValue, int iLine, int iColumn) {
    CypherAst *pNode = cypherAstCreate(CYPHER_AST_LITERAL, iLine, iColumn);
    if (pNode) {
        pNode->zValue = cypher_malloc(strlen(zValue) + 1);
        strcpy(pNode->zValue, zValue);
    }
    return pNode;
}

CypherAst *cypherAstCreateBinaryOp(const char *zOp, CypherAst *pLeft, CypherAst *pRight, int iLine, int iColumn) {
    CypherAst *pNode = cypherAstCreate(CYPHER_AST_BINARY_OP, iLine, iColumn);
    if (pNode) {
        pNode->zValue = cypher_malloc(strlen(zOp) + 1);
        strcpy(pNode->zValue, zOp);
        cypherAstAddChild(pNode, pLeft);
        cypherAstAddChild(pNode, pRight);
    }
    return pNode;
}

CypherAst *cypherAstCreateProperty(CypherAst *pObj, const char *zProp, int iLine, int iColumn) {
    CypherAst *pNode = cypherAstCreate(CYPHER_AST_PROPERTY, iLine, iColumn);
    if (pNode) {
        pNode->zValue = cypher_malloc(strlen(zProp) + 1);
        strcpy(pNode->zValue, zProp);
        cypherAstAddChild(pNode, pObj);
    }
    return pNode;
}

// AST Node manipulation functions
void cypherAstAddChild(CypherAst *pParent, CypherAst *pChild) {
    if (!pParent || !pChild) return;

    if (pParent->nChildren == pParent->nChildrenAlloc) {
        int newAlloc = pParent->nChildrenAlloc == 0 ? 2 : pParent->nChildrenAlloc * 2;
        pParent->apChildren = cypher_realloc(pParent->apChildren, sizeof(CypherAst *) * newAlloc);
        pParent->nChildrenAlloc = newAlloc;
    }
    pParent->apChildren[pParent->nChildren++] = pChild;
}

void cypherAstSetValue(CypherAst *pNode, const char *zValue) {
    if (!pNode) return;
    if (pNode->zValue) {
        cypher_free(pNode->zValue);
    }
    if (zValue) {
        pNode->zValue = cypher_malloc(strlen(zValue) + 1);
        strcpy(pNode->zValue, zValue);
    } else {
        pNode->zValue = NULL;
    }
}

CypherAst *cypherAstGetChild(CypherAst *pNode, int iChild) {
    if (!pNode || iChild < 0 || iChild >= pNode->nChildren) {
        return NULL;
    }
    return pNode->apChildren[iChild];
}

int cypherAstGetChildCount(CypherAst *pNode) {
    return pNode ? pNode->nChildren : 0;
}

const char *cypherAstGetValue(CypherAst *pNode) {
    return pNode ? pNode->zValue : NULL;
}

// AST Node destruction
void cypherAstDestroy(CypherAst *pNode) {
    if (!pNode) return;

    for (int i = 0; i < pNode->nChildren; i++) {
        cypherAstDestroy(pNode->apChildren[i]);
    }
    if (pNode->apChildren) {
        cypher_free(pNode->apChildren);
    }
    if (pNode->zValue) {
        cypher_free(pNode->zValue);
    }
    cypher_free(pNode);
}

// Debugging
const char *cypherAstNodeTypeName(CypherAstNodeType type) {
    switch (type) {
        case CYPHER_AST_QUERY: return "QUERY";
        case CYPHER_AST_SINGLE_QUERY: return "SINGLE_QUERY";
        case CYPHER_AST_UNION: return "UNION";
        case CYPHER_AST_MATCH: return "MATCH";
        case CYPHER_AST_OPTIONAL_MATCH: return "OPTIONAL_MATCH";
        case CYPHER_AST_WHERE: return "WHERE";
        case CYPHER_AST_RETURN: return "RETURN";
        case CYPHER_AST_PROJECTION_LIST: return "PROJECTION_LIST";
        case CYPHER_AST_PROJECTION_ITEM: return "PROJECTION_ITEM";
        case CYPHER_AST_ORDER_BY: return "ORDER_BY";
        case CYPHER_AST_SORT_LIST: return "SORT_LIST";
        case CYPHER_AST_SORT_ITEM: return "SORT_ITEM";
        case CYPHER_AST_LIMIT: return "LIMIT";
        case CYPHER_AST_SKIP: return "SKIP";
        case CYPHER_AST_PATTERN: return "PATTERN";
        case CYPHER_AST_NODE_PATTERN: return "NODE_PATTERN";
        case CYPHER_AST_REL_PATTERN: return "REL_PATTERN";
        case CYPHER_AST_LABELS: return "LABELS";
        case CYPHER_AST_PATH: return "PATH";
        case CYPHER_AST_IDENTIFIER: return "IDENTIFIER";
        case CYPHER_AST_LITERAL: return "LITERAL";
        case CYPHER_AST_UNARY_OP: return "UNARY_OP";
        case CYPHER_AST_BINARY_OP: return "BINARY_OP";
        case CYPHER_AST_PROPERTY: return "PROPERTY";
        case CYPHER_AST_MAP: return "MAP";
        case CYPHER_AST_LIST: return "LIST";
        case CYPHER_AST_FUNCTION_CALL: return "FUNCTION_CALL";
        case CYPHER_AST_CASE: return "CASE";
        case CYPHER_AST_COUNT: return "AST_COUNT";
    }
    return "UNKNOWN_AST_TYPE";
}

void cypherAstPrint(CypherAst *pNode, int iIndent) {
    if (!pNode) return;

    for (int i = 0; i < iIndent; i++) {
        printf("  ");
    }

    printf("- %s", cypherAstNodeTypeName(pNode->type));
    if (pNode->zValue) {
        printf(": %s", pNode->zValue);
    }
    printf(" (Line: %d, Col: %d)\n", pNode->iLine, pNode->iColumn);

    for (int i = 0; i < pNode->nChildren; i++) {
        cypherAstPrint(pNode->apChildren[i], iIndent + 1);
    }
}
