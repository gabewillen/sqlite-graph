# Testing Documentation

This document describes how to add tests, build the test suite, run with sanitizers, and interpret results for the SQLite Graph Extension.

## Overview

The test suite provides comprehensive coverage for:
- Core graph operations
- Memory safety and bounds checking
- Cypher query language parsing and execution
- Performance benchmarks
- Thread safety and concurrent operations

## Quick Start

### Single Command Build and Test
```bash
# Clean build and basic verification
./build_and_test.sh
```

### Build Test Suite Only
```bash
make -C tests clean
make -C tests
```

### Run All Tests
```bash
cd tests
./run_all_tests.sh
```

## Adding a New Test

### 1. Create a Test File

Create a new test file following the naming convention `test_<component>.c`:

```c
// tests/test_myfeature.c
#include <unity.h>
#include <sqlite3.h>
#include <graph.h>

sqlite3 *db;

void setUp(void) {
    int rc = sqlite3_open(":memory:", &db);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    
    // Enable SQLite extension support
    sqlite3_db_config(db, SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION, 1, NULL);
    sqlite3_enable_load_extension(db, 1);
    
    // Initialize graph extension
    rc = sqlite3_graph_init(db, NULL, NULL);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
}

void tearDown(void) {
    if (db) {
        sqlite3_close(db);
        db = NULL;
    }
}

void test_myfeature_basic(void) {
    // Your test code here
    TEST_ASSERT_TRUE(1); // Replace with actual test
}

void test_myfeature_edge_cases(void) {
    // Test edge cases
    TEST_ASSERT_NOT_NULL(db);
}

// Test runner function
void runMyFeatureTests(void) {
    RUN_TEST(test_myfeature_basic);
    RUN_TEST(test_myfeature_edge_cases);
}

int main(void) {
    UNITY_BEGIN();
    
    runMyFeatureTests();
    
    return UNITY_END();
}
```

### 2. Memory Safety Tests

For memory-sensitive components, add bounds checking and leak detection:

```c
void test_memory_safety_example(void) {
    // Test NULL pointer handling
    int result = myFunction(NULL);
    TEST_ASSERT_EQUAL(GRAPH_ERROR_NULL_PARAM, result);
    
    // Test buffer overflow protection
    char small_buffer[10];
    result = safeCopyFunction(small_buffer, sizeof(small_buffer), "very long string");
    TEST_ASSERT_EQUAL(GRAPH_ERROR_BUFFER_TOO_SMALL, result);
    
    // Verify buffer wasn't overwritten
    TEST_ASSERT_TRUE(strlen(small_buffer) < sizeof(small_buffer));
}
```

### 3. Test Registration

The test system automatically discovers test files matching `test_*.c`. No manual registration required.

## Building the Test Suite

### Standard Build
```bash
make -C tests
```

### Build with Debug Information
```bash
make -C tests CFLAGS="-g -O0 -DDEBUG"
```

### Build Individual Tests
```bash
cd tests
gcc -I../include -I../src -I../_deps/sqlite-src -I../_deps/Unity-2.5.2/src \
    -g -O0 -std=gnu99 -DSQLITE_ENABLE_LOAD_EXTENSION=1 \
    test_myfeature.c -o test_myfeature \
    -L../build -lgraph_static -lunity -lsqlite3 -lm -ldl -lpthread
```

## Running with Sanitizers

### Address Sanitizer (Memory Errors)
```bash
export ASAN_OPTIONS="abort_on_error=1:halt_on_error=1:detect_leaks=1"
make -C tests EXTRA_CFLAGS="-fsanitize=address -fno-omit-frame-pointer" \
              EXTRA_LDFLAGS="-fsanitize=address"
cd tests && ./run_all_tests.sh
```

### Memory Sanitizer (Uninitialized Memory)
```bash
make -C tests EXTRA_CFLAGS="-fsanitize=memory -fno-omit-frame-pointer" \
              EXTRA_LDFLAGS="-fsanitize=memory"
cd tests && ./run_all_tests.sh
```

### Thread Sanitizer (Race Conditions)
```bash
export TSAN_OPTIONS="halt_on_error=1"
make -C tests EXTRA_CFLAGS="-fsanitize=thread" \
              EXTRA_LDFLAGS="-fsanitize=thread"
cd tests && ./run_all_tests.sh
```

### Undefined Behavior Sanitizer
```bash
make -C tests EXTRA_CFLAGS="-fsanitize=undefined -fno-omit-frame-pointer" \
              EXTRA_LDFLAGS="-fsanitize=undefined"
cd tests && ./run_all_tests.sh
```

### All Sanitizers Combined
```bash
make -C tests EXTRA_CFLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
              EXTRA_LDFLAGS="-fsanitize=address,undefined"
cd tests && ./run_all_tests.sh
```

## Interpreting Results

### Successful Test Run
```
Unity Framework v2.5.2
...
All Tests: 45 Passed, 0 Failed, 0 Ignored
Overall Test Result: PASSED
```

### Memory Leak Detection
```
ERROR: AddressSanitizer: detected memory leaks

Direct leak of 1024 bytes in 1 objects allocated from:
    #0 0x... in malloc
    #1 0x... in graphAllocateNode src/graph.c:123
    #2 0x... in test_node_creation tests/test_graph.c:45
```
**Action**: Check for missing `free()` calls or use `graph_safe_free()` wrapper.

### Buffer Overflow Detection
```
ERROR: AddressSanitizer: stack-buffer-overflow on address 0x...
READ of size 1 at 0x... thread T0
    #0 0x... in strcpy
    #1 0x... in copyNodeName src/graph.c:156
```
**Action**: Use `strncpy()` or `snprintf()` with proper bounds checking.

### Use-After-Free Detection
```
ERROR: AddressSanitizer: heap-use-after-free on address 0x...
    #0 0x... in graphAccessNode src/graph.c:234
    #1 0x... in test_node_access tests/test_graph.c:78
```
**Action**: Ensure proper object lifecycle management and avoid dangling pointers.

### Race Condition Detection
```
WARNING: ThreadSanitizer: data race (pid=1234)
  Write of size 4 at 0x... by thread T2:
    #0 graphUpdateCounter src/graph.c:345
  Previous read of size 4 at 0x... by thread T1:
    #0 graphGetCounter src/graph.c:351
```
**Action**: Add proper synchronization (mutexes, atomic operations).

## Test Categories

### Unit Tests (`test_*.c`)
- Test individual functions and components
- Focus on correctness and edge cases
- Should run quickly (< 100ms per test)

### Integration Tests (`tck_*.c`)
- Test component interactions
- Database operations and SQL integration
- May take longer to execute

### Performance Tests (`benchmark_*.c`)
- Measure execution time and memory usage
- Regression testing for performance
- Generate performance reports

### Memory Safety Tests
- Verify bounds checking
- Test error handling paths
- Validate resource cleanup

## Test Data Management

### Temporary Files
Tests create temporary databases that are automatically cleaned up:
```c
// Temporary test database - automatically removed
sqlite3_open(":memory:", &db);  // In-memory DB
// OR
sqlite3_open("test_temp.db", &db);  // File-based - cleaned up in tearDown
```

### Test Fixtures
Large test datasets are stored in `tests/fixtures/`:
- `karate_club.json` - Standard graph theory test case
- `social_network.sql` - Sample social network data
- `performance_data.csv` - Large dataset for benchmarks

## Continuous Integration

The CI pipeline runs tests with:
1. Multiple compiler versions (GCC, Clang)
2. All sanitizers enabled
3. Various SQLite versions
4. Different optimization levels
5. Static analysis tools (cppcheck, scan-build)

### Local CI Simulation
```bash
# Run the same checks as CI
./scripts/ci_test_simulation.sh
```

## Troubleshooting

### Common Issues

#### Tests Not Building
- **Issue**: Missing dependencies
- **Solution**: Run `make -C _deps` first

#### Segmentation Faults
- **Issue**: Memory corruption or NULL pointer access
- **Solution**: Run with AddressSanitizer to get detailed stack trace

#### Tests Hanging
- **Issue**: Deadlock or infinite loop
- **Solution**: Run with timeout or ThreadSanitizer

#### False Positives with Sanitizers
- **Issue**: SQLite internal operations trigger warnings
- **Solution**: Use sanitizer suppression files in `tests/sanitizers/`

### Getting Help

1. Check existing tests for similar patterns
2. Review `tests/test_database_utils.c` for utility functions
3. Run with verbose output: `./run_all_tests.sh -v`
4. Use debugger: `gdb ./build/tests/test_myfeature`

## Best Practices

### Test Writing
1. **One concept per test** - Each test should verify one specific behavior
2. **Descriptive names** - `test_graph_creation_with_invalid_params` not `test1`
3. **Clear assertions** - Use specific TEST_ASSERT_* macros
4. **Proper setup/teardown** - Always clean up resources

### Memory Safety
1. **Test error paths** - Verify behavior when malloc() fails
2. **Boundary testing** - Test with zero, one, and maximum values
3. **Resource cleanup** - Ensure tests don't leak memory
4. **Thread safety** - Test concurrent access patterns

### Performance
1. **Baseline measurements** - Record expected performance ranges
2. **Regression testing** - Alert on significant slowdowns
3. **Memory usage** - Monitor heap growth during operations
4. **Scaling tests** - Verify O(n) complexity assumptions
