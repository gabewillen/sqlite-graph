/*
** SQLite Graph Database Extension - Cypher SQL Functions
**
** This file implements SQL functions that expose Cypher parser and query
** execution capabilities to SQLite users. These functions allow users to
** execute Cypher queries through SQL function calls.
**
** Functions provided:
** - cypher_parse(query_text) - Parse Cypher query and return AST
** - cypher_validate(query_text) - Validate Cypher query syntax
** - cypher_execute(query_text) - Execute Cypher query (Phase 7+)
** - cypher_explain(query_text) - Show query execution plan (Phase 7+)
**
** Memory allocation: All functions use sqlite3_malloc()/sqlite3_free()
** Error handling: Functions return SQLite error codes or NULL on error
*/

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1
#include "cypher.h"
#include <string.h>
#include <assert.h>
#include <stdio.h>

/*
** SQL function: cypher_parse(query_text)
**
** Parses a Cypher query and returns the AST as a formatted string.
** This is useful for debugging and understanding query structure.
**
** Usage: SELECT cypher_parse('MATCH (n) RETURN n');
**
** Returns: String representation of the AST, or NULL on error
*/
static void cypherParseSqlFunc(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
) {
  const char *zQuery;
  char *zResult;
  
  /* Validate arguments */
  if( argc != 1 ) {
    sqlite3_result_error(context, "cypher_parse() requires exactly one argument", -1);
    return;
  }
  
  zQuery = (const char*)sqlite3_value_text(argv[0]);
  if( !zQuery ) {
    sqlite3_result_null(context);
    return;
  }
  
  /* Parse the query and get string representation */
  zResult = cypherParseTestQuery(zQuery);
  if( zResult ) {
    sqlite3_result_text(context, zResult, -1, sqlite3_free);
  } else {
    sqlite3_result_error(context, "Failed to parse Cypher query", -1);
  }
}

/*
** SQL function: cypher_validate(query_text)
**
** Validates a Cypher query syntax without executing it.
** Returns 1 if valid, 0 if invalid, with error details.
**
** Usage: SELECT cypher_validate('MATCH (n) RETURN n');
**
** Returns: 1 for valid syntax, 0 for invalid syntax
*/
static void cypherValidateSqlFunc(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
) {
  const char *zQuery;
  CypherParser *pParser;
  int rc;
  
  /* Validate arguments */
  if( argc != 1 ) {
    sqlite3_result_error(context, "cypher_validate() requires exactly one argument", -1);
    return;
  }
  
  zQuery = (const char*)sqlite3_value_text(argv[0]);
  if( !zQuery ) {
    sqlite3_result_int(context, 0);
    return;
  }
  
  /* Create parser and validate query */
  pParser = cypherParserCreate(sqlite3_context_db_handle(context));
  if( !pParser ) {
    sqlite3_result_error_nomem(context);
    return;
  }
  
  rc = cypherParserParse(pParser, zQuery, -1);
  
  if( rc == SQLITE_OK ) {
    sqlite3_result_int(context, 1); /* Valid */
  } else {
    /* Invalid - could optionally set error message as auxiliary data */
    sqlite3_result_int(context, 0);
  }
  
  cypherParserDestroy(pParser);
}

/*
** SQL function: cypher_tokenize(query_text)
**
** Tokenizes a Cypher query and returns token information as JSON.
** This is useful for debugging lexical analysis.
**
** Usage: SELECT cypher_tokenize('MATCH (n) RETURN n');
**
** Returns: JSON array of token objects with type, value, line, column
*/
static void cypherTokenizeSqlFunc(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
) {
  const char *zQuery;
  CypherLexer *pLexer;
  CypherToken *pToken;
  char *zResult;
  int nResult = 0;
  int nAlloc = 256;
  int rc;
  
  /* Validate arguments */
  if( argc != 1 ) {
    sqlite3_result_error(context, "cypher_tokenize() requires exactly one argument", -1);
    return;
  }
  
  zQuery = (const char*)sqlite3_value_text(argv[0]);
  if( !zQuery ) {
    sqlite3_result_null(context);
    return;
  }
  
  /* Create lexer and tokenize */
  pLexer = cypherLexerCreate(zQuery, -1);
  if( !pLexer ) {
    sqlite3_result_error_nomem(context);
    return;
  }
  
  rc = cypherLexerTokenize(pLexer);
  if( rc != SQLITE_OK ) {
    const char *zError = pLexer->zErrorMsg ? pLexer->zErrorMsg : "Tokenization failed";
    sqlite3_result_error(context, zError, -1);
    cypherLexerDestroy(pLexer);
    return;
  }
  
  /* Build JSON result */
  zResult = sqlite3_malloc(nAlloc);
  if( !zResult ) {
    sqlite3_result_error_nomem(context);
    cypherLexerDestroy(pLexer);
    return;
  }
  
  /* Start JSON array */
  nResult = snprintf(zResult, nAlloc, "[");
  
  pToken = pLexer->pTokens;
  int isFirst = 1;
  while( pToken && pToken->type != CYPHER_TOK_EOF ) {
    /* Get token type name */
    const char *zTypeName = "UNKNOWN";
    switch( pToken->type ) {
      case CYPHER_TOK_IDENTIFIER: zTypeName = "IDENTIFIER"; break;
      case CYPHER_TOK_INTEGER: zTypeName = "INTEGER"; break;
      case CYPHER_TOK_FLOAT: zTypeName = "FLOAT"; break;
      case CYPHER_TOK_STRING: zTypeName = "STRING"; break;
      case CYPHER_TOK_MATCH: zTypeName = "MATCH"; break;
      case CYPHER_TOK_RETURN: zTypeName = "RETURN"; break;
      case CYPHER_TOK_WHERE: zTypeName = "WHERE"; break;
      case CYPHER_TOK_LPAREN: zTypeName = "LPAREN"; break;
      case CYPHER_TOK_RPAREN: zTypeName = "RPAREN"; break;
      /* Add more cases as needed */
    }
    
    /* Format token as JSON object */
    const char *zValue = pToken->zValue ? pToken->zValue : "";
    int nNeeded = snprintf(NULL, 0, 
      "%s{\"type\":\"%s\",\"value\":\"%s\",\"line\":%d,\"column\":%d}",
      isFirst ? "" : ",", zTypeName, zValue, pToken->iLine, pToken->iColumn);
    
    /* Resize buffer if needed */
    if( nResult + nNeeded + 2 >= nAlloc ) {
      nAlloc = (nResult + nNeeded + 256) * 2;
      char *zNew = sqlite3_realloc(zResult, nAlloc);
      if( !zNew ) {
        sqlite3_free(zResult);
        sqlite3_result_error_nomem(context);
        cypherLexerDestroy(pLexer);
        return;
      }
      zResult = zNew;
    }
    
    /* Add token to result */
    nResult += snprintf(zResult + nResult, nAlloc - nResult,
      "%s{\"type\":\"%s\",\"value\":\"%s\",\"line\":%d,\"column\":%d}",
      isFirst ? "" : ",", zTypeName, zValue, pToken->iLine, pToken->iColumn);
    
    isFirst = 0;
    pToken = pToken->pNext;
  }
  
  /* Close JSON array */
  if( nResult + 2 < nAlloc ) {
    zResult[nResult++] = ']';
    zResult[nResult] = '\0';
  }
  
  sqlite3_result_text(context, zResult, nResult, sqlite3_free);
  cypherLexerDestroy(pLexer);
}

/*
** SQL function: cypher_ast_info(query_text)
**
** Parses a Cypher query and returns detailed AST information.
** This includes node types, structure, and validation results.
**
** Usage: SELECT cypher_ast_info('MATCH (n) RETURN n');
**
** Returns: Detailed AST information as formatted text
*/
static void cypherAstInfoSqlFunc(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
) {
  const char *zQuery;
  CypherParser *pParser;
  CypherAst *pAst;
  char *zAstString;
  char *zValidationError;
  char *zResult;
  int rc, rcValidation;
  
  /* Validate arguments */
  if( argc != 1 ) {
    sqlite3_result_error(context, "cypher_ast_info() requires exactly one argument", -1);
    return;
  }
  
  zQuery = (const char*)sqlite3_value_text(argv[0]);
  if( !zQuery ) {
    sqlite3_result_null(context);
    return;
  }
  
  /* Create parser and parse query */
  pParser = cypherParserCreate(sqlite3_context_db_handle(context));
  if( !pParser ) {
    sqlite3_result_error_nomem(context);
    return;
  }
  
  rc = cypherParserParse(pParser, zQuery, -1);
  
  if( rc == SQLITE_OK ) {
    pAst = cypherParserGetAst(pParser);
    if( pAst ) {
      /* Get AST string representation */
      zAstString = cypherAstToString(pAst);
      
      /* Validate AST */
      zValidationError = NULL;
      rcValidation = cypherAstValidate(pAst, &zValidationError);
      
      /* Build comprehensive result */
      if( rcValidation == SQLITE_OK ) {
        zResult = sqlite3_mprintf(
          "Parse Status: SUCCESS\n"
          "AST Root Type: %s\n"
          "Child Count: %d\n"
          "Validation: PASSED\n"
          "\nAST Structure:\n%s",
          cypherAstNodeTypeName(pAst->type),
          cypherAstGetChildCount(pAst),
          zAstString ? zAstString : "(no AST string)"
        );
      } else {
        zResult = sqlite3_mprintf(
          "Parse Status: SUCCESS\n"
          "AST Root Type: %s\n"
          "Child Count: %d\n"
          "Validation: FAILED - %s\n"
          "\nAST Structure:\n%s",
          cypherAstNodeTypeName(pAst->type),
          cypherAstGetChildCount(pAst),
          zValidationError ? zValidationError : "Unknown validation error",
          zAstString ? zAstString : "(no AST string)"
        );
      }
      
      sqlite3_free(zAstString);
      sqlite3_free(zValidationError);
    } else {
      zResult = sqlite3_mprintf("Parse Status: SUCCESS but no AST generated");
    }
  } else {
    const char *zError = cypherParserGetError(pParser);
    zResult = sqlite3_mprintf(
      "Parse Status: FAILED\n"
      "Error: %s",
      zError ? zError : "Unknown parse error"
    );
  }
  
  if( zResult ) {
    sqlite3_result_text(context, zResult, -1, sqlite3_free);
  } else {
    sqlite3_result_error_nomem(context);
  }
  
  cypherParserDestroy(pParser);
}

/*
** Register all Cypher SQL functions with the database.
** This should be called during extension initialization.
*/
int cypherRegisterSqlFunctions(sqlite3 *db) {
  int rc = SQLITE_OK;
  
  /* Register cypher_parse function */
  rc = sqlite3_create_function(db, "cypher_parse", 1, 
                              SQLITE_UTF8 | SQLITE_DETERMINISTIC,
                              0, cypherParseSqlFunc, 0, 0);
  if( rc != SQLITE_OK ) return rc;
  
  /* Register cypher_validate function */
  rc = sqlite3_create_function(db, "cypher_validate", 1,
                              SQLITE_UTF8 | SQLITE_DETERMINISTIC,
                              0, cypherValidateSqlFunc, 0, 0);
  if( rc != SQLITE_OK ) return rc;
  
  /* Register cypher_tokenize function */
  rc = sqlite3_create_function(db, "cypher_tokenize", 1,
                              SQLITE_UTF8 | SQLITE_DETERMINISTIC,
                              0, cypherTokenizeSqlFunc, 0, 0);
  if( rc != SQLITE_OK ) return rc;
  
  /* Register cypher_ast_info function */
  rc = sqlite3_create_function(db, "cypher_ast_info", 1,
                              SQLITE_UTF8 | SQLITE_DETERMINISTIC,
                              0, cypherAstInfoSqlFunc, 0, 0);
  if( rc != SQLITE_OK ) return rc;
  
  return SQLITE_OK;
}

/*
** Test function to demonstrate Cypher SQL function usage.
** This can be called from tests or demos.
*/
char *cypherTestSqlFunctions(sqlite3 *db) {
  sqlite3_stmt *pStmt;
  char *zResult;
  int rc;
  
  if( !db ) return NULL;
  
  /* Test cypher_parse function */
  const char *zSql = "SELECT cypher_parse('MATCH (n) RETURN n') AS parse_result";
  
  rc = sqlite3_prepare_v2(db, zSql, -1, &pStmt, NULL);
  if( rc != SQLITE_OK ) {
    return sqlite3_mprintf("Failed to prepare test SQL: %s", sqlite3_errmsg(db));
  }
  
  rc = sqlite3_step(pStmt);
  if( rc == SQLITE_ROW ) {
    const char *zText = (const char*)sqlite3_column_text(pStmt, 0);
    zResult = sqlite3_mprintf("cypher_parse() test result:\n%s", 
                             zText ? zText : "(null)");
  } else {
    zResult = sqlite3_mprintf("Failed to execute test SQL: %s", sqlite3_errmsg(db));
  }
  
  sqlite3_finalize(pStmt);
  return zResult;
}