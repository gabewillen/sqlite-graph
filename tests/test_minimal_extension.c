/*
** test_minimal_extension.c - Minimal SQLite Extension for Testing
**
** This is a minimal SQLite extension that just registers a simple
** function to verify extension loading works.
*/

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

/* Simple test function */
static void test_func(
    sqlite3_context *context,
    int argc,
    sqlite3_value **argv
) {
    sqlite3_result_text(context, "Graph extension loaded!", -1, SQLITE_STATIC);
}

/* Extension entry point */
#ifdef _WIN32
__declspec(dllexport)
#endif
int sqlite3_testgraph_init(
    sqlite3 *db,
    char **pzErrMsg,
    const sqlite3_api_routines *pApi
) {
    int rc = SQLITE_OK;
    SQLITE_EXTENSION_INIT2(pApi);
    
    /* Register a simple function */
    rc = sqlite3_create_function(db, "graph_test", 0, SQLITE_UTF8, 0,
                                 test_func, 0, 0);
    
    if (rc != SQLITE_OK) {
        *pzErrMsg = sqlite3_mprintf("Failed to create function");
        return rc;
    }
    
    return SQLITE_OK;
}