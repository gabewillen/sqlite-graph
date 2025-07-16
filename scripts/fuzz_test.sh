#!/bin/bash

# Fuzzing Script for SQLite Graph Extension
# Uses AFL++ to test for crash resistance and security vulnerabilities

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "=== SQLite Graph Extension Fuzzing Tests ==="
echo ""

# Change to project root
cd "$(dirname "$0")/.."

# Results directory
FUZZ_DIR="fuzz_results"
mkdir -p "$FUZZ_DIR"

# Function to print status
print_status() {
    local status=$1
    local message=$2
    if [ "$status" = "PASS" ]; then
        echo -e "${GREEN}✓ $message${NC}"
    elif [ "$status" = "FAIL" ]; then
        echo -e "${RED}✗ $message${NC}"
    elif [ "$status" = "WARN" ]; then
        echo -e "${YELLOW}⚠ $message${NC}"
    else
        echo "  $message"
    fi
}

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Check for AFL++
if ! command_exists afl-fuzz; then
    print_status "FAIL" "AFL++ not found - install with: sudo apt-get install afl++"
    exit 1
fi

print_status "INFO" "AFL++ found"

# Create fuzzing target
echo "Creating fuzzing target..."

cat > "$FUZZ_DIR/fuzz_target.c" << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// Maximum input size for fuzzing
#define MAX_INPUT_SIZE 65536

// Fuzz target that exercises various SQL operations
int fuzz_sql_operations(const char *input, size_t input_len) {
    sqlite3 *db = NULL;
    char *zErrMsg = NULL;
    int rc;
    
    // Open in-memory database
    rc = sqlite3_open(":memory:", &db);
    if (rc != SQLITE_OK) {
        return -1;
    }
    
    // Load graph extension
    rc = sqlite3_load_extension(db, "./build/libgraph.so", NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return -1;
    }
    
    // Create test graph
    rc = sqlite3_exec(db, "CREATE VIRTUAL TABLE fuzz_graph USING graph()", NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return -1;
    }
    
    // Execute the fuzzed input as SQL
    rc = sqlite3_exec(db, input, NULL, NULL, &zErrMsg);
    
    // Clean up
    sqlite3_free(zErrMsg);
    sqlite3_close(db);
    
    return 0;
}

// Fuzz target for Cypher queries
int fuzz_cypher_queries(const char *input, size_t input_len) {
    sqlite3 *db = NULL;
    char *zErrMsg = NULL;
    int rc;
    
    rc = sqlite3_open(":memory:", &db);
    if (rc != SQLITE_OK) {
        return -1;
    }
    
    rc = sqlite3_load_extension(db, "./build/libgraph.so", NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return -1;
    }
    
    rc = sqlite3_exec(db, "CREATE VIRTUAL TABLE cypher_graph USING graph()", NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return -1;
    }
    
    // Try to execute as Cypher query using extension function
    char query_sql[MAX_INPUT_SIZE + 100];
    snprintf(query_sql, sizeof(query_sql), "SELECT cypher_query('cypher_graph', '%.*s')", 
             (int)input_len, input);
    
    rc = sqlite3_exec(db, query_sql, NULL, NULL, &zErrMsg);
    
    sqlite3_free(zErrMsg);
    sqlite3_close(db);
    
    return 0;
}

// Fuzz target for JSON property parsing
int fuzz_json_properties(const char *input, size_t input_len) {
    sqlite3 *db = NULL;
    char *zErrMsg = NULL;
    int rc;
    
    rc = sqlite3_open(":memory:", &db);
    if (rc != SQLITE_OK) {
        return -1;
    }
    
    rc = sqlite3_load_extension(db, "./build/libgraph.so", NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return -1;
    }
    
    rc = sqlite3_exec(db, "CREATE VIRTUAL TABLE json_graph USING graph()", NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return -1;
    }
    
    // Insert node with fuzzed JSON properties
    char insert_sql[MAX_INPUT_SIZE + 200];
    snprintf(insert_sql, sizeof(insert_sql), 
             "INSERT INTO json_graph (node_id, labels, properties) VALUES (1, '[\"Test\"]', '%.*s')", 
             (int)input_len, input);
    
    rc = sqlite3_exec(db, insert_sql, NULL, NULL, &zErrMsg);
    
    sqlite3_free(zErrMsg);
    sqlite3_close(db);
    
    return 0;
}

int main(int argc, char **argv) {
    char input[MAX_INPUT_SIZE];
    size_t input_len;
    
    // Read input from stdin (AFL++ standard)
    input_len = fread(input, 1, sizeof(input) - 1, stdin);
    if (input_len == 0) {
        return 0;
    }
    
    input[input_len] = '\0';
    
    // Run different fuzz targets based on input characteristics
    if (input_len > 0) {
        switch (input[0] % 3) {
            case 0:
                fuzz_sql_operations(input, input_len);
                break;
            case 1:
                fuzz_cypher_queries(input, input_len);
                break;
            case 2:
                fuzz_json_properties(input, input_len);
                break;
        }
    }
    
    return 0;
}
EOF

# Compile fuzzing target with AFL++ instrumentation
echo "Compiling fuzzing target with AFL++ instrumentation..."
afl-gcc -o "$FUZZ_DIR/fuzz_target" "$FUZZ_DIR/fuzz_target.c" -lsqlite3 -ldl

if [ ! -f "$FUZZ_DIR/fuzz_target" ]; then
    print_status "FAIL" "Failed to compile fuzzing target"
    exit 1
fi

print_status "PASS" "Fuzzing target compiled successfully"

# Create test case directory
mkdir -p "$FUZZ_DIR/testcases"

# Create initial test cases
echo "Creating initial test cases..."

# SQL test cases
cat > "$FUZZ_DIR/testcases/sql_basic.sql" << 'EOF'
INSERT INTO fuzz_graph (node_id, labels, properties) VALUES (1, '["Test"]', '{"key":"value"}');
EOF

cat > "$FUZZ_DIR/testcases/sql_edge.sql" << 'EOF'
INSERT INTO fuzz_graph (edge_id, from_id, to_id, edge_type, weight) VALUES (1, 1, 2, 'CONNECTS', 1.0);
EOF

# Cypher test cases
cat > "$FUZZ_DIR/testcases/cypher_match.cypher" << 'EOF'
MATCH (n) RETURN n;
EOF

cat > "$FUZZ_DIR/testcases/cypher_create.cypher" << 'EOF'
CREATE (n:Person {name: "Test"});
EOF

# JSON test cases
cat > "$FUZZ_DIR/testcases/json_simple.json" << 'EOF'
{"name": "test", "value": 123}
EOF

cat > "$FUZZ_DIR/testcases/json_complex.json" << 'EOF'
{"nested": {"array": [1, 2, 3], "object": {"key": "value"}}}
EOF

# Edge cases
cat > "$FUZZ_DIR/testcases/edge_empty.txt" << 'EOF'
EOF

cat > "$FUZZ_DIR/testcases/edge_long.txt" << 'EOF'
aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
EOF

cat > "$FUZZ_DIR/testcases/edge_special.txt" << 'EOF'
';DROP TABLE fuzz_graph;--
EOF

print_status "PASS" "Test cases created"

# Run quick fuzzing session
echo ""
echo "=== Running Fuzzing Session ==="
echo "Note: Running short session for testing (60 seconds)"
echo "For production, run longer sessions (hours/days)"

# Create output directory
mkdir -p "$FUZZ_DIR/findings"

# Set AFL++ environment
export AFL_SKIP_CPUFREQ=1
export AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=1

# Run AFL++ for a short time
timeout 60s afl-fuzz -i "$FUZZ_DIR/testcases" -o "$FUZZ_DIR/findings" -t 5000 -- "$FUZZ_DIR/fuzz_target" 2>&1 || {
    print_status "INFO" "Fuzzing session completed (60 seconds)"
}

# Check results
echo ""
echo "=== Analyzing Fuzzing Results ==="

if [ -d "$FUZZ_DIR/findings/default/crashes" ] && [ "$(ls -A $FUZZ_DIR/findings/default/crashes 2>/dev/null)" ]; then
    CRASH_COUNT=$(ls "$FUZZ_DIR/findings/default/crashes" | wc -l)
    print_status "FAIL" "Found $CRASH_COUNT crashes during fuzzing"
    
    echo "Crash samples:"
    for crash in "$FUZZ_DIR/findings/default/crashes"/*; do
        if [ -f "$crash" ]; then
            echo "  $(basename "$crash"): $(head -c 50 "$crash" | tr '\n' ' ')..."
        fi
    done
else
    print_status "PASS" "No crashes found during fuzzing"
fi

if [ -d "$FUZZ_DIR/findings/default/hangs" ] && [ "$(ls -A $FUZZ_DIR/findings/default/hangs 2>/dev/null)" ]; then
    HANG_COUNT=$(ls "$FUZZ_DIR/findings/default/hangs" | wc -l)
    print_status "WARN" "Found $HANG_COUNT hangs during fuzzing"
else
    print_status "PASS" "No hangs found during fuzzing"
fi

# Show stats
if [ -f "$FUZZ_DIR/findings/default/fuzzer_stats" ]; then
    echo ""
    echo "Fuzzing Statistics:"
    grep -E "(execs_done|paths_total|unique_crashes|unique_hangs)" "$FUZZ_DIR/findings/default/fuzzer_stats" || true
fi

# Create fuzzing report
cat > "$FUZZ_DIR/fuzz_report.txt" << EOF
SQLite Graph Extension Fuzzing Report
=====================================

Date: $(date)
Duration: 60 seconds (test run)
Target: SQLite Graph Extension

Test Coverage:
- SQL operations (INSERT, SELECT, etc.)
- Cypher query parsing
- JSON property parsing
- Edge cases and malformed input

Results:
- Crashes: $([ -d "$FUZZ_DIR/findings/default/crashes" ] && ls "$FUZZ_DIR/findings/default/crashes" 2>/dev/null | wc -l || echo "0")
- Hangs: $([ -d "$FUZZ_DIR/findings/default/hangs" ] && ls "$FUZZ_DIR/findings/default/hangs" 2>/dev/null | wc -l || echo "0")
- Total executions: $(grep "execs_done" "$FUZZ_DIR/findings/default/fuzzer_stats" 2>/dev/null | cut -d: -f2 | tr -d ' ' || echo "N/A")

Recommendation:
For production deployment, run fuzzing for extended periods (24+ hours)
to discover edge cases and potential security vulnerabilities.

Files:
- Crash samples: $FUZZ_DIR/findings/default/crashes/
- Hang samples: $FUZZ_DIR/findings/default/hangs/
- Full stats: $FUZZ_DIR/findings/default/fuzzer_stats
EOF

echo ""
echo "=== Fuzzing Summary ==="
cat "$FUZZ_DIR/fuzz_report.txt"

# Check if any critical issues found
CRITICAL_ISSUES=0

if [ -d "$FUZZ_DIR/findings/default/crashes" ] && [ "$(ls -A $FUZZ_DIR/findings/default/crashes 2>/dev/null)" ]; then
    CRITICAL_ISSUES=$((CRITICAL_ISSUES + 1))
fi

if [ $CRITICAL_ISSUES -eq 0 ]; then
    print_status "PASS" "No critical security issues found in short fuzzing session"
    exit 0
else
    print_status "FAIL" "Found $CRITICAL_ISSUES critical security issues - investigate crashes"
    exit 1
fi