/*
** test_minimal_load.c - Test Minimal Extension Loading
*/

#include <stdio.h>
#include <stdlib.h>
#include "build/_deps/sqlite3-src/sqlite3.h"

static int callback(void* data, int argc, char** argv, char** col_names) {
    for (int i = 0; i < argc; i++) {
        printf("%s = %s\n", col_names[i], argv[i] ? argv[i] : "NULL");
    }
    return 0;
}

int main() {
    sqlite3* db;
    char* err_msg = NULL;
    int rc;
    
    printf("=== MINIMAL EXTENSION LOADING TEST ===\n\n");
    
    rc = sqlite3_open(":memory:", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }
    
    printf("1. Database opened\n");
    
    rc = sqlite3_enable_load_extension(db, 1);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot enable extensions: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }
    
    printf("2. Extension loading enabled\n");
    
    rc = sqlite3_load_extension(db, "./testgraph", NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot load extension: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }
    
    printf("3. Extension loaded successfully!\n");
    
    /* Test the function */
    rc = sqlite3_exec(db, "SELECT graph_test()", callback, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Function call failed: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }
    
    printf("\n=== TEST PASSED ===\n");
    
    sqlite3_close(db);
    return 0;
}