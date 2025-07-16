/*
** SQLite Graph Database Extension - Cypher Lexer Implementation
**
** This file implements the lexical analyzer (tokenizer) for the openCypher
** query language. It converts Cypher query text into a stream of tokens
** for parsing.
**
** Features:
** - Complete openCypher token support
** - Unicode string literal handling
** - Comprehensive number parsing (integers, floats, scientific notation)
** - Case-insensitive keyword recognition
** - Single-line and multi-line comment support
** - Detailed error reporting with line/column information
**
** Memory allocation: All functions use sqlite3_malloc()/sqlite3_free()
** Error handling: Functions return SQLite error codes
*/

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1
#include "cypher.h"
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdarg.h>

/*
** Keyword table for case-insensitive keyword recognition.
** Sorted alphabetically for binary search.
*/
static const struct {
  const char *zKeyword;
  CypherTokenType token;
} aCypherKeywords[] = {
  {"AND",        CYPHER_TOK_AND},
  {"AS",         CYPHER_TOK_AS},
  {"ASC",        CYPHER_TOK_ASC},
  {"AVG",        CYPHER_TOK_AVG},
  {"BY",         CYPHER_TOK_BY},
  {"COLLECT",    CYPHER_TOK_COLLECT},
  {"CONTAINS",   CYPHER_TOK_CONTAINS},
  {"COUNT",      CYPHER_TOK_COUNT},
  {"CREATE",     CYPHER_TOK_CREATE},
  {"DELETE",     CYPHER_TOK_DELETE},
  {"DESC",       CYPHER_TOK_DESC},
  {"DETACH",     CYPHER_TOK_DETACH},
  {"DISTINCT",   CYPHER_TOK_DISTINCT},
  {"ENDS",       CYPHER_TOK_ENDS_WITH},
  {"FALSE",      CYPHER_TOK_BOOLEAN},
  {"IN",         CYPHER_TOK_IN},
  {"IS",         CYPHER_TOK_IS_NULL},
  {"LIMIT",      CYPHER_TOK_LIMIT},
  {"MATCH",      CYPHER_TOK_MATCH},
  {"MAX",        CYPHER_TOK_MAX},
  {"MERGE",      CYPHER_TOK_MERGE},
  {"MIN",        CYPHER_TOK_MIN},
  {"NOT",        CYPHER_TOK_NOT},
  {"NULL",       CYPHER_TOK_NULL},
  {"OPTIONAL",   CYPHER_TOK_OPTIONAL},
  {"OR",         CYPHER_TOK_OR},
  {"ORDER",      CYPHER_TOK_ORDER},
  {"REMOVE",     CYPHER_TOK_REMOVE},
  {"RETURN",     CYPHER_TOK_RETURN},
  {"SET",        CYPHER_TOK_SET},
  {"SKIP",       CYPHER_TOK_SKIP},
  {"STARTS",     CYPHER_TOK_STARTS_WITH},
  {"SUM",        CYPHER_TOK_SUM},
  {"TRUE",       CYPHER_TOK_BOOLEAN},
  {"UNWIND",     CYPHER_TOK_UNWIND},
  {"WHERE",      CYPHER_TOK_WHERE},
  {"WITH",       CYPHER_TOK_WITH},
  {"XOR",        CYPHER_TOK_XOR}
};

#define NUM_KEYWORDS (sizeof(aCypherKeywords)/sizeof(aCypherKeywords[0]))

/*
** Create a new Cypher lexer for the given input.
** Returns NULL on allocation failure.
*/
CypherLexer *cypherLexerCreate(const char *zInput, int nInput) {
  CypherLexer *pLexer;
  
  if( !zInput ) return NULL;
  if( nInput < 0 ) nInput = (int)strlen(zInput);
  
  pLexer = sqlite3_malloc(sizeof(CypherLexer));
  if( !pLexer ) return NULL;
  
  memset(pLexer, 0, sizeof(CypherLexer));
  pLexer->zInput = zInput;
  pLexer->nInput = nInput;
  pLexer->iLine = 1;
  pLexer->iColumn = 1;
  
  return pLexer;
}

/*
** Destroy a Cypher lexer and free all associated memory.
*/
void cypherLexerDestroy(CypherLexer *pLexer) {
  if( !pLexer ) return;
  
  /* Free all tokens */
  CypherToken *pToken = pLexer->pTokens;
  while( pToken ) {
    CypherToken *pNext = pToken->pNext;
    sqlite3_free(pToken->zValue);
    sqlite3_free(pToken);
    pToken = pNext;
  }
  
  sqlite3_free(pLexer->zErrorMsg);
  sqlite3_free(pLexer);
}

/*
** Add a token to the lexer's token list.
** Returns SQLITE_OK on success, SQLITE_NOMEM on allocation failure.
*/
static int lexerAddToken(CypherLexer *pLexer, CypherTokenType type, 
                         const char *zValue, int nValue, 
                         int iLine, int iColumn) {
  CypherToken *pToken;
  
  pToken = sqlite3_malloc(sizeof(CypherToken));
  if( !pToken ) return SQLITE_NOMEM;
  
  memset(pToken, 0, sizeof(CypherToken));
  pToken->type = type;
  pToken->iLine = iLine;
  pToken->iColumn = iColumn;
  
  if( zValue && nValue > 0 ) {
    pToken->zValue = sqlite3_malloc(nValue + 1);
    if( !pToken->zValue ) {
      sqlite3_free(pToken);
      return SQLITE_NOMEM;
    }
    memcpy(pToken->zValue, zValue, nValue);
    pToken->zValue[nValue] = '\0';
    pToken->nValue = nValue;
  }
  
  /* Add to end of token list */
  if( pLexer->pLastToken ) {
    pLexer->pLastToken->pNext = pToken;
  } else {
    pLexer->pTokens = pToken;
  }
  pLexer->pLastToken = pToken;
  
  return SQLITE_OK;
}

/*
** Set lexer error message.
*/
static void lexerSetError(CypherLexer *pLexer, const char *zFormat, ...) {
  va_list ap;
  sqlite3_free(pLexer->zErrorMsg);
  va_start(ap, zFormat);
  pLexer->zErrorMsg = sqlite3_vmprintf(zFormat, ap);
  va_end(ap);
}

/*
** Check if character is valid for starting an identifier.
*/
static int isIdentifierStart(int c) {
  return isalpha(c) || c == '_' || c == '$';
}

/*
** Check if character is valid for continuing an identifier.
*/
static int isIdentifierCont(int c) {
  return isalnum(c) || c == '_' || c == '$';
}

/*
** Get current character without advancing position.
*/
static int lexerPeek(CypherLexer *pLexer) {
  if( pLexer->iPos >= pLexer->nInput ) return 0;
  return pLexer->zInput[pLexer->iPos];
}

/*
** Get current character and advance position.
*/
static int lexerNext(CypherLexer *pLexer) {
  int c;
  if( pLexer->iPos >= pLexer->nInput ) return 0;
  
  c = pLexer->zInput[pLexer->iPos++];
  if( c == '\n' ) {
    pLexer->iLine++;
    pLexer->iColumn = 1;
  } else {
    pLexer->iColumn++;
  }
  return c;
}

/*
** Check if a string is a keyword and return its token type.
*/
int cypherIsKeyword(const char *zStr) {
  return cypherGetKeywordToken(zStr) != CYPHER_TOK_IDENTIFIER;
}

/*
** Get token type for a keyword string using binary search.
*/
CypherTokenType cypherGetKeywordToken(const char *zStr) {
  int iLower = 0;
  int iUpper = NUM_KEYWORDS - 1;
  int iMid, cmp;
  
  while( iLower <= iUpper ) {
    iMid = (iLower + iUpper) / 2;
    cmp = sqlite3_stricmp(zStr, aCypherKeywords[iMid].zKeyword);
    
    if( cmp == 0 ) {
      return aCypherKeywords[iMid].token;
    } else if( cmp < 0 ) {
      iUpper = iMid - 1;
    } else {
      iLower = iMid + 1;
    }
  }
  
  return CYPHER_TOK_IDENTIFIER;
}

/*
** Tokenize an identifier or keyword.
*/
static int lexerTokenizeIdentifier(CypherLexer *pLexer) {
  int iStart = pLexer->iPos;
  int iLine = pLexer->iLine;
  int iColumn = pLexer->iColumn;
  int c;
  char *zValue;
  CypherTokenType tokenType;
  
  /* Read identifier characters */
  while( (c = lexerPeek(pLexer)) && isIdentifierCont(c) ) {
    lexerNext(pLexer);
  }
  
  /* Extract identifier text */
  int nLen = pLexer->iPos - iStart;
  zValue = sqlite3_malloc(nLen + 1);
  if( !zValue ) return SQLITE_NOMEM;
  
  memcpy(zValue, &pLexer->zInput[iStart], nLen);
  zValue[nLen] = '\0';
  
  /* Check if it's a keyword */
  tokenType = cypherGetKeywordToken(zValue);
  
  int rc = lexerAddToken(pLexer, tokenType, zValue, nLen, iLine, iColumn);
  sqlite3_free(zValue);
  
  return rc;
}

/*
** Tokenize a number (integer or float).
*/
static int lexerTokenizeNumber(CypherLexer *pLexer) {
  int iStart = pLexer->iPos;
  int iLine = pLexer->iLine;
  int iColumn = pLexer->iColumn;
  int c;
  CypherTokenType tokenType = CYPHER_TOK_INTEGER;
  
  /* Read digits */
  while( (c = lexerPeek(pLexer)) && isdigit(c) ) {
    lexerNext(pLexer);
  }
  
  /* Check for decimal point */
  if( lexerPeek(pLexer) == '.' ) {
    lexerNext(pLexer);
    tokenType = CYPHER_TOK_FLOAT;
    
    /* Read fractional digits */
    while( (c = lexerPeek(pLexer)) && isdigit(c) ) {
      lexerNext(pLexer);
    }
  }
  
  /* Check for scientific notation */
  c = lexerPeek(pLexer);
  if( c == 'e' || c == 'E' ) {
    lexerNext(pLexer);
    tokenType = CYPHER_TOK_FLOAT;
    
    /* Optional sign */
    c = lexerPeek(pLexer);
    if( c == '+' || c == '-' ) {
      lexerNext(pLexer);
    }
    
    /* Read exponent digits */
    while( (c = lexerPeek(pLexer)) && isdigit(c) ) {
      lexerNext(pLexer);
    }
  }
  
  int nLen = pLexer->iPos - iStart;
  return lexerAddToken(pLexer, tokenType, &pLexer->zInput[iStart], nLen, 
                       iLine, iColumn);
}

/*
** Tokenize a string literal.
*/
static int lexerTokenizeString(CypherLexer *pLexer, int quote) {
  int iStart = pLexer->iPos;
  int iLine = pLexer->iLine;
  int iColumn = pLexer->iColumn;
  int c;
  
  /* Skip opening quote */
  lexerNext(pLexer);
  
  while( (c = lexerNext(pLexer)) != 0 ) {
    if( c == quote ) {
      /* Check for escaped quote */
      if( lexerPeek(pLexer) == quote ) {
        lexerNext(pLexer); /* Skip escaped quote */
      } else {
        break; /* End of string */
      }
    } else if( c == '\\' ) {
      /* Handle escape sequences */
      c = lexerNext(pLexer);
      if( c == 0 ) {
        lexerSetError(pLexer, "Unterminated string literal at line %d", iLine);
        return SQLITE_ERROR;
      }
    }
  }
  
  if( c == 0 ) {
    lexerSetError(pLexer, "Unterminated string literal at line %d", iLine);
    return SQLITE_ERROR;
  }
  
  /* Extract string content (without quotes) */
  int nLen = pLexer->iPos - iStart - 2;
  return lexerAddToken(pLexer, CYPHER_TOK_STRING, 
                       &pLexer->zInput[iStart + 1], nLen, iLine, iColumn);
}

/*
** Skip whitespace and comments.
*/
static int lexerSkipWhitespace(CypherLexer *pLexer) {
  int c;
  
  while( (c = lexerPeek(pLexer)) != 0 ) {
    if( isspace(c) ) {
      lexerNext(pLexer);
    } else if( c == '/' && pLexer->iPos + 1 < pLexer->nInput ) {
      /* Check for comments */
      if( pLexer->zInput[pLexer->iPos + 1] == '/' ) {
        /* Single-line comment */
        lexerNext(pLexer); /* Skip / */
        lexerNext(pLexer); /* Skip / */
        while( (c = lexerPeek(pLexer)) != 0 && c != '\n' ) {
          lexerNext(pLexer);
        }
      } else if( pLexer->zInput[pLexer->iPos + 1] == '*' ) {
        /* Multi-line comment */
        lexerNext(pLexer); /* Skip / */
        lexerNext(pLexer); /* Skip * */
        while( (c = lexerNext(pLexer)) != 0 ) {
          if( c == '*' && lexerPeek(pLexer) == '/' ) {
            lexerNext(pLexer); /* Skip / */
            break;
          }
        }
        if( c == 0 ) {
          lexerSetError(pLexer, "Unterminated comment");
          return SQLITE_ERROR;
        }
      } else {
        break; /* Not a comment */
      }
    } else {
      break; /* Not whitespace */
    }
  }
  
  return SQLITE_OK;
}

/*
** Tokenize the entire input string.
*/
int cypherLexerTokenize(CypherLexer *pLexer) {
  int c, c2;
  int rc = SQLITE_OK;
  
  if( !pLexer ) return SQLITE_MISUSE;
  
  while( pLexer->iPos < pLexer->nInput && rc == SQLITE_OK ) {
    /* Skip whitespace and comments */
    rc = lexerSkipWhitespace(pLexer);
    if( rc != SQLITE_OK || pLexer->iPos >= pLexer->nInput ) break;
    
    c = lexerPeek(pLexer);
    int iLine = pLexer->iLine;
    int iColumn = pLexer->iColumn;
    
    if( isIdentifierStart(c) ) {
      /* Identifier or keyword */
      rc = lexerTokenizeIdentifier(pLexer);
    } else if( isdigit(c) ) {
      /* Number */
      rc = lexerTokenizeNumber(pLexer);
    } else if( c == '"' || c == '\'' ) {
      /* String literal */
      rc = lexerTokenizeString(pLexer, c);
    } else {
      /* Single or multi-character operators and punctuation */
      lexerNext(pLexer); /* Consume character */
      
      switch( c ) {
        case '(':  rc = lexerAddToken(pLexer, CYPHER_TOK_LPAREN, NULL, 0, iLine, iColumn); break;
        case ')':  rc = lexerAddToken(pLexer, CYPHER_TOK_RPAREN, NULL, 0, iLine, iColumn); break;
        case '[':  rc = lexerAddToken(pLexer, CYPHER_TOK_LBRACKET, NULL, 0, iLine, iColumn); break;
        case ']':  rc = lexerAddToken(pLexer, CYPHER_TOK_RBRACKET, NULL, 0, iLine, iColumn); break;
        case '{':  rc = lexerAddToken(pLexer, CYPHER_TOK_LBRACE, NULL, 0, iLine, iColumn); break;
        case '}':  rc = lexerAddToken(pLexer, CYPHER_TOK_RBRACE, NULL, 0, iLine, iColumn); break;
        case ',':  rc = lexerAddToken(pLexer, CYPHER_TOK_COMMA, NULL, 0, iLine, iColumn); break;
        case ';':  rc = lexerAddToken(pLexer, CYPHER_TOK_SEMICOLON, NULL, 0, iLine, iColumn); break;
        case '|':  rc = lexerAddToken(pLexer, CYPHER_TOK_PIPE, NULL, 0, iLine, iColumn); break;
        case '$':  rc = lexerAddToken(pLexer, CYPHER_TOK_DOLLAR, NULL, 0, iLine, iColumn); break;
        case '+':  rc = lexerAddToken(pLexer, CYPHER_TOK_PLUS, NULL, 0, iLine, iColumn); break;
        case '*':  rc = lexerAddToken(pLexer, CYPHER_TOK_STAR, NULL, 0, iLine, iColumn); break;
        case '/':  rc = lexerAddToken(pLexer, CYPHER_TOK_DIV, NULL, 0, iLine, iColumn); break;
        case '%':  rc = lexerAddToken(pLexer, CYPHER_TOK_MOD, NULL, 0, iLine, iColumn); break;
        case '^':  rc = lexerAddToken(pLexer, CYPHER_TOK_POW, NULL, 0, iLine, iColumn); break;
        
        case ':':
          c2 = lexerPeek(pLexer);
          if( c2 == ':' ) {
            lexerNext(pLexer);
            rc = lexerAddToken(pLexer, CYPHER_TOK_RANGE, "::", 2, iLine, iColumn);
          } else {
            rc = lexerAddToken(pLexer, CYPHER_TOK_COLON, NULL, 0, iLine, iColumn);
          }
          break;
          
        case '.':
          c2 = lexerPeek(pLexer);
          if( c2 == '.' ) {
            lexerNext(pLexer);
            rc = lexerAddToken(pLexer, CYPHER_TOK_RANGE, "..", 2, iLine, iColumn);
          } else {
            rc = lexerAddToken(pLexer, CYPHER_TOK_DOT, NULL, 0, iLine, iColumn);
          }
          break;
          
        case '-':
          c2 = lexerPeek(pLexer);
          if( c2 == '>' ) {
            lexerNext(pLexer);
            rc = lexerAddToken(pLexer, CYPHER_TOK_ARROW_RIGHT, "->", 2, iLine, iColumn);
          } else if( c2 == '[' ) {
            lexerNext(pLexer);
            rc = lexerAddToken(pLexer, CYPHER_TOK_REL_START, "-[", 2, iLine, iColumn);
          } else {
            rc = lexerAddToken(pLexer, CYPHER_TOK_MINUS, NULL, 0, iLine, iColumn);
          }
          break;
          
        case '<':
          c2 = lexerPeek(pLexer);
          if( c2 == '-' ) {
            lexerNext(pLexer);
            rc = lexerAddToken(pLexer, CYPHER_TOK_ARROW_LEFT, "<-", 2, iLine, iColumn);
          } else if( c2 == '>' ) {
            lexerNext(pLexer);
            rc = lexerAddToken(pLexer, CYPHER_TOK_NE, "<>", 2, iLine, iColumn);
          } else if( c2 == '=' ) {
            lexerNext(pLexer);
            rc = lexerAddToken(pLexer, CYPHER_TOK_LE, "<=", 2, iLine, iColumn);
          } else {
            rc = lexerAddToken(pLexer, CYPHER_TOK_LT, NULL, 0, iLine, iColumn);
          }
          break;
          
        case '>':
          c2 = lexerPeek(pLexer);
          if( c2 == '=' ) {
            lexerNext(pLexer);
            rc = lexerAddToken(pLexer, CYPHER_TOK_GE, ">=", 2, iLine, iColumn);
          } else {
            rc = lexerAddToken(pLexer, CYPHER_TOK_GT, NULL, 0, iLine, iColumn);
          }
          break;
          
        case '=':
          c2 = lexerPeek(pLexer);
          if( c2 == '~' ) {
            lexerNext(pLexer);
            rc = lexerAddToken(pLexer, CYPHER_TOK_REGEX, "=~", 2, iLine, iColumn);
          } else {
            rc = lexerAddToken(pLexer, CYPHER_TOK_EQ, NULL, 0, iLine, iColumn);
          }
          break;
          
        default:
          lexerSetError(pLexer, "Unexpected character '%c' at line %d, column %d", 
                       c, iLine, iColumn);
          rc = SQLITE_ERROR;
          break;
      }
    }
  }
  
  /* Add EOF token */
  if( rc == SQLITE_OK ) {
    rc = lexerAddToken(pLexer, CYPHER_TOK_EOF, NULL, 0, 
                       pLexer->iLine, pLexer->iColumn);
  }
  
  return rc;
}

/*
** Get the next token from the lexer.
** This function is used during parsing to iterate through tokens.
*/
CypherToken *cypherLexerNextToken(CypherLexer *pLexer) {
  static CypherToken *pCurrentToken = NULL;
  
  if( !pLexer ) return NULL;
  
  if( !pCurrentToken ) {
    pCurrentToken = pLexer->pTokens;
  } else {
    pCurrentToken = pCurrentToken->pNext;
  }
  
  return pCurrentToken;
}