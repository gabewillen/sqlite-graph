# AGENT.md - SQLite Graph Database Extension

## Build Commands
- **Build all:** `make` or `cmake --build build`
- **Clean:** `make clean` or `rm -rf build`
- **Test all:** `make test` or `./tests/run_all_tests.sh`
- **Single test:** `cd build/tests && ./test_<name>` (e.g., `./test_cypher_basic`)
- **Coverage:** `cmake --build build --target coverage`

## Architecture
- **Main Extension:** `src/graph.c` - SQLite extension entry point
- **Virtual Table:** `src/graph-vtab.c` - Graph virtual table implementation
- **Cypher Engine:** `src/cypher/` - Query parser, planner, executor
- **Algorithms:** `src/graph-algo.c` - Graph algorithms (shortest path, PageRank, etc.)
- **Tests:** `tests/` - Unit tests using Unity framework
- **Headers:** `include/` - Public API definitions

## Code Style (SQLite Standard)
- **Standard:** C99 with GNU extensions (`-std=gnu99`)
- **Indent:** 2 spaces, 80 character lines, ANSI C-89 style
- **Naming:** `snake_case` for functions, `UPPER_CASE` for macros
- **Headers:** Use `UNUSED(x)` macro for unused parameters
- **Warnings:** Suppress unused variable warnings with pragmas
- **Compilation:** `-fPIC -g -O0` for debug, `-O3 -DNDEBUG` for release
