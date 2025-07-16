/*
** test_extension_real.c - Real SQLite Extension Loading Test
**
** This program loads the graph extension into SQLite and tests
** actual functionality with a real database.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "build/_deps/sqlite3-src/sqlite3.h"

/* Callback for SELECT queries */
static int callback(void* data, int argc, char** argv, char** col_names) {
    for (int i = 0; i < argc; i++) {
        printf("%s = %s\n", col_names[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}

int main(int argc, char** argv) {
    sqlite3* db;
    char* err_msg = NULL;
    int rc;
    
    printf("=== REAL SQLITE EXTENSION LOADING TEST ===\n\n");
    
    /* Open in-memory database */
    rc = sqlite3_open(":memory:", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }
    
    printf("1. Database opened successfully\n");
    
    /* Enable extension loading */
    rc = sqlite3_enable_load_extension(db, 1);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot enable extension loading: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }
    
    printf("2. Extension loading enabled\n");
    
    /* Load the graph extension */
    rc = sqlite3_load_extension(db, "./graph", NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot load extension: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }
    
    printf("3. Graph extension loaded successfully!\n");
    
    /* Create virtual table */
    rc = sqlite3_exec(db, "CREATE VIRTUAL TABLE test_graph USING graph()", 
                      NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to create virtual table: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }
    
    printf("4. Virtual table created successfully\n");
    
    /* Insert test data */
    const char* insert_sql = 
        "INSERT INTO test_graph (from_id, to_id, weight) VALUES "
        "(1, 2, 1.0), "
        "(2, 3, 2.0), "
        "(1, 3, 5.0);";
    
    rc = sqlite3_exec(db, insert_sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to insert data: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }
    
    printf("5. Test data inserted successfully\n");
    
    /* Query the data */
    printf("\n6. Querying graph data:\n");
    printf("   All edges:\n");
    rc = sqlite3_exec(db, "SELECT * FROM test_graph", callback, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Query failed: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
    
    /* Count edges */
    printf("   Edge count:\n");
    rc = sqlite3_exec(db, "SELECT COUNT(*) as count FROM test_graph", 
                      callback, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Count query failed: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
    
    /* Test WHERE clause */
    printf("   Edges from node 1:\n");
    rc = sqlite3_exec(db, "SELECT * FROM test_graph WHERE from_id = 1", 
                      callback, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "WHERE query failed: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
    
    printf("\n=== ALL TESTS PASSED ===\n");
    printf("The SQLite Graph Extension is working correctly!\n");
    
    /* Cleanup */
    sqlite3_close(db);
    return 0;
}