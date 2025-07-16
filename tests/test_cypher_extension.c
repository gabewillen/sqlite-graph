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
    
    printf("Testing SQLite Graph Extension - Full Cypher Support\n");
    printf("==================================================\n");
    
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
    
    // Test 2: Test Cypher parsing functions
    print_separator("Cypher Parsing Functions");
    execute_sql(db, "SELECT cypher_parse('MATCH (n) RETURN n')", "Test cypher_parse function");
    execute_sql(db, "SELECT cypher_validate('MATCH (n:Person) RETURN n.name')", "Test cypher_validate function");
    execute_sql(db, "SELECT cypher_ast_info('MATCH (n) RETURN n')", "Test cypher_ast_info function");
    
    // Test 3: Test Cypher execution functions
    print_separator("Cypher Execution Functions");
    execute_sql(db, "SELECT cypher_execute('MATCH (n) RETURN n')", "Test cypher_execute function");
    execute_sql(db, "SELECT cypher_execute_explain('MATCH (n) RETURN n')", "Test cypher_execute_explain function");
    execute_sql(db, "SELECT cypher_test_execute()", "Test cypher_test_execute function");
    
    // Test 4: Test Cypher write operations
    print_separator("Cypher Write Operations");
    execute_sql(db, "SELECT cypher_create_node('n', '[\"Person\"]', '{\"name\": \"Alice\", \"age\": 30}')", "Test cypher_create_node");
    execute_sql(db, "SELECT cypher_write_test()", "Test cypher_write_test function");
    execute_sql(db, "SELECT cypher_write_comprehensive_test()", "Test comprehensive write operations");
    
    print_separator("Test Summary");
    printf("Cypher extension testing completed!\n");
    printf("All major Cypher SQL functions have been tested.\n");
    printf("Review the output above to verify functionality.\n");
    
    sqlite3_close(db);
    return 0;
}