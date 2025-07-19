# tests/Makefile

# Compiler and flags
CC = gcc
CFLAGS = -I../include -I../src -I../_deps/sqlite-src -I../_deps/Unity-2.5.2/src -I../include/cypher -g -O0 -std=gnu99
# Ensure SQLite extension support is enabled
CFLAGS += -DSQLITE_ENABLE_LOAD_EXTENSION=1
LDFLAGS = -L../build -lgraph_static -lgraph_test_util -lsqlite3 -lunity -lm -ldl -lpthread

# Directories
BUILD_DIR = ../build
OBJ_DIR = $(BUILD_DIR)/obj/tests
TEST_EXEC_DIR = $(BUILD_DIR)/tests

# Test executables we want to build individually (based on run_all_tests.sh)
INDIVIDUAL_TESTS = test_cypher_basic test_planner_compile test_executor_compile \
                  test_write_simple test_merge_simple test_write_comprehensive \

# Additional tests that can be built
ADDITIONAL_TESTS = test_cypher_lexer test_cypher_parser test_cypher_ast \
                  test_extension_loading test_virtual_table_comprehensive \
                  test_graph_updates

# TCK specific tests
TCK_TESTS = tck_main

# All test executables
ALL_TESTS = $(INDIVIDUAL_TESTS) $(ADDITIONAL_TESTS) $(TCK_TESTS)

# Default target builds all individual tests
.PHONY: all clean test rebuild individual additional tck

all: individual

individual: $(addprefix $(TEST_EXEC_DIR)/,$(INDIVIDUAL_TESTS))

additional: $(addprefix $(TEST_EXEC_DIR)/,$(ADDITIONAL_TESTS))

# TCK target - builds and runs all TCK tests with extension support
tck: $(addprefix $(TEST_EXEC_DIR)/,$(TCK_TESTS))
	@echo "=== Running TCK Test Suite ==="
	$(TEST_EXEC_DIR)/tck_main

full: $(addprefix $(TEST_EXEC_DIR)/,$(ALL_TESTS))

# Rule to build individual test executables
$(TEST_EXEC_DIR)/%: %.c | $(TEST_EXEC_DIR) $(OBJ_DIR)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# Special case for tck_main which needs additional includes
$(TEST_EXEC_DIR)/tck_main: tck_main.c | $(TEST_EXEC_DIR) $(OBJ_DIR)
	$(CC) $(CFLAGS) -I../tck $< -o $@ -L../build -lgraph_static -lgraph_test_util -lsqlite3 -lunity -lm -ldl -lpthread

# Special case for tests that need additional source files
$(TEST_EXEC_DIR)/test_extension_loading: test_extension_loading.c test_database_utils.c | $(TEST_EXEC_DIR) $(OBJ_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(TEST_EXEC_DIR)/test_virtual_table_comprehensive: test_virtual_table_comprehensive.c test_database_utils.c | $(TEST_EXEC_DIR) $(OBJ_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Create directories
$(TEST_EXEC_DIR):
	@mkdir -p $@

$(OBJ_DIR):
	@mkdir -p $@

# Run individual tests using the existing script
test: individual
	@echo "Running individual tests..."
	cd .. && tests/run_all_tests.sh

rebuild: clean all

clean:
	rm -rf $(TEST_EXEC_DIR)
	rm -rf $(OBJ_DIR)

# Help target
help:
	@echo "Available targets:"
	@echo "  all        - Build individual tests (default)"
	@echo "  individual - Build core individual tests"
	@echo "  additional - Build additional tests"
	@echo "  tck        - Build and run TCK test suite"
	@echo "  full       - Build all tests"
	@echo "  test       - Build and run tests using run_all_tests.sh"
	@echo "  clean      - Clean build files"
	@echo ""
	@echo "Individual tests:"
	@for test in $(INDIVIDUAL_TESTS); do echo "    $$test"; done
	@echo ""
	@echo "Additional tests:"
	@for test in $(ADDITIONAL_TESTS); do echo "    $$test"; done
	@echo ""
	@echo "TCK tests:"
	@for test in $(TCK_TESTS); do echo "    $$test"; done

	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# TCK Coverage targets
.PHONY: tck-coverage tck-report tck-generate

tck-coverage: coverage.json
	@python3 ../scripts/update_compliance.py

tck-report: tck-coverage
	@echo "=== TCK Compliance Report ==="
	@jq -r '"Total Scenarios: " + (.statistics.total_scenarios | tostring)' coverage.json
	@jq -r '"Passed: " + (.statistics.passed | tostring)' coverage.json  
	@jq -r '"Failed: " + (.statistics.failed | tostring)' coverage.json
	@jq -r '"Skipped: " + (.statistics.skipped | tostring)' coverage.json
	@jq -r '"Pending: " + (.statistics.pending | tostring)' coverage.json
	@jq -r '"Compliance: " + (.statistics.compliance_percentage | tostring) + "%"' coverage.json
	@echo "=== High Priority Pending ==="
	@jq -r '.scenarios[] | select(.status == "pending" and .priority == "high") | .unity_test_function' coverage.json | head -10

tck-generate:
	@python3 ../scripts/generate_unity_tests.py

# Update coverage after running tests
update-coverage:
	@python3 ../scripts/update_compliance.py > /dev/null
	@echo "Coverage updated: $$(jq -r '.statistics.compliance_percentage' coverage.json)%"
