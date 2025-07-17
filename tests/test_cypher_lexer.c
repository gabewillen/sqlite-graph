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
#include <stdio.h>
#include <string.h>
#include <assert.h>

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
        assert(token->type == expected_tokens[i]);
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

int main() {
    test_simple_keywords();
    test_operators();
    test_literals();
    return 0;
}
