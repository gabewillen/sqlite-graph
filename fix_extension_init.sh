#!/bin/bash
# Fix SQLITE_EXTENSION_INIT1 in all source files
# Only graph.c should have SQLITE_EXTENSION_INIT1

echo "Fixing SQLITE_EXTENSION_INIT1 in all source files..."

# Create backup directory
mkdir -p src_backup
cp -r src/* src_backup/

# List of files to fix (all except graph.c)
FILES_TO_FIX=(
    "src/graph-vtab.c"
    "src/graph-algo.c"
    "src/graph-traverse.c"
    "src/graph-util.c"
    "src/graph-json.c"
    "src/graph-tvf.c"
    "src/graph-advanced.c"
    "src/graph-schema.c"
    "src/graph-enhanced.c"
    "src/graph-performance.c"
    "src/graph-benchmark.c"
    "src/graph-parallel.c"
    "src/graph-cache.c"
    "src/graph-compress.c"
    "src/graph-bulk.c"
    "src/cypher/cypher-lexer.c"
    "src/cypher/cypher-ast.c"
    "src/cypher/cypher-parser.c"
    "src/cypher/cypher-sql.c"
    "src/cypher/cypher-logical-plan.c"
    "src/cypher/cypher-physical-plan.c"
    "src/cypher/cypher-planner.c"
    "src/cypher/cypher-planner-sql.c"
    "src/cypher/cypher-execution-context.c"
    "src/cypher/cypher-iterators.c"
    "src/cypher/cypher-executor.c"
    "src/cypher/cypher-executor-sql.c"
    "src/cypher/cypher-expressions.c"
    "src/cypher/cypher-write.c"
    "src/cypher/cypher-write-sql.c"
)

# Fix each file
for file in "${FILES_TO_FIX[@]}"; do
    if [ -f "$file" ]; then
        echo "Fixing $file..."
        # Replace SQLITE_EXTENSION_INIT1 with a comment
        sed -i 's/^SQLITE_EXTENSION_INIT1$/\/* SQLITE_EXTENSION_INIT1 - removed to prevent multiple definition *\//' "$file"
        
        # Add extern declaration for sqlite3_api after sqlite3ext.h include
        sed -i '/^#include "sqlite3ext.h"$/a\
#ifndef SQLITE_CORE\
extern const sqlite3_api_routines *sqlite3_api;\
#endif' "$file"
    fi
done

echo "Fixed ${#FILES_TO_FIX[@]} files"
echo "Backup saved in src_backup/"