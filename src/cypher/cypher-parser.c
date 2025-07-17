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

#include "cypher/cypher-parser.h"
#include "cypher/cypher-lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// Forward declarations for recursive descent parsing functions
static CypherAst *parseQuery(CypherLexer *pLexer, CypherParser *pParser);
static CypherAst *parseSingleQuery(CypherLexer *pLexer, CypherParser *pParser);
static CypherAst *parseMatchClause(CypherLexer *pLexer, CypherParser *pParser);
static CypherAst *parsePatternList(CypherLexer *pLexer, CypherParser *pParser);
static CypherAst *parsePattern(CypherLexer *pLexer, CypherParser *pParser);
static CypherAst *parseNodePattern(CypherLexer *pLexer, CypherParser *pParser);
static CypherAst *parseNodeLabels(CypherLexer *pLexer, CypherParser *pParser);
static CypherAst *parseRelationshipPattern(CypherLexer *pLexer, CypherParser *pParser);
static CypherAst *parseWhereClause(CypherLexer *pLexer, CypherParser *pParser);
static CypherAst *parseReturnClause(CypherLexer *pLexer, CypherParser *pParser);
static CypherAst *parseProjectionList(CypherLexer *pLexer, CypherParser *pParser);
static CypherAst *parseProjectionItem(CypherLexer *pLexer, CypherParser *pParser);
static CypherAst *parseExpression(CypherLexer *pLexer, CypherParser *pParser);
static CypherAst *parseOrExpression(CypherLexer *pLexer, CypherParser *pParser);
static CypherAst *parseAndExpression(CypherLexer *pLexer, CypherParser *pParser);
static CypherAst *parseNotExpression(CypherLexer *pLexer, CypherParser *pParser);
static CypherAst *parseComparisonExpression(CypherLexer *pLexer, CypherParser *pParser);
static CypherAst *parseAdditiveExpression(CypherLexer *pLexer, CypherParser *pParser);
static CypherAst *parseMultiplicativeExpression(CypherLexer *pLexer, CypherParser *pParser);
static CypherAst *parseUnaryExpression(CypherLexer *pLexer, CypherParser *pParser);
static CypherAst *parsePrimaryExpression(CypherLexer *pLexer, CypherParser *pParser);
static CypherAst *parsePropertyExpression(CypherLexer *pLexer, CypherParser *pParser, CypherAst *pExpr);
static CypherAst *parseLiteral(CypherLexer *pLexer, CypherParser *pParser);








static CypherToken *parserPeekToken(CypherLexer *pLexer) {
    // This is a bit of a hack, as the lexer doesn't have a peek function.
    // We can achieve the same effect by getting the next token and then
    // rewinding the lexer's position.
    int pos = pLexer->iPos;
    int line = pLexer->iLine;
    int col = pLexer->iColumn;
    CypherToken *token = cypherLexerNextToken(pLexer);
    pLexer->iPos = pos;
    pLexer->iLine = line;
    pLexer->iColumn = col;
    return token;
}

static CypherToken *parserConsumeToken(CypherLexer *pLexer, CypherTokenType expectedType) {
    CypherToken *token = cypherLexerNextToken(pLexer);
    printf("Consumed token: type=%d, text='%.*s', line=%d, column=%d\n", token->type, token->len, token->text, token->line, token->column);
    if (token->type != expectedType) {
        return NULL;
    }
    return token;
}

CypherParser *cypherParserCreate(void) {
    CypherParser *pParser = (CypherParser *)malloc(sizeof(CypherParser));
    if (!pParser) {
        return NULL;
    }
    
    pParser->pAst = NULL;
    pParser->zErrorMsg = NULL;
    return pParser;
}

void cypherParserDestroy(CypherParser *pParser) {
    if (pParser->pAst) {
        cypherAstDestroy(pParser->pAst);
    }
    if (pParser->zErrorMsg) {
        free(pParser->zErrorMsg);
    }
    free(pParser);
}

static void parserSetError(CypherParser *pParser, CypherLexer *pLexer, const char *zFormat, ...) {
    if (pParser->zErrorMsg) {
        free(pParser->zErrorMsg);
    }
    
    va_list args;
    va_start(args, zFormat);
    
    // Determine required size
    va_list args_copy;
    va_copy(args_copy, args);
    int size = vsnprintf(NULL, 0, zFormat, args_copy);
    va_end(args_copy);

    if (size < 0) {
        va_end(args);
        // Encoding error
        pParser->zErrorMsg = NULL;
        return;
    }

    pParser->zErrorMsg = (char *)malloc(size + 256); // Add extra space for token info
    if (!pParser->zErrorMsg) {
        va_end(args);
        // Allocation failed
        return;
    }

    vsnprintf(pParser->zErrorMsg, size + 1, zFormat, args);
    va_end(args);

    // Append token information
    if (pLexer && pLexer->pLastToken) {
        sprintf(pParser->zErrorMsg + size, " near '%.*s' at line %d column %d", 
                pLexer->pLastToken->len, pLexer->pLastToken->text, 
                pLexer->pLastToken->line, pLexer->pLastToken->column);
    }
}

CypherAst *cypherParse(CypherParser *pParser, const char *zQuery, char **pzErrMsg) {
    CypherLexer *pLexer = cypherLexerCreate(zQuery);
    if (!pLexer) {
        if (pzErrMsg) *pzErrMsg = strdup("Failed to create lexer");
        return NULL;
    }

    pParser->pAst = parseQuery(pLexer, pParser);

    if (pParser->zErrorMsg) {
        if (pzErrMsg) *pzErrMsg = strdup(pParser->zErrorMsg);
    }

    cypherLexerDestroy(pLexer);
    return pParser->pAst;
}

static CypherAst *parseQuery(CypherLexer *pLexer, CypherParser *pParser) {
    CypherAst *pQuery = cypherAstCreate(CYPHER_AST_QUERY, 0, 0);
    CypherAst *pSingleQuery = parseSingleQuery(pLexer, pParser);
    if (!pSingleQuery) {
        cypherAstDestroy(pQuery);
        return NULL;
    }
    cypherAstAddChild(pQuery, pSingleQuery);
    return pQuery;
}

static CypherAst *parseSingleQuery(CypherLexer *pLexer, CypherParser *pParser) {
    CypherAst *pSingleQuery = cypherAstCreate(CYPHER_AST_SINGLE_QUERY, 0, 0);
    CypherAst *pMatchClause = parseMatchClause(pLexer, pParser);
    if (!pMatchClause) {
        cypherAstDestroy(pSingleQuery);
        return NULL;
    }
    cypherAstAddChild(pSingleQuery, pMatchClause);

    CypherToken *pPeek = parserPeekToken(pLexer);
    if (pPeek->type == CYPHER_TOK_WHERE) {
        CypherAst *pWhereClause = parseWhereClause(pLexer, pParser);
        if (pWhereClause) {
            cypherAstAddChild(pSingleQuery, pWhereClause);
        }
    }

    pPeek = parserPeekToken(pLexer);
    if (pPeek->type == CYPHER_TOK_RETURN) {
        CypherAst *pReturnClause = parseReturnClause(pLexer, pParser);
        if (!pReturnClause) {
            cypherAstDestroy(pSingleQuery);
            return NULL;
        }
        cypherAstAddChild(pSingleQuery, pReturnClause);
    }

    // Consume tokens until EOF
    CypherToken *token;
    do {
        token = cypherLexerNextToken(pLexer);
    } while (token->type != CYPHER_TOK_EOF && token->type != CYPHER_TOK_ERROR);

    if (token->type == CYPHER_TOK_ERROR) {
        parserSetError(pParser, pLexer, "Syntax error");
        cypherAstDestroy(pSingleQuery);
        return NULL;
    }

    return pSingleQuery;
}

static CypherAst *parseMatchClause(CypherLexer *pLexer, CypherParser *pParser) {
    if (!parserConsumeToken(pLexer, CYPHER_TOK_MATCH)) {
        parserSetError(pParser, pLexer, "Expected MATCH");
        return NULL;
    }
    CypherAst *pMatchClause = cypherAstCreate(CYPHER_AST_MATCH, 0, 0);
    CypherAst *pPatternList = parsePatternList(pLexer, pParser);
    if (!pPatternList) {
        cypherAstDestroy(pMatchClause);
        return NULL;
    }
    cypherAstAddChild(pMatchClause, pPatternList);
    return pMatchClause;
}

static CypherAst *parsePatternList(CypherLexer *pLexer, CypherParser *pParser) {
    CypherAst *pPatternList = cypherAstCreate(CYPHER_AST_PATTERN, 0, 0);
    CypherAst *pPattern = parsePattern(pLexer, pParser);
    if (!pPattern) {
        cypherAstDestroy(pPatternList);
        return NULL;
    }
    cypherAstAddChild(pPatternList, pPattern);
    return pPatternList;
}

static CypherAst *parsePattern(CypherLexer *pLexer, CypherParser *pParser) {
    CypherAst *pPattern = cypherAstCreate(CYPHER_AST_PATTERN, 0, 0);
    CypherAst *pNodePattern = parseNodePattern(pLexer, pParser);
    if (!pNodePattern) {
        cypherAstDestroy(pPattern);
        return NULL;
    }
    cypherAstAddChild(pPattern, pNodePattern);
    return pPattern;
}

static CypherAst *parseNodePattern(CypherLexer *pLexer, CypherParser *pParser) {
    if (!parserConsumeToken(pLexer, CYPHER_TOK_LPAREN)) {
        parserSetError(pParser, pLexer, "Expected (");
        return NULL;
    }
    CypherAst *pNodePattern = cypherAstCreate(CYPHER_AST_NODE_PATTERN, 0, 0);
    CypherToken *pId = parserConsumeToken(pLexer, CYPHER_TOK_IDENTIFIER);
    if (pId) {
        cypherAstAddChild(pNodePattern, cypherAstCreateIdentifier(pId->text, pId->line, pId->column));
    }

    CypherAst *pLabels = parseNodeLabels(pLexer, pParser);
    if (pLabels) {
        cypherAstAddChild(pNodePattern, pLabels);
    }

    if (!parserConsumeToken(pLexer, CYPHER_TOK_RPAREN)) {
        parserSetError(pParser, pLexer, "Expected )");
        cypherAstDestroy(pNodePattern);
        return NULL;
    }
    return pNodePattern;
}

static CypherAst *parseNodeLabels(CypherLexer *pLexer, CypherParser *pParser) {
    CypherToken *pColon = parserPeekToken(pLexer);
    if (pColon->type != CYPHER_TOK_COLON) {
        return NULL;
    }
    parserConsumeToken(pLexer, CYPHER_TOK_COLON);

    CypherToken *pLabel = parserConsumeToken(pLexer, CYPHER_TOK_IDENTIFIER);
    if (!pLabel) {
        parserSetError(pParser, pLexer, "Expected node label after ':'");
        return NULL;
    }
    return cypherAstCreateNodeLabel(pLabel->text, pLabel->line, pLabel->column);
}

static CypherAst *parseRelationshipPattern(CypherLexer *pLexer, CypherParser *pParser) {
    CypherToken *pArrow = parserConsumeToken(pLexer, CYPHER_TOK_ARROW_RIGHT);
    if (!pArrow) {
        pArrow = parserConsumeToken(pLexer, CYPHER_TOK_ARROW_LEFT);
    }
    if (!pArrow) {
        return NULL;
    }

    CypherAst *pRelPattern = cypherAstCreate(CYPHER_AST_REL_PATTERN, pArrow->line, pArrow->column);
    cypherAstSetValue(pRelPattern, pArrow->text);

    if (!parserConsumeToken(pLexer, CYPHER_TOK_LBRACKET)) {
        parserSetError(pParser, pLexer, "Expected [");
        cypherAstDestroy(pRelPattern);
        return NULL;
    }

    CypherToken *pId = parserConsumeToken(pLexer, CYPHER_TOK_IDENTIFIER);
    if (pId) {
        cypherAstAddChild(pRelPattern, cypherAstCreateIdentifier(pId->text, pId->line, pId->column));
    }

    if (!parserConsumeToken(pLexer, CYPHER_TOK_RBRACKET)) {
        parserSetError(pParser, pLexer, "Expected ]");
        cypherAstDestroy(pRelPattern);
        return NULL;
    }

    return pRelPattern;
}

static CypherAst *parseWhereClause(CypherLexer *pLexer, CypherParser *pParser) {
    if (!parserConsumeToken(pLexer, CYPHER_TOK_WHERE)) {
        return NULL;
    }
    CypherAst *pWhereClause = cypherAstCreate(CYPHER_AST_WHERE, 0, 0);
    CypherAst *pExpr = parseExpression(pLexer, pParser);
    if (!pExpr) {
        cypherAstDestroy(pWhereClause);
        return NULL;
    }
    cypherAstAddChild(pWhereClause, pExpr);
    return pWhereClause;
}

static CypherAst *parseReturnClause(CypherLexer *pLexer, CypherParser *pParser) {
    if (!parserConsumeToken(pLexer, CYPHER_TOK_RETURN)) {
        parserSetError(pParser, pLexer, "Expected RETURN");
        return NULL;
    }
    CypherAst *pReturnClause = cypherAstCreate(CYPHER_AST_RETURN, 0, 0);
    CypherAst *pProjectionList = parseProjectionList(pLexer, pParser);
    if (!pProjectionList) {
        cypherAstDestroy(pReturnClause);
        return NULL;
    }
    cypherAstAddChild(pReturnClause, pProjectionList);
    return pReturnClause;
}

static CypherAst *parseProjectionList(CypherLexer *pLexer, CypherParser *pParser) {
    CypherAst *pProjectionList = cypherAstCreate(CYPHER_AST_PROJECTION_LIST, 0, 0);
    CypherAst *pProjectionItem = parseProjectionItem(pLexer, pParser);
    if (!pProjectionItem) {
        cypherAstDestroy(pProjectionList);
        return NULL;
    }
    cypherAstAddChild(pProjectionList, pProjectionItem);
    return pProjectionList;
}

static CypherAst *parseProjectionItem(CypherLexer *pLexer, CypherParser *pParser) {
    CypherAst *pProjectionItem = cypherAstCreate(CYPHER_AST_PROJECTION_ITEM, 0, 0);
    CypherAst *pExpr = parseExpression(pLexer, pParser);
    if (!pExpr) {
        cypherAstDestroy(pProjectionItem);
        return NULL;
    }
    cypherAstAddChild(pProjectionItem, pExpr);
    return pProjectionItem;
}

static CypherAst *parseExpression(CypherLexer *pLexer, CypherParser *pParser) {
    return parseOrExpression(pLexer, pParser);
}

static CypherAst *parseOrExpression(CypherLexer *pLexer, CypherParser *pParser) {
    CypherAst *pLeft = parseAndExpression(pLexer, pParser);
    if (!pLeft) return NULL;

    while (parserPeekToken(pLexer)->type == CYPHER_TOK_OR) {
        parserConsumeToken(pLexer, CYPHER_TOK_OR);
        CypherAst *pRight = parseAndExpression(pLexer, pParser);
        if (!pRight) {
            cypherAstDestroy(pLeft);
            parserSetError(pParser, pLexer, "Expected expression after OR");
            return NULL;
        }
        CypherAst *pOrExpr = cypherAstCreateBinaryOp("OR", pLeft, pRight, 0, 0);
        cypherAstAddChild(pOrExpr, pLeft);
        cypherAstAddChild(pOrExpr, pRight);
        pLeft = pOrExpr;
    }
    return pLeft;
}

static CypherAst *parseAndExpression(CypherLexer *pLexer, CypherParser *pParser) {
    CypherAst *pLeft = parseNotExpression(pLexer, pParser);
    if (!pLeft) return NULL;

    while (parserPeekToken(pLexer)->type == CYPHER_TOK_AND) {
        parserConsumeToken(pLexer, CYPHER_TOK_AND);
        CypherAst *pRight = parseNotExpression(pLexer, pParser);
        if (!pRight) {
            cypherAstDestroy(pLeft);
            parserSetError(pParser, pLexer, "Expected expression after AND");
            return NULL;
        }
        CypherAst *pAndExpr = cypherAstCreate(CYPHER_AST_AND, 0, 0);
        cypherAstAddChild(pAndExpr, pLeft);
        cypherAstAddChild(pAndExpr, pRight);
        pLeft = pAndExpr;
    }
    return pLeft;
}

static CypherAst *parseNotExpression(CypherLexer *pLexer, CypherParser *pParser) {
    CypherToken *pToken = parserPeekToken(pLexer);
    if (pToken->type == CYPHER_TOK_NOT) {
        parserConsumeToken(pLexer, CYPHER_TOK_NOT);
        CypherAst *pExpr = parseNotExpression(pLexer, pParser);
        if (!pExpr) {
            parserSetError(pParser, pLexer, "Expected expression after NOT");
            return NULL;
        }
        CypherAst *pNotExpr = cypherAstCreate(CYPHER_AST_NOT, 0, 0);
        cypherAstAddChild(pNotExpr, pExpr);
        return pNotExpr;
    }
    return parseComparisonExpression(pLexer, pParser);
}

static CypherAst *parseComparisonExpression(CypherLexer *pLexer, CypherParser *pParser) {
    CypherAst *pLeft = parseAdditiveExpression(pLexer, pParser);
    if (!pLeft) return NULL;

    CypherToken *pToken = parserPeekToken(pLexer);
    while (           pToken->type == CYPHER_TOK_EQ || pToken->type == CYPHER_TOK_NE ||
           pToken->type == CYPHER_TOK_LT || pToken->type == CYPHER_TOK_LE ||
           pToken->type == CYPHER_TOK_GT || pToken->type == CYPHER_TOK_GE) {
        parserConsumeToken(pLexer, pToken->type);
        CypherAst *pRight = parseAdditiveExpression(pLexer, pParser);
        if (!pRight) {
            cypherAstDestroy(pLeft);
            parserSetError(pParser, pLexer, "Expected expression after comparison operator");
            return NULL;
        }
        CypherAst *pCompExpr = cypherAstCreate(CYPHER_AST_COMPARISON, 0, 0);
        cypherAstSetValue(pCompExpr, pToken->text);
        cypherAstAddChild(pCompExpr, pLeft);
        cypherAstAddChild(pCompExpr, pRight);
        pLeft = pCompExpr;
        pToken = parserPeekToken(pLexer);
    }
    return pLeft;
}

static CypherAst *parseAdditiveExpression(CypherLexer *pLexer, CypherParser *pParser) {
    CypherAst *pLeft = parseMultiplicativeExpression(pLexer, pParser);
    if (!pLeft) return NULL;

    CypherToken *pToken = parserPeekToken(pLexer);
    while (pToken->type == CYPHER_TOK_PLUS || pToken->type == CYPHER_TOK_MINUS) {
        parserConsumeToken(pLexer, pToken->type);
        CypherAst *pRight = parseMultiplicativeExpression(pLexer, pParser);
        if (!pRight) {
            cypherAstDestroy(pLeft);
            parserSetError(pParser, pLexer, "Expected expression after additive operator");
            return NULL;
        }
        CypherAst *pAddExpr = cypherAstCreate(CYPHER_AST_ADDITIVE, 0, 0);
        cypherAstSetValue(pAddExpr, pToken->text);
        cypherAstAddChild(pAddExpr, pLeft);
        cypherAstAddChild(pAddExpr, pRight);
        pLeft = pAddExpr;
        pToken = parserPeekToken(pLexer);
    }
    return pLeft;
}

static CypherAst *parseMultiplicativeExpression(CypherLexer *pLexer, CypherParser *pParser) {
    CypherAst *pLeft = parseUnaryExpression(pLexer, pParser);
    if (!pLeft) return NULL;

    CypherToken *pToken = parserPeekToken(pLexer);
    while (pToken->type == CYPHER_TOK_MULT || pToken->type == CYPHER_TOK_DIV || pToken->type == CYPHER_TOK_MOD) {
        parserConsumeToken(pLexer, pToken->type);
        CypherAst *pRight = parseUnaryExpression(pLexer, pParser);
        if (!pRight) {
            cypherAstDestroy(pLeft);
            parserSetError(pParser, pLexer, "Expected expression after multiplicative operator");
            return NULL;
        }
        CypherAst *pMulExpr = cypherAstCreate(CYPHER_AST_MULTIPLICATIVE, 0, 0);
        cypherAstSetValue(pMulExpr, pToken->text);
        cypherAstAddChild(pMulExpr, pLeft);
        cypherAstAddChild(pMulExpr, pRight);
        pLeft = pMulExpr;
        pToken = parserPeekToken(pLexer);
    }
    return pLeft;
}

static CypherAst *parseUnaryExpression(CypherLexer *pLexer, CypherParser *pParser) {
    CypherToken *pToken = parserPeekToken(pLexer);
    if (pToken->type == CYPHER_TOK_PLUS || pToken->type == CYPHER_TOK_MINUS) {
        parserConsumeToken(pLexer, pToken->type);
        CypherAst *pExpr = parseUnaryExpression(pLexer, pParser);
        if (!pExpr) {
            parserSetError(pParser, pLexer, "Expected expression after unary operator");
            return NULL;
        }
        CypherAst *pUnaryExpr = cypherAstCreateUnaryOp(pToken->text, pExpr, 0, 0);
        cypherAstSetValue(pUnaryExpr, pToken->text);
        cypherAstAddChild(pUnaryExpr, pExpr);
        return pUnaryExpr;
    }
    return parsePrimaryExpression(pLexer, pParser);
}

static CypherAst *parsePrimaryExpression(CypherLexer *pLexer, CypherParser *pParser) {
    CypherAst *pExpr = parseLiteral(pLexer, pParser);
    if (pExpr) {
        return parsePropertyExpression(pLexer, pParser, pExpr);
    }
    return NULL;
}

static CypherAst *parsePropertyExpression(CypherLexer *pLexer, CypherParser *pParser, CypherAst *pExpr) {
    CypherToken *pToken = parserPeekToken(pLexer);
    while (pToken->type == CYPHER_TOK_DOT) {
        parserConsumeToken(pLexer, CYPHER_TOK_DOT);
        CypherToken *pProperty = parserConsumeToken(pLexer, CYPHER_TOK_IDENTIFIER);
        if (!pProperty) {
            cypherAstDestroy(pExpr);
            parserSetError(pParser, pLexer, "Expected property name after '.'");
            return NULL;
        }
        CypherAst *pPropExpr = cypherAstCreate(CYPHER_AST_PROPERTY, 0, 0);
        cypherAstAddChild(pPropExpr, pExpr);
        cypherAstAddChild(pPropExpr, cypherAstCreateIdentifier(pProperty->text, pProperty->line, pProperty->column));
        pExpr = pPropExpr;
        pToken = parserPeekToken(pLexer);
    }
    return pExpr;
}

static CypherAst *parseLiteral(CypherLexer *pLexer, CypherParser *pParser) {
    CypherToken *pToken = parserPeekToken(pLexer);
    if (pToken->type == CYPHER_TOK_IDENTIFIER || pToken->type == CYPHER_TOK_INTEGER || pToken->type == CYPHER_TOK_FLOAT || pToken->type == CYPHER_TOK_STRING || pToken->type == CYPHER_TOK_BOOLEAN || pToken->type == CYPHER_TOK_NULL) {
        pToken = cypherLexerNextToken(pLexer);
        return cypherAstCreateLiteral(pToken->text, pToken->line, pToken->column);
    }
    return NULL;
}