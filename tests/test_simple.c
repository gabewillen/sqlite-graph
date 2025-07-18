#define SQLITE_CORE
#include "unity.h"
#include "sqlite3.h"
#include "../include/graph.h"
#include "../include/graph-vtab.h"

static sqlite3 *pDb = 0;

void setUp(void) {
    int rc = sqlite3_open(":memory:", &pDb);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
}

void tearDown(void) {
    if (pDb) {
        sqlite3_close(pDb);
        pDb = 0;
    }
}

// Test the main failing cases from the original report
void testGraphBasicWorkflow(void) {
    // This was returning 1 instead of 100
    int result = 1; // Placeholder - need to implement actual test
    TEST_ASSERT_EQUAL(100, result);
}

void testGraphConnectivity(void) {
    // This was returning 0 instead of 1  
    int result = 0; // Placeholder - need to implement actual test
    TEST_ASSERT_EQUAL(1, result);
}

void testAllNodesScanIterator(void) {
    // This was returning 1 instead of 0
    int result = 1; // Placeholder - need to implement actual test
    TEST_ASSERT_EQUAL(0, result);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(testGraphBasicWorkflow);
    RUN_TEST(testGraphConnectivity);
    RUN_TEST(testAllNodesScanIterator);
    return UNITY_END();
}