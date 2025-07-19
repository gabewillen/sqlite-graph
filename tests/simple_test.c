#include <stdio.h>
#include <sqlite3.h>

int main() {
    sqlite3 *db;
    int rc;
    
    rc = sqlite3_open(":memory:", &db);
    if (rc != SQLITE_OK) {
        printf("Cannot open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }
    
    printf("Database opened successfully!\n");
    
    sqlite3_close(db);
    return 0;
}
