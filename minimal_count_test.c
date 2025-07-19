#include <sqlite3.h>
#include <stdio.h>

int main() {
    sqlite3 *db;
    sqlite3_open(":memory:", &db);
    
    // First test COUNT on a regular table
    printf("=== Testing COUNT on regular table ===\n");
    sqlite3_exec(db, "CREATE TABLE regular (id INTEGER, name TEXT)", NULL, NULL, NULL);
    sqlite3_exec(db, "INSERT INTO regular VALUES (1, 'test1')", NULL, NULL, NULL);
    sqlite3_exec(db, "INSERT INTO regular VALUES (2, 'test2')", NULL, NULL, NULL);
    
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM regular", -1, &stmt, NULL);
    printf("Regular table COUNT prepare: %d\n", rc);
    rc = sqlite3_step(stmt);
    printf("Regular table COUNT step: %d (expect %d=SQLITE_ROW)\n", rc, SQLITE_ROW);
    if (rc == SQLITE_ROW) {
        printf("Regular table count: %d\n", sqlite3_column_int(stmt, 0));
    }
    sqlite3_finalize(stmt);
    
    // Load our extension
    sqlite3_enable_load_extension(db, 1);
    char *err = NULL;
    rc = sqlite3_load_extension(db, "./build/libgraph.so", 0, &err);
    if (rc != SQLITE_OK) {
        printf("Extension failed: %s\n", err);
        return 1;
    }
    
    sqlite3_exec(db, "CREATE VIRTUAL TABLE test_graph USING graph()", NULL, NULL, &err);
    sqlite3_exec(db, "INSERT INTO test_graph (type, properties) VALUES ('node', '{\"test\":1}')", NULL, NULL, NULL);
    sqlite3_exec(db, "INSERT INTO test_graph (type, properties) VALUES ('node', '{\"test\":2}')", NULL, NULL, NULL);
    
    printf("\n=== Testing COUNT on virtual table ===\n");
    rc = sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM test_graph", -1, &stmt, NULL);
    printf("Virtual table COUNT prepare: %d\n", rc);
    rc = sqlite3_step(stmt);
    printf("Virtual table COUNT step: %d (expect %d=SQLITE_ROW)\n", rc, SQLITE_ROW);
    if (rc == SQLITE_ROW) {
        printf("Virtual table count: %d\n", sqlite3_column_int(stmt, 0));
    } else {
        printf("Virtual table COUNT failed with: %s\n", sqlite3_errmsg(db));
    }
    sqlite3_finalize(stmt);
    
    // Test if basic SELECT works on virtual table
    printf("\n=== Testing basic SELECT on virtual table ===\n");
    rc = sqlite3_prepare_v2(db, "SELECT type, id FROM test_graph", -1, &stmt, NULL);
    printf("Virtual table SELECT prepare: %d\n", rc);
    int row_count = 0;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        printf("Row %d: type=%s, id=%d\n", ++row_count, 
               sqlite3_column_text(stmt, 0), sqlite3_column_int(stmt, 1));
    }
    printf("Virtual table SELECT final step: %d (expect %d=SQLITE_DONE)\n", rc, SQLITE_DONE);
    sqlite3_finalize(stmt);
    
    sqlite3_close(db);
    return 0;
}
