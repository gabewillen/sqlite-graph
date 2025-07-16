#include "cypher.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

// Helper function to allocate memory and handle errors
static void *cypher_malloc(size_t size) {
    void *ptr = sqlite3_malloc(size);
    if (!ptr) {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

// Helper function to free memory
static void cypher_free(void *ptr) {
    sqlite3_free(ptr);
}

// Parser Functions
CypherParser *cypherParserCreate(void) {
    CypherParser *pParser = cypher_malloc(sizeof(CypherParser));
    pParser->zError = NULL;
    return pParser;
}

void cypherParserDestroy(CypherParser *pParser) {
    if (pParser) {
        if (pParser->zError) {
            cypher_free(pParser->zError);
        }
        cypher_free(pParser);
    }
}

static void parserSetError(CypherParser *pParser, CypherLexer *pLexer, const char *zFormat, ...) {
    va_list args;
    va_start(args, zFormat);
    // Determine size needed
    int size = vsnprintf(NULL, 0, zFormat, args);
    va_end(args);

    pParser->zError = cypher_malloc(size + 1);
    va_start(args, zFormat);
    vsnprintf(pParser->zError, size + 1, zFormat, args);
    va_end(args);

    // Also set lexer error to stop further tokenization
    if (pLexer->zErrorMsg) {
        cypher_free(pLexer->zErrorMsg);
    }
    pLexer->zErrorMsg = cypher_malloc(size + 1);
    strcpy(pLexer->zErrorMsg, pParser->zError);
}

static CypherToken *parserPeekToken(CypherLexer *pLexer) {
    // The lexer's pLastToken is the last token returned, so we need to get the next one
    // This is a simplification; a real parser would have a lookahead buffer
    // For now, we'll just call nextToken and store it.
    if (!pLexer->pLastToken || pLexer->pLastToken->type == CYPHER_TOK_EOF) {
        pLexer->pLastToken = cypherLexerNextToken(pLexer);
    }
    return pLexer->pLastToken;
}

static CypherToken *parserConsumeToken(CypherLexer *pLexer, CypherTokenType expectedType, CypherParser *pParser) {
    CypherToken *token = cypherLexerNextToken(pLexer);
    if (token == NULL) { // Lexer error
        parserSetError(pParser, pLexer, "Lexer error: %s", pLexer->zErrorMsg);
        return NULL;
    }
    if (token->type != expectedType) {
        parserSetError(pParser, pLexer, "Expected token %s but got %s at line %d, column %d",
                       cypherTokenTypeName(expectedType), cypherTokenTypeName(token->type), token->iLine, token->iColumn);
        return NULL;
    }
    return token;
}

// Recursive Descent Parsing Functions (forward declarations)
static CypherAst *parseQuery(CypherLexer *pLexer, CypherParser *pParser);
static CypherAst *parseSingleQuery(CypherLexer *pLexer, CypherParser *pParser);
static CypherAst *parseReadingClauseList(CypherLexer *pLexer, CypherParser *pParser);
static CypherAst *parseReadingClause(CypherLexer *pLexer, CypherParser *pParser);
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
static CypherAst *parseOrderByClause(CypherLexer *pLexer, CypherParser *pParser);
static CypherAst *parseSortList(CypherLexer *pLexer, CypherParser *pParser);
static CypherAst *parseSortItem(CypherLexer *pLexer, CypherParser *pParser);
static CypherAst *parseLimitClause(CypherLexer *pLexer, CypherParser *pParser);
static CypherAst *parseSkipClause(CypherLexer *pLexer, CypherParser *pParser);
static CypherAst *parseExpression(CypherLexer *pLexer, CypherParser *pParser);
static CypherAst *parseOrExpression(CypherLexer *pLexer, CypherParser *pParser);
static CypherAst *parseAndExpression(CypherLexer *pLexer, CypherParser *pParser);
static CypherAst *parseEqualityExpression(CypherLexer *pLexer, CypherParser *pParser);
static CypherAst *parseRelationalExpression(CypherLexer *pLexer, CypherParser *pParser);
static CypherAst *parseAdditiveExpression(CypherLexer *pLexer, CypherParser *pParser);
static CypherAst *parseMultiplicativeExpression(CypherLexer *pLexer, CypherParser *pParser);
static CypherAst *parseUnaryExpression(CypherLexer *pLexer, CypherParser *pParser);
static CypherAst *parsePrimaryExpression(CypherLexer *pLexer, CypherParser *pParser);
static CypherAst *parsePropertyExpression(CypherLexer *pLexer, CypherParser *pParser);
static CypherAst *parseLiteral(CypherLexer *pLexer, CypherParser *pParser);

// Implementations of parsing functions

static CypherAst *parseQuery(CypherLexer *pLexer, CypherParser *pParser) {
    CypherAst *queryAst = cypherAstCreate(CYPHER_AST_QUERY, pLexer->iLine, pLexer->iColumn);
    if (!queryAst) return NULL;

    CypherAst *singleQuery = parseSingleQuery(pLexer, pParser);
    if (!singleQuery) {
        cypherAstDestroy(queryAst);
        return NULL;
    }
    cypherAstAddChild(queryAst, singleQuery);

    // Handle UNION
    while (parserPeekToken(pLexer)->type == CYPHER_TOK_UNION) {
        parserConsumeToken(pLexer, CYPHER_TOK_UNION, pParser); // Consume UNION
        if (pParser->zError) { cypherAstDestroy(queryAst); return NULL; }

        CypherAst *nextSingleQuery = parseSingleQuery(pLexer, pParser);
        if (!nextSingleQuery) {
            cypherAstDestroy(queryAst);
            return NULL;
        }
        // If we have multiple UNIONs, we need to wrap them in a UNION AST node
        if (queryAst->type != CYPHER_AST_UNION) {
            CypherAst *unionAst = cypherAstCreate(CYPHER_AST_UNION, queryAst->iLine, queryAst->iColumn);
            if (!unionAst) { cypherAstDestroy(queryAst); return NULL; }
            cypherAstAddChild(unionAst, queryAst);
            queryAst = unionAst;
        }
        cypherAstAddChild(queryAst, nextSingleQuery);
    }

    return queryAst;
}

static CypherAst *parseSingleQuery(CypherLexer *pLexer, CypherParser *pParser) {
    CypherAst *singleQueryAst = cypherAstCreate(CYPHER_AST_SINGLE_QUERY, pLexer->iLine, pLexer->iColumn);
    if (!singleQueryAst) return NULL;

    CypherAst *readingClauses = parseReadingClauseList(pLexer, pParser);
    if (!readingClauses) { cypherAstDestroy(singleQueryAst); return NULL; }
    cypherAstAddChild(singleQueryAst, readingClauses);

    CypherAst *returnClause = parseReturnClause(pLexer, pParser);
    if (!returnClause) { cypherAstDestroy(singleQueryAst); return NULL; }
    cypherAstAddChild(singleQueryAst, returnClause);

    // Optional clauses
    if (parserPeekToken(pLexer)->type == CYPHER_TOK_ORDER) {
        CypherAst *orderByClause = parseOrderByClause(pLexer, pParser);
        if (!orderByClause) { cypherAstDestroy(singleQueryAst); return NULL; }
        cypherAstAddChild(singleQueryAst, orderByClause);
    }

    if (parserPeekToken(pLexer)->type == CYPHER_TOK_LIMIT) {
        CypherAst *limitClause = parseLimitClause(pLexer, pParser);
        if (!limitClause) { cypherAstDestroy(singleQueryAst); return NULL; }
        cypherAstAddChild(singleQueryAst, limitClause);
    }

    if (parserPeekToken(pLexer)->type == CYPHER_TOK_SKIP) {
        CypherAst *skipClause = parseSkipClause(pLexer, pParser);
        if (!skipClause) { cypherAstDestroy(singleQueryAst); return NULL; }
        cypherAstAddChild(singleQueryAst, skipClause);
    }

    return singleQueryAst;
}

static CypherAst *parseReadingClauseList(CypherLexer *pLexer, CypherParser *pParser) {
    CypherAst *list = NULL;
    CypherAst *clause = NULL;

    while (1) {
        CypherTokenType nextType = parserPeekToken(pLexer)->type;
        if (nextType == CYPHER_TOK_MATCH || nextType == CYPHER_TOK_OPTIONAL || nextType == CYPHER_TOK_WHERE) {
            clause = parseReadingClause(pLexer, pParser);
            if (!clause) { cypherAstDestroy(list); return NULL; }
            if (!list) {
                list = clause;
            } else {
                cypherAstAddChild(list, clause);
            }
        } else {
            break;
        }
    }
    return list;
}

static CypherAst *parseReadingClause(CypherLexer *pLexer, CypherParser *pParser) {
    CypherTokenType nextType = parserPeekToken(pLexer)->type;
    if (nextType == CYPHER_TOK_MATCH || nextType == CYPHER_TOK_OPTIONAL) {
        return parseMatchClause(pLexer, pParser);
    } else if (nextType == CYPHER_TOK_WHERE) {
        return parseWhereClause(pLexer, pParser);
    } else {
        parserSetError(pParser, pLexer, "Expected a reading clause (MATCH, OPTIONAL MATCH, WHERE) but got %s at line %d, column %d",
                       cypherTokenTypeName(nextType), pLexer->iLine, pLexer->iColumn);
        return NULL;
    }
}

static CypherAst *parseMatchClause(CypherLexer *pLexer, CypherParser *pParser) {
    CypherToken *token = parserPeekToken(pLexer);
    CypherAstNodeType astType = CYPHER_AST_MATCH;

    if (token->type == CYPHER_TOK_OPTIONAL) {
        parserConsumeToken(pLexer, CYPHER_TOK_OPTIONAL, pParser);
        if (pParser->zError) return NULL;
        astType = CYPHER_AST_OPTIONAL_MATCH;
    }

    token = parserConsumeToken(pLexer, CYPHER_TOK_MATCH, pParser);
    if (pParser->zError) return NULL;

    CypherAst *matchAst = cypherAstCreate(astType, token->iLine, token->iColumn);
    if (!matchAst) return NULL;

    CypherAst *patternList = parsePatternList(pLexer, pParser);
    if (!patternList) { cypherAstDestroy(matchAst); return NULL; }
    cypherAstAddChild(matchAst, patternList);

    return matchAst;
}

static CypherAst *parsePatternList(CypherLexer *pLexer, CypherParser *pParser) {
    CypherAst *list = NULL;
    CypherAst *pattern = NULL;

    pattern = parsePattern(pLexer, pParser);
    if (!pattern) return NULL;
    list = pattern;

    while (parserPeekToken(pLexer)->type == CYPHER_TOK_COMMA) {
        parserConsumeToken(pLexer, CYPHER_TOK_COMMA, pParser);
        if (pParser->zError) { cypherAstDestroy(list); return NULL; }

        pattern = parsePattern(pLexer, pParser);
        if (!pattern) { cypherAstDestroy(list); return NULL; }
        cypherAstAddChild(list, pattern);
    }
    return list;
}

static CypherAst *parsePattern(CypherLexer *pLexer, CypherParser *pParser) {
    CypherAst *nodePattern1 = parseNodePattern(pLexer, pParser);
    if (!nodePattern1) return NULL;

    if (parserPeekToken(pLexer)->type == CYPHER_TOK_DASH || parserPeekToken(pLexer)->type == CYPHER_TOK_ARROW_LEFT) {
        CypherAst *relPattern = parseRelationshipPattern(pLexer, pParser);
        if (!relPattern) { cypherAstDestroy(nodePattern1); return NULL; }

        CypherAst *nodePattern2 = parseNodePattern(pLexer, pParser);
        if (!nodePattern2) { cypherAstDestroy(nodePattern1); cypherAstDestroy(relPattern); return NULL; }

        CypherAst *pathAst = cypherAstCreate(CYPHER_AST_PATH, nodePattern1->iLine, nodePattern1->iColumn);
        if (!pathAst) { cypherAstDestroy(nodePattern1); cypherAstDestroy(relPattern); cypherAstDestroy(nodePattern2); return NULL; }
        cypherAstAddChild(pathAst, nodePattern1);
        cypherAstAddChild(pathAst, relPattern);
        cypherAstAddChild(pathAst, nodePattern2);
        return pathAst;
    } else {
        return nodePattern1;
    }
}

static CypherAst *parseNodePattern(CypherLexer *pLexer, CypherParser *pParser) {
    CypherToken *token = parserConsumeToken(pLexer, CYPHER_TOK_LPAREN, pParser);
    if (pParser->zError) return NULL;

    CypherAst *nodePatternAst = cypherAstCreate(CYPHER_AST_NODE_PATTERN, token->iLine, token->iColumn);
    if (!nodePatternAst) return NULL;

    CypherToken *idToken = parserPeekToken(pLexer);
    if (idToken->type == CYPHER_TOK_IDENTIFIER) {
        token = parserConsumeToken(pLexer, CYPHER_TOK_IDENTIFIER, pParser);
        if (pParser->zError) { cypherAstDestroy(nodePatternAst); return NULL; }
        cypherAstAddChild(nodePatternAst, cypherAstCreateIdentifier(token->zValue, token->iLine, token->iColumn));
    }

    if (parserPeekToken(pLexer)->type == CYPHER_TOK_COLON) {
        CypherAst *nodeLabels = parseNodeLabels(pLexer, pParser);
        if (!nodeLabels) { cypherAstDestroy(nodePatternAst); return NULL; }
        cypherAstAddChild(nodePatternAst, nodeLabels);
    }

    token = parserConsumeToken(pLexer, CYPHER_TOK_RPAREN, pParser);
    if (pParser->zError) { cypherAstDestroy(nodePatternAst); return NULL; }

    return nodePatternAst;
}

static CypherAst *parseNodeLabels(CypherLexer *pLexer, CypherParser *pParser) {
    CypherAst *labelsAst = cypherAstCreate(CYPHER_AST_LABELS, pLexer->iLine, pLexer->iColumn);
    if (!labelsAst) return NULL;

    while (parserPeekToken(pLexer)->type == CYPHER_TOK_COLON) {
        parserConsumeToken(pLexer, CYPHER_TOK_COLON, pParser);
        if (pParser->zError) { cypherAstDestroy(labelsAst); return NULL; }

        CypherToken *idToken = parserConsumeToken(pLexer, CYPHER_TOK_IDENTIFIER, pParser);
        if (pParser->zError) { cypherAstDestroy(labelsAst); return NULL; }
        cypherAstAddChild(labelsAst, cypherAstCreateIdentifier(idToken->zValue, idToken->iLine, idToken->iColumn));
    }
    return labelsAst;
}

static CypherAst *parseRelationshipPattern(CypherLexer *pLexer, CypherParser *pParser) {
    CypherToken *token = parserPeekToken(pLexer);
    CypherAst *relAst = NULL;

    // Handle left arrow if present
    if (token->type == CYPHER_TOK_ARROW_LEFT) {
        token = parserConsumeToken(pLexer, CYPHER_TOK_ARROW_LEFT, pParser);
        if (pParser->zError) return NULL;
        relAst = cypherAstCreate(CYPHER_AST_REL_PATTERN, token->iLine, token->iColumn);
        if (!relAst) return NULL;
        cypherAstSetValue(relAst, "<-");
    }

    token = parserConsumeToken(pLexer, CYPHER_TOK_DASH, pParser);
    if (pParser->zError) { cypherAstDestroy(relAst); return NULL; }
    if (!relAst) { // If no left arrow, create it now
        relAst = cypherAstCreate(CYPHER_AST_REL_PATTERN, token->iLine, token->iColumn);
        if (!relAst) return NULL;
        cypherAstSetValue(relAst, "--");
    }

    if (parserPeekToken(pLexer)->type == CYPHER_TOK_LBRACKET) {
        parserConsumeToken(pLexer, CYPHER_TOK_LBRACKET, pParser);
        if (pParser->zError) { cypherAstDestroy(relAst); return NULL; }

        CypherToken *idToken = parserPeekToken(pLexer);
        if (idToken->type == CYPHER_TOK_IDENTIFIER) {
            token = parserConsumeToken(pLexer, CYPHER_TOK_IDENTIFIER, pParser);
            if (pParser->zError) { cypherAstDestroy(relAst); return NULL; }
            cypherAstAddChild(relAst, cypherAstCreateIdentifier(token->zValue, token->iLine, token->iColumn));
        }

        if (parserPeekToken(pLexer)->type == CYPHER_TOK_COLON) {
            parserConsumeToken(pLexer, CYPHER_TOK_COLON, pParser);
            if (pParser->zError) { cypherAstDestroy(relAst); return NULL; }

            CypherToken *typeToken = parserConsumeToken(pLexer, CYPHER_TOK_IDENTIFIER, pParser);
            if (pParser->zError) { cypherAstDestroy(relAst); return NULL; }
            cypherAstAddChild(relAst, cypherAstCreateIdentifier(typeToken->zValue, typeToken->iLine, typeToken->iColumn));
        }

        parserConsumeToken(pLexer, CYPHER_TOK_RBRACKET, pParser);
        if (pParser->zError) { cypherAstDestroy(relAst); return NULL; }
    }

    // Handle right arrow if present
    if (parserPeekToken(pLexer)->type == CYPHER_TOK_ARROW_RIGHT) {
        token = parserConsumeToken(pLexer, CYPHER_TOK_ARROW_RIGHT, pParser);
        if (pParser->zError) { cypherAstDestroy(relAst); return NULL; }
        if (strcmp(cypherAstGetValue(relAst), "<-") == 0) {
            cypherAstSetValue(relAst, "<-"); // Should be <->
        } else {
            cypherAstSetValue(relAst, "->");
        }
    } else if (strcmp(cypherAstGetValue(relAst), "<-") == 0) {
        // If it was only a left arrow, keep it as such
    } else {
        // If no arrows, it's just a dash
        cypherAstSetValue(relAst, "--");
    }

    return relAst;
}

static CypherAst *parseWhereClause(CypherLexer *pLexer, CypherParser *pParser) {
    CypherToken *token = parserConsumeToken(pLexer, CYPHER_TOK_WHERE, pParser);
    if (pParser->zError) return NULL;

    CypherAst *whereAst = cypherAstCreate(CYPHER_AST_WHERE, token->iLine, token->iColumn);
    if (!whereAst) return NULL;

    CypherAst *expression = parseExpression(pLexer, pParser);
    if (!expression) { cypherAstDestroy(whereAst); return NULL; }
    cypherAstAddChild(whereAst, expression);

    return whereAst;
}

static CypherAst *parseReturnClause(CypherLexer *pLexer, CypherParser *pParser) {
    CypherToken *token = parserConsumeToken(pLexer, CYPHER_TOK_RETURN, pParser);
    if (pParser->zError) return NULL;

    CypherAst *returnAst = cypherAstCreate(CYPHER_AST_RETURN, token->iLine, token->iColumn);
    if (!returnAst) return NULL;

    if (parserPeekToken(pLexer)->type == CYPHER_TOK_DISTINCT) {
        parserConsumeToken(pLexer, CYPHER_TOK_DISTINCT, pParser);
        if (pParser->zError) { cypherAstDestroy(returnAst); return NULL; }
        returnAst->iFlags |= 1; // Set DISTINCT flag
    }

    CypherAst *projectionList = parseProjectionList(pLexer, pParser);
    if (!projectionList) { cypherAstDestroy(returnAst); return NULL; }
    cypherAstAddChild(returnAst, projectionList);

    return returnAst;
}

static CypherAst *parseProjectionList(CypherLexer *pLexer, CypherParser *pParser) {
    CypherAst *list = cypherAstCreate(CYPHER_AST_PROJECTION_LIST, pLexer->iLine, pLexer->iColumn);
    if (!list) return NULL;

    CypherAst *item = parseProjectionItem(pLexer, pParser);
    if (!item) { cypherAstDestroy(list); return NULL; }
    cypherAstAddChild(list, item);

    while (parserPeekToken(pLexer)->type == CYPHER_TOK_COMMA) {
        parserConsumeToken(pLexer, CYPHER_TOK_COMMA, pParser);
        if (pParser->zError) { cypherAstDestroy(list); return NULL; }

        item = parseProjectionItem(pLexer, pParser);
        if (!item) { cypherAstDestroy(list); return NULL; }
        cypherAstAddChild(list, item);
    }
    return list;
}

static CypherAst *parseProjectionItem(CypherLexer *pLexer, CypherParser *pParser) {
    CypherAst *expression = parseExpression(pLexer, pParser);
    if (!expression) return NULL;

    CypherAst *itemAst = cypherAstCreate(CYPHER_AST_PROJECTION_ITEM, expression->iLine, expression->iColumn);
    if (!itemAst) { cypherAstDestroy(expression); return NULL; }
    cypherAstAddChild(itemAst, expression);

    if (parserPeekToken(pLexer)->type == CYPHER_TOK_AS) {
        parserConsumeToken(pLexer, CYPHER_TOK_AS, pParser);
        if (pParser->zError) { cypherAstDestroy(itemAst); return NULL; }

        CypherToken *idToken = parserConsumeToken(pLexer, CYPHER_TOK_IDENTIFIER, pParser);
        if (pParser->zError) { cypherAstDestroy(itemAst); return NULL; }
        cypherAstAddChild(itemAst, cypherAstCreateIdentifier(idToken->zValue, idToken->iLine, idToken->iColumn));
    }
    return itemAst;
}

static CypherAst *parseOrderByClause(CypherLexer *pLexer, CypherParser *pParser) {
    CypherToken *token = parserConsumeToken(pLexer, CYPHER_TOK_ORDER, pParser);
    if (pParser->zError) return NULL;
    token = parserConsumeToken(pLexer, CYPHER_TOK_BY, pParser);
    if (pParser->zError) return NULL;

    CypherAst *orderByAst = cypherAstCreate(CYPHER_AST_ORDER_BY, token->iLine, token->iColumn);
    if (!orderByAst) return NULL;

    CypherAst *sortList = parseSortList(pLexer, pParser);
    if (!sortList) { cypherAstDestroy(orderByAst); return NULL; }
    cypherAstAddChild(orderByAst, sortList);

    return orderByAst;
}

static CypherAst *parseSortList(CypherLexer *pLexer, CypherParser *pParser) {
    CypherAst *list = cypherAstCreate(CYPHER_AST_SORT_LIST, pLexer->iLine, pLexer->iColumn);
    if (!list) return NULL;

    CypherAst *item = parseSortItem(pLexer, pParser);
    if (!item) { cypherAstDestroy(list); return NULL; }
    cypherAstAddChild(list, item);

    while (parserPeekToken(pLexer)->type == CYPHER_TOK_COMMA) {
        parserConsumeToken(pLexer, CYPHER_TOK_COMMA, pParser);
        if (pParser->zError) { cypherAstDestroy(list); return NULL; }

        item = parseSortItem(pLexer, pParser);
        if (!item) { cypherAstDestroy(list); return NULL; }
        cypherAstAddChild(list, item);
    }
    return list;
}

static CypherAst *parseSortItem(CypherLexer *pLexer, CypherParser *pParser) {
    CypherAst *expression = parseExpression(pLexer, pParser);
    if (!expression) return NULL;

    CypherAst *sortItemAst = cypherAstCreate(CYPHER_AST_SORT_ITEM, expression->iLine, expression->iColumn);
    if (!sortItemAst) { cypherAstDestroy(expression); return NULL; }
    cypherAstAddChild(sortItemAst, expression);

    CypherTokenType nextType = parserPeekToken(pLexer)->type;
    if (nextType == CYPHER_TOK_ASC) {
        parserConsumeToken(pLexer, CYPHER_TOK_ASC, pParser);
        if (pParser->zError) { cypherAstDestroy(sortItemAst); return NULL; }
        cypherAstSetValue(sortItemAst, "ASC");
    } else if (nextType == CYPHER_TOK_DESC) {
        parserConsumeToken(pLexer, CYPHER_TOK_DESC, pParser);
        if (pParser->zError) { cypherAstDestroy(sortItemAst); return NULL; }
        cypherAstSetValue(sortItemAst, "DESC");
    }
    return sortItemAst;
}

static CypherAst *parseLimitClause(CypherLexer *pLexer, CypherParser *pParser) {
    CypherToken *token = parserConsumeToken(pLexer, CYPHER_TOK_LIMIT, pParser);
    if (pParser->zError) return NULL;

    CypherAst *limitAst = cypherAstCreate(CYPHER_AST_LIMIT, token->iLine, token->iColumn);
    if (!limitAst) return NULL;

    CypherToken *intToken = parserConsumeToken(pLexer, CYPHER_TOK_INTEGER, pParser);
    if (pParser->zError) { cypherAstDestroy(limitAst); return NULL; }
    cypherAstAddChild(limitAst, cypherAstCreateLiteral(intToken->zValue, intToken->iLine, intToken->iColumn));

    return limitAst;
}

static CypherAst *parseSkipClause(CypherLexer *pLexer, CypherParser *pParser) {
    CypherToken *token = parserConsumeToken(pLexer, CYPHER_TOK_SKIP, pParser);
    if (pParser->zError) return NULL;

    CypherAst *skipAst = cypherAstCreate(CYPHER_AST_SKIP, token->iLine, token->iColumn);
    if (!skipAst) return NULL;

    CypherToken *intToken = parserConsumeToken(pLexer, CYPHER_TOK_INTEGER, pParser);
    if (pParser->zError) { cypherAstDestroy(skipAst); return NULL; }
    cypherAstAddChild(skipAst, cypherAstCreateLiteral(intToken->zValue, intToken->iLine, intToken->iColumn));

    return skipAst;
}

static CypherAst *parseExpression(CypherLexer *pLexer, CypherParser *pParser) {
    return parseOrExpression(pLexer, pParser);
}

static CypherAst *parseOrExpression(CypherLexer *pLexer, CypherParser *pParser) {
    CypherAst *left = parseAndExpression(pLexer, pParser);
    if (!left) return NULL;

    while (parserPeekToken(pLexer)->type == CYPHER_TOK_OR) {
        CypherToken *opToken = parserConsumeToken(pLexer, CYPHER_TOK_OR, pParser);
        if (pParser->zError) { cypherAstDestroy(left); return NULL; }

        CypherAst *right = parseAndExpression(pLexer, pParser);
        if (!right) { cypherAstDestroy(left); return NULL; }

        left = cypherAstCreateBinaryOp(opToken->zValue, left, right, opToken->iLine, opToken->iColumn);
        if (!left) { cypherAstDestroy(right); return NULL; }
    }
    return left;
}

static CypherAst *parseAndExpression(CypherLexer *pLexer, CypherParser *pParser) {
    CypherAst *left = parseEqualityExpression(pLexer, pParser);
    if (!left) return NULL;

    while (parserPeekToken(pLexer)->type == CYPHER_TOK_AND) {
        CypherToken *opToken = parserConsumeToken(pLexer, CYPHER_TOK_AND, pParser);
        if (pParser->zError) { cypherAstDestroy(left); return NULL; }

        CypherAst *right = parseEqualityExpression(pLexer, pParser);
        if (!right) { cypherAstDestroy(left); return NULL; }

        left = cypherAstCreateBinaryOp(opToken->zValue, left, right, opToken->iLine, opToken->iColumn);
        if (!left) { cypherAstDestroy(right); return NULL; }
    }
    return left;
}

static CypherAst *parseEqualityExpression(CypherLexer *pLexer, CypherParser *pParser) {
    CypherAst *left = parseRelationalExpression(pLexer, pParser);
    if (!left) return NULL;

    while (parserPeekToken(pLexer)->type == CYPHER_TOK_EQ || parserPeekToken(pLexer)->type == CYPHER_TOK_NE) {
        CypherToken *opToken = cypherLexerNextToken(pLexer);
        if (pParser->zError) { cypherAstDestroy(left); return NULL; }

        CypherAst *right = parseRelationalExpression(pLexer, pParser);
        if (!right) { cypherAstDestroy(left); return NULL; }

        left = cypherAstCreateBinaryOp(opToken->zValue, left, right, opToken->iLine, opToken->iColumn);
        if (!left) { cypherAstDestroy(right); return NULL; }
    }
    return left;
}

static CypherAst *parseRelationalExpression(CypherLexer *pLexer, CypherParser *pParser) {
    CypherAst *left = parseAdditiveExpression(pLexer, pParser);
    if (!left) return NULL;

    while (parserPeekToken(pLexer)->type == CYPHER_TOK_LT ||
           parserPeekToken(pLexer)->type == CYPHER_TOK_LE ||
           parserPeekToken(pLexer)->type == CYPHER_TOK_GT ||
           parserPeekToken(pLexer)->type == CYPHER_TOK_GE) {
        CypherToken *opToken = cypherLexerNextToken(pLexer);
        if (pParser->zError) { cypherAstDestroy(left); return NULL; }

        CypherAst *right = parseAdditiveExpression(pLexer, pParser);
        if (!right) { cypherAstDestroy(left); return NULL; }

        left = cypherAstCreateBinaryOp(opToken->zValue, left, right, opToken->iLine, opToken->iColumn);
        if (!left) { cypherAstDestroy(right); return NULL; }
    }
    return left;
}

static CypherAst *parseAdditiveExpression(CypherLexer *pLexer, CypherParser *pParser) {
    CypherAst *left = parseMultiplicativeExpression(pLexer, pParser);
    if (!left) return NULL;

    while (parserPeekToken(pLexer)->type == CYPHER_TOK_PLUS || parserPeekToken(pLexer)->type == CYPHER_TOK_MINUS) {
        CypherToken *opToken = cypherLexerNextToken(pLexer);
        if (pParser->zError) { cypherAstDestroy(left); return NULL; }

        CypherAst *right = parseMultiplicativeExpression(pLexer, pParser);
        if (!right) { cypherAstDestroy(left); return NULL; }

        left = cypherAstCreateBinaryOp(opToken->zValue, left, right, opToken->iLine, opToken->iColumn);
        if (!left) { cypherAstDestroy(right); return NULL; }
    }
    return left;
}

static CypherAst *parseMultiplicativeExpression(CypherLexer *pLexer, CypherParser *pParser) {
    CypherAst *left = parseUnaryExpression(pLexer, pParser);
    if (!left) return NULL;

    while (parserPeekToken(pLexer)->type == CYPHER_TOK_MULT ||
           parserPeekToken(pLexer)->type == CYPHER_TOK_DIV ||
           parserPeekToken(pLexer)->type == CYPHER_TOK_MOD ||
           parserPeekToken(pLexer)->type == CYPHER_TOK_POW) {
        CypherToken *opToken = cypherLexerNextToken(pLexer);
        if (pParser->zError) { cypherAstDestroy(left); return NULL; }

        CypherAst *right = parseUnaryExpression(pLexer, pParser);
        if (!right) { cypherAstDestroy(left); return NULL; }

        left = cypherAstCreateBinaryOp(opToken->zValue, left, right, opToken->iLine, opToken->iColumn);
        if (!left) { cypherAstDestroy(right); return NULL; }
    }
    return left;
}

static CypherAst *parseUnaryExpression(CypherLexer *pLexer, CypherParser *pParser) {
    CypherTokenType nextType = parserPeekToken(pLexer)->type;
    if (nextType == CYPHER_TOK_NOT || nextType == CYPHER_TOK_MINUS) {
        CypherToken *opToken = cypherLexerNextToken(pLexer);
        if (pParser->zError) return NULL;

        CypherAst *operand = parsePrimaryExpression(pLexer, pParser);
        if (!operand) return NULL;

        CypherAst *unaryAst = cypherAstCreate(CYPHER_AST_UNARY_OP, opToken->iLine, opToken->iColumn);
        if (!unaryAst) { cypherAstDestroy(operand); return NULL; }
        cypherAstSetValue(unaryAst, opToken->zValue);
        cypherAstAddChild(unaryAst, operand);
        return unaryAst;
    } else {
        return parsePrimaryExpression(pLexer, pParser);
    }
}

static CypherAst *parsePrimaryExpression(CypherLexer *pLexer, CypherParser *pParser) {
    CypherToken *token = parserPeekToken(pLexer);
    CypherAst *ast = NULL;

    switch (token->type) {
        case CYPHER_TOK_INTEGER:
        case CYPHER_TOK_FLOAT:
        case CYPHER_TOK_STRING:
        case CYPHER_TOK_BOOLEAN:
        case CYPHER_TOK_NULL:
            ast = parseLiteral(pLexer, pParser);
            break;
        case CYPHER_TOK_IDENTIFIER:
            // Could be identifier or property access
            token = cypherLexerNextToken(pLexer);
            if (pParser->zError) return NULL;
            ast = cypherAstCreateIdentifier(token->zValue, token->iLine, token->iColumn);
            if (parserPeekToken(pLexer)->type == CYPHER_TOK_DOT) {
                // It's a property expression
                CypherAst *propAst = parsePropertyExpression(pLexer, pParser);
                if (!propAst) { cypherAstDestroy(ast); return NULL; }
                // The property expression parser expects to create the identifier itself.
                // So, we need to replace the current `ast` with the `propAst`.
                // This is a bit of a hack due to the recursive descent structure.
                // A better approach would be to pass the already parsed identifier to parsePropertyExpression.
                // For now, we'll just return the propAst and rely on its internal creation.
                return propAst;
            }
            break;
        case CYPHER_TOK_LPAREN:
            parserConsumeToken(pLexer, CYPHER_TOK_LPAREN, pParser);
            if (pParser->zError) return NULL;
            ast = parseExpression(pLexer, pParser);
            if (!ast) return NULL;
            parserConsumeToken(pLexer, CYPHER_TOK_RPAREN, pParser);
            if (pParser->zError) { cypherAstDestroy(ast); return NULL; }
            break;
        default:
            parserSetError(pParser, pLexer, "Unexpected token %s in primary expression at line %d, column %d",
                           cypherTokenTypeName(token->type), token->iLine, token->iColumn);
            return NULL;
    }
    return ast;
}

static CypherAst *parsePropertyExpression(CypherLexer *pLexer, CypherParser *pParser) {
    // This function is called after an IDENTIFIER has been consumed and a DOT is peeked.
    // The initial IDENTIFIER is already part of the AST passed as pObj.
    // We need to re-parse the identifier as the object, then consume the dot and the property name.
    // This is a bit tricky with the current peek/consume model, might need adjustment.
    // For simplicity, let's assume the previous IDENTIFIER is already handled and we just need to get the property.

    // Re-get the identifier that was already consumed as the object
    CypherToken *objToken = pLexer->pLastToken; // This is a hack, needs proper lookahead buffer
    if (!objToken || objToken->type != CYPHER_TOK_IDENTIFIER) {
        parserSetError(pParser, pLexer, "Internal parser error: Expected identifier for property access");
        return NULL;
    }
    CypherAst *pObj = cypherAstCreateIdentifier(objToken->zValue, objToken->iLine, objToken->iColumn);
    if (!pObj) return NULL;

    parserConsumeToken(pLexer, CYPHER_TOK_DOT, pParser);
    if (pParser->zError) { cypherAstDestroy(pObj); return NULL; }

    CypherToken *propToken = parserConsumeToken(pLexer, CYPHER_TOK_IDENTIFIER, pParser);
    if (pParser->zError) { cypherAstDestroy(pObj); return NULL; }

    CypherAst *propAst = cypherAstCreateProperty(pObj, propToken->zValue, propToken->iLine, propToken->iColumn);
    if (!propAst) { cypherAstDestroy(pObj); return NULL; }
    return propAst;
}

static CypherAst *parseLiteral(CypherLexer *pLexer, CypherParser *pParser) {
    CypherToken *token = cypherLexerNextToken(pLexer);
    if (token == NULL) { // Lexer error
        parserSetError(pParser, pLexer, "Lexer error: %s", pLexer->zErrorMsg);
        return NULL;
    }
    switch (token->type) {
        case CYPHER_TOK_INTEGER:
        case CYPHER_TOK_FLOAT:
        case CYPHER_TOK_STRING:
        case CYPHER_TOK_BOOLEAN:
        case CYPHER_TOK_NULL:
            return cypherAstCreateLiteral(token->zValue, token->iLine, token->iColumn);
        default:
            parserSetError(pParser, pLexer, "Expected a literal but got %s at line %d, column %d",
                           cypherTokenTypeName(token->type), token->iLine, token->iColumn);
            return NULL;
    }
}

// Main parsing entry point
CypherAst *cypherParse(CypherParser *pParser, const char *zQuery, char **pzErrMsg) {
    CypherLexer *pLexer = cypherLexerCreate(zQuery);
    if (!pLexer) {
        if (pzErrMsg) *pzErrMsg = cypher_malloc(strlen("Failed to create lexer") + 1); strcpy(*pzErrMsg, "Failed to create lexer");
        return NULL;
    }

    // Prime the lexer with the first token
    pLexer->pLastToken = cypherLexerNextToken(pLexer);
    if (pLexer->zErrorMsg) {
        if (pzErrMsg) *pzErrMsg = pLexer->zErrorMsg; // Transfer ownership
        pLexer->zErrorMsg = NULL; // Prevent double free
        cypherLexerDestroy(pLexer);
        return NULL;
    }

    CypherAst *pAst = parseQuery(pLexer, pParser);

    if (pParser->zError) {
        if (pzErrMsg) {
            *pzErrMsg = pParser->zError; // Transfer ownership
            pParser->zError = NULL; // Prevent double free
        }
        cypherAstDestroy(pAst); // Clean up partially built AST
        pAst = NULL;
    }

    cypherLexerDestroy(pLexer);
    return pAst;
}