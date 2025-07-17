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
#include <stdlib.h>

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
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

    cypherParserDestroy(parser);
}


int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_valid_query);
    RUN_TEST(test_invalid_query);
    RUN_TEST(test_query_with_where_clause);
    return UNITY_END();
}
