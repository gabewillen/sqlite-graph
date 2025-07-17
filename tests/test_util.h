#ifndef GRAPH_TEST_UTIL_H
#define GRAPH_TEST_UTIL_H

#include "sqlite3.h"

/* Declare graphExtensionLoad for test files */
int graphExtensionLoad(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi);

#endif /* GRAPH_TEST_UTIL_H */