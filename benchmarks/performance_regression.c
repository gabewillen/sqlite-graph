/*
 * Performance Regression Test Suite
 * SQLite Graph Extension
 * 
 * This test suite validates that performance meets established baselines
 * and detects performance regressions across releases.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>
#include <sqlite3.h>
#include <unistd.h>
#include <sys/resource.h>

/* Graph extension initialization function */
extern int sqlite3_graph_init(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi);

/* Performance targets (in milliseconds) */
#define TARGET_NODE_LOOKUP_MS 1.0
#define TARGET_1HOP_TRAVERSAL_MS 5.0
#define TARGET_3HOP_TRAVERSAL_MS 50.0
#define TARGET_SHORTEST_PATH_MS 100.0
#define TARGET_PAGERANK_MS 1000.0
#define TARGET_BULK_LOAD_NODES_PER_SEC 10000

/* Test configuration */
#define TEST_NODES_SMALL 1000
#define TEST_NODES_MEDIUM 10000
#define TEST_NODES_LARGE 100000
#define TEST_EDGES_RATIO 3  /* Average edges per node */

/* Performance test result */
typedef struct PerfResult {
    char *test_name;
    double execution_time_ms;
    double target_time_ms;
    int passed;
    double memory_mb;
    int result_count;
    char *error_message;
} PerfResult;

/* Utility functions */
static double get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

static double get_memory_mb(void) {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss / 1024.0;  /* Convert KB to MB */
}

static PerfResult *create_result(const char *test_name, double target_ms) {
    PerfResult *result = calloc(1, sizeof(PerfResult));
    if (result) {
        result->test_name = strdup(test_name);
        result->target_time_ms = target_ms;
        result->passed = 0;
        result->memory_mb = 0.0;
        result->result_count = 0;
        result->error_message = NULL;
    }
    return result;
}

static void free_result(PerfResult *result) {
    if (result) {
        free(result->test_name);
        free(result->error_message);
        free(result);
    }
}

/* Database setup functions */
static int create_test_graph(sqlite3 *db, const char *graph_name) {
    char sql[256];
    snprintf(sql, sizeof(sql), 
        "CREATE VIRTUAL TABLE %s USING graph("
        "cache_size=10000, max_depth=10, thread_pool_size=4)", 
        graph_name);
    
    char *zErrMsg = NULL;
    int rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error creating graph %s: %s\n", graph_name, zErrMsg);
        sqlite3_free(zErrMsg);
    }
    return rc;
}

static int load_test_data(sqlite3 *db, const char *graph_name, int num_nodes) {
    printf("Loading %d nodes into %s...\n", num_nodes, graph_name);
    
    char sql[512];
    sqlite3_stmt *stmt;
    
    /* Insert nodes */
    snprintf(sql, sizeof(sql),
        "INSERT INTO %s (node_id, labels, properties) VALUES (?, ?, ?)",
        graph_name);
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return rc;
    
    sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);
    
    for (int i = 1; i <= num_nodes; i++) {
        char properties[256];
        snprintf(properties, sizeof(properties),
            "{\"id\":%d,\"value\":%d,\"group\":%d}",
            i, rand() % 1000, i % 10);
        
        sqlite3_bind_int64(stmt, 1, i);
        sqlite3_bind_text(stmt, 2, "[\"Node\"]", -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, properties, -1, SQLITE_TRANSIENT);
        
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            fprintf(stderr, "Error inserting node %d\n", i);
            break;
        }
        sqlite3_reset(stmt);
    }
    
    sqlite3_finalize(stmt);
    
    /* Insert edges */
    snprintf(sql, sizeof(sql),
        "INSERT INTO %s (edge_id, from_id, to_id, edge_type, weight, properties) VALUES (?, ?, ?, ?, ?, ?)",
        graph_name);
    
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        sqlite3_exec(db, "ROLLBACK", NULL, NULL, NULL);
        return rc;
    }
    
    int edge_id = 1;
    int num_edges = num_nodes * TEST_EDGES_RATIO;
    
    for (int i = 0; i < num_edges; i++) {
        int from_id = 1 + (rand() % num_nodes);
        int to_id = 1 + (rand() % num_nodes);
        
        if (from_id == to_id) continue;
        
        sqlite3_bind_int64(stmt, 1, edge_id++);
        sqlite3_bind_int64(stmt, 2, from_id);
        sqlite3_bind_int64(stmt, 3, to_id);
        sqlite3_bind_text(stmt, 4, "CONNECTS", -1, SQLITE_STATIC);
        sqlite3_bind_double(stmt, 5, 1.0 + (rand() % 100) / 100.0);
        sqlite3_bind_text(stmt, 6, "{}", -1, SQLITE_STATIC);
        
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            fprintf(stderr, "Error inserting edge %d\n", i);
            break;
        }
        sqlite3_reset(stmt);
    }
    
    sqlite3_finalize(stmt);
    sqlite3_exec(db, "COMMIT", NULL, NULL, NULL);
    
    /* Create indexes */
    snprintf(sql, sizeof(sql),
        "CREATE INDEX idx_%s_from_to ON %s(from_id, to_id) WHERE edge_id IS NOT NULL",
        graph_name, graph_name);
    sqlite3_exec(db, sql, NULL, NULL, NULL);
    
    snprintf(sql, sizeof(sql), "ANALYZE %s", graph_name);
    sqlite3_exec(db, sql, NULL, NULL, NULL);
    
    printf("Loaded %d nodes and %d edges\n", num_nodes, edge_id - 1);
    return SQLITE_OK;
}

/* Performance test implementations */

/* Test 1: Single node lookup */
static PerfResult *test_node_lookup(sqlite3 *db, const char *graph_name) {
    PerfResult *result = create_result("Node Lookup", TARGET_NODE_LOOKUP_MS);
    if (!result) return NULL;
    
    double start_memory = get_memory_mb();
    double start_time = get_time_ms();
    
    sqlite3_stmt *stmt;
    char sql[256];
    snprintf(sql, sizeof(sql),
        "SELECT node_id, properties FROM %s WHERE node_id = ? AND edge_id IS NULL",
        graph_name);
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        result->error_message = strdup(sqlite3_errmsg(db));
        result->execution_time_ms = get_time_ms() - start_time;
        return result;
    }
    
    int total_found = 0;
    
    /* Test 1000 random lookups */
    for (int i = 0; i < 1000; i++) {
        int node_id = 1 + (rand() % TEST_NODES_MEDIUM);
        sqlite3_bind_int64(stmt, 1, node_id);
        
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            total_found++;
        }
        sqlite3_reset(stmt);
    }
    
    sqlite3_finalize(stmt);
    
    result->execution_time_ms = get_time_ms() - start_time;
    result->memory_mb = get_memory_mb() - start_memory;
    result->result_count = total_found;
    result->passed = (result->execution_time_ms < result->target_time_ms);
    
    return result;
}

/* Test 2: 1-hop traversal */
static PerfResult *test_1hop_traversal(sqlite3 *db, const char *graph_name) {
    PerfResult *result = create_result("1-Hop Traversal", TARGET_1HOP_TRAVERSAL_MS);
    if (!result) return NULL;
    
    double start_memory = get_memory_mb();
    double start_time = get_time_ms();
    
    sqlite3_stmt *stmt;
    char sql[512];
    snprintf(sql, sizeof(sql),
        "SELECT neighbor.node_id, neighbor.properties "
        "FROM %s edge "
        "JOIN %s neighbor ON neighbor.node_id = edge.to_id "
        "WHERE edge.from_id = ? AND edge.edge_type = 'CONNECTS' "
        "  AND neighbor.edge_id IS NULL",
        graph_name, graph_name);
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        result->error_message = strdup(sqlite3_errmsg(db));
        result->execution_time_ms = get_time_ms() - start_time;
        return result;
    }
    
    int total_neighbors = 0;
    
    /* Test 100 random nodes */
    for (int i = 0; i < 100; i++) {
        int node_id = 1 + (rand() % TEST_NODES_MEDIUM);
        sqlite3_bind_int64(stmt, 1, node_id);
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            total_neighbors++;
        }
        sqlite3_reset(stmt);
    }
    
    sqlite3_finalize(stmt);
    
    result->execution_time_ms = get_time_ms() - start_time;
    result->memory_mb = get_memory_mb() - start_memory;
    result->result_count = total_neighbors;
    result->passed = (result->execution_time_ms < result->target_time_ms);
    
    return result;
}

/* Test 3: 3-hop traversal */
static PerfResult *test_3hop_traversal(sqlite3 *db, const char *graph_name) {
    PerfResult *result = create_result("3-Hop Traversal", TARGET_3HOP_TRAVERSAL_MS);
    if (!result) return NULL;
    
    double start_memory = get_memory_mb();
    double start_time = get_time_ms();
    
    sqlite3_stmt *stmt;
    char sql[1024];
    snprintf(sql, sizeof(sql),
        "WITH RECURSIVE traversal AS ( "
        "  SELECT node_id, 0 as depth FROM %s "
        "  WHERE node_id = ? AND edge_id IS NULL "
        "  UNION ALL "
        "  SELECT e.to_id as node_id, t.depth + 1 as depth "
        "  FROM traversal t "
        "  JOIN %s e ON e.from_id = t.node_id "
        "  WHERE e.edge_type = 'CONNECTS' AND t.depth < 3 "
        ") "
        "SELECT DISTINCT t.node_id, n.properties "
        "FROM traversal t "
        "JOIN %s n ON n.node_id = t.node_id "
        "WHERE n.edge_id IS NULL AND t.depth = 3",
        graph_name, graph_name, graph_name);
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        result->error_message = strdup(sqlite3_errmsg(db));
        result->execution_time_ms = get_time_ms() - start_time;
        return result;
    }
    
    int total_reachable = 0;
    
    /* Test 10 random starting nodes */
    for (int i = 0; i < 10; i++) {
        int node_id = 1 + (rand() % TEST_NODES_MEDIUM);
        sqlite3_bind_int64(stmt, 1, node_id);
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            total_reachable++;
        }
        sqlite3_reset(stmt);
    }
    
    sqlite3_finalize(stmt);
    
    result->execution_time_ms = get_time_ms() - start_time;
    result->memory_mb = get_memory_mb() - start_memory;
    result->result_count = total_reachable;
    result->passed = (result->execution_time_ms < result->target_time_ms);
    
    return result;
}

/* Test 4: Shortest path */
static PerfResult *test_shortest_path(sqlite3 *db, const char *graph_name) {
    PerfResult *result = create_result("Shortest Path", TARGET_SHORTEST_PATH_MS);
    if (!result) return NULL;
    
    double start_memory = get_memory_mb();
    double start_time = get_time_ms();
    
    sqlite3_stmt *stmt;
    char sql[256];
    snprintf(sql, sizeof(sql),
        "SELECT * FROM graph_dijkstra('%s', ?, ?)",
        graph_name);
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        result->error_message = strdup(sqlite3_errmsg(db));
        result->execution_time_ms = get_time_ms() - start_time;
        return result;
    }
    
    int total_paths = 0;
    
    /* Test 10 random path queries */
    for (int i = 0; i < 10; i++) {
        int start_id = 1 + (rand() % TEST_NODES_MEDIUM);
        int end_id = 1 + (rand() % TEST_NODES_MEDIUM);
        
        if (start_id == end_id) continue;
        
        sqlite3_bind_int64(stmt, 1, start_id);
        sqlite3_bind_int64(stmt, 2, end_id);
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            total_paths++;
        }
        sqlite3_reset(stmt);
    }
    
    sqlite3_finalize(stmt);
    
    result->execution_time_ms = get_time_ms() - start_time;
    result->memory_mb = get_memory_mb() - start_memory;
    result->result_count = total_paths;
    result->passed = (result->execution_time_ms < result->target_time_ms);
    
    return result;
}

/* Test 5: PageRank algorithm */
static PerfResult *test_pagerank(sqlite3 *db, const char *graph_name) {
    PerfResult *result = create_result("PageRank", TARGET_PAGERANK_MS);
    if (!result) return NULL;
    
    double start_memory = get_memory_mb();
    double start_time = get_time_ms();
    
    sqlite3_stmt *stmt;
    char sql[256];
    snprintf(sql, sizeof(sql),
        "SELECT * FROM graph_pagerank('%s', 0.85, 50)",
        graph_name);
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        result->error_message = strdup(sqlite3_errmsg(db));
        result->execution_time_ms = get_time_ms() - start_time;
        return result;
    }
    
    int total_nodes = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        total_nodes++;
    }
    
    sqlite3_finalize(stmt);
    
    result->execution_time_ms = get_time_ms() - start_time;
    result->memory_mb = get_memory_mb() - start_memory;
    result->result_count = total_nodes;
    result->passed = (result->execution_time_ms < result->target_time_ms);
    
    return result;
}

/* Test 6: Bulk loading performance */
static PerfResult *test_bulk_loading(sqlite3 *db) {
    PerfResult *result = create_result("Bulk Loading", 0.0);  /* Special case */
    if (!result) return NULL;
    
    const char *graph_name = "bulk_test_graph";
    int num_nodes = 10000;
    
    /* Create fresh graph */
    char sql[256];
    snprintf(sql, sizeof(sql), "DROP TABLE IF EXISTS %s", graph_name);
    sqlite3_exec(db, sql, NULL, NULL, NULL);
    
    int rc = create_test_graph(db, graph_name);
    if (rc != SQLITE_OK) {
        result->error_message = strdup("Failed to create bulk test graph");
        return result;
    }
    
    double start_time = get_time_ms();
    
    /* Bulk load nodes */
    rc = load_test_data(db, graph_name, num_nodes);
    
    double end_time = get_time_ms();
    double duration_seconds = (end_time - start_time) / 1000.0;
    double nodes_per_second = num_nodes / duration_seconds;
    
    result->execution_time_ms = end_time - start_time;
    result->result_count = num_nodes;
    result->passed = (nodes_per_second >= TARGET_BULK_LOAD_NODES_PER_SEC);
    
    /* Store the rate in target_time_ms for reporting */
    result->target_time_ms = TARGET_BULK_LOAD_NODES_PER_SEC;
    
    printf("Bulk loading: %.0f nodes/sec (target: %.0f nodes/sec)\n",
           nodes_per_second, TARGET_BULK_LOAD_NODES_PER_SEC);
    
    return result;
}

/* Test 7: Memory usage scaling */
static PerfResult *test_memory_scaling(sqlite3 *db) {
    PerfResult *result = create_result("Memory Scaling", 100.0);  /* 100MB target */
    if (!result) return NULL;
    
    const char *graph_name = "memory_test_graph";
    
    /* Create fresh graph */
    char sql[256];
    snprintf(sql, sizeof(sql), "DROP TABLE IF EXISTS %s", graph_name);
    sqlite3_exec(db, sql, NULL, NULL, NULL);
    
    int rc = create_test_graph(db, graph_name);
    if (rc != SQLITE_OK) {
        result->error_message = strdup("Failed to create memory test graph");
        return result;
    }
    
    double start_memory = get_memory_mb();
    double start_time = get_time_ms();
    
    /* Load test data */
    rc = load_test_data(db, graph_name, TEST_NODES_MEDIUM);
    
    double end_memory = get_memory_mb();
    double memory_used = end_memory - start_memory;
    
    result->execution_time_ms = get_time_ms() - start_time;
    result->memory_mb = memory_used;
    result->result_count = TEST_NODES_MEDIUM;
    result->passed = (memory_used < result->target_time_ms);  /* Using target_time_ms for memory limit */
    
    printf("Memory usage: %.2f MB for %d nodes (%.2f MB/1K nodes)\n",
           memory_used, TEST_NODES_MEDIUM, memory_used * 1000.0 / TEST_NODES_MEDIUM);
    
    return result;
}

/* Main test runner */
static void run_performance_regression_tests(sqlite3 *db) {
    printf("\n=== Performance Regression Test Suite ===\n\n");
    
    /* Setup test graph */
    const char *graph_name = "perf_test_graph";
    printf("Setting up test graph with %d nodes...\n", TEST_NODES_MEDIUM);
    
    int rc = create_test_graph(db, graph_name);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to create test graph\n");
        return;
    }
    
    rc = load_test_data(db, graph_name, TEST_NODES_MEDIUM);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to load test data\n");
        return;
    }
    
    printf("Test data loaded successfully.\n\n");
    
    /* Run tests */
    PerfResult *results[7];
    
    printf("Running performance tests...\n");
    
    results[0] = test_node_lookup(db, graph_name);
    results[1] = test_1hop_traversal(db, graph_name);
    results[2] = test_3hop_traversal(db, graph_name);
    results[3] = test_shortest_path(db, graph_name);
    results[4] = test_pagerank(db, graph_name);
    results[5] = test_bulk_loading(db);
    results[6] = test_memory_scaling(db);
    
    /* Print results */
    printf("\n=== Performance Test Results ===\n");
    printf("%-20s %-10s %-10s %-8s %-10s %-8s\n",
           "Test", "Time(ms)", "Target(ms)", "Status", "Memory(MB)", "Results");
    printf("%-20s %-10s %-10s %-8s %-10s %-8s\n",
           "----", "--------", "---------", "------", "---------", "-------");
    
    int passed = 0, total = 7;
    
    for (int i = 0; i < total; i++) {
        if (results[i]) {
            if (results[i]->error_message) {
                printf("%-20s %-10s %-10s %-8s %-10s %-8s\n",
                       results[i]->test_name, "ERROR", "-", "FAIL", "-", "-");
                printf("  Error: %s\n", results[i]->error_message);
            } else {
                const char *status = results[i]->passed ? "PASS" : "FAIL";
                
                if (strcmp(results[i]->test_name, "Bulk Loading") == 0) {
                    /* Special formatting for bulk loading */
                    double rate = results[i]->result_count * 1000.0 / results[i]->execution_time_ms;
                    printf("%-20s %-10.2f %-10.0f %-8s %-10.2f %-8d\n",
                           results[i]->test_name, rate, results[i]->target_time_ms,
                           status, results[i]->memory_mb, results[i]->result_count);
                } else {
                    printf("%-20s %-10.2f %-10.2f %-8s %-10.2f %-8d\n",
                           results[i]->test_name, results[i]->execution_time_ms,
                           results[i]->target_time_ms, status, results[i]->memory_mb,
                           results[i]->result_count);
                }
                
                if (results[i]->passed) passed++;
            }
        }
    }
    
    printf("\n=== Summary ===\n");
    printf("Tests passed: %d/%d\n", passed, total);
    printf("Overall status: %s\n", (passed == total) ? "PASS" : "FAIL");
    
    if (passed == total) {
        printf("✓ All performance targets met\n");
    } else {
        printf("✗ Performance regression detected in %d test(s)\n", total - passed);
    }
    
    /* Cleanup */
    for (int i = 0; i < total; i++) {
        free_result(results[i]);
    }
}

int main(int argc, char **argv) {
    sqlite3 *db;
    int rc;
    
    printf("SQLite Graph Extension Performance Regression Tests\n");
    printf("===================================================\n");
    
    /* Initialize random seed */
    srand(time(NULL));
    
    /* Open database */
    rc = sqlite3_open(":memory:", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error opening database: %s\n", sqlite3_errmsg(db));
        return 1;
    }
    
    /* Initialize graph extension directly (since we're using static library) */
    char *zErrMsg = NULL;
    rc = sqlite3_graph_init(db, &zErrMsg, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error loading extension: %s\n", 
                zErrMsg ? zErrMsg : sqlite3_errmsg(db));
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return 1;
    }
    
    /* Run tests */
    run_performance_regression_tests(db);
    
    /* Cleanup */
    sqlite3_close(db);
    
    return 0;
}