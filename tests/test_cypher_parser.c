/*
** SQLite Graph Database Extension - Cypher Parser Tests
**
** Comprehensive tests for the Cypher language parser, lexer, and AST
** functionality. These tests verify the Phase 6 implementation of
** openCypher parsing capabilities.
*/

#define SQLITE_CORE
#include "unity.h"
#include "sqlite3.h"
#include "../include/cypher.h"
#include <string.h>
#include <stdlib.h>

/*
** Test lexer functionality.
*/
void testCypherLexerBasic(void) {
  CypherLexer *pLexer;
  int rc;
  
  /* Test basic tokenization */
  const char *zQuery = "MATCH (n) RETURN n";
  pLexer = cypherLexerCreate(zQuery, -1);
  TEST_ASSERT_NOT_NULL(pLexer);
  
  rc = cypherLexerTokenize(pLexer);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  /* Verify tokens exist */
  TEST_ASSERT_NOT_NULL(pLexer->pTokens);
  
  /* Check first few tokens */
  CypherToken *pToken = pLexer->pTokens;
  TEST_ASSERT_NOT_NULL(pToken);
  TEST_ASSERT_EQUAL(CYPHER_TOK_MATCH, pToken->type);
  
  pToken = pToken->pNext;
  TEST_ASSERT_NOT_NULL(pToken);
  TEST_ASSERT_EQUAL(CYPHER_TOK_LPAREN, pToken->type);
  
  pToken = pToken->pNext;
  TEST_ASSERT_NOT_NULL(pToken);
  TEST_ASSERT_EQUAL(CYPHER_TOK_IDENTIFIER, pToken->type);
  TEST_ASSERT_EQUAL_STRING("n", pToken->zValue);
  
  cypherLexerDestroy(pLexer);
}

/*
** Test lexer with complex query.
*/
void testCypherLexerComplex(void) {
  CypherLexer *pLexer;
  int rc;
  
  const char *zQuery = "MATCH (p:Person)-[:KNOWS]->(f) WHERE p.age > 30 RETURN p.name, f.name";
  pLexer = cypherLexerCreate(zQuery, -1);
  TEST_ASSERT_NOT_NULL(pLexer);
  
  rc = cypherLexerTokenize(pLexer);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  /* Count tokens */
  int nTokens = 0;
  CypherToken *pToken = pLexer->pTokens;
  while( pToken && pToken->type != CYPHER_TOK_EOF ) {
    nTokens++;
    pToken = pToken->pNext;
  }
  
  TEST_ASSERT_TRUE(nTokens > 10); /* Should have many tokens */
  
  cypherLexerDestroy(pLexer);
}

/*
** Test lexer with string literals.
*/
void testCypherLexerStrings(void) {
  CypherLexer *pLexer;
  int rc;
  
  const char *zQuery = "MATCH (n) WHERE n.name = 'Alice' RETURN n";
  pLexer = cypherLexerCreate(zQuery, -1);
  TEST_ASSERT_NOT_NULL(pLexer);
  
  rc = cypherLexerTokenize(pLexer);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  /* Find the string token */
  CypherToken *pToken = pLexer->pTokens;
  while( pToken && pToken->type != CYPHER_TOK_STRING ) {
    pToken = pToken->pNext;
  }
  
  TEST_ASSERT_NOT_NULL(pToken);
  TEST_ASSERT_EQUAL(CYPHER_TOK_STRING, pToken->type);
  TEST_ASSERT_EQUAL_STRING("Alice", pToken->zValue);
  
  cypherLexerDestroy(pLexer);
}

/*
** Test lexer with numbers.
*/
void testCypherLexerNumbers(void) {
  CypherLexer *pLexer;
  int rc;
  
  const char *zQuery = "MATCH (n) WHERE n.age = 42 AND n.height = 5.8 RETURN n";
  pLexer = cypherLexerCreate(zQuery, -1);
  TEST_ASSERT_NOT_NULL(pLexer);
  
  rc = cypherLexerTokenize(pLexer);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  /* Find integer token */
  CypherToken *pToken = pLexer->pTokens;
  while( pToken && (pToken->type != CYPHER_TOK_INTEGER || 
                   strcmp(pToken->zValue, "42") != 0) ) {
    pToken = pToken->pNext;
  }
  
  TEST_ASSERT_NOT_NULL(pToken);
  TEST_ASSERT_EQUAL(CYPHER_TOK_INTEGER, pToken->type);
  TEST_ASSERT_EQUAL_STRING("42", pToken->zValue);
  
  /* Find float token */
  pToken = pLexer->pTokens;
  while( pToken && pToken->type != CYPHER_TOK_FLOAT ) {
    pToken = pToken->pNext;
  }
  
  TEST_ASSERT_NOT_NULL(pToken);
  TEST_ASSERT_EQUAL(CYPHER_TOK_FLOAT, pToken->type);
  TEST_ASSERT_EQUAL_STRING("5.8", pToken->zValue);
  
  cypherLexerDestroy(pLexer);
}

/*
** Test keyword recognition.
*/
void testCypherLexerKeywords(void) {
  /* Test individual keyword recognition */
  TEST_ASSERT_TRUE(cypherIsKeyword("MATCH"));
  TEST_ASSERT_TRUE(cypherIsKeyword("match")); /* Case insensitive */
  TEST_ASSERT_TRUE(cypherIsKeyword("WHERE"));
  TEST_ASSERT_TRUE(cypherIsKeyword("RETURN"));
  TEST_ASSERT_FALSE(cypherIsKeyword("notakeyword"));
  
  /* Test token type mapping */
  TEST_ASSERT_EQUAL(CYPHER_TOK_MATCH, cypherGetKeywordToken("MATCH"));
  TEST_ASSERT_EQUAL(CYPHER_TOK_WHERE, cypherGetKeywordToken("WHERE"));
  TEST_ASSERT_EQUAL(CYPHER_TOK_RETURN, cypherGetKeywordToken("RETURN"));
  TEST_ASSERT_EQUAL(CYPHER_TOK_IDENTIFIER, cypherGetKeywordToken("notakeyword"));
}

/*
** Test AST node creation and manipulation.
*/
void testCypherAstBasic(void) {
  CypherAst *pAst;
  CypherAst *pChild1, *pChild2;
  int rc;
  
  /* Create root node */
  pAst = cypherAstCreate(CYPHER_AST_QUERY, 1, 1);
  TEST_ASSERT_NOT_NULL(pAst);
  TEST_ASSERT_EQUAL(CYPHER_AST_QUERY, pAst->type);
  TEST_ASSERT_EQUAL(1, pAst->iLine);
  TEST_ASSERT_EQUAL(1, pAst->iColumn);
  TEST_ASSERT_EQUAL(0, pAst->nChildren);
  
  /* Test setting value */
  rc = cypherAstSetValue(pAst, "test_value");
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_EQUAL_STRING("test_value", pAst->zValue);
  
  /* Create child nodes */
  pChild1 = cypherAstCreate(CYPHER_AST_MATCH, 1, 1);
  pChild2 = cypherAstCreate(CYPHER_AST_RETURN, 1, 15);
  TEST_ASSERT_NOT_NULL(pChild1);
  TEST_ASSERT_NOT_NULL(pChild2);
  
  /* Add children */
  rc = cypherAstAddChild(pAst, pChild1);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_EQUAL(1, pAst->nChildren);
  
  rc = cypherAstAddChild(pAst, pChild2);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_EQUAL(2, pAst->nChildren);
  
  /* Verify children */
  TEST_ASSERT_EQUAL(pChild1, cypherAstGetChild(pAst, 0));
  TEST_ASSERT_EQUAL(pChild2, cypherAstGetChild(pAst, 1));
  TEST_ASSERT_NULL(cypherAstGetChild(pAst, 2)); /* Out of bounds */
  
  /* Test utility functions */
  TEST_ASSERT_EQUAL(2, cypherAstGetChildCount(pAst));
  TEST_ASSERT_TRUE(cypherAstIsType(pAst, CYPHER_AST_QUERY));
  TEST_ASSERT_FALSE(cypherAstIsType(pAst, CYPHER_AST_MATCH));
  TEST_ASSERT_EQUAL_STRING("test_value", cypherAstGetValue(pAst));
  
  cypherAstDestroy(pAst);
}

/*
** Test AST convenience functions.
*/
void testCypherAstConvenience(void) {
  CypherAst *pLiteral, *pIdentifier, *pBinaryOp, *pProperty;
  
  /* Test literal creation */
  pLiteral = cypherAstCreateLiteral("42", 1, 10);
  TEST_ASSERT_NOT_NULL(pLiteral);
  TEST_ASSERT_EQUAL(CYPHER_AST_LITERAL, pLiteral->type);
  TEST_ASSERT_EQUAL_STRING("42", pLiteral->zValue);
  
  /* Test identifier creation */
  pIdentifier = cypherAstCreateIdentifier("n", 1, 5);
  TEST_ASSERT_NOT_NULL(pIdentifier);
  TEST_ASSERT_EQUAL(CYPHER_AST_IDENTIFIER, pIdentifier->type);
  TEST_ASSERT_EQUAL_STRING("n", pIdentifier->zValue);
  
  /* Test binary operation creation */
  CypherAst *pLeft = cypherAstCreateIdentifier("a", 1, 1);
  CypherAst *pRight = cypherAstCreateLiteral("10", 1, 5);
  pBinaryOp = cypherAstCreateBinaryOp("=", pLeft, pRight, 1, 3);
  TEST_ASSERT_NOT_NULL(pBinaryOp);
  TEST_ASSERT_EQUAL(CYPHER_AST_BINARY_OP, pBinaryOp->type);
  TEST_ASSERT_EQUAL_STRING("=", pBinaryOp->zValue);
  TEST_ASSERT_EQUAL(2, pBinaryOp->nChildren);
  
  /* Test property creation */
  CypherAst *pObject = cypherAstCreateIdentifier("n", 1, 1);
  pProperty = cypherAstCreateProperty(pObject, "name", 1, 3);
  TEST_ASSERT_NOT_NULL(pProperty);
  TEST_ASSERT_EQUAL(CYPHER_AST_PROPERTY, pProperty->type);
  TEST_ASSERT_EQUAL(2, pProperty->nChildren);
  
  cypherAstDestroy(pLiteral);
  cypherAstDestroy(pIdentifier);
  cypherAstDestroy(pBinaryOp);
  cypherAstDestroy(pProperty);
}

/*
** Test AST string representation.
*/
void testCypherAstToString(void) {
  CypherAst *pAst;
  char *zString;
  
  /* Create simple AST */
  pAst = cypherAstCreateIdentifier("n", 1, 1);
  TEST_ASSERT_NOT_NULL(pAst);
  
  /* Get string representation */
  zString = cypherAstToString(pAst);
  TEST_ASSERT_NOT_NULL(zString);
  TEST_ASSERT_NOT_NULL(strstr(zString, "IDENTIFIER"));
  TEST_ASSERT_NOT_NULL(strstr(zString, "n"));
  
  sqlite3_free(zString);
  cypherAstDestroy(pAst);
}

/*
** Test AST validation.
*/
void testCypherAstValidation(void) {
  CypherAst *pAst;
  char *zError = NULL;
  int rc;
  
  /* Test valid AST */
  pAst = cypherAstCreateIdentifier("valid_name", 1, 1);
  rc = cypherAstValidate(pAst, &zError);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  TEST_ASSERT_NULL(zError);
  cypherAstDestroy(pAst);
  
  /* Test invalid AST (empty identifier) */
  pAst = cypherAstCreate(CYPHER_AST_IDENTIFIER, 1, 1);
  cypherAstSetValue(pAst, ""); /* Empty identifier */
  rc = cypherAstValidate(pAst, &zError);
  TEST_ASSERT_EQUAL(SQLITE_ERROR, rc);
  TEST_ASSERT_NOT_NULL(zError);
  sqlite3_free(zError);
  cypherAstDestroy(pAst);
}

/*
** Test parser creation and destruction.
*/
void testCypherParserBasic(void) {
  CypherParser *pParser;
  
  pParser = cypherParserCreate(NULL);
  TEST_ASSERT_NOT_NULL(pParser);
  
  /* Initially should have no AST or error */
  TEST_ASSERT_NULL(cypherParserGetAst(pParser));
  TEST_ASSERT_NULL(cypherParserGetError(pParser));
  
  cypherParserDestroy(pParser);
  
  /* Test destruction with NULL */
  cypherParserDestroy(NULL); /* Should not crash */
}

/*
** Test basic query parsing.
*/
void testCypherParserSimple(void) {
  CypherParser *pParser;
  int rc;
  CypherAst *pAst;
  
  pParser = cypherParserCreate(NULL);
  TEST_ASSERT_NOT_NULL(pParser);
  
  /* Parse simple query */
  const char *zQuery = "MATCH (n) RETURN n";
  rc = cypherParserParse(pParser, zQuery, -1);
  TEST_ASSERT_EQUAL(SQLITE_OK, rc);
  
  /* Should have AST */
  pAst = cypherParserGetAst(pParser);
  TEST_ASSERT_NOT_NULL(pAst);
  
  /* Should be a query node */
  TEST_ASSERT_EQUAL(CYPHER_AST_QUERY, pAst->type);
  
  cypherParserDestroy(pParser);
}

/*
** Test parser with invalid input.
*/
void testCypherParserError(void) {
  CypherParser *pParser;
  int rc;
  const char *zError;
  
  pParser = cypherParserCreate(NULL);
  TEST_ASSERT_NOT_NULL(pParser);
  
  /* Parse invalid query (unclosed string) */
  const char *zQuery = "MATCH (n) WHERE n.name = 'unclosed";
  rc = cypherParserParse(pParser, zQuery, -1);
  TEST_ASSERT_NOT_EQUAL(SQLITE_OK, rc);
  
  /* Should have error message */
  zError = cypherParserGetError(pParser);
  TEST_ASSERT_NOT_NULL(zError);
  
  cypherParserDestroy(pParser);
}

/*
** Test the test query function.
*/
void testCypherParseTestQuery(void) {
  char *zResult;
  
  /* Test simple query */
  zResult = cypherParseTestQuery("MATCH (n) RETURN n");
  TEST_ASSERT_NOT_NULL(zResult);
  /* Should contain AST representation */
  TEST_ASSERT_NOT_NULL(strstr(zResult, "QUERY"));
  
  sqlite3_free(zResult);
  
  /* Test invalid query */
  zResult = cypherParseTestQuery("INVALID SYNTAX @#$");
  TEST_ASSERT_NOT_NULL(zResult);
  /* Should contain error */
  TEST_ASSERT_NOT_NULL(strstr(zResult, "ERROR"));
  
  sqlite3_free(zResult);
}

/*
** Test AST node type name function.
*/
void testCypherAstNodeTypeName(void) {
  TEST_ASSERT_EQUAL_STRING("QUERY", cypherAstNodeTypeName(CYPHER_AST_QUERY));
  TEST_ASSERT_EQUAL_STRING("MATCH", cypherAstNodeTypeName(CYPHER_AST_MATCH));
  TEST_ASSERT_EQUAL_STRING("RETURN", cypherAstNodeTypeName(CYPHER_AST_RETURN));
  TEST_ASSERT_EQUAL_STRING("IDENTIFIER", cypherAstNodeTypeName(CYPHER_AST_IDENTIFIER));
  TEST_ASSERT_EQUAL_STRING("UNKNOWN", cypherAstNodeTypeName((CypherAstNodeType)999));
}

/*
** Test test AST creation.
*/
void testCypherCreateTestAst(void) {
  CypherAst *pAst;
  
  pAst = cypherCreateTestAst();
  TEST_ASSERT_NOT_NULL(pAst);
  TEST_ASSERT_EQUAL(CYPHER_AST_QUERY, pAst->type);
  
  /* Should have structure: Query -> SingleQuery -> (Match, Return) */
  TEST_ASSERT_TRUE(pAst->nChildren >= 1);
  
  cypherAstDestroy(pAst);
}

/*
** Main test runner for Cypher parser tests.
*/
void runCypherParserTests(void) {
  RUN_TEST(testCypherLexerBasic);
  RUN_TEST(testCypherLexerComplex);
  RUN_TEST(testCypherLexerStrings);
  RUN_TEST(testCypherLexerNumbers);
  RUN_TEST(testCypherLexerKeywords);
  RUN_TEST(testCypherAstBasic);
  RUN_TEST(testCypherAstConvenience);
  RUN_TEST(testCypherAstToString);
  RUN_TEST(testCypherAstValidation);
  RUN_TEST(testCypherParserBasic);
  RUN_TEST(testCypherParserSimple);
  RUN_TEST(testCypherParserError);
  RUN_TEST(testCypherParseTestQuery);
  RUN_TEST(testCypherAstNodeTypeName);
  RUN_TEST(testCypherCreateTestAst);
}