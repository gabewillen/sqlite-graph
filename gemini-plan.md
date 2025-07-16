# Gemini Development Plan for sqlite-graph

## 1. Project Status Summary

This project is a C-based extension for SQLite designed to provide graph database capabilities, including a Cypher query engine.

**Current State:**
- **Foundation:** The project has a solid architectural foundation, using a standard SQLite virtual table mechanism.
- **Code Quality:** There is a strong emphasis on engineering best practices, evidenced by a comprehensive testing and analysis infrastructure (`tests/`, `benchmarks/`, static analysis, memory checking).
- **Build System:** A CMake-based build system is in place, though accompanied by several shell scripts for various build configurations.
- **Core Implementation:** The core graph functionality is **incomplete**.
    - The virtual table (`graph-vtab.c`) provides an in-memory graph data structure but does not persist data.
    - The numerous registered graph algorithms (`graph.c`) are currently placeholder stubs that return errors.
    - The Cypher query parser (`cypher-parser.c`) is a temporary mock-up; it does not yet use the Lemon grammar to build a full Abstract Syntax Tree (AST).

## 2. High-Level Goals

The primary goal is to evolve the project from a promising foundation into a feature-complete, robust, and persistent graph database extension.

1.  **Complete Cypher Query Support:** Implement a full-featured query engine capable of parsing, planning, and executing Cypher queries.
2.  **Implement All Graph Algorithms:** Provide correct and efficient implementations for all defined graph analysis functions.
3.  **Introduce Data Persistence:** Re-architect the virtual table to store graph data in underlying SQLite tables, making it durable.
4.  **Enhance Query Optimization:** Improve the query planner to leverage indexes for efficient query execution.
5.  **Improve Developer Experience:** Clean up the build process, remove obsolete files, and create comprehensive documentation.

## 3. Detailed Development Plan (Phased Approach)

### Phase 1: Core Query Engine Completion (COMPLETE)

This phase focuses on making the Cypher query functionality operational.

-   **Task 1.1: Finalize the Cypher Parser (COMPLETE)**
    -   **Action:** Replace the temporary `strstr`-based parser in `cypher-parser.c` with a full implementation that uses the Lemon-generated parser (`cypher.lemon`).
    -   **Acceptance Criteria:**
        -   The parser correctly processes all clauses defined in the grammar (MATCH, WHERE, RETURN, ORDER BY, etc.).
        -   It generates a complete and accurate Abstract Syntax Tree (AST) for valid queries.
        -   It produces meaningful errors for invalid syntax.
        -   Unit tests in `tests/` are created to validate a wide range of Cypher queries and error conditions.

-   **Task 1.2: Implement the AST-to-Execution-Plan Planner (COMPLETE)**
    -   **Action:** Develop the logic in `cypher-planner.c`. This component will traverse the AST from the parser and translate it into a logical execution plan.
    -   **Acceptance Criteria:**
        -   The planner can walk the entire AST for a complex query.
        -   It generates a step-by-step plan that the executor can follow.
        -   The plan identifies which nodes/edges to scan, what filters to apply, and what data to return.

-   **Task 1.3: Build the Cypher Executor (COMPLETE)**
    -   **Action:** Implement the logic in `cypher-executor.c` to consume the execution plan from the planner and retrieve data from the virtual table.
    -   **Acceptance Criteria:**
        -   The executor correctly interacts with the `graph` virtual table's cursor (`graphFilter`, `graphNext`, etc.).
        -   It can process the full execution plan, including data retrieval, filtering, and projection.
        -   The results of a `MATCH ... RETURN` query are correctly formatted and returned to the user.

### Phase 2: Graph Algorithm and Function Implementation (COMPLETE)

This phase focuses on implementing the core graph manipulation and analysis features.

-   **Task 2.1: Implement Core Graph Data Functions (COMPLETE)**
    -   **Action:** Implement the logic for the placeholder functions in `src/graph.c` (e.g., `graph_node_add`, `graph_edge_add`).
    -   **Acceptance Criteria:**
        -   These functions must correctly modify the in-memory `GraphVtab` struct.
        -   A mechanism must be established to get a handle to the correct virtual table instance from within these SQL functions.
        -   Tests are added to verify node/edge creation and property handling.

-   **Task 2.2: Implement Graph Algorithms (COMPLETE)**
    -   **Action:** Systematically implement the library of graph algorithms (e.g., `graph_shortest_path`, `graph_pagerank`, `graph_connected_components`).
    -   **Acceptance Criteria:**
        -   Each algorithm is implemented efficiently.
        -   A dedicated test file (e.g., `tests/test_algorithms.c`) is created or expanded to verify the correctness of each algorithm against known graph structures and expected results.

### Phase 3: Persistence and Optimization (COMPLETE)

This phase transforms the extension from an in-memory tool to a durable database solution.

-   **Task 3.1: Implement Data Persistence (COMPLETE)**
    -   **Action:** Re-architect `graph-vtab.c`. Instead of using in-memory linked lists, the virtual table will act as a facade over standard SQLite tables.
    -   **Acceptance Criteria:**
        -   `xCreate`: Creates the necessary backing tables (e.g., `_graph_nodes`, `_graph_edges`) to store graph data.
        -   `xUpdate`: Implement this method to handle `INSERT`, `UPDATE`, and `DELETE` operations on the virtual table, which translate to modifications of the backing tables.
        -   `xFilter`/`xNext`: These methods will now issue `SELECT` queries against the backing tables.
        -   The graph data is now persistent across database connections.

-   **Task 3.2: Enhance the Query Optimizer (`graphBestIndex`) (COMPLETE)**
    -   **Action:** Improve the `graphBestIndex` function to provide intelligent query plans to SQLite.
    -   **Acceptance Criteria:**
        -   The function analyzes `WHERE` clause constraints to identify opportunities for indexed lookups (e.g., `WHERE id = ?` or `WHERE labels = ?`).
        -   It communicates different query strategies (full scan, index lookup) to SQLite via the `idxNum` and `estimatedCost` fields.
        -   Appropriate `CREATE INDEX` statements are issued on the backing tables to support these optimized lookups.

### Phase 4: Project Cleanup and Refinement (COMPLETE)

This phase focuses on improving usability, maintainability, and documentation.

-   **Task 4.1: Refactor the Build System (COMPLETE)**
    -   **Action:** Consolidate the various `build_*.sh` scripts. Move towards a single, well-documented `Makefile` or primary `CMakeLists.txt` target.
    -   **Acceptance Criteria:**
        -   There is a single, clear command for building the full extension.
        -   All build artifacts are generated exclusively within the `build/` directory, keeping the root directory clean.

-   **Task 4.2: Code and Repository Cleanup (COMPLETE)**
    -   **Action:** Remove obsolete files and directories.
    -   **Acceptance Criteria:**
        -   The `src_backup/` directory is deleted after confirming no unique code exists within it.
        -   The logic from one-off scripts like `fix_*.py` and `fix_*.sh` is integrated into the main build or testing framework, and the scripts are deleted.

-   **Task 4.3: Create Comprehensive Documentation (COMPLETE)**
    -   **Action:** Create a high-quality `README.md` and update the `docs/` directory.
    -   **Acceptance Criteria:**
        -   A `README.md` file is added to the project root. It contains a project overview, build instructions, test instructions, and basic usage examples.
        -   The documentation in `docs/` is updated to reflect the new persistent storage model and the complete set of features.
