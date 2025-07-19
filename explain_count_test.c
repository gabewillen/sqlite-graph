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
    sqlite3_open(":memory:", &db);
    sqlite3_enable_load_extension(db, 1);
    
    char *err = NULL;
    int rc = sqlite3_load_extension(db, "./build/libgraph.so", 0, &err);
    if (rc != SQLITE_OK) {
        printf("Extension failed: %s\n", err);
        return 1;
    }
    
    sqlite3_exec(db, "CREATE VIRTUAL TABLE test_graph USING graph()", NULL, NULL, &err);
    sqlite3_exec(db, "INSERT INTO test_graph (type, properties) VALUES ('node', '{\"test\":1}')", NULL, NULL, NULL);
    sqlite3_exec(db, "INSERT INTO test_graph (type, properties) VALUES ('node', '{\"test\":2}')", NULL, NULL, NULL);
    
    printf("=== EXPLAIN QUERY PLAN for COUNT ===\n");
    sqlite3_exec(db, "EXPLAIN QUERY PLAN SELECT COUNT(*) FROM test_graph", callback, 0, &err);
    
    printf("\n=== EXPLAIN QUERY PLAN for SELECT ===\n");
    sqlite3_exec(db, "EXPLAIN QUERY PLAN SELECT * FROM test_graph", callback, 0, &err);
    
    printf("\n=== EXPLAIN for COUNT ===\n");
    sqlite3_exec(db, "EXPLAIN SELECT COUNT(*) FROM test_graph", callback, 0, &err);
    
    sqlite3_close(db);
    return 0;
}
