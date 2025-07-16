/*
** SQLite Graph Database Extension - Cypher AST Implementation
**
** This file implements the Abstract Syntax Tree (AST) structures and
** operations for the openCypher query language. The AST represents
** the parsed structure of Cypher queries.
**
** Features:
** - Type-safe AST node creation and manipulation
** - Hierarchical tree structure with parent-child relationships
** - Memory management following SQLite patterns
** - String representation for debugging
** - Semantic validation framework
**
** Memory allocation: All functions use sqlite3_malloc()/sqlite3_free()
** Error handling: Functions return SQLite error codes
*/

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1
#include "cypher.h"
#include <string.h>
#include <assert.h>

/*
** Initial capacity for child node arrays.
*/
#define AST_INITIAL_CHILDREN 4

/*
** Create a new AST node of the specified type.
** Returns NULL on allocation failure.
*/
CypherAst *cypherAstCreate(CypherAstNodeType type, int iLine, int iColumn) {
  CypherAst *pAst;
  
  pAst = sqlite3_malloc(sizeof(CypherAst));
  if( !pAst ) return NULL;
  
  memset(pAst, 0, sizeof(CypherAst));
  pAst->type = type;
  pAst->iLine = iLine;
  pAst->iColumn = iColumn;
  pAst->nMaxChildren = AST_INITIAL_CHILDREN;
  
  /* Allocate initial child array */
  pAst->apChildren = sqlite3_malloc(sizeof(CypherAst*) * AST_INITIAL_CHILDREN);
  if( !pAst->apChildren ) {
    sqlite3_free(pAst);
    return NULL;
  }
  
  memset(pAst->apChildren, 0, sizeof(CypherAst*) * AST_INITIAL_CHILDREN);
  
  return pAst;
}

/*
** Destroy an AST node and all its children.
** Safe to call with NULL pointer.
*/
void cypherAstDestroy(CypherAst *pAst) {
  if( !pAst ) return;
  
  /* Destroy all children first */
  for( int i = 0; i < pAst->nChildren; i++ ) {
    cypherAstDestroy(pAst->apChildren[i]);
  }
  
  /* Free allocated memory */
  sqlite3_free(pAst->apChildren);
  sqlite3_free(pAst->zValue);
  sqlite3_free(pAst->pExtra);
  sqlite3_free(pAst);
}

/*
** Add a child node to an AST node.
** Returns SQLITE_OK on success, SQLITE_NOMEM on allocation failure.
*/
int cypherAstAddChild(CypherAst *pParent, CypherAst *pChild) {
  if( !pParent || !pChild ) return SQLITE_MISUSE;
  
  /* Grow child array if needed */
  if( pParent->nChildren >= pParent->nMaxChildren ) {
    int nNewMax = pParent->nMaxChildren * 2;
    CypherAst **apNew = sqlite3_realloc(pParent->apChildren, 
                                       sizeof(CypherAst*) * nNewMax);
    if( !apNew ) return SQLITE_NOMEM;
    
    pParent->apChildren = apNew;
    pParent->nMaxChildren = nNewMax;
    
    /* Initialize new slots to NULL */
    for( int i = pParent->nChildren; i < nNewMax; i++ ) {
      pParent->apChildren[i] = NULL;
    }
  }
  
  /* Add child */
  pParent->apChildren[pParent->nChildren++] = pChild;
  return SQLITE_OK;
}

/*
** Set the string value of an AST node.
** Makes a copy of the string using sqlite3_malloc().
*/
int cypherAstSetValue(CypherAst *pAst, const char *zValue) {
  if( !pAst ) return SQLITE_MISUSE;
  
  /* Free existing value */
  sqlite3_free(pAst->zValue);
  pAst->zValue = NULL;
  
  if( zValue ) {
    pAst->zValue = sqlite3_mprintf("%s", zValue);
    if( !pAst->zValue ) return SQLITE_NOMEM;
  }
  
  return SQLITE_OK;
}

/*
** Get string representation of AST node type.
** Returns static string, do not free.
*/
const char *cypherAstNodeTypeName(CypherAstNodeType type) {
  switch( type ) {
    case CYPHER_AST_QUERY:           return "QUERY";
    case CYPHER_AST_SINGLE_QUERY:    return "SINGLE_QUERY";
    case CYPHER_AST_UNION:           return "UNION";
    case CYPHER_AST_MATCH:           return "MATCH";
    case CYPHER_AST_OPTIONAL_MATCH:  return "OPTIONAL_MATCH";
    case CYPHER_AST_WHERE:           return "WHERE";
    case CYPHER_AST_RETURN:          return "RETURN";
    case CYPHER_AST_WITH:            return "WITH";
    case CYPHER_AST_CREATE:          return "CREATE";
    case CYPHER_AST_MERGE:           return "MERGE";
    case CYPHER_AST_SET:             return "SET";
    case CYPHER_AST_DELETE:          return "DELETE";
    case CYPHER_AST_REMOVE:          return "REMOVE";
    case CYPHER_AST_ORDER_BY:        return "ORDER_BY";
    case CYPHER_AST_LIMIT:           return "LIMIT";
    case CYPHER_AST_SKIP:            return "SKIP";
    case CYPHER_AST_PATTERN:         return "PATTERN";
    case CYPHER_AST_PATH:            return "PATH";
    case CYPHER_AST_NODE_PATTERN:    return "NODE_PATTERN";
    case CYPHER_AST_REL_PATTERN:     return "REL_PATTERN";
    case CYPHER_AST_LITERAL:         return "LITERAL";
    case CYPHER_AST_IDENTIFIER:      return "IDENTIFIER";
    case CYPHER_AST_PROPERTY:        return "PROPERTY";
    case CYPHER_AST_LABELS:          return "LABELS";
    case CYPHER_AST_BINARY_OP:       return "BINARY_OP";
    case CYPHER_AST_UNARY_OP:        return "UNARY_OP";
    case CYPHER_AST_FUNCTION_CALL:   return "FUNCTION_CALL";
    case CYPHER_AST_CASE:            return "CASE";
    case CYPHER_AST_LIST:            return "LIST";
    case CYPHER_AST_MAP:             return "MAP";
    case CYPHER_AST_COUNT:           return "COUNT";
    case CYPHER_AST_SUM:             return "SUM";
    case CYPHER_AST_AVG:             return "AVG";
    case CYPHER_AST_MIN:             return "MIN";
    case CYPHER_AST_MAX:             return "MAX";
    case CYPHER_AST_COLLECT:         return "COLLECT";
    case CYPHER_AST_PROJECTION_ITEM: return "PROJECTION_ITEM";
    case CYPHER_AST_PROJECTION_LIST: return "PROJECTION_LIST";
    case CYPHER_AST_SORT_ITEM:       return "SORT_ITEM";
    case CYPHER_AST_SORT_LIST:       return "SORT_LIST";
    default:                         return "UNKNOWN";
  }
}

/*
** Convert AST to string representation (for debugging).
** Caller must sqlite3_free() the returned string.
*/
char *cypherAstToString(CypherAst *pAst) {
  char *zResult, *zChildren, *zNew;
  
  if( !pAst ) return sqlite3_mprintf("NULL");
  
  /* Start with node type and value */
  if( pAst->zValue ) {
    zResult = sqlite3_mprintf("(%s \"%s\"", 
                             cypherAstNodeTypeName(pAst->type), 
                             pAst->zValue);
  } else {
    zResult = sqlite3_mprintf("(%s", cypherAstNodeTypeName(pAst->type));
  }
  
  if( !zResult ) return NULL;
  
  /* Add children */
  for( int i = 0; i < pAst->nChildren; i++ ) {
    zChildren = cypherAstToString(pAst->apChildren[i]);
    if( !zChildren ) {
      sqlite3_free(zResult);
      return NULL;
    }
    
    zNew = sqlite3_mprintf("%s %s", zResult, zChildren);
    sqlite3_free(zResult);
    sqlite3_free(zChildren);
    
    if( !zNew ) return NULL;
    zResult = zNew;
  }
  
  /* Close parentheses */
  zNew = sqlite3_mprintf("%s)", zResult);
  sqlite3_free(zResult);
  
  return zNew;
}

/*
** Validate AST for semantic correctness.
** Returns SQLITE_OK if valid, error code if invalid.
*/
int cypherAstValidate(CypherAst *pAst, char **pzError) {
  if( !pAst ) {
    if( pzError ) *pzError = sqlite3_mprintf("NULL AST node");
    return SQLITE_ERROR;
  }
  
  /* Basic validation - check children recursively */
  for( int i = 0; i < pAst->nChildren; i++ ) {
    int rc = cypherAstValidate(pAst->apChildren[i], pzError);
    if( rc != SQLITE_OK ) return rc;
  }
  
  /* Type-specific validation */
  switch( pAst->type ) {
    case CYPHER_AST_IDENTIFIER:
      if( !pAst->zValue || strlen(pAst->zValue) == 0 ) {
        if( pzError ) *pzError = sqlite3_mprintf("Empty identifier");
        return SQLITE_ERROR;
      }
      break;
      
    case CYPHER_AST_LITERAL:
      if( !pAst->zValue ) {
        if( pzError ) *pzError = sqlite3_mprintf("Literal without value");
        return SQLITE_ERROR;
      }
      break;
      
    case CYPHER_AST_PROPERTY:
      if( pAst->nChildren != 2 ) {
        if( pzError ) *pzError = sqlite3_mprintf("Property access requires object and property name");
        return SQLITE_ERROR;
      }
      break;
      
    case CYPHER_AST_BINARY_OP:
      if( pAst->nChildren != 2 ) {
        if( pzError ) *pzError = sqlite3_mprintf("Binary operation requires two operands");
        return SQLITE_ERROR;
      }
      break;
      
    case CYPHER_AST_UNARY_OP:
      if( pAst->nChildren != 1 ) {
        if( pzError ) *pzError = sqlite3_mprintf("Unary operation requires one operand");
        return SQLITE_ERROR;
      }
      break;
      
    case CYPHER_AST_MATCH:
      if( pAst->nChildren == 0 ) {
        if( pzError ) *pzError = sqlite3_mprintf("MATCH clause requires at least one pattern");
        return SQLITE_ERROR;
      }
      break;
      
    case CYPHER_AST_RETURN:
      if( pAst->nChildren == 0 ) {
        if( pzError ) *pzError = sqlite3_mprintf("RETURN clause requires at least one projection");
        return SQLITE_ERROR;
      }
      break;
      
    default:
      /* No specific validation for other node types */
      break;
  }
  
  return SQLITE_OK;
}

/*
** Create a literal AST node with the given value.
** Convenience function for common case.
*/
CypherAst *cypherAstCreateLiteral(const char *zValue, int iLine, int iColumn) {
  CypherAst *pAst = cypherAstCreate(CYPHER_AST_LITERAL, iLine, iColumn);
  if( pAst && cypherAstSetValue(pAst, zValue) != SQLITE_OK ) {
    cypherAstDestroy(pAst);
    return NULL;
  }
  return pAst;
}

/*
** Create an identifier AST node with the given name.
** Convenience function for common case.
*/
CypherAst *cypherAstCreateIdentifier(const char *zName, int iLine, int iColumn) {
  CypherAst *pAst = cypherAstCreate(CYPHER_AST_IDENTIFIER, iLine, iColumn);
  if( pAst && cypherAstSetValue(pAst, zName) != SQLITE_OK ) {
    cypherAstDestroy(pAst);
    return NULL;
  }
  return pAst;
}

/*
** Create a binary operation AST node.
** Convenience function for common case.
*/
CypherAst *cypherAstCreateBinaryOp(const char *zOp, CypherAst *pLeft, 
                                   CypherAst *pRight, int iLine, int iColumn) {
  CypherAst *pAst = cypherAstCreate(CYPHER_AST_BINARY_OP, iLine, iColumn);
  if( !pAst ) return NULL;
  
  if( cypherAstSetValue(pAst, zOp) != SQLITE_OK ||
      cypherAstAddChild(pAst, pLeft) != SQLITE_OK ||
      cypherAstAddChild(pAst, pRight) != SQLITE_OK ) {
    cypherAstDestroy(pAst);
    return NULL;
  }
  
  return pAst;
}

/*
** Create a property access AST node.
** Convenience function for common case.
*/
CypherAst *cypherAstCreateProperty(CypherAst *pObject, const char *zProperty,
                                   int iLine, int iColumn) {
  CypherAst *pAst = cypherAstCreate(CYPHER_AST_PROPERTY, iLine, iColumn);
  CypherAst *pPropName = cypherAstCreateIdentifier(zProperty, iLine, iColumn);
  
  if( !pAst || !pPropName ) {
    cypherAstDestroy(pAst);
    cypherAstDestroy(pPropName);
    return NULL;
  }
  
  if( cypherAstAddChild(pAst, pObject) != SQLITE_OK ||
      cypherAstAddChild(pAst, pPropName) != SQLITE_OK ) {
    cypherAstDestroy(pAst);
    cypherAstDestroy(pPropName);
    return NULL;
  }
  
  return pAst;
}

/*
** Get the child AST node at the specified index.
** Returns NULL if index is out of bounds.
*/
CypherAst *cypherAstGetChild(CypherAst *pAst, int iChild) {
  if( !pAst || iChild < 0 || iChild >= pAst->nChildren ) {
    return NULL;
  }
  return pAst->apChildren[iChild];
}

/*
** Get the number of children in an AST node.
*/
int cypherAstGetChildCount(CypherAst *pAst) {
  return pAst ? pAst->nChildren : 0;
}

/*
** Check if an AST node has the specified type.
*/
int cypherAstIsType(CypherAst *pAst, CypherAstNodeType type) {
  return pAst && pAst->type == type;
}

/*
** Get the value of an AST node.
** Returns NULL if node has no value.
*/
const char *cypherAstGetValue(CypherAst *pAst) {
  return pAst ? pAst->zValue : NULL;
}