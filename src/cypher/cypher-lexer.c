#include "cypher.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

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

// Keyword lookup table
typedef struct {
    const char *zKeyword;
    CypherTokenType type;
} KeywordInfo;

static KeywordInfo aCypherKeywords[] = {
    { "MATCH", CYPHER_TOK_MATCH },
    { "OPTIONAL", CYPHER_TOK_OPTIONAL },
    { "WHERE", CYPHER_TOK_WHERE },
    { "RETURN", CYPHER_TOK_RETURN },
    { "CREATE", CYPHER_TOK_CREATE },
    { "MERGE", CYPHER_TOK_MERGE },
    { "SET", CYPHER_TOK_SET },
    { "DELETE", CYPHER_TOK_DELETE },
    { "DETACH", CYPHER_TOK_DETACH },
    { "REMOVE", CYPHER_TOK_REMOVE },
    { "WITH", CYPHER_TOK_WITH },
    { "UNION", CYPHER_TOK_UNION },
    { "AS", CYPHER_TOK_AS },
    { "ORDER", CYPHER_TOK_ORDER },
    { "BY", CYPHER_TOK_BY },
    { "ASC", CYPHER_TOK_ASC },
    { "DESC", CYPHER_TOK_DESC },
    { "LIMIT", CYPHER_TOK_LIMIT },
    { "SKIP", CYPHER_TOK_SKIP },
    { "DISTINCT", CYPHER_TOK_DISTINCT },
    { "AND", CYPHER_TOK_AND },
    { "OR", CYPHER_TOK_OR },
    { "XOR", CYPHER_TOK_XOR },
    { "NOT", CYPHER_TOK_NOT },
    { "IN", CYPHER_TOK_IN },
    { "STARTS WITH", CYPHER_TOK_STARTS_WITH },
    { "ENDS WITH", CYPHER_TOK_ENDS_WITH },
    { "CONTAINS", CYPHER_TOK_CONTAINS },
    { "IS NULL", CYPHER_TOK_IS_NULL },
    { "IS NOT NULL", CYPHER_TOK_IS_NOT_NULL },
    { "NULL", CYPHER_TOK_NULL },
    { 0, 0 } // Sentinel
};

CypherTokenType cypherGetKeywordToken(const char *zKeyword) {
    for (int i = 0; aCypherKeywords[i].zKeyword; i++) {
        if (strcmp(aCypherKeywords[i].zKeyword, zKeyword) == 0) {
            return aCypherKeywords[i].type;
        }
    }
    return CYPHER_TOK_IDENTIFIER;
}

// Lexer Functions
CypherLexer *cypherLexerCreate(const char *zInput) {
    CypherLexer *pLexer = cypher_malloc(sizeof(CypherLexer));
    pLexer->zInput = cypher_malloc(strlen(zInput) + 1);
    strcpy((char *)pLexer->zInput, zInput);
    pLexer->iPos = 0;
    pLexer->iLine = 1;
    pLexer->iColumn = 1;
    pLexer->zErrorMsg = NULL;
    pLexer->pLastToken = NULL;
    return pLexer;
}

void cypherLexerDestroy(CypherLexer *pLexer) {
    if (pLexer) {
        cypher_free((void *)pLexer->zInput);
        if (pLexer->zErrorMsg) {
            cypher_free(pLexer->zErrorMsg);
        }
        if (pLexer->pLastToken) {
            cypher_free(pLexer->pLastToken->zValue);
            cypher_free(pLexer->pLastToken);
        }
        cypher_free(pLexer);
    }
}

static void lexerSetError(CypherLexer *pLexer, const char *zFormat, ...) {
    va_list args;
    va_start(args, zFormat);
    // Determine size needed
    int size = vsnprintf(NULL, 0, zFormat, args);
    va_end(args);

    pLexer->zErrorMsg = cypher_malloc(size + 1);
    va_start(args, zFormat);
    vsnprintf(pLexer->zErrorMsg, size + 1, zFormat, args);
    va_end(args);
}

static char lexerPeek(CypherLexer *pLexer, int offset) {
    if (pLexer->iPos + offset < strlen(pLexer->zInput)) {
        return pLexer->zInput[pLexer->iPos + offset];
    }
    return '\0';
}

static char lexerNext(CypherLexer *pLexer) {
    char c = lexerPeek(pLexer, 0);
    if (c != '\0') {
        pLexer->iPos++;
        if (c == '\n') {
            pLexer->iLine++;
            pLexer->iColumn = 1;
        } else {
            pLexer->iColumn++;
        }
    }
    return c;
}

static void lexerSkipWhitespace(CypherLexer *pLexer) {
    while (isspace(lexerPeek(pLexer, 0))) {
        lexerNext(pLexer);
    }
}

static void lexerSkipComment(CypherLexer *pLexer) {
    if (lexerPeek(pLexer, 0) == '/' && lexerPeek(pLexer, 1) == '/') {
        // Single-line comment
        while (lexerPeek(pLexer, 0) != '\n' && lexerPeek(pLexer, 0) != '\0') {
            lexerNext(pLexer);
        }
        lexerNext(pLexer); // Consume newline
    } else if (lexerPeek(pLexer, 0) == '/' && lexerPeek(pLexer, 1) == '*') {
        // Multi-line comment
        lexerNext(pLexer); // Consume '/'
        lexerNext(pLexer); // Consume '*'
        while (!(lexerPeek(pLexer, 0) == '*' && lexerPeek(pLexer, 1) == '/') && lexerPeek(pLexer, 0) != '\0') {
            lexerNext(pLexer);
        }
        if (lexerPeek(pLexer, 0) == '\0') {
            lexerSetError(pLexer, "Unterminated multi-line comment");
        } else {
            lexerNext(pLexer); // Consume '*'
            lexerNext(pLexer); // Consume '/'
        }
    }
}

static CypherToken *lexerAddToken(CypherLexer *pLexer, CypherTokenType type, int startPos, int endPos) {
    if (pLexer->pLastToken) {
        cypher_free(pLexer->pLastToken->zValue);
        cypher_free(pLexer->pLastToken);
    }
    pLexer->pLastToken = cypher_malloc(sizeof(CypherToken));
    pLexer->pLastToken->type = type;
    pLexer->pLastToken->iLine = pLexer->iLine;
    pLexer->pLastToken->iColumn = pLexer->iColumn - (endPos - startPos); // Adjust column for token length
    
    int len = endPos - startPos;
    pLexer->pLastToken->zValue = cypher_malloc(len + 1);
    strncpy(pLexer->pLastToken->zValue, pLexer->zInput + startPos, len);
    pLexer->pLastToken->zValue[len] = '\0';
    
    return pLexer->pLastToken;
}

static CypherToken *lexerTokenizeIdentifier(CypherLexer *pLexer) {
    int startPos = pLexer->iPos;
    while (isalnum(lexerPeek(pLexer, 0)) || lexerPeek(pLexer, 0) == '_') {
        lexerNext(pLexer);
    }
    int endPos = pLexer->iPos;
    int len = endPos - startPos;
    char *zIdent = cypher_malloc(len + 1);
    strncpy(zIdent, pLexer->zInput + startPos, len);
    zIdent[len] = '\0';

    CypherTokenType type = cypherGetKeywordToken(zIdent);
    CypherToken *token = lexerAddToken(pLexer, type, startPos, endPos);
    cypher_free(zIdent); // Free temporary identifier string
    return token;
}

static CypherToken *lexerTokenizeNumber(CypherLexer *pLexer) {
    int startPos = pLexer->iPos;
    CypherTokenType type = CYPHER_TOK_INTEGER;

    while (isdigit(lexerPeek(pLexer, 0))) {
        lexerNext(pLexer);
    }

    if (lexerPeek(pLexer, 0) == '.') {
        type = CYPHER_TOK_FLOAT;
        lexerNext(pLexer); // Consume '.'
        while (isdigit(lexerPeek(pLexer, 0))) {
            lexerNext(pLexer);
        }
    }

    // Handle scientific notation (e.g., 1.23e-4)
    if (tolower(lexerPeek(pLexer, 0)) == 'e') {
        type = CYPHER_TOK_FLOAT;
        lexerNext(pLexer); // Consume 'e'
        if (lexerPeek(pLexer, 0) == '+' || lexerPeek(pLexer, 0) == '-') {
            lexerNext(pLexer);
        }
        if (!isdigit(lexerPeek(pLexer, 0))) {
            lexerSetError(pLexer, "Malformed scientific notation");
            return NULL;
        }
        while (isdigit(lexerPeek(pLexer, 0))) {
            lexerNext(pLexer);
        }
    }

    return lexerAddToken(pLexer, type, startPos, pLexer->iPos);
}

static CypherToken *lexerTokenizeString(CypherLexer *pLexer) {
    int startPos = pLexer->iPos;
    char quoteChar = lexerNext(pLexer); // Consume the opening quote

    while (lexerPeek(pLexer, 0) != quoteChar && lexerPeek(pLexer, 0) != '\0') {
        if (lexerPeek(pLexer, 0) == '\\') { // Handle escape sequences
            lexerNext(pLexer); // Consume '\\'
            if (lexerPeek(pLexer, 0) == '\0') {
                lexerSetError(pLexer, "Unterminated string literal with escape");
                return NULL;
            }
        }
        lexerNext(pLexer);
    }

    if (lexerPeek(pLexer, 0) == '\0') {
        lexerSetError(pLexer, "Unterminated string literal");
        return NULL;
    }
    lexerNext(pLexer); // Consume closing quote

    return lexerAddToken(pLexer, CYPHER_TOK_STRING, startPos + 1, pLexer->iPos - 1); // Exclude quotes
}

CypherToken *cypherLexerNextToken(CypherLexer *pLexer) {
    lexerSkipWhitespace(pLexer);
    lexerSkipComment(pLexer);
    lexerSkipWhitespace(pLexer);

    if (pLexer->zErrorMsg) return NULL; // Previous error occurred

    int startPos = pLexer->iPos;
    char c = lexerPeek(pLexer, 0);

    if (c == '\0') {
        return lexerAddToken(pLexer, CYPHER_TOK_EOF, startPos, startPos);
    }

    if (isalpha(c) || c == '_') {
        return lexerTokenizeIdentifier(pLexer);
    }

    if (isdigit(c)) {
        return lexerTokenizeNumber(pLexer);
    }

    if (c == '\'' || c == '"') {
        return lexerTokenizeString(pLexer);
    }

    // Handle operators and punctuation
    switch (c) {
        case '=':
            lexerNext(pLexer);
            if (lexerPeek(pLexer, 0) == '~') {
                lexerNext(pLexer);
                return lexerAddToken(pLexer, CYPHER_TOK_REGEX, startPos, pLexer->iPos);
            }
            return lexerAddToken(pLexer, CYPHER_TOK_EQ, startPos, pLexer->iPos);
        case '<':
            lexerNext(pLexer);
            if (lexerPeek(pLexer, 0) == '>') {
                lexerNext(pLexer);
                return lexerAddToken(pLexer, CYPHER_TOK_NE, startPos, pLexer->iPos);
            } else if (lexerPeek(pLexer, 0) == '-') {
                lexerNext(pLexer);
                if (lexerPeek(pLexer, 0) == '>') {
                    lexerNext(pLexer);
                    return lexerAddToken(pLexer, CYPHER_TOK_ARROW_BOTH, startPos, pLexer->iPos);
                }
                return lexerAddToken(pLexer, CYPHER_TOK_ARROW_LEFT, startPos, pLexer->iPos);
            } else if (lexerPeek(pLexer, 0) == '=') {
                lexerNext(pLexer);
                return lexerAddToken(pLexer, CYPHER_TOK_LE, startPos, pLexer->iPos);
            }
            return lexerAddToken(pLexer, CYPHER_TOK_LT, startPos, pLexer->iPos);
        case '>':
            lexerNext(pLexer);
            if (lexerPeek(pLexer, 0) == '=') {
                lexerNext(pLexer);
                return lexerAddToken(pLexer, CYPHER_TOK_GE, startPos, pLexer->iPos);
            }
            return lexerAddToken(pLexer, CYPHER_TOK_GT, startPos, pLexer->iPos);
        case '+': return lexerAddToken(pLexer, CYPHER_TOK_PLUS, startPos, lexerNext(pLexer));
        case '-': 
            lexerNext(pLexer);
            if (lexerPeek(pLexer, 0) == '>') {
                lexerNext(pLexer);
                return lexerAddToken(pLexer, CYPHER_TOK_ARROW_RIGHT, startPos, pLexer->iPos);
            }
            return lexerAddToken(pLexer, CYPHER_TOK_DASH, startPos, pLexer->iPos);
        case '*': return lexerAddToken(pLexer, CYPHER_TOK_MULT, startPos, lexerNext(pLexer));
        case '/': return lexerAddToken(pLexer, CYPHER_TOK_DIV, startPos, lexerNext(pLexer));
        case '%': return lexerAddToken(pLexer, CYPHER_TOK_MOD, startPos, lexerNext(pLexer));
        case '^': return lexerAddToken(pLexer, CYPHER_TOK_POW, startPos, lexerNext(pLexer));
        case '.': return lexerAddToken(pLexer, CYPHER_TOK_DOT, startPos, lexerNext(pLexer));
        case ':': return lexerAddToken(pLexer, CYPHER_TOK_COLON, startPos, lexerNext(pLexer));
        case ',': return lexerAddToken(pLexer, CYPHER_TOK_COMMA, startPos, lexerNext(pLexer));
        case ';': return lexerAddToken(pLexer, CYPHER_TOK_SEMICOLON, startPos, lexerNext(pLexer));
        case '(': return lexerAddToken(pLexer, CYPHER_TOK_LPAREN, startPos, lexerNext(pLexer));
        case ')': return lexerAddToken(pLexer, CYPHER_TOK_RPAREN, startPos, lexerNext(pLexer));
        case '[': return lexerAddToken(pLexer, CYPHER_TOK_LBRACKET, startPos, lexerNext(pLexer));
        case ']': return lexerAddToken(pLexer, CYPHER_TOK_RBRACKET, startPos, lexerNext(pLexer));
        case '{': return lexerAddToken(pLexer, CYPHER_TOK_LBRACE, startPos, lexerNext(pLexer));
        case '}': return lexerAddToken(pLexer, CYPHER_TOK_RBRACE, startPos, lexerNext(pLexer));
        case '|': return lexerAddToken(pLexer, CYPHER_TOK_PIPE, startPos, lexerNext(pLexer));
        case '$': return lexerAddToken(pLexer, CYPHER_TOK_DOLLAR, startPos, lexerNext(pLexer));
        default:
            lexerSetError(pLexer, "Unexpected character: '%c' at line %d, column %d", c, pLexer->iLine, pLexer->iColumn);
            return NULL;
    }
}

const char *cypherTokenTypeName(CypherTokenType type) {
    switch (type) {
        case CYPHER_TOK_EOF: return "EOF";
        case CYPHER_TOK_ERROR: return "ERROR";
        case CYPHER_TOK_WHITESPACE: return "WHITESPACE";
        case CYPHER_TOK_COMMENT: return "COMMENT";
        case CYPHER_TOK_MATCH: return "MATCH";
        case CYPHER_TOK_OPTIONAL: return "OPTIONAL";
        case CYPHER_TOK_WHERE: return "WHERE";
        case CYPHER_TOK_RETURN: return "RETURN";
        case CYPHER_TOK_CREATE: return "CREATE";
        case CYPHER_TOK_MERGE: return "MERGE";
        case CYPHER_TOK_SET: return "SET";
        case CYPHER_TOK_DELETE: return "DELETE";
        case CYPHER_TOK_DETACH: return "DETACH";
        case CYPHER_TOK_REMOVE: return "REMOVE";
        case CYPHER_TOK_WITH: return "WITH";
        case CYPHER_TOK_UNION: return "UNION";
        case CYPHER_TOK_AS: return "AS";
        case CYPHER_TOK_ORDER: return "ORDER";
        case CYPHER_TOK_BY: return "BY";
        case CYPHER_TOK_ASC: return "ASC";
        case CYPHER_TOK_DESC: return "DESC";
        case CYPHER_TOK_LIMIT: return "LIMIT";
        case CYPHER_TOK_SKIP: return "SKIP";
        case CYPHER_TOK_DISTINCT: return "DISTINCT";
        case CYPHER_TOK_AND: return "AND";
        case CYPHER_TOK_OR: return "OR";
        case CYPHER_TOK_XOR: return "XOR";
        case CYPHER_TOK_NOT: return "NOT";
        case CYPHER_TOK_IN: return "IN";
        case CYPHER_TOK_STARTS_WITH: return "STARTS WITH";
        case CYPHER_TOK_ENDS_WITH: return "ENDS WITH";
        case CYPHER_TOK_CONTAINS: return "CONTAINS";
        case CYPHER_TOK_IS_NULL: return "IS NULL";
        case CYPHER_TOK_IS_NOT_NULL: return "IS NOT NULL";
        case CYPHER_TOK_NULL: return "NULL";
        case CYPHER_TOK_EQ: return "EQ";
        case CYPHER_TOK_NE: return "NE";
        case CYPHER_TOK_LT: return "LT";
        case CYPHER_TOK_LE: return "LE";
        case CYPHER_TOK_GT: return "GT";
        case CYPHER_TOK_GE: return "GE";
        case CYPHER_TOK_PLUS: return "PLUS";
        case CYPHER_TOK_MINUS: return "MINUS";
        case CYPHER_TOK_MULT: return "MULT";
        case CYPHER_TOK_DIV: return "DIV";
        case CYPHER_TOK_MOD: return "MOD";
        case CYPHER_TOK_POW: return "POW";
        case CYPHER_TOK_DOT: return "DOT";
        case CYPHER_TOK_COLON: return "COLON";
        case CYPHER_TOK_COMMA: return "COMMA";
        case CYPHER_TOK_SEMICOLON: return "SEMICOLON";
        case CYPHER_TOK_LPAREN: return "LPAREN";
        case CYPHER_TOK_RPAREN: return "RPAREN";
        case CYPHER_TOK_LBRACKET: return "LBRACKET";
        case CYPHER_TOK_RBRACKET: return "RBRACKET";
        case CYPHER_TOK_LBRACE: return "LBRACE";
        case CYPHER_TOK_RBRACE: return "RBRACE";
        case CYPHER_TOK_DASH: return "DASH";
        case CYPHER_TOK_ARROW_RIGHT: return "ARROW_RIGHT";
        case CYPHER_TOK_ARROW_LEFT: return "ARROW_LEFT";
        case CYPHER_TOK_ARROW_BOTH: return "ARROW_BOTH";
        case CYPHER_TOK_PIPE: return "PIPE";
        case CYPHER_TOK_REGEX: return "REGEX";
        case CYPHER_TOK_DOLLAR: return "DOLLAR";
        case CYPHER_TOK_INTEGER: return "INTEGER";
        case CYPHER_TOK_FLOAT: return "FLOAT";
        case CYPHER_TOK_STRING: return "STRING";
        case CYPHER_TOK_BOOLEAN: return "BOOLEAN";
        case CYPHER_TOK_IDENTIFIER: return "IDENTIFIER";
        case CYPHER_TOK_LABEL: return "LABEL";
        case CYPHER_TOK_PROPERTY: return "PROPERTY";
        case CYPHER_TOK_REL_TYPE: return "REL_TYPE";
        case CYPHER_TOK_MAX: return "MAX_TOKEN";
    }
    return "UNKNOWN";
}
