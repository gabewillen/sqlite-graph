/*
 * Memory Profiler for SQLite Graph Extension
 * 
 * Tracks memory usage patterns, detects leaks, and profiles
 * memory efficiency across different graph operations.
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

/* Memory tracking structure */
typedef struct MemorySnapshot {
    double timestamp_ms;
    long memory_kb;
    int operation_id;
    char operation_name[64];
} MemorySnapshot;

/* Memory profile result */
typedef struct MemoryProfile {
    char test_name[64];
    long initial_memory_kb;
    long peak_memory_kb;
    long final_memory_kb;
    long memory_delta_kb;
    double duration_ms;
    int snapshots_count;
    MemorySnapshot *snapshots;
    int leaked_memory;
} MemoryProfile;

/* Global tracking */
static MemorySnapshot *g_snapshots = NULL;
static int g_snapshot_count = 0;
static int g_snapshot_capacity = 0;

/* Utility functions */
static double get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

static long get_memory_kb(void) {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss;  /* Already in KB on Linux */
}

/* Memory tracking functions */
static void init_memory_tracking(void) {
    g_snapshot_capacity = 1000;
    g_snapshots = malloc(sizeof(MemorySnapshot) * g_snapshot_capacity);
    g_snapshot_count = 0;
}

static void cleanup_memory_tracking(void) {
    free(g_snapshots);
    g_snapshots = NULL;
    g_snapshot_count = 0;
    g_snapshot_capacity = 0;
}

static void take_memory_snapshot(int operation_id, const char *operation_name) {
    if (g_snapshot_count >= g_snapshot_capacity) {
        g_snapshot_capacity *= 2;
        g_snapshots = realloc(g_snapshots, sizeof(MemorySnapshot) * g_snapshot_capacity);
    }
    
    MemorySnapshot *snapshot = &g_snapshots[g_snapshot_count++];
    snapshot->timestamp_ms = get_time_ms();
    snapshot->memory_kb = get_memory_kb();
    snapshot->operation_id = operation_id;
    strncpy(snapshot->operation_name, operation_name, sizeof(snapshot->operation_name) - 1);
    snapshot->operation_name[sizeof(snapshot->operation_name) - 1] = '\0';
}

/* Memory profiling functions */
static MemoryProfile *start_memory_profile(const char *test_name) {
    MemoryProfile *profile = calloc(1, sizeof(MemoryProfile));
    if (!profile) return NULL;
    
    strncpy(profile->test_name, test_name, sizeof(profile->test_name) - 1);
    profile->test_name[sizeof(profile->test_name) - 1] = '\0';
    
    init_memory_tracking();
    take_memory_snapshot(0, "start");
    
    profile->initial_memory_kb = get_memory_kb();
    profile->peak_memory_kb = profile->initial_memory_kb;
    
    return profile;
}

static void end_memory_profile(MemoryProfile *profile) {
    if (!profile) return;
    
    take_memory_snapshot(999, "end");
    
    profile->final_memory_kb = get_memory_kb();
    profile->memory_delta_kb = profile->final_memory_kb - profile->initial_memory_kb;
    profile->snapshots_count = g_snapshot_count;
    
    /* Copy snapshots */
    profile->snapshots = malloc(sizeof(MemorySnapshot) * g_snapshot_count);
    if (profile->snapshots) {
        memcpy(profile->snapshots, g_snapshots, sizeof(MemorySnapshot) * g_snapshot_count);
    }
    
    /* Calculate peak memory and duration */
    profile->peak_memory_kb = profile->initial_memory_kb;
    double start_time = profile->snapshots[0].timestamp_ms;
    double end_time = profile->snapshots[g_snapshot_count - 1].timestamp_ms;
    profile->duration_ms = end_time - start_time;
    
    for (int i = 0; i < g_snapshot_count; i++) {
        if (profile->snapshots[i].memory_kb > profile->peak_memory_kb) {
            profile->peak_memory_kb = profile->snapshots[i].memory_kb;
        }
    }
    
    /* Check for memory leaks */
    profile->leaked_memory = (profile->memory_delta_kb > 1024);  /* > 1MB considered leak */
    
    cleanup_memory_tracking();
}

static void free_memory_profile(MemoryProfile *profile) {
    if (profile) {
        free(profile->snapshots);
        free(profile);
    }
}

/* Graph creation and operations for testing */
static int create_memory_test_graph(sqlite3 *db, const char *graph_name) {
    char sql[256];
    snprintf(sql, sizeof(sql), 
        "CREATE VIRTUAL TABLE %s USING graph("
        "cache_size=1000, max_depth=10, thread_pool_size=2)", 
        graph_name);
    
    char *zErrMsg = NULL;
    int rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error creating graph %s: %s\n", graph_name, zErrMsg);
        sqlite3_free(zErrMsg);
    }
    return rc;
}

/* Memory test implementations */

/* Test 1: Node creation memory pattern */
static MemoryProfile *test_node_creation_memory(sqlite3 *db) {
    MemoryProfile *profile = start_memory_profile("Node Creation");
    if (!profile) return NULL;
    
    const char *graph_name = "memory_test_nodes";
    
    /* Create graph */
    char sql[256];
    snprintf(sql, sizeof(sql), "DROP TABLE IF EXISTS %s", graph_name);
    sqlite3_exec(db, sql, NULL, NULL, NULL);
    
    create_memory_test_graph(db, graph_name);
    take_memory_snapshot(1, "graph_created");
    
    /* Prepare statement */
    snprintf(sql, sizeof(sql),
        "INSERT INTO %s (node_id, labels, properties) VALUES (?, ?, ?)",
        graph_name);
    
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    take_memory_snapshot(2, "statement_prepared");
    
    /* Insert nodes in batches */
    sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);
    
    for (int batch = 0; batch < 10; batch++) {
        for (int i = 0; i < 1000; i++) {
            int node_id = batch * 1000 + i + 1;
            char properties[256];
            snprintf(properties, sizeof(properties),
                "{\"id\":%d,\"value\":%d,\"batch\":%d}",
                node_id, rand() % 1000, batch);
            
            sqlite3_bind_int64(stmt, 1, node_id);
            sqlite3_bind_text(stmt, 2, "[\"TestNode\"]", -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 3, properties, -1, SQLITE_TRANSIENT);
            
            sqlite3_step(stmt);
            sqlite3_reset(stmt);
        }
        
        char snapshot_name[64];
        snprintf(snapshot_name, sizeof(snapshot_name), "batch_%d_complete", batch + 1);
        take_memory_snapshot(10 + batch, snapshot_name);
    }
    
    sqlite3_exec(db, "COMMIT", NULL, NULL, NULL);
    sqlite3_finalize(stmt);
    take_memory_snapshot(20, "nodes_committed");
    
    end_memory_profile(profile);
    return profile;
}

/* Test 2: Edge creation memory pattern */
static MemoryProfile *test_edge_creation_memory(sqlite3 *db) {
    MemoryProfile *profile = start_memory_profile("Edge Creation");
    if (!profile) return NULL;
    
    const char *graph_name = "memory_test_edges";
    
    /* Create graph with existing nodes */
    char sql[256];
    snprintf(sql, sizeof(sql), "DROP TABLE IF EXISTS %s", graph_name);
    sqlite3_exec(db, sql, NULL, NULL, NULL);
    
    create_memory_test_graph(db, graph_name);
    
    /* Add nodes first */
    snprintf(sql, sizeof(sql),
        "INSERT INTO %s (node_id, labels, properties) VALUES (?, ?, ?)",
        graph_name);
    
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    
    sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);
    for (int i = 1; i <= 1000; i++) {
        sqlite3_bind_int64(stmt, 1, i);
        sqlite3_bind_text(stmt, 2, "[\"Node\"]", -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, "{}", -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }
    sqlite3_exec(db, "COMMIT", NULL, NULL, NULL);
    sqlite3_finalize(stmt);
    
    take_memory_snapshot(1, "nodes_loaded");
    
    /* Now add edges */
    snprintf(sql, sizeof(sql),
        "INSERT INTO %s (edge_id, from_id, to_id, edge_type, weight, properties) VALUES (?, ?, ?, ?, ?, ?)",
        graph_name);
    
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);
    
    for (int batch = 0; batch < 10; batch++) {
        for (int i = 0; i < 500; i++) {
            int edge_id = batch * 500 + i + 1;
            int from_id = 1 + (rand() % 1000);
            int to_id = 1 + (rand() % 1000);
            
            if (from_id == to_id) continue;
            
            sqlite3_bind_int64(stmt, 1, edge_id);
            sqlite3_bind_int64(stmt, 2, from_id);
            sqlite3_bind_int64(stmt, 3, to_id);
            sqlite3_bind_text(stmt, 4, "CONNECTS", -1, SQLITE_STATIC);
            sqlite3_bind_double(stmt, 5, 1.0);
            sqlite3_bind_text(stmt, 6, "{}", -1, SQLITE_STATIC);
            
            sqlite3_step(stmt);
            sqlite3_reset(stmt);
        }
        
        char snapshot_name[64];
        snprintf(snapshot_name, sizeof(snapshot_name), "edge_batch_%d", batch + 1);
        take_memory_snapshot(10 + batch, snapshot_name);
    }
    
    sqlite3_exec(db, "COMMIT", NULL, NULL, NULL);
    sqlite3_finalize(stmt);
    take_memory_snapshot(20, "edges_committed");
    
    end_memory_profile(profile);
    return profile;
}

/* Test 3: Query operation memory usage */
static MemoryProfile *test_query_memory(sqlite3 *db) {
    MemoryProfile *profile = start_memory_profile("Query Operations");
    if (!profile) return NULL;
    
    const char *graph_name = "memory_test_query";
    
    /* Setup test graph */
    char sql[512];
    snprintf(sql, sizeof(sql), "DROP TABLE IF EXISTS %s", graph_name);
    sqlite3_exec(db, sql, NULL, NULL, NULL);
    
    create_memory_test_graph(db, graph_name);
    
    /* Load test data */
    snprintf(sql, sizeof(sql),
        "INSERT INTO %s (node_id, labels, properties) VALUES (?, ?, ?)",
        graph_name);
    
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    
    sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);
    for (int i = 1; i <= 5000; i++) {
        char properties[128];
        snprintf(properties, sizeof(properties), "{\"id\":%d,\"group\":%d}", i, i % 10);
        
        sqlite3_bind_int64(stmt, 1, i);
        sqlite3_bind_text(stmt, 2, "[\"Node\"]", -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, properties, -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }
    sqlite3_exec(db, "COMMIT", NULL, NULL, NULL);
    sqlite3_finalize(stmt);
    
    take_memory_snapshot(1, "data_loaded");
    
    /* Test different query types */
    
    /* 1. Simple node lookups */
    snprintf(sql, sizeof(sql),
        "SELECT * FROM %s WHERE node_id = ? AND edge_id IS NULL", graph_name);
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    
    for (int i = 0; i < 1000; i++) {
        sqlite3_bind_int64(stmt, 1, 1 + (rand() % 5000));
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }
    sqlite3_finalize(stmt);
    take_memory_snapshot(2, "node_lookups_complete");
    
    /* 2. Range queries */
    snprintf(sql, sizeof(sql),
        "SELECT * FROM %s WHERE edge_id IS NULL AND node_id BETWEEN ? AND ?", graph_name);
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    
    for (int i = 0; i < 100; i++) {
        int start = rand() % 4000;
        sqlite3_bind_int64(stmt, 1, start);
        sqlite3_bind_int64(stmt, 2, start + 100);
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            /* Process result */
        }
        sqlite3_reset(stmt);
    }
    sqlite3_finalize(stmt);
    take_memory_snapshot(3, "range_queries_complete");
    
    /* 3. Property-based queries */
    snprintf(sql, sizeof(sql),
        "SELECT * FROM %s WHERE edge_id IS NULL AND json_extract(properties, '$.group') = ?", 
        graph_name);
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    
    for (int group = 0; group < 10; group++) {
        sqlite3_bind_int(stmt, 1, group);
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            /* Process result */
        }
        sqlite3_reset(stmt);
    }
    sqlite3_finalize(stmt);
    take_memory_snapshot(4, "property_queries_complete");
    
    end_memory_profile(profile);
    return profile;
}

/* Test 4: Algorithm memory usage */
static MemoryProfile *test_algorithm_memory(sqlite3 *db) {
    MemoryProfile *profile = start_memory_profile("Graph Algorithms");
    if (!profile) return NULL;
    
    const char *graph_name = "memory_test_algo";
    
    /* Setup connected graph */
    char sql[512];
    snprintf(sql, sizeof(sql), "DROP TABLE IF EXISTS %s", graph_name);
    sqlite3_exec(db, sql, NULL, NULL, NULL);
    
    create_memory_test_graph(db, graph_name);
    
    /* Load nodes */
    snprintf(sql, sizeof(sql),
        "INSERT INTO %s (node_id, labels, properties) VALUES (?, ?, ?)",
        graph_name);
    
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    
    sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);
    for (int i = 1; i <= 1000; i++) {
        sqlite3_bind_int64(stmt, 1, i);
        sqlite3_bind_text(stmt, 2, "[\"Node\"]", -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, "{}", -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }
    sqlite3_finalize(stmt);
    
    /* Load edges */
    snprintf(sql, sizeof(sql),
        "INSERT INTO %s (edge_id, from_id, to_id, edge_type, weight, properties) VALUES (?, ?, ?, ?, ?, ?)",
        graph_name);
    
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    
    for (int i = 1; i <= 3000; i++) {
        int from_id = 1 + (rand() % 1000);
        int to_id = 1 + (rand() % 1000);
        if (from_id == to_id) continue;
        
        sqlite3_bind_int64(stmt, 1, i);
        sqlite3_bind_int64(stmt, 2, from_id);
        sqlite3_bind_int64(stmt, 3, to_id);
        sqlite3_bind_text(stmt, 4, "CONNECTS", -1, SQLITE_STATIC);
        sqlite3_bind_double(stmt, 5, 1.0);
        sqlite3_bind_text(stmt, 6, "{}", -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }
    sqlite3_exec(db, "COMMIT", NULL, NULL, NULL);
    sqlite3_finalize(stmt);
    
    take_memory_snapshot(1, "graph_loaded");
    
    /* Test PageRank */
    snprintf(sql, sizeof(sql), "SELECT * FROM graph_pagerank('%s', 0.85, 20)", graph_name);
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        /* Process result */
    }
    sqlite3_finalize(stmt);
    take_memory_snapshot(2, "pagerank_complete");
    
    /* Test shortest path */
    snprintf(sql, sizeof(sql), "SELECT * FROM graph_dijkstra('%s', 1, 500)", graph_name);
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        /* Process result */
    }
    sqlite3_finalize(stmt);
    take_memory_snapshot(3, "dijkstra_complete");
    
    /* Test connected components */
    snprintf(sql, sizeof(sql), "SELECT * FROM graph_components('%s')", graph_name);
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        /* Process result */
    }
    sqlite3_finalize(stmt);
    take_memory_snapshot(4, "components_complete");
    
    end_memory_profile(profile);
    return profile;
}

/* Report generation */
static void print_memory_profile_report(MemoryProfile *profile) {
    if (!profile) return;
    
    printf("\n=== Memory Profile: %s ===\n", profile->test_name);
    printf("Duration: %.2f ms\n", profile->duration_ms);
    printf("Initial Memory: %ld KB (%.2f MB)\n", 
           profile->initial_memory_kb, profile->initial_memory_kb / 1024.0);
    printf("Peak Memory: %ld KB (%.2f MB)\n", 
           profile->peak_memory_kb, profile->peak_memory_kb / 1024.0);
    printf("Final Memory: %ld KB (%.2f MB)\n", 
           profile->final_memory_kb, profile->final_memory_kb / 1024.0);
    printf("Memory Delta: %ld KB (%.2f MB)\n", 
           profile->memory_delta_kb, profile->memory_delta_kb / 1024.0);
    printf("Memory Leaked: %s\n", profile->leaked_memory ? "YES" : "NO");
    
    if (profile->leaked_memory) {
        printf("⚠️  WARNING: Potential memory leak detected!\n");
    }
    
    /* Print memory usage timeline */
    printf("\nMemory Usage Timeline:\n");
    printf("%-20s %-12s %-12s %-12s\n", "Operation", "Time(ms)", "Memory(KB)", "Delta(KB)");
    printf("%-20s %-12s %-12s %-12s\n", "--------", "-------", "---------", "--------");
    
    long prev_memory = profile->snapshots[0].memory_kb;
    double start_time = profile->snapshots[0].timestamp_ms;
    
    for (int i = 0; i < profile->snapshots_count; i++) {
        MemorySnapshot *snap = &profile->snapshots[i];
        long delta = snap->memory_kb - prev_memory;
        double elapsed = snap->timestamp_ms - start_time;
        
        printf("%-20s %-12.2f %-12ld %-12ld\n",
               snap->operation_name, elapsed, snap->memory_kb, delta);
        
        prev_memory = snap->memory_kb;
    }
}

/* Main memory profiling runner */
static void run_memory_profiling_tests(sqlite3 *db) {
    printf("=== SQLite Graph Extension Memory Profiling ===\n");
    
    MemoryProfile *profiles[4];
    
    printf("Running memory profiling tests...\n");
    
    profiles[0] = test_node_creation_memory(db);
    profiles[1] = test_edge_creation_memory(db);
    profiles[2] = test_query_memory(db);
    profiles[3] = test_algorithm_memory(db);
    
    /* Generate reports */
    for (int i = 0; i < 4; i++) {
        if (profiles[i]) {
            print_memory_profile_report(profiles[i]);
        }
    }
    
    /* Overall summary */
    printf("\n=== Memory Profiling Summary ===\n");
    
    int leaked_tests = 0;
    long total_peak_memory = 0;
    
    for (int i = 0; i < 4; i++) {
        if (profiles[i]) {
            if (profiles[i]->leaked_memory) {
                leaked_tests++;
            }
            total_peak_memory += profiles[i]->peak_memory_kb;
        }
    }
    
    printf("Tests completed: 4\n");
    printf("Tests with memory leaks: %d\n", leaked_tests);
    printf("Total peak memory across all tests: %.2f MB\n", total_peak_memory / 1024.0);
    
    if (leaked_tests == 0) {
        printf("✓ PASS: No memory leaks detected\n");
    } else {
        printf("✗ FAIL: Memory leaks detected in %d test(s)\n", leaked_tests);
    }
    
    /* Cleanup */
    for (int i = 0; i < 4; i++) {
        free_memory_profile(profiles[i]);
    }
}

int main(int argc, char **argv) {
    sqlite3 *db;
    int rc;
    
    printf("SQLite Graph Extension Memory Profiler\n");
    printf("=======================================\n");
    
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
    
    /* Run memory profiling */
    run_memory_profiling_tests(db);
    
    /* Cleanup */
    sqlite3_close(db);
    
    printf("\nMemory profiling completed.\n");
    return 0;
}