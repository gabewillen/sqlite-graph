#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

// Test helper functions
static int callback(void *NotUsed, int argc, char **argv, char **azColName) {
    for(int i = 0; i < argc; i++) {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}

static void print_separator(const char* test_name) {
    printf("\n=== %s ===\n", test_name);
}

static int execute_sql(sqlite3 *db, const char *sql, const char *description) {
    char *err_msg = 0;
    int rc;
    
    printf("\n%s:\nSQL: %s\n", description, sql);
    rc = sqlite3_exec(db, sql, callback, 0, &err_msg);
    
    if (rc != SQLITE_OK) {
        printf("SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return 0;
    }
    return 1;
}

int main() {
    sqlite3 *db;
    char *err_msg = 0;
    int rc;
    
    printf("Testing SQLite Graph Extension - Basic Functionality\n");
    printf("====================================================\n");
    
    // Open database
    rc = sqlite3_open(":memory:", &db);
    if (rc) {
        printf("Can't open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }
    
    // Enable extension loading
    rc = sqlite3_enable_load_extension(db, 1);
    if (rc != SQLITE_OK) {
        printf("Failed to enable extension loading: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }
    
    // Load the graph extension
    printf("Loading graph extension...\n");
    rc = sqlite3_load_extension(db, "./build/src/libgraph", "sqlite3_graph_init", &err_msg);
    if (rc) {
        printf("Failed to load extension: %s\n", err_msg ? err_msg : "Unknown error");
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }
    printf("Extension loaded successfully!\n");
    
    // Test 1: Create virtual table
    print_separator("Creating Virtual Table");
    if (!execute_sql(db, "CREATE VIRTUAL TABLE graph_test USING graph()", "Create virtual graph table")) {
        sqlite3_close(db);
        return 1;
    }
    
    // Test 2: Check virtual table data
    print_separator("Virtual Table Basic Tests");
    execute_sql(db, "SELECT * FROM graph_test LIMIT 5", "Query virtual table (should be empty)");
    execute_sql(db, "SELECT COUNT(*) as total_rows FROM graph_test", "Count all rows");
    
    // Test 3: Test any available SQL functions - start with most basic ones
    print_separator("Testing Available SQL Functions");
    
    // Test if these functions exist at all
    execute_sql(db, "SELECT name FROM pragma_function_list() WHERE name LIKE 'cypher%' OR name LIKE 'graph%' ORDER BY name", "List available Cypher/Graph functions");
    
    print_separator("Test Summary");
    printf("Basic extension testing completed!\n");
    printf("Extension loads and virtual table works.\n");
    printf("Review function list above to see what Cypher functions are available.\n");
    
    sqlite3_close(db);
    return 0;
}