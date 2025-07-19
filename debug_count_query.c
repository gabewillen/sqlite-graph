#include <sqlite3.h>
#include <stdio.h>

static int callback(void *NotUsed, int argc, char **argv, char **azColName) {
    (void)NotUsed;
    for (int i = 0; i < argc; i++) {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    return 0;
}

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
    
    // Insert some test data
    printf("Inserting test data...\n");
    rc = sqlite3_exec(db, "INSERT INTO test_graph (type, properties) VALUES ('node', '{\"name\":\"test1\"}')", NULL, NULL, &err);
    printf("First insert: rc=%d\n", rc);
    rc = sqlite3_exec(db, "INSERT INTO test_graph (type, properties) VALUES ('node', '{\"name\":\"test2\"}')", NULL, NULL, &err);
    printf("Second insert: rc=%d\n", rc);
    
    // Try a simple SELECT first
    printf("\n=== Simple SELECT ===\n");
    rc = sqlite3_exec(db, "SELECT type, id FROM test_graph", callback, 0, &err);
    printf("SELECT result: rc=%d\n", rc);
    if (err) {
        printf("SELECT error: %s\n", err);
        sqlite3_free(err);
        err = NULL;
    }
    
    // Now try COUNT
    printf("\n=== COUNT Query ===\n");
    rc = sqlite3_exec(db, "SELECT COUNT(*) FROM test_graph", callback, 0, &err);
    printf("COUNT result: rc=%d\n", rc);
    if (err) {
        printf("COUNT error: %s\n", err);
        sqlite3_free(err);
    }
    
    sqlite3_close(db);
    return 0;
}
