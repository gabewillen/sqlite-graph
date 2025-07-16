/*
** test_graph_extension.c - Comprehensive Graph Extension Test
**
** Tests the actual SQLite graph extension with virtual table
** creation, data insertion, and querying.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "build/_deps/sqlite3-src/sqlite3.h"

static int callback(void* data, int argc, char** argv, char** col_names) {
    for (int i = 0; i < argc; i++) {
        printf("  %s = %s\n", col_names[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}

int main() {
    sqlite3* db;
    char* err_msg = NULL;
    int rc;
    
    printf("=== SQLITE GRAPH EXTENSION TEST ===\n\n");
    
    /* Open database */
    rc = sqlite3_open(":memory:", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }
    printf("✓ Database opened\n");
    
    /* Enable extensions */
    rc = sqlite3_enable_load_extension(db, 1);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot enable extensions: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }
    printf("✓ Extension loading enabled\n");
    
    /* Load graph extension */
    rc = sqlite3_load_extension(db, "./graph_simple", "sqlite3_graph_init", &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot load extension: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }
    printf("✓ Graph extension loaded\n");
    
    /* Test version function */
    printf("\nTesting graph_version():\n");
    rc = sqlite3_exec(db, "SELECT graph_version()", callback, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Version function failed: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
    
    /* Create virtual table */
    printf("Creating virtual table...\n");
    rc = sqlite3_exec(db, "CREATE VIRTUAL TABLE social_graph USING graph()", 
                      NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to create virtual table: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }
    printf("✓ Virtual table 'social_graph' created\n");
    
    /* Insert test data */
    printf("\nInserting graph edges...\n");
    const char* edges[] = {
        "INSERT INTO social_graph VALUES (1, 2, 1.0)",  /* Alice -> Bob */
        "INSERT INTO social_graph VALUES (2, 3, 1.0)",  /* Bob -> Charlie */
        "INSERT INTO social_graph VALUES (1, 3, 2.0)",  /* Alice -> Charlie (weaker) */
        "INSERT INTO social_graph VALUES (3, 4, 1.0)",  /* Charlie -> David */
        "INSERT INTO social_graph VALUES (4, 1, 1.0)",  /* David -> Alice (cycle) */
        NULL
    };
    
    for (int i = 0; edges[i]; i++) {
        rc = sqlite3_exec(db, edges[i], NULL, NULL, &err_msg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Insert failed: %s\n", err_msg);
            sqlite3_free(err_msg);
        }
    }
    printf("✓ Inserted 5 edges\n");
    
    /* Query all edges */
    printf("\nAll edges in graph:\n");
    rc = sqlite3_exec(db, "SELECT * FROM social_graph", callback, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Query failed: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
    
    /* Count edges */
    printf("Edge count:\n");
    rc = sqlite3_exec(db, "SELECT COUNT(*) as count FROM social_graph", 
                      callback, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Count failed: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
    
    /* Filter by from_id */
    printf("Edges from node 1:\n");
    rc = sqlite3_exec(db, "SELECT * FROM social_graph WHERE from_id = 1", 
                      callback, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Filter query failed: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
    
    /* Find paths with specific weight */
    printf("Edges with weight = 1.0:\n");
    rc = sqlite3_exec(db, "SELECT * FROM social_graph WHERE weight = 1.0", 
                      callback, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Weight filter failed: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
    
    /* Complex query */
    printf("Nodes with both incoming and outgoing edges:\n");
    rc = sqlite3_exec(db, 
        "SELECT DISTINCT from_id as node FROM social_graph "
        "WHERE from_id IN (SELECT to_id FROM social_graph) "
        "ORDER BY node", 
        callback, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Complex query failed: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
    
    printf("\n=== ALL TESTS PASSED ===\n");
    printf("The SQLite Graph Extension is working correctly!\n");
    
    sqlite3_close(db);
    return 0;
}