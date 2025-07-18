# tests/Makefile

# Compiler and flags
CC = gcc
CFLAGS = -I../include -I../src -I../_deps/sqlite-src -I../_deps/Unity-2.5.2/src -g -O0 -std=gnu99
LDFLAGS = -L../build -lgraph_static -lgraph_test_util -lsqlite3_lib -lunity -lm -ldl -lpthread

# Directories
BUILD_DIR = ../build
OBJ_DIR = $(BUILD_DIR)/obj/tests
TEST_EXEC_DIR = $(BUILD_DIR)/tests

# Test executables we want to build individually (based on run_all_tests.sh)
INDIVIDUAL_TESTS = test_cypher_basic test_planner_compile test_executor_compile \
                  test_write_simple test_merge_simple test_write_comprehensive \
                  test_transaction_complete test_tck_basic

# Additional tests that can be built
ADDITIONAL_TESTS = test_cypher_lexer test_cypher_parser test_cypher_ast \
                  test_extension_loading test_virtual_table_comprehensive

# All test executables
ALL_TESTS = $(INDIVIDUAL_TESTS) $(ADDITIONAL_TESTS)

# Default target builds all individual tests
.PHONY: all clean test rebuild individual additional

all: individual

individual: $(addprefix $(TEST_EXEC_DIR)/,$(INDIVIDUAL_TESTS))

additional: $(addprefix $(TEST_EXEC_DIR)/,$(ADDITIONAL_TESTS))

full: $(addprefix $(TEST_EXEC_DIR)/,$(ALL_TESTS))

# Rule to build individual test executables
$(TEST_EXEC_DIR)/%: %.c | $(TEST_EXEC_DIR) $(OBJ_DIR)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

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
	@echo "  full       - Build all tests"
	@echo "  test       - Build and run tests using run_all_tests.sh"
	@echo "  clean      - Clean build files"
	@echo ""
	@echo "Individual tests:"
	@for test in $(INDIVIDUAL_TESTS); do echo "    $$test"; done
	@echo ""
	@echo "Additional tests:"
	@for test in $(ADDITIONAL_TESTS); do echo "    $$test"; done
