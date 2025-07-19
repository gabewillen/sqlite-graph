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
    
    sqlite3_enable_load_extension(db, 1);
    
    char *err = NULL;
    rc = sqlite3_load_extension(db, "./build/libgraph.so", 0, &err);
    if (rc != SQLITE_OK) {
        printf("Extension loading failed: %s\n", err ? err : "unknown error");
        sqlite3_free(err);
        return 1;
    }
    
    rc = sqlite3_exec(db, "CREATE VIRTUAL TABLE test_graph USING graph()", NULL, NULL, &err);
    if (rc != SQLITE_OK) {
        printf("Virtual table creation failed: %s\n", err);
        sqlite3_free(err);
        return 1;
    }
    
    printf("=== Checking what tables were created ===\n");
    rc = sqlite3_exec(db, "SELECT name FROM sqlite_master WHERE type='table'", callback, 0, &err);
    printf("Tables query result: rc=%d\n", rc);
    
    printf("\n=== Checking nodes table ===\n");
    rc = sqlite3_exec(db, "SELECT COUNT(*) FROM test_graph_nodes", callback, 0, &err);
    printf("Nodes count result: rc=%d\n", rc);
    if (err) { printf("Nodes error: %s\n", err); sqlite3_free(err); err = NULL; }
    
    printf("\n=== Checking edges table ===\n");
    rc = sqlite3_exec(db, "SELECT COUNT(*) FROM test_graph_edges", callback, 0, &err);
    printf("Edges count result: rc=%d\n", rc);
    if (err) { printf("Edges error: %s\n", err); sqlite3_free(err); err = NULL; }
    
    printf("\n=== Insert test data ===\n");
    rc = sqlite3_exec(db, "INSERT INTO test_graph (type, properties) VALUES ('node', '{\"name\":\"test1\"}')", NULL, NULL, &err);
    printf("Insert result: rc=%d\n", rc);
    
    printf("\n=== Check nodes table after insert ===\n");
    rc = sqlite3_exec(db, "SELECT COUNT(*) FROM test_graph_nodes", callback, 0, &err);
    printf("Nodes count after insert: rc=%d\n", rc);
    
    printf("\n=== Check edges table after insert ===\n");
    rc = sqlite3_exec(db, "SELECT COUNT(*) FROM test_graph_edges", callback, 0, &err);
    printf("Edges count after insert: rc=%d\n", rc);
    
    sqlite3_close(db);
    return 0;
}
