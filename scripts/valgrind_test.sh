#!/bin/bash

# Valgrind Memory Testing Script for SQLite Graph Extension
# Tests for memory leaks, buffer overflows, and other memory issues

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "=== SQLite Graph Extension Valgrind Memory Testing ==="
echo ""

# Change to project root
cd "$(dirname "$0")/.."

# Results directory
RESULTS_DIR="valgrind_results"
mkdir -p "$RESULTS_DIR"

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

# Check for valgrind
if ! command_exists valgrind; then
    print_status "FAIL" "valgrind not found - install with: sudo apt-get install valgrind"
    exit 1
fi

print_status "INFO" "valgrind found"

# Build the extension and tests
echo "Building extension and tests..."
if ! make clean all >/dev/null 2>&1; then
    print_status "FAIL" "Failed to build extension"
    exit 1
fi

# Check if extension exists
if [ ! -f "build/libgraph.so" ]; then
    print_status "FAIL" "Extension library not found"
    exit 1
fi

print_status "PASS" "Extension built successfully"

# Create a simple test program for memory testing
cat > "$RESULTS_DIR/memory_test.c" << 'EOF'
#include <stdio.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>

int main() {
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;
    
    // Open database
    rc = sqlite3_open(":memory:", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }
    
    // Load graph extension
    rc = sqlite3_load_extension(db, "./build/libgraph.so", 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Can't load extension: %s\n", zErrMsg ? zErrMsg : sqlite3_errmsg(db));
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return 1;
    }
    
    // Create a test graph
    rc = sqlite3_exec(db, "CREATE VIRTUAL TABLE test_graph USING graph()", 0, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return 1;
    }
    
    // Insert test data
    const char *sql_nodes = 
        "INSERT INTO test_graph (node_id, labels, properties) VALUES "
        "(1, '[\"Person\"]', '{\"name\":\"Alice\",\"age\":30}'), "
        "(2, '[\"Person\"]', '{\"name\":\"Bob\",\"age\":25}'), "
        "(3, '[\"Person\"]', '{\"name\":\"Charlie\",\"age\":35}')";
    
    rc = sqlite3_exec(db, sql_nodes, 0, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return 1;
    }
    
    // Insert test edges
    const char *sql_edges = 
        "INSERT INTO test_graph (edge_id, from_id, to_id, edge_type, weight, properties) VALUES "
        "(101, 1, 2, 'KNOWS', 1.0, '{\"since\":\"2020\"}'), "
        "(102, 2, 3, 'KNOWS', 1.0, '{\"since\":\"2021\"}')";
    
    rc = sqlite3_exec(db, sql_edges, 0, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return 1;
    }
    
    // Test queries
    sqlite3_stmt *stmt;
    
    // Test node lookup
    rc = sqlite3_prepare_v2(db, "SELECT * FROM test_graph WHERE node_id = ?", -1, &stmt, 0);
    if (rc == SQLITE_OK) {
        for (int i = 1; i <= 3; i++) {
            sqlite3_bind_int(stmt, 1, i);
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                // Process row
            }
            sqlite3_reset(stmt);
        }
        sqlite3_finalize(stmt);
    }
    
    // Test traversal
    rc = sqlite3_prepare_v2(db, "SELECT * FROM test_graph WHERE from_id = ?", -1, &stmt, 0);
    if (rc == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, 1);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            // Process row
        }
        sqlite3_finalize(stmt);
    }
    
    printf("Memory test completed successfully\n");
    
    sqlite3_close(db);
    return 0;
}
EOF

# Compile test program
echo "Compiling memory test program..."
gcc -o "$RESULTS_DIR/memory_test" "$RESULTS_DIR/memory_test.c" -lsqlite3 -ldl

# Test 1: Basic memory leak check
echo ""
echo "=== Test 1: Memory Leak Detection ==="
valgrind --tool=memcheck \
         --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --error-exitcode=1 \
         --log-file="$RESULTS_DIR/memcheck.log" \
         "$RESULTS_DIR/memory_test" 2>&1

MEMCHECK_EXIT=$?

if [ $MEMCHECK_EXIT -eq 0 ]; then
    print_status "PASS" "No memory leaks detected"
else
    print_status "FAIL" "Memory leaks detected"
fi

# Check for specific leak types
if grep -q "definitely lost" "$RESULTS_DIR/memcheck.log"; then
    DEFINITELY_LOST=$(grep "definitely lost" "$RESULTS_DIR/memcheck.log" | head -1)
    print_status "FAIL" "Definite memory leaks: $DEFINITELY_LOST"
else
    print_status "PASS" "No definite memory leaks"
fi

if grep -q "possibly lost" "$RESULTS_DIR/memcheck.log"; then
    POSSIBLY_LOST=$(grep "possibly lost" "$RESULTS_DIR/memcheck.log" | head -1)
    print_status "WARN" "Possible memory leaks: $POSSIBLY_LOST"
fi

# Test 2: Buffer overflow detection  
echo ""
echo "=== Test 2: Buffer Overflow Detection ==="

# Create a test program that exercises string operations
cat > "$RESULTS_DIR/buffer_test.c" << 'EOF'
#include <stdio.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>

int main() {
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;
    
    rc = sqlite3_open(":memory:", &db);
    if (rc != SQLITE_OK) return 1;
    
    rc = sqlite3_load_extension(db, "./build/libgraph.so", 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return 1;
    }
    
    rc = sqlite3_exec(db, "CREATE VIRTUAL TABLE buf_test USING graph()", 0, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return 1;
    }
    
    // Test with various string lengths
    char large_property[2048];
    memset(large_property, 'A', sizeof(large_property) - 1);
    large_property[sizeof(large_property) - 1] = '\0';
    
    char sql[4096];
    snprintf(sql, sizeof(sql), 
        "INSERT INTO buf_test (node_id, labels, properties) VALUES "
        "(1, '[\"Test\"]', '{\"data\":\"%s\"}');", large_property);
    
    rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        sqlite3_free(zErrMsg);
    }
    
    printf("Buffer test completed\n");
    
    sqlite3_close(db);
    return 0;
}
EOF

gcc -o "$RESULTS_DIR/buffer_test" "$RESULTS_DIR/buffer_test.c" -lsqlite3 -ldl

valgrind --tool=memcheck \
         --track-origins=yes \
         --error-exitcode=1 \
         --log-file="$RESULTS_DIR/buffer_check.log" \
         "$RESULTS_DIR/buffer_test" 2>&1

BUFFER_EXIT=$?

if [ $BUFFER_EXIT -eq 0 ]; then
    print_status "PASS" "No buffer overflows detected"
else
    print_status "FAIL" "Buffer overflows detected"
fi

# Test 3: Performance with Cachegrind
echo ""
echo "=== Test 3: Performance Analysis with Cachegrind ==="

valgrind --tool=cachegrind \
         --log-file="$RESULTS_DIR/cachegrind.log" \
         "$RESULTS_DIR/memory_test" 2>&1

print_status "INFO" "Cachegrind analysis complete"

# Test 4: Heap usage with Massif
echo ""
echo "=== Test 4: Heap Usage Analysis with Massif ==="

valgrind --tool=massif \
         --massif-out-file="$RESULTS_DIR/massif.out" \
         "$RESULTS_DIR/memory_test" 2>&1

print_status "INFO" "Massif heap analysis complete"

# Test 5: Thread safety with Helgrind
echo ""
echo "=== Test 5: Thread Safety Analysis with Helgrind ==="

# Create a multi-threaded test
cat > "$RESULTS_DIR/thread_test.c" << 'EOF'
#include <stdio.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

sqlite3 *global_db = NULL;

void* worker_thread(void* arg) {
    int thread_id = *(int*)arg;
    sqlite3_stmt *stmt;
    char sql[256];
    
    // Each thread inserts some nodes
    snprintf(sql, sizeof(sql), 
        "INSERT INTO thread_test (node_id, labels, properties) VALUES "
        "(%d, '[\"Worker\"]', '{\"thread\":%d}')", 
        1000 + thread_id, thread_id);
    
    int rc = sqlite3_exec(global_db, sql, 0, 0, NULL);
    
    // Query data
    rc = sqlite3_prepare_v2(global_db, "SELECT COUNT(*) FROM thread_test", -1, &stmt, 0);
    if (rc == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int count = sqlite3_column_int(stmt, 0);
            printf("Thread %d sees %d rows\n", thread_id, count);
        }
        sqlite3_finalize(stmt);
    }
    
    return NULL;
}

int main() {
    char *zErrMsg = 0;
    int rc;
    
    rc = sqlite3_open(":memory:", &global_db);
    if (rc != SQLITE_OK) return 1;
    
    rc = sqlite3_load_extension(global_db, "./build/libgraph.so", 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        sqlite3_free(zErrMsg);
        sqlite3_close(global_db);
        return 1;
    }
    
    rc = sqlite3_exec(global_db, "CREATE VIRTUAL TABLE thread_test USING graph()", 0, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        sqlite3_free(zErrMsg);
        sqlite3_close(global_db);
        return 1;
    }
    
    // Create multiple threads
    pthread_t threads[4];
    int thread_ids[4];
    
    for (int i = 0; i < 4; i++) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, worker_thread, &thread_ids[i]);
    }
    
    // Wait for threads
    for (int i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("Thread test completed\n");
    
    sqlite3_close(global_db);
    return 0;
}
EOF

gcc -o "$RESULTS_DIR/thread_test" "$RESULTS_DIR/thread_test.c" -lsqlite3 -ldl -lpthread

valgrind --tool=helgrind \
         --error-exitcode=1 \
         --log-file="$RESULTS_DIR/helgrind.log" \
         "$RESULTS_DIR/thread_test" 2>&1

HELGRIND_EXIT=$?

if [ $HELGRIND_EXIT -eq 0 ]; then
    print_status "PASS" "No thread safety issues detected"
else
    print_status "FAIL" "Thread safety issues detected"
fi

# Summary
echo ""
echo "=== Valgrind Testing Summary ==="

TOTAL_ISSUES=0

if [ $MEMCHECK_EXIT -ne 0 ]; then
    TOTAL_ISSUES=$((TOTAL_ISSUES + 1))
fi

if [ $BUFFER_EXIT -ne 0 ]; then
    TOTAL_ISSUES=$((TOTAL_ISSUES + 1))
fi

if [ $HELGRIND_EXIT -ne 0 ]; then
    TOTAL_ISSUES=$((TOTAL_ISSUES + 1))
fi

echo "Results saved in: $RESULTS_DIR/"
echo ""
echo "Test Results:"
echo "- Memory leak check: $([ $MEMCHECK_EXIT -eq 0 ] && echo "PASS" || echo "FAIL")"
echo "- Buffer overflow check: $([ $BUFFER_EXIT -eq 0 ] && echo "PASS" || echo "FAIL")"
echo "- Thread safety check: $([ $HELGRIND_EXIT -eq 0 ] && echo "PASS" || echo "FAIL")"

if [ $TOTAL_ISSUES -eq 0 ]; then
    print_status "PASS" "All valgrind tests passed"
    exit 0
else
    print_status "FAIL" "Found $TOTAL_ISSUES critical memory issues"
    exit 1
fi