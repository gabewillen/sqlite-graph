# Lexer Plan

## 1. Objective
To develop a robust and efficient lexer (tokenizer) for the Cypher query language, responsible for converting raw input strings into a stream of meaningful tokens.

## 2. Design Considerations
*   **Input:** Raw Cypher query string.
*   **Output:** A stream of `CypherToken` objects.
*   **Token Types:** Define an enum for all possible token types (keywords, identifiers, literals, operators, punctuation).
*   **Error Handling:** Gracefully handle invalid characters or malformed token sequences.
*   **Performance:** Efficiently process input, minimizing unnecessary operations.

## 3. Token Definition (`include/cypher.h` or similar)
```c
// Placeholder - actual structure might be in cypher.h
typedef enum {
    CYPHER_TOK_EOF = 0,
    CYPHER_TOK_ERROR,
    CYPHER_TOK_WHITESPACE, // To be skipped
    CYPHER_TOK_COMMENT,    // To be skipped

    // Keywords
    CYPHER_TOK_MATCH,
    CYPHER_TOK_OPTIONAL,
    CYPHER_TOK_WHERE,
    CYPHER_TOK_RETURN,
    CYPHER_TOK_CREATE,
    CYPHER_TOK_MERGE,
    CYPHER_TOK_SET,
    CYPHER_TOK_DELETE,
    CYPHER_TOK_DETACH,
    CYPHER_TOK_REMOVE,
    CYPHER_TOK_WITH,
    CYPHER_TOK_UNION,
    CYPHER_TOK_AS,
    CYPHER_TOK_ORDER,
    CYPHER_TOK_BY,
    CYPHER_TOK_ASC,
    CYPHER_TOK_DESC,
    CYPHER_TOK_LIMIT,
    CYPHER_TOK_SKIP,
    CYPHER_TOK_DISTINCT,
    CYPHER_TOK_AND,
    CYPHER_TOK_OR,
    CYPHER_TOK_XOR,
    CYPHER_TOK_NOT,
    CYPHER_TOK_IN,
    CYPHER_TOK_STARTS_WITH,
    CYPHER_TOK_ENDS_WITH,
    CYPHER_TOK_CONTAINS,
    CYPHER_TOK_IS_NULL,
    CYPHER_TOK_IS_NOT_NULL,
    CYPHER_TOK_NULL, // Actual NULL keyword

    // Operators
    CYPHER_TOK_EQ,  // =
    CYPHER_TOK_NE,  // <>
    CYPHER_TOK_LT,  // <
    CYPHER_TOK_LE,  // <=
    CYPHER_TOK_GT,  // >
    CYPHER_TOK_GE,  // >=
    CYPHER_TOK_PLUS, // +
    CYPHER_TOK_MINUS, // -
    CYPHER_TOK_MULT, // *
    CYPHER_TOK_DIV,  // /
    CYPHER_TOK_MOD,  // %
    CYPHER_TOK_POW,  // ^
    CYPHER_TOK_DOT,  // .
    CYPHER_TOK_COLON, // :
    CYPHER_TOK_COMMA, // ,
    CYPHER_TOK_SEMICOLON, // ;
    CYPHER_TOK_LPAREN, // (
    CYPHER_TOK_RPAREN, // )
    CYPHER_TOK_LBRACKET, // [
    CYPHER_TOK_RBRACKET, // ]
    CYPHER_TOK_LBRACE, // {
    CYPHER_TOK_RBRACE, // }
    CYPHER_TOK_DASH, // - (for relationships)
    CYPHER_TOK_ARROW_RIGHT, // ->
    CYPHER_TOK_ARROW_LEFT, // <-
    CYPHER_TOK_ARROW_BOTH, // <->
    CYPHER_TOK_PIPE, // | (for ranges)
    CYPHER_TOK_REGEX, // =~
    CYPHER_TOK_DOLLAR, // $ (for parameters)

    // Literals
    CYPHER_TOK_INTEGER,
    CYPHER_TOK_FLOAT,
    CYPHER_TOK_STRING,
    CYPHER_TOK_BOOLEAN, // TRUE, FALSE

    // Identifiers
    CYPHER_TOK_IDENTIFIER,
    CYPHER_TOK_LABEL,    // For node/relationship labels
    CYPHER_TOK_PROPERTY, // For property names
    CYPHER_TOK_REL_TYPE, // For relationship types

    CYPHER_TOK_MAX // Sentinel for max token value
} CypherTokenType;

typedef struct CypherToken {
    CypherTokenType type;
    char *zValue; // Dynamically allocated string for value (e.g., identifier name, string literal)
    int iLine;
    int iColumn;
} CypherToken;

// Lexer context structure
typedef struct CypherLexer {
    const char *zInput;
    int iPos;
    int iLine;
    int iColumn;
    char *zErrorMsg;
    CypherToken *pLastToken; // Last token returned
} CypherLexer;
```

## 4. Lexer Functions (`src/cypher/cypher-lexer.c`)

### `CypherLexer *cypherLexerCreate(const char *zInput)`
*   Initializes a new lexer instance.
*   Allocates memory for `CypherLexer` and copies `zInput`.
*   Sets initial position, line, column, and error message.

### `void cypherLexerDestroy(CypherLexer *pLexer)`
*   Frees all allocated memory associated with the lexer, including `zInput` copy and `pLastToken->zValue`.

### `CypherToken *cypherLexerNextToken(CypherLexer *pLexer)`
*   Main tokenization function.
*   Skips whitespace and comments.
*   Identifies the next token based on character patterns.
*   Handles multi-character operators (e.g., `<=`, `->`).
*   Distinguishes keywords from identifiers.
*   Parses numeric (integer, float) and string literals.
*   Populates and returns a `CypherToken` structure.
*   Sets `pLexer->zErrorMsg` on error.

### Helper Functions (Internal)
*   `char lexerPeek(CypherLexer *pLexer, int offset)`: Peeks at a character without advancing.
*   `char lexerNext(CypherLexer *pLexer)`: Consumes and returns the next character, updates position/line/column.
*   `void lexerSkipWhitespace(CypherLexer *pLexer)`: Skips whitespace characters.
*   `void lexerSkipComment(CypherLexer *pLexer)`: Skips comments (e.g., `//` or `/* */`).
*   `CypherToken *lexerTokenizeIdentifier(CypherLexer *pLexer)`: Tokenizes identifiers and checks for keywords.
*   `CypherToken *lexerTokenizeNumber(CypherLexer *pLexer)`: Tokenizes integers and floats.
*   `CypherToken *lexerTokenizeString(CypherLexer *pLexer)`: Tokenizes string literals (handles escapes).
*   `CypherToken *lexerAddToken(CypherLexer *pLexer, CypherTokenType type, int startPos, int endPos)`: Creates and adds a token to the stream.
*   `void lexerSetError(CypherLexer *pLexer, const char *zFormat, ...)`: Sets an error message.
*   `CypherTokenType cypherGetKeywordToken(const char *zKeyword)`: Maps a string to a keyword token type.

## 5. TODOs
- [ ] Define `CypherTokenType` enum in `include/cypher.h`.
- [ ] Define `CypherToken` struct in `include/cypher.h`.
- [ ] Define `CypherLexer` struct in `include/cypher.h`.
- [ ] Implement `cypherLexerCreate` in `src/cypher/cypher-lexer.c`.
- [ ] Implement `cypherLexerDestroy` in `src/cypher/cypher-lexer.c`.
- [ ] Implement `lexerPeek` and `lexerNext` helper functions.
- [ ] Implement `lexerSkipWhitespace` and `lexerSkipComment`.
- [ ] Implement `lexerTokenizeIdentifier` (including keyword lookup).
- [ ] Implement `lexerTokenizeNumber` (for integers and floats).
- [ ] Implement `lexerTokenizeString` (handle single and double quotes, escape sequences).
- [ ] Implement `lexerAddToken`.
- [ ] Implement `lexerSetError`.
- [ ] Implement `cypherGetKeywordToken` (using a lookup table or similar).
- [ ] Implement `cypherLexerNextToken` to orchestrate tokenization.
- [ ] Add unit tests for all lexer functionalities (valid tokens, invalid input, edge cases).
- [ ] Integrate lexer with the parser (Lemon's `yylex` equivalent).
