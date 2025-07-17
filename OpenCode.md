# OpenCode - sqlite-graph

This document provides guidelines for contributing to this project.

## Build & Test

*   **Build:** `make`
*   **Run all tests:** `make test` or `ctest --output-on-failure`
*   **Run a single test:** `./build/test_your_test_name`
*   **Clean:** `make clean`

## Code Style

*   **Language:** C (C99 standard).
*   **Formatting:**
    *   80 character line limit.
    *   2-space indentation.
*   **Naming:**
    *   Use `snake_case` for functions and variables.
    *   Prefix public functions with `graph_`.
*   **Error Handling:**
    *   Functions should return `GRAPH_SUCCESS` or an error code.
*   **Comments:**
    *   Use `//` for single-line comments.
    *   Use `/* ... */` for multi-line comments.
*   **Headers:**
    *   Include guards should be used in all header files.
    *   Format: `#ifndef MY_HEADER_H`.
*   **Dependencies:**
    *   Uses Unity for testing.
    *   Based on SQLite.
