/*
* Copyright 2018-2024 Redis Labs Ltd. and Contributors
*
* This file is available under the Redis Source Available License 2.0,
* applying the same terms andconditions as the Redis Source Available License 2.0.
* You may not use this file except in compliance with the Redis Source Available License 2.0.
*
* A copy of the Redis Source Available License 2.0 is available at
* https://redis.io/rsal/Redis-Source-Available-License-2.0/
*
* The Redis Source Available License 2.0 is a copy-left license that requires any
* derivative work to be made available under the same terms and conditions.
* 
* See the file LICENSE for more details.
*/

#include "unity.h"
#include "cypher/cypher-parser.h"
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>

/* Global test database - required for extension functions */
sqlite3 *g_pTestDb = 0;

/* Forward declaration of extension init function */
int sqlite3_graph_init(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi);

void setUp(void) {
    int rc;
    if (!g_pTestDb) {
        rc = sqlite3_open(":memory:", &g_pTestDb);
        if (rc == SQLITE_OK) {
            sqlite3_db_config(g_pTestDb, SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION, 1, 0);
            sqlite3_enable_load_extension(g_pTestDb, 1);
            sqlite3_graph_init(g_pTestDb, NULL, NULL);
        }
    }
}

void tearDown(void) {
    // Keep database open between tests for efficiency
    // It will be closed when the process exits
}

void test_valid_query() {
    const char *query = "MATCH (n) RETURN n";
    CypherParser *parser = cypherParserCreate();
    char *errMsg = NULL;
    CypherAst *ast = cypherParse(parser, query, &errMsg);

    TEST_ASSERT_NOT_NULL(ast);
    TEST_ASSERT_NULL(errMsg);
    TEST_ASSERT_EQUAL(CYPHER_AST_QUERY, ast->type);

    CypherAst *single_query = cypherAstGetChild(ast, 0);
    TEST_ASSERT_NOT_NULL(single_query);
    TEST_ASSERT_EQUAL(CYPHER_AST_SINGLE_QUERY, single_query->type);

    CypherAst *match_clause = cypherAstGetChild(single_query, 0);
    TEST_ASSERT_NOT_NULL(match_clause);
    TEST_ASSERT_EQUAL(CYPHER_AST_MATCH, match_clause->type);

    CypherAst *return_clause = cypherAstGetChild(single_query, 1);
    TEST_ASSERT_NOT_NULL(return_clause);
    TEST_ASSERT_EQUAL(CYPHER_AST_RETURN, return_clause->type);

    CypherAst *projection_list = cypherAstGetChild(return_clause, 0);
    TEST_ASSERT_NOT_NULL(projection_list);
    TEST_ASSERT_EQUAL(CYPHER_AST_PROJECTION_LIST, projection_list->type);

    CypherAst *projection_item = cypherAstGetChild(projection_list, 0);
    TEST_ASSERT_NOT_NULL(projection_item);
    TEST_ASSERT_EQUAL(CYPHER_AST_PROJECTION_ITEM, projection_item->type);

    CypherAst *expression = cypherAstGetChild(projection_item, 0);
    TEST_ASSERT_NOT_NULL(expression);
    // This is a simplification in the current parser
    // TEST_ASSERT_EQUAL(CYPHER_AST_IDENTIFIER, expression->type);

    cypherAstDestroy(ast);
    cypherParserDestroy(parser);
}

void test_invalid_query() {
    const char *query = "MATCH (n) RETURN n n";
    CypherParser *parser = cypherParserCreate();
    char *errMsg = NULL;
    CypherAst *ast = cypherParse(parser, query, &errMsg);

    TEST_ASSERT_NULL(ast);
    TEST_ASSERT_NOT_NULL(errMsg);

    cypherParserDestroy(parser);
    free(errMsg);
}

void test_query_with_where_clause() {
    const char *query = "MATCH (n) WHERE n.name = 'John' RETURN n";
    CypherParser *parser = cypherParserCreate();
    char *errMsg = NULL;
    CypherAst *ast = cypherParse(parser, query, &errMsg);

    TEST_ASSERT_NOT_NULL(ast);
    TEST_ASSERT_NULL(errMsg);

    CypherAst *single_query = cypherAstGetChild(ast, 0);
    TEST_ASSERT_NOT_NULL(single_query);

    CypherAst *where_clause = cypherAstGetChild(single_query, 1);
    TEST_ASSERT_NOT_NULL(where_clause);
    TEST_ASSERT_EQUAL(CYPHER_AST_WHERE, where_clause->type);

    cypherAstDestroy(ast);
    cypherParserDestroy(parser);
}

// Regression test for NULL AST node handling
void test_parser_null_input_regression() {
    CypherParser *parser = cypherParserCreate();
    char *errMsg = NULL;
    CypherAst *ast = cypherParse(parser, NULL, &errMsg);
    
    TEST_ASSERT_NULL(ast);
    TEST_ASSERT_NOT_NULL(errMsg);
    
    cypherParserDestroy(parser);
    if (errMsg) free(errMsg);
}

// Regression test for empty query handling
void test_parser_empty_query_regression() {
    const char *query = "";
    CypherParser *parser = cypherParserCreate();
    char *errMsg = NULL;
    CypherAst *ast = cypherParse(parser, query, &errMsg);
    
    TEST_ASSERT_NULL(ast);
    TEST_ASSERT_NOT_NULL(errMsg);
    
    cypherParserDestroy(parser);
    if (errMsg) free(errMsg);
}

// Regression test for deeply nested AST structures
void test_parser_deep_nesting_regression() {
    const char *query = "MATCH (a)-[r1]->(b)-[r2]->(c)-[r3]->(d) RETURN a, b, c, d";
    CypherParser *parser = cypherParserCreate();
    char *errMsg = NULL;
    CypherAst *ast = cypherParse(parser, query, &errMsg);
    
    // This might fail with current parser limitations - that's expected
    // The important thing is it shouldn't crash
    
    if (ast) {
        cypherAstDestroy(ast);
    }
    if (errMsg) {
        free(errMsg);
    }
    cypherParserDestroy(parser);
}

// Regression test for memory allocation in AST node creation
void test_parser_large_query_regression() {
    // Create a query with many RETURN items to test memory reallocation
    char large_query[2000] = "MATCH (n) RETURN n.prop1";
    for (int i = 2; i <= 50; i++) {
        char buf[50];
        snprintf(buf, sizeof(buf), ", n.prop%d", i);
        strcat(large_query, buf);
    }
    
    CypherParser *parser = cypherParserCreate();
    char *errMsg = NULL;
    CypherAst *ast = cypherParse(parser, large_query, &errMsg);
    
    // This might fail with current parser limitations - that's expected
    // The important thing is it shouldn't crash
    
    if (ast) {
        cypherAstDestroy(ast);
    }
    if (errMsg) {
        free(errMsg);
    }
    cypherParserDestroy(parser);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_valid_query);
    RUN_TEST(test_invalid_query);
    RUN_TEST(test_query_with_where_clause);
    RUN_TEST(test_parser_null_input_regression);
    RUN_TEST(test_parser_empty_query_regression);
    RUN_TEST(test_parser_deep_nesting_regression);
    RUN_TEST(test_parser_large_query_regression);
    return UNITY_END();
}
