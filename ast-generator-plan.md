# AST Generator Plan

## 1. Objective
To define the structure of the Abstract Syntax Tree (AST) for Cypher queries and implement functions for creating, manipulating, and destroying AST nodes.

## 2. Design Considerations
*   **Node Types:** Define an enum for all possible AST node types (e.g., `QUERY`, `MATCH`, `NODE_PATTERN`, `LITERAL`, `BINARY_OP`).
*   **Node Structure:** Each AST node should contain its type, a value (if applicable, e.g., for literals or identifiers), and a list of child nodes.
*   **Memory Management:** All AST nodes and their internal data (like string values) must be allocated and freed properly to prevent memory leaks.
*   **Debugging/Visualization:** Include a function to print the AST for debugging purposes.

## 3. AST Definition (`include/cypher.h`)
```c
// AST Node Types
typedef enum {
    CYPHER_AST_QUERY = 0,
    CYPHER_AST_SINGLE_QUERY,
    CYPHER_AST_UNION,
    CYPHER_AST_MATCH,
    CYPHER_AST_OPTIONAL_MATCH,
    CYPHER_AST_WHERE,
    CYPHER_AST_RETURN,
    CYPHER_AST_PROJECTION_LIST,
    CYPHER_AST_PROJECTION_ITEM,
    CYPHER_AST_ORDER_BY,
    CYPHER_AST_SORT_LIST,
    CYPHER_AST_SORT_ITEM,
    CYPHER_AST_LIMIT,
    CYPHER_AST_SKIP,
    CYPHER_AST_PATTERN,
    CYPHER_AST_NODE_PATTERN,
    CYPHER_AST_REL_PATTERN,
    CYPHER_AST_LABELS,
    CYPHER_AST_PATH,
    CYPHER_AST_IDENTIFIER,
    CYPHER_AST_LITERAL,
    CYPHER_AST_UNARY_OP,
    CYPHER_AST_BINARY_OP,
    CYPHER_AST_PROPERTY,
    CYPHER_AST_MAP,
    CYPHER_AST_LIST,
    CYPHER_AST_FUNCTION_CALL,
    CYPHER_AST_CASE,
    CYPHER_AST_COUNT // Sentinel for max AST node type
} CypherAstNodeType;

// AST Node Structure
typedef struct CypherAst {
    CypherAstNodeType type;
    char *zValue; // Value for literals, identifiers, operators (e.g., "+", "Person")
    struct CypherAst **apChildren; // Array of child AST nodes
    int nChildren; // Number of children
    int nChildrenAlloc; // Allocated size of apChildren
    int iLine; // Line number from source
    int iColumn; // Column number from source
    int iFlags; // General purpose flags (e.g., DISTINCT for RETURN clause)
} CypherAst;

// AST Node creation functions
CypherAst *cypherAstCreate(CypherAstNodeType type, int iLine, int iColumn);
CypherAst *cypherAstCreateIdentifier(const char *zName, int iLine, int iColumn);
CypherAst *cypherAstCreateLiteral(const char *zValue, int iLine, int iColumn);
CypherAst *cypherAstCreateBinaryOp(const char *zOp, CypherAst *pLeft, CypherAst *pRight, int iLine, int iColumn);
CypherAst *cypherAstCreateProperty(CypherAst *pObj, const char *zProp, int iLine, int iColumn);

// AST Node manipulation functions
void cypherAstAddChild(CypherAst *pParent, CypherAst *pChild);
void cypherAstSetValue(CypherAst *pNode, const char *zValue);
CypherAst *cypherAstGetChild(CypherAst *pNode, int iChild);
int cypherAstGetChildCount(CypherAst *pNode);
const char *cypherAstGetValue(CypherAst *pNode);

// AST Node destruction
void cypherAstDestroy(CypherAst *pNode);

// Debugging
void cypherAstPrint(CypherAst *pNode, int iIndent);
const char *cypherAstNodeTypeName(CypherAstNodeType type);
```

## 4. AST Generator Functions (`src/cypher/cypher-ast.c`)

### `CypherAst *cypherAstCreate(CypherAstNodeType type, int iLine, int iColumn)`
*   Allocates and initializes a new `CypherAst` node.
*   Sets `type`, `iLine`, `iColumn`.
*   Initializes `zValue` to NULL, `apChildren` to NULL, `nChildren` and `nChildrenAlloc` to 0.

### `CypherAst *cypherAstCreateIdentifier(const char *zName, int iLine, int iColumn)`
*   Creates an `IDENTIFIER` AST node.
*   Copies `zName` to `zValue`.

### `CypherAst *cypherAstCreateLiteral(const char *zValue, int iLine, int iColumn)`
*   Creates a `LITERAL` AST node.
*   Copies `zValue` to `zValue`.

### `CypherAst *cypherAstCreateBinaryOp(const char *zOp, CypherAst *pLeft, CypherAst *pRight, int iLine, int iColumn)`
*   Creates a `BINARY_OP` AST node.
*   Sets `zValue` to `zOp`.
*   Adds `pLeft` and `pRight` as children.

### `CypherAst *cypherAstCreateProperty(CypherAst *pObj, const char *zProp, int iLine, int iColumn)`
*   Creates a `PROPERTY` AST node.
*   Adds `pObj` as the first child.
*   Sets `zValue` to `zProp`.

### `void cypherAstAddChild(CypherAst *pParent, CypherAst *pChild)`
*   Adds `pChild` to `pParent->apChildren`.
*   Dynamically resizes `apChildren` array if necessary.

### `void cypherAstSetValue(CypherAst *pNode, const char *zValue)`
*   Sets or updates the `zValue` of an AST node.
*   Frees existing `zValue` if present and copies new `zValue`.

### `CypherAst *cypherAstGetChild(CypherAst *pNode, int iChild)`
*   Returns the child node at `iChild` index.

### `int cypherAstGetChildCount(CypherAst *pNode)`
*   Returns the number of children of `pNode`.

### `const char *cypherAstGetValue(CypherAst *pNode)`
*   Returns the `zValue` of an AST node.

### `void cypherAstDestroy(CypherAst *pNode)`
*   Recursively frees all memory associated with the AST, starting from `pNode`.
*   Frees `zValue` and `apChildren` for each node.

### `void cypherAstPrint(CypherAst *pNode, int iIndent)`
*   Prints the AST structure to stdout for debugging.

### `const char *cypherAstNodeTypeName(CypherAstNodeType type)`
*   Returns a string representation of `CypherAstNodeType`.

## 5. TODOs
- [ ] Define `CypherAstNodeType` enum in `include/cypher.h`.
- [ ] Define `CypherAst` struct in `include/cypher.h`.
- [ ] Declare all AST creation, manipulation, and destruction functions in `include/cypher.h`.
- [ ] Implement `cypherAstCreate` in `src/cypher/cypher-ast.c`.
- [ ] Implement `cypherAstCreateIdentifier`.
- [ ] Implement `cypherAstCreateLiteral`.
- [ ] Implement `cypherAstCreateBinaryOp`.
- [ ] Implement `cypherAstCreateProperty`.
- [ ] Implement `cypherAstAddChild` (including dynamic array resizing).
- [ ] Implement `cypherAstSetValue`.
- [ ] Implement `cypherAstGetChild`.
- [ ] Implement `cypherAstGetChildCount`.
- [ ] Implement `cypherAstGetValue`.
- [ ] Implement `cypherAstDestroy` (recursive).
- [ ] Implement `cypherAstPrint` for debugging.
- [ ] Implement `cypherAstNodeTypeName`.
- [ ] Add unit tests for AST creation, manipulation, and destruction.
- [ ] Ensure all memory allocations are paired with deallocations.
