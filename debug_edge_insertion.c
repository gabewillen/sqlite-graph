#include <sqlite3.h>
#include <stdio.h>

int main() {
    sqlite3 *db;
    int rc = sqlite3_open(":memory:", &db);
    
    // Enable extensions
    sqlite3_enable_load_extension(db, 1);
    
    // Load our extension
    char *err = NULL;
    rc = sqlite3_load_extension(db, "./build/libgraph.so", 0, &err);
    if (rc != SQLITE_OK) {
        printf("Extension loading failed: %s\n", err ? err : "unknown error");
        sqlite3_free(err);
        return 1;
    }
    
    // Create virtual table
    rc = sqlite3_exec(db, "CREATE VIRTUAL TABLE test_graph USING graph()", NULL, NULL, &err);
    if (rc != SQLITE_OK) {
        printf("Virtual table creation failed: %s\n", err);
        sqlite3_free(err);
        return 1;
    }
    
    // First insert a node
    printf("Inserting node...\n");
    rc = sqlite3_exec(db, "INSERT INTO test_graph (type, properties) VALUES ('node', '{\"name\":\"test\"}')", NULL, NULL, &err);
    printf("Node insert result: %d\n", rc);
    if (err) {
        printf("Node insert error: %s\n", err);
        sqlite3_free(err);
        err = NULL;
    }
    
    // Insert another node  
    printf("Inserting second node...\n");
    rc = sqlite3_exec(db, "INSERT INTO test_graph (type, properties) VALUES ('node', '{\"name\":\"test2\"}')", NULL, NULL, &err);
    printf("Second node insert result: %d\n", rc);
    if (err) {
        printf("Second node insert error: %s\n", err);
        sqlite3_free(err);
        err = NULL;
    }
    
    // Now try to insert an edge
    printf("Inserting edge...\n");
    rc = sqlite3_exec(db, "INSERT INTO test_graph (type, from_id, to_id, weight, properties) VALUES ('edge', 1, 2, 1.5, '{\"rel\":\"test\"}')", NULL, NULL, &err);
    printf("Edge insert result: %d\n", rc);
    if (err) {
        printf("Edge insert error: %s\n", err);
        sqlite3_free(err);
        err = NULL;
    }
    
    // Try a count query
    printf("Running COUNT query...\n");
    rc = sqlite3_exec(db, "SELECT COUNT(*) FROM test_graph WHERE type = 'node'", NULL, NULL, &err);
    printf("Count query result: %d\n", rc);
    if (err) {
        printf("Count query error: %s\n", err);
        sqlite3_free(err);
    }
    
    sqlite3_close(db);
    return 0;
}
