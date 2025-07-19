/*
* Copyright 2018-2024 Redis Labs Ltd. and Contributors
*
* This file is available under the Redis Source Available License 2.0,
* applying the same terms and conditions as the Redis Source Available License 2.0.
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

#include "cypher/cypher-lexer.h"
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* Global test database - required for extension functions */
sqlite3 *g_pTestDb = 0;

/* Forward declaration of extension init function */
int sqlite3_graph_init(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi);

/* Setup global test database */
void test_setUp(void) {
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

/* Cleanup global test database */
void test_tearDown(void) {
    if (g_pTestDb) {
        sqlite3_close(g_pTestDb);
        g_pTestDb = NULL;
    }
}

void test_simple_keywords() {
    const char *query = "MATCH n RETURN n";
    CypherLexer *lexer = cypherLexerCreate(query);
    CypherToken *token;

    token = cypherLexerNextToken(lexer);
    assert(token->type == CYPHER_TOK_MATCH);

    token = cypherLexerNextToken(lexer);
    assert(token->type == CYPHER_TOK_IDENTIFIER);

    token = cypherLexerNextToken(lexer);
    assert(token->type == CYPHER_TOK_RETURN);

    token = cypherLexerNextToken(lexer);
    assert(token->type == CYPHER_TOK_IDENTIFIER);

    token = cypherLexerNextToken(lexer);
    assert(token->type == CYPHER_TOK_EOF);

    cypherLexerDestroy(lexer);
    printf("test_simple_keywords passed\n");
}

void test_operators() {
    const char *query = "= <> < <= > >= + - * / % ^ . : , ; ( ) [ ] { } -> <- <-> | =~ $";
    CypherLexer *lexer = cypherLexerCreate(query);
    CypherTokenType expected_tokens[] = {
        CYPHER_TOK_EQ, CYPHER_TOK_NE, CYPHER_TOK_LT, CYPHER_TOK_LE, CYPHER_TOK_GT,
        CYPHER_TOK_GE, CYPHER_TOK_PLUS, CYPHER_TOK_MINUS, CYPHER_TOK_MULT, CYPHER_TOK_DIV,
        CYPHER_TOK_MOD, CYPHER_TOK_POW, CYPHER_TOK_DOT, CYPHER_TOK_COLON, CYPHER_TOK_COMMA,
        CYPHER_TOK_SEMICOLON, CYPHER_TOK_LPAREN, CYPHER_TOK_RPAREN, CYPHER_TOK_LBRACKET,
        CYPHER_TOK_RBRACKET, CYPHER_TOK_LBRACE, CYPHER_TOK_RBRACE, CYPHER_TOK_ARROW_RIGHT,
        CYPHER_TOK_ARROW_LEFT, CYPHER_TOK_ARROW_BOTH, CYPHER_TOK_PIPE, CYPHER_TOK_REGEX,
        CYPHER_TOK_DOLLAR, CYPHER_TOK_EOF
    };

    for (int i = 0; i < sizeof(expected_tokens) / sizeof(CypherTokenType); i++) {
        CypherToken *token = cypherLexerNextToken(lexer);
        if (token->type != expected_tokens[i]) {
            printf("Expected token %d, got %d at index %d\n", expected_tokens[i], token->type, i);
            assert(0);
        }
    }

    cypherLexerDestroy(lexer);
    printf("test_operators passed\n");
}

void test_literals() {
    const char *query = "123 45.67 'hello' \"world\" TRUE false";
    CypherLexer *lexer = cypherLexerCreate(query);
    CypherToken *token;

    token = cypherLexerNextToken(lexer);
    assert(token->type == CYPHER_TOK_INTEGER);
    assert(strncmp(token->text, "123", token->len) == 0);

    token = cypherLexerNextToken(lexer);
    assert(token->type == CYPHER_TOK_FLOAT);
    assert(strncmp(token->text, "45.67", token->len) == 0);

    token = cypherLexerNextToken(lexer);
    assert(token->type == CYPHER_TOK_STRING);
    assert(strncmp(token->text, "hello", token->len) == 0);

    token = cypherLexerNextToken(lexer);
    assert(token->type == CYPHER_TOK_STRING);
    assert(strncmp(token->text, "world", token->len) == 0);

    token = cypherLexerNextToken(lexer);
    assert(token->type == CYPHER_TOK_BOOLEAN);
    assert(strncasecmp(token->text, "TRUE", token->len) == 0);

    token = cypherLexerNextToken(lexer);
    assert(token->type == CYPHER_TOK_BOOLEAN);
    assert(strncasecmp(token->text, "false", token->len) == 0);

    token = cypherLexerNextToken(lexer);
    assert(token->type == CYPHER_TOK_EOF);

    cypherLexerDestroy(lexer);
    printf("test_literals passed\n");
}

// Regression test for null dereference in lexer
void test_lexer_null_input_regression() {
    CypherLexer *lexer = cypherLexerCreate(NULL);
    assert(lexer == NULL);
    printf("test_lexer_null_input_regression passed\n");
}

// Regression test for uninitialized token array
void test_lexer_empty_input_regression() {
    const char *query = "";
    CypherLexer *lexer = cypherLexerCreate(query);
    assert(lexer != NULL);
    
    CypherToken *token = cypherLexerNextToken(lexer);
    assert(token->type == CYPHER_TOK_EOF);
    
    cypherLexerDestroy(lexer);
    printf("test_lexer_empty_input_regression passed\n");
}

// Regression test for memory reallocation issues
void test_lexer_long_identifier_regression() {
    // Create a very long identifier to test memory reallocation
    char long_query[1000] = "very";
    for (int i = 4; i < 990; i++) {
        long_query[i] = 'a' + (i % 26);
    }
    long_query[990] = '\0';
    
    CypherLexer *lexer = cypherLexerCreate(long_query);
    assert(lexer != NULL);
    
    CypherToken *token = cypherLexerNextToken(lexer);
    assert(token->type == CYPHER_TOK_IDENTIFIER);
    assert(token->len == 990);
    
    cypherLexerDestroy(lexer);
    printf("test_lexer_long_identifier_regression passed\n");
}

int main() {
    test_setUp();
    
    test_simple_keywords();
    test_operators();
    test_literals();
    test_lexer_null_input_regression();
    test_lexer_empty_input_regression();
    test_lexer_long_identifier_regression();
    
    test_tearDown();
    printf("All lexer tests passed!\n");
    return 0;
}
