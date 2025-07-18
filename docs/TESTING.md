# Testing Guide

This project uses Make for all build and test operations.

## Building and Running Tests

### Quick Start - From Project Root

```bash
# Build the project
make

# Run all tests
make test

# Run specific test
./build/tests/test_cypher_basic
```

### Traditional Method - From Build Directory

```bash
# Configure the build
mkdir build
cd build
cmake ..

# Build everything
make -j$(nproc)

# Run all tests
ctest

# Or run with verbose output
ctest -V

# Run specific test by name
ctest -R graph_tests

# Run tests by label
ctest -L unit
ctest -L cypher
ctest -L performance
```

### Test Organization

All tests are managed through Make:

1. **Main Test Suite** (`test_graph`)
   - Combines all unit tests into a single executable
   - Includes 150+ test functions
   - Run with: `ctest -R graph_tests`

2. **Coverage Test Suite** (`test_graph_coverage`)
   - Same as main suite but with coverage instrumentation
   - Run with: `ctest -R graph_coverage_tests`

3. **Individual Test Executables**
   - Specific test scenarios that can be run independently
   - Useful for debugging specific features
   - Examples:
     - `test_cypher_basic` - Basic Cypher functionality
     - `test_write_comprehensive` - Comprehensive write operations
     - `test_transaction_complete` - Transaction management

### Running Tests with CTest

```bash
# Run all tests
ctest

# Run with parallel execution
ctest -j4

# Run specific test pattern
ctest -R cypher  # Runs all tests with "cypher" in name

# Run by label
ctest -L unit          # Unit tests
ctest -L integration   # Integration tests
ctest -L cypher        # Cypher-specific tests
ctest -L write_ops     # Write operation tests
ctest -L performance   # Performance tests

# Exclude tests
ctest -E performance   # Run all except performance tests

# Run with timeout
ctest --timeout 30     # 30 second timeout per test

# Show test output on failure
ctest --output-on-failure

# Generate test report
ctest -T Test
```

### Test Coverage

Generate coverage reports using the coverage script:

```bash
./scripts/coverage.sh
```

Or manually with CMake:

```bash
cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DCOVERAGE=ON ..
make test_graph_coverage
./tests/test_graph_coverage
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_report
```

### Debugging Tests

```bash
# Run test under debugger
gdb ./tests/test_graph
(gdb) run

# Run with Valgrind
valgrind --leak-check=full ./tests/test_graph

# Run specific test function with Unity
./tests/test_graph -n testGraphAddNodeSuccess
```

### Continuous Integration

Tests are automatically run on:
- Every push to main branch
- Every pull request
- Nightly builds

See `.github/workflows/test.yml` for CI configuration.

### Adding New Tests

1. **Add to existing test file**:
   ```c
   // In test_algorithms.c
   void testNewAlgorithm(void) {
       TEST_ASSERT_EQUAL(expected, actual);
   }
   
   // In main()
   RUN_TEST(testNewAlgorithm);
   ```

2. **Create new test file**:
   - Create `tests/test_new_feature.c`
   - Add to `tests/Makefile`:
```makefile
     add_executable(test_graph
         # ... existing files ...
         test_new_feature.c
     )
     ```

3. **Run your new test**:
   ```bash
   cd build
   make
   ctest -R graph_tests
   ```

### Test Categories

| Category | Label | Description |
|----------|-------|-------------|
| Unit Tests | `unit` | Individual function tests |
| Integration Tests | `integration` | Multi-component tests |
| Cypher Tests | `cypher` | Cypher language tests |
| Write Operations | `write_ops` | CREATE/UPDATE/DELETE tests |
| Transaction Tests | `transactions` | ACID compliance tests |
| Performance Tests | `performance` | Benchmarks and stress tests |
| TCK Tests | `tck` | openCypher compliance tests |

### Performance Testing

Run performance regression tests:

```bash
./scripts/perf_regression.sh baseline  # Create baseline
./scripts/perf_regression.sh test      # Compare with baseline
```

### Best Practices

1. **Always use CTest** - Don't create Makefiles or custom test scripts
2. **Group related tests** - Use test labels for organization
3. **Test isolation** - Each test should be independent
4. **Clean state** - Use setup/teardown functions
5. **Meaningful names** - Test names should describe what they test
6. **Fast tests** - Keep individual tests under 1 second
7. **Deterministic** - Tests should not be flaky

### Troubleshooting

**Tests not found:**
```bash
# Regenerate build files
cd build
cmake ..
make
```

**Test failures:**
```bash
# Run with verbose output
ctest -V -R failing_test

# Check test logs
cat Testing/Temporary/LastTest.log
```

**Coverage not working:**
```bash
# Ensure Debug build
cmake -DCMAKE_BUILD_TYPE=Debug ..
```