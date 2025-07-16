/*
** SQLite Graph Database Extension - Integration Tests
**
** End-to-end workflow testing for complete graph operations.
*/

#define SQLITE_CORE
#include "unity.h"
#include "sqlite3.h"

extern sqlite3 *g_pTestDb;

