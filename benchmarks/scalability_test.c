/*
 * Scalability Test Suite
 * SQLite Graph Extension
 * 
 * Tests performance characteristics across different graph sizes
 * to validate scalability and identify bottlenecks.
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

/* Test configurations */
static const int TEST_SIZES[] = {1000, 5000, 10000, 50000, 100000};
static const int NUM_TEST_SIZES = sizeof(TEST_SIZES) / sizeof(TEST_SIZES[0]);
static const int EDGES_PER_NODE = 3;

/* Scalability result structure */
typedef struct ScalabilityResult {
    int graph_size;
    double load_time_ms;
    double node_lookup_ms;
    double traversal_ms;
    double shortest_path_ms;
    double memory_mb;
    int success;
} ScalabilityResult;

/* Utility functions */
static double get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

static double get_memory_mb(void) {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss / 1024.0;
}

/* Graph creation and loading */
static int create_scalability_graph(sqlite3 *db, const char *graph_name) {
    char sql[256];
    snprintf(sql, sizeof(sql), 
        "CREATE VIRTUAL TABLE %s USING graph("
        "cache_size=50000, max_depth=15, thread_pool_size=8)", 
        graph_name);
    
    char *zErrMsg = NULL;
    int rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error creating graph %s: %s\n", graph_name, zErrMsg);
        sqlite3_free(zErrMsg);
    }
    return rc;
}

static double load_graph_data(sqlite3 *db, const char *graph_name, int num_nodes) {
    printf("  Loading %d nodes...", num_nodes);
    fflush(stdout);
    
    double start_time = get_time_ms();
    
    sqlite3_stmt *stmt;
    char sql[512];
    
    /* Insert nodes */
    snprintf(sql, sizeof(sql),
        "INSERT INTO %s (node_id, labels, properties) VALUES (?, ?, ?)",
        graph_name);
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return -1;
    
    sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);
    
    for (int i = 1; i <= num_nodes; i++) {
        char properties[256];
        snprintf(properties, sizeof(properties),
            "{\"id\":%d,\"value\":%d,\"category\":\"%s\"}",
            i, rand() % 1000, (i % 2 == 0) ? "even" : "odd");
        
        sqlite3_bind_int64(stmt, 1, i);
        sqlite3_bind_text(stmt, 2, "[\"TestNode\"]", -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, properties, -1, SQLITE_TRANSIENT);
        
        sqlite3_step(stmt);
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
        return -1;
    }
    
    int edge_id = 1;
    int num_edges = num_nodes * EDGES_PER_NODE;
    
    for (int i = 0; i < num_edges; i++) {
        int from_id = 1 + (rand() % num_nodes);
        int to_id = 1 + (rand() % num_nodes);
        
        if (from_id == to_id) continue;
        
        sqlite3_bind_int64(stmt, 1, edge_id++);
        sqlite3_bind_int64(stmt, 2, from_id);
        sqlite3_bind_int64(stmt, 3, to_id);
        sqlite3_bind_text(stmt, 4, "LINKS", -1, SQLITE_STATIC);
        sqlite3_bind_double(stmt, 5, 1.0 + (rand() % 100) / 100.0);
        sqlite3_bind_text(stmt, 6, "{}", -1, SQLITE_STATIC);
        
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }
    
    sqlite3_finalize(stmt);
    sqlite3_exec(db, "COMMIT", NULL, NULL, NULL);
    
    /* Create indexes */
    snprintf(sql, sizeof(sql),
        "CREATE INDEX idx_%s_nodes ON %s(node_id) WHERE edge_id IS NULL",
        graph_name, graph_name);
    sqlite3_exec(db, sql, NULL, NULL, NULL);
    
    snprintf(sql, sizeof(sql),
        "CREATE INDEX idx_%s_edges ON %s(from_id, to_id) WHERE edge_id IS NOT NULL",
        graph_name, graph_name);
    sqlite3_exec(db, sql, NULL, NULL, NULL);
    
    snprintf(sql, sizeof(sql), "ANALYZE %s", graph_name);
    sqlite3_exec(db, sql, NULL, NULL, NULL);
    
    double end_time = get_time_ms();
    printf(" done (%.2f sec)\n", (end_time - start_time) / 1000.0);
    
    return end_time - start_time;
}

/* Performance tests */
static double test_node_lookup_performance(sqlite3 *db, const char *graph_name, int num_nodes) {
    sqlite3_stmt *stmt;
    char sql[256];
    snprintf(sql, sizeof(sql),
        "SELECT node_id, properties FROM %s WHERE node_id = ? AND edge_id IS NULL",
        graph_name);
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return -1;
    
    double start_time = get_time_ms();
    
    /* Test 1000 random lookups */
    for (int i = 0; i < 1000; i++) {
        int node_id = 1 + (rand() % num_nodes);
        sqlite3_bind_int64(stmt, 1, node_id);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }
    
    sqlite3_finalize(stmt);
    
    return get_time_ms() - start_time;
}

static double test_traversal_performance(sqlite3 *db, const char *graph_name, int num_nodes) {
    sqlite3_stmt *stmt;
    char sql[512];
    snprintf(sql, sizeof(sql),
        "SELECT neighbor.node_id "
        "FROM %s edge "
        "JOIN %s neighbor ON neighbor.node_id = edge.to_id "
        "WHERE edge.from_id = ? AND edge.edge_type = 'LINKS' "
        "  AND neighbor.edge_id IS NULL "
        "LIMIT 100",
        graph_name, graph_name);
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return -1;
    
    double start_time = get_time_ms();
    
    /* Test 100 random traversals */
    for (int i = 0; i < 100; i++) {
        int node_id = 1 + (rand() % num_nodes);
        sqlite3_bind_int64(stmt, 1, node_id);
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            /* Process result */
        }
        sqlite3_reset(stmt);
    }
    
    sqlite3_finalize(stmt);
    
    return get_time_ms() - start_time;
}

static double test_shortest_path_performance(sqlite3 *db, const char *graph_name, int num_nodes) {
    sqlite3_stmt *stmt;
    char sql[256];
    snprintf(sql, sizeof(sql),
        "SELECT * FROM graph_dijkstra('%s', ?, ?) LIMIT 1",
        graph_name);
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return -1;
    
    double start_time = get_time_ms();
    
    /* Test 10 random shortest paths */
    for (int i = 0; i < 10; i++) {
        int start_id = 1 + (rand() % num_nodes);
        int end_id = 1 + (rand() % num_nodes);
        
        if (start_id == end_id) continue;
        
        sqlite3_bind_int64(stmt, 1, start_id);
        sqlite3_bind_int64(stmt, 2, end_id);
        
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }
    
    sqlite3_finalize(stmt);
    
    return get_time_ms() - start_time;
}

/* Run scalability test for a specific graph size */
static ScalabilityResult test_scalability_at_size(sqlite3 *db, int graph_size) {
    ScalabilityResult result = {0};
    result.graph_size = graph_size;
    result.success = 0;
    
    printf("Testing scalability with %d nodes:\n", graph_size);
    
    char graph_name[64];
    snprintf(graph_name, sizeof(graph_name), "scale_test_%d", graph_size);
    
    /* Drop existing table */
    char sql[128];
    snprintf(sql, sizeof(sql), "DROP TABLE IF EXISTS %s", graph_name);
    sqlite3_exec(db, sql, NULL, NULL, NULL);
    
    /* Create graph */
    if (create_scalability_graph(db, graph_name) != SQLITE_OK) {
        printf("  ERROR: Failed to create graph\n");
        return result;
    }
    
    double start_memory = get_memory_mb();
    
    /* Load data */
    result.load_time_ms = load_graph_data(db, graph_name, graph_size);
    if (result.load_time_ms < 0) {
        printf("  ERROR: Failed to load data\n");
        return result;
    }
    
    /* Test node lookup performance */
    printf("  Testing node lookups...");
    fflush(stdout);
    result.node_lookup_ms = test_node_lookup_performance(db, graph_name, graph_size);
    if (result.node_lookup_ms < 0) {
        printf(" ERROR\n");
        return result;
    }
    printf(" %.2f ms\n", result.node_lookup_ms);
    
    /* Test traversal performance */
    printf("  Testing traversals...");
    fflush(stdout);
    result.traversal_ms = test_traversal_performance(db, graph_name, graph_size);
    if (result.traversal_ms < 0) {
        printf(" ERROR\n");
        return result;
    }
    printf(" %.2f ms\n", result.traversal_ms);
    
    /* Test shortest path performance */
    printf("  Testing shortest paths...");
    fflush(stdout);
    result.shortest_path_ms = test_shortest_path_performance(db, graph_name, graph_size);
    if (result.shortest_path_ms < 0) {
        printf(" ERROR\n");
        return result;
    }
    printf(" %.2f ms\n", result.shortest_path_ms);
    
    /* Measure memory usage */
    result.memory_mb = get_memory_mb() - start_memory;
    
    result.success = 1;
    printf("  Memory usage: %.2f MB\n", result.memory_mb);
    printf("  Load rate: %.0f nodes/sec\n", graph_size * 1000.0 / result.load_time_ms);
    
    return result;
}

/* Calculate scaling factors */
static void analyze_scalability_results(ScalabilityResult *results, int num_results) {
    printf("\n=== Scalability Analysis ===\n");
    printf("%-10s %-12s %-12s %-12s %-12s %-12s %-12s\n",
           "Nodes", "Load(ms)", "Lookup(ms)", "Traverse(ms)", "Path(ms)", "Memory(MB)", "Rate(n/s)");
    printf("%-10s %-12s %-12s %-12s %-12s %-12s %-12s\n",
           "-----", "-------", "--------", "----------", "-------", "--------", "--------");
    
    for (int i = 0; i < num_results; i++) {
        if (results[i].success) {
            double rate = results[i].graph_size * 1000.0 / results[i].load_time_ms;
            printf("%-10d %-12.2f %-12.2f %-12.2f %-12.2f %-12.2f %-12.0f\n",
                   results[i].graph_size,
                   results[i].load_time_ms,
                   results[i].node_lookup_ms,
                   results[i].traversal_ms,
                   results[i].shortest_path_ms,
                   results[i].memory_mb,
                   rate);
        } else {
            printf("%-10d %-12s %-12s %-12s %-12s %-12s %-12s\n",
                   results[i].graph_size, "FAILED", "FAILED", "FAILED", "FAILED", "FAILED", "FAILED");
        }
    }
    
    /* Calculate scaling factors */
    printf("\n=== Scaling Factors (relative to smallest test) ===\n");
    if (results[0].success && num_results > 1) {
        printf("%-10s %-12s %-12s %-12s %-12s %-12s\n",
               "Size Ratio", "Load Factor", "Lookup Factor", "Traverse Factor", "Path Factor", "Memory Factor");
        printf("%-10s %-12s %-12s %-12s %-12s %-12s\n",
               "----------", "-----------", "------------", "--------------", "-----------", "-------------");
        
        for (int i = 1; i < num_results; i++) {
            if (results[i].success) {
                double size_ratio = (double)results[i].graph_size / results[0].graph_size;
                double load_factor = results[i].load_time_ms / results[0].load_time_ms;
                double lookup_factor = results[i].node_lookup_ms / results[0].node_lookup_ms;
                double traverse_factor = results[i].traversal_ms / results[0].traversal_ms;
                double path_factor = results[i].shortest_path_ms / results[0].shortest_path_ms;
                double memory_factor = results[i].memory_mb / results[0].memory_mb;
                
                printf("%-10.1fx %-12.2fx %-12.2fx %-12.2fx %-12.2fx %-12.2fx\n",
                       size_ratio, load_factor, lookup_factor, traverse_factor, 
                       path_factor, memory_factor);
            }
        }
    }
    
    /* Performance targets validation */
    printf("\n=== Performance Target Validation ===\n");
    int passed_targets = 0;
    int total_targets = 0;
    
    for (int i = 0; i < num_results; i++) {
        if (results[i].success) {
            int graph_size = results[i].graph_size;
            
            /* Node lookup should be < 1ms per lookup (for 1000 lookups) */
            total_targets++;
            if (results[i].node_lookup_ms < 1000.0) {
                printf("✓ Node lookup performance acceptable for %d nodes\n", graph_size);
                passed_targets++;
            } else {
                printf("✗ Node lookup too slow for %d nodes: %.2f ms\n", 
                       graph_size, results[i].node_lookup_ms);
            }
            
            /* Traversal should scale reasonably */
            total_targets++;
            double expected_traversal = 100.0 * (graph_size / 10000.0);  /* Scale from 10K baseline */
            if (results[i].traversal_ms < expected_traversal * 2.0) {
                printf("✓ Traversal performance acceptable for %d nodes\n", graph_size);
                passed_targets++;
            } else {
                printf("✗ Traversal too slow for %d nodes: %.2f ms (expected < %.2f ms)\n",
                       graph_size, results[i].traversal_ms, expected_traversal * 2.0);
            }
            
            /* Memory usage should be reasonable */
            total_targets++;
            double expected_memory = graph_size / 1000.0;  /* ~1MB per 1K nodes */
            if (results[i].memory_mb < expected_memory * 3.0) {
                printf("✓ Memory usage acceptable for %d nodes: %.2f MB\n", 
                       graph_size, results[i].memory_mb);
                passed_targets++;
            } else {
                printf("✗ Memory usage too high for %d nodes: %.2f MB (expected < %.2f MB)\n",
                       graph_size, results[i].memory_mb, expected_memory * 3.0);
            }
        }
    }
    
    printf("\nOverall scalability: %d/%d targets met\n", passed_targets, total_targets);
    if (passed_targets == total_targets) {
        printf("✓ PASS: All scalability targets met\n");
    } else {
        printf("✗ FAIL: Scalability issues detected\n");
    }
}

/* Main scalability test runner */
static void run_scalability_tests(sqlite3 *db) {
    printf("=== SQLite Graph Extension Scalability Tests ===\n\n");
    
    ScalabilityResult results[NUM_TEST_SIZES];
    
    /* Run tests for each graph size */
    for (int i = 0; i < NUM_TEST_SIZES; i++) {
        results[i] = test_scalability_at_size(db, TEST_SIZES[i]);
        printf("\n");
    }
    
    /* Analyze results */
    analyze_scalability_results(results, NUM_TEST_SIZES);
}

int main(int argc, char **argv) {
    sqlite3 *db;
    int rc;
    
    printf("SQLite Graph Extension Scalability Test Suite\n");
    printf("=============================================\n\n");
    
    /* Initialize random seed */
    srand(time(NULL));
    
    /* Open database */
    rc = sqlite3_open(":memory:", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error opening database: %s\n", sqlite3_errmsg(db));
        return 1;
    }
    
    /* Load graph extension */
    char *zErrMsg = NULL;
    rc = sqlite3_load_extension(db, "./graph_extension", NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error loading extension: %s\n", 
                zErrMsg ? zErrMsg : sqlite3_errmsg(db));
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return 1;
    }
    
    /* Run scalability tests */
    run_scalability_tests(db);
    
    /* Cleanup */
    sqlite3_close(db);
    
    printf("\nScalability testing completed.\n");
    return 0;
}