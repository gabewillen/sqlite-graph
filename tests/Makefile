BUILD_DIR = ../build
CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99
LDFLAGS = -lm -ldl -lpthread

# Unity and SQLite libraries (assuming they are built by the main Makefile or are available)
UNITY_LIB = $(BUILD_DIR)/unity.a
SQLITE_OBJ = $(BUILD_DIR)/sqlite3.o
GRAPH_STATIC_LIB = $(BUILD_DIR)/libgraph.a

# Test source files
TEST_SRCS_COMMON = \
	test_main.c \
	test_storage.c \
	test_algorithms.c \
	test_traversal.c \
	test_virtual_table.c \
	test_integration.c \
	test_performance.c \
	test_advanced.c \
	test_schema.c \
	 \
	test_performance_optimization.c \
	test_performance_features.c

TEST_SRCS_EXTENSION_LOADING = \
	test_extension_loading.c \
	test_database_utils.c

TEST_SRCS_VIRTUAL_TABLE_COMPREHENSIVE = \
	test_virtual_table_comprehensive.c \
	test_database_utils.c

INDIVIDUAL_TEST_NAMES = \
	test_cypher_basic \
	test_planner_compile \
	test_executor_compile \
	test_write_simple \
	test_merge_simple \
	test_write_comprehensive \
	test_transaction_complete \
	test_tck_basic

TEST_SRCS_INDIVIDUAL = $(addsuffix .c, $(INDIVIDUAL_TEST_NAMES))

# All test source files
ALL_TEST_SRCS = $(TEST_SRCS_COMMON) $(TEST_SRCS_EXTENSION_LOADING) $(TEST_SRCS_VIRTUAL_TABLE_COMPREHENSIVE) $(TEST_SRCS_INDIVIDUAL)

# All test object files
ALL_TEST_OBJS = $(patsubst %.c,$(BUILD_DIR)/tests/%.o,$(ALL_TEST_SRCS))

# All test executables
TEST_EXES = \
	$(BUILD_DIR)/tests/test_graph \
	$(BUILD_DIR)/tests/test_graph_coverage \
	$(BUILD_DIR)/tests/test_extension_loading \
	$(BUILD_DIR)/tests/test_virtual_table_comprehensive \
	$(addprefix $(BUILD_DIR)/tests/, $(INDIVIDUAL_TEST_NAMES))

INCLUDES = -I../include -I../_deps/Unity-$(shell echo $(UNITY_VERSION) | sed 's/^v//')/src -I../_deps/sqlite-src

.PHONY: all clean test

all: $(TEST_EXES)

# Generic rule for compiling test C files
$(BUILD_DIR)/tests/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Specific rule for test_graph_coverage objects
$(BUILD_DIR)/tests/test_graph_coverage.o: test_graph_coverage.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDES) --coverage -c $< -o $@

# Link test_graph
$(BUILD_DIR)/tests/test_graph: $(patsubst %.c,$(BUILD_DIR)/tests/%.o,$(TEST_SRCS_COMMON)) $(GRAPH_STATIC_LIB) $(UNITY_LIB) $(SQLITE_OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

# Link test_graph_coverage
$(BUILD_DIR)/tests/test_graph_coverage: $(patsubst %.c,$(BUILD_DIR)/tests/%.o,$(TEST_SRCS_COMMON)) $(GRAPH_STATIC_LIB) $(UNITY_LIB) $(SQLITE_OBJ)
	$(CC) $^ -o $@ $(LDFLAGS) --coverage

# Link test_extension_loading
$(BUILD_DIR)/tests/test_extension_loading: $(patsubst %.c,$(BUILD_DIR)/tests/%.o,$(TEST_SRCS_EXTENSION_LOADING)) $(GRAPH_STATIC_LIB) $(UNITY_LIB) $(SQLITE_OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

# Link test_virtual_table_comprehensive
$(BUILD_DIR)/tests/test_virtual_table_comprehensive: $(patsubst %.c,$(BUILD_DIR)/tests/%.o,$(TEST_SRCS_VIRTUAL_TABLE_COMPREHENSIVE)) $(GRAPH_STATIC_LIB) $(UNITY_LIB) $(SQLITE_OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

# Generic rule for linking individual tests
$(BUILD_DIR)/tests/%: %.o $(GRAPH_STATIC_LIB) $(UNITY_LIB) $(SQLITE_OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

# Re-define individual test linking to use specific object files
$(foreach test_name, $(INDIVIDUAL_TEST_NAMES),\n
$(BUILD_DIR)/tests/$(test_name): $(BUILD_DIR)/tests/$(test_name).o $(GRAPH_STATIC_LIB) $(UNITY_LIB) $(SQLITE_OBJ)\n
	$(CC) $(BUILD_DIR)/tests/$(test_name).o $(GRAPH_STATIC_LIB) $(UNITY_LIB) $(SQLITE_OBJ) -o $@ $(LDFLAGS)\n
)

test: $(TEST_EXES)
	@echo "Running tests..."
	@for exe in $(TEST_EXES); do \
		echo "Running $$exe..."; \
		$$exe || exit 1; \
	done
	@echo "All tests passed."

clean:
	rm -f $(ALL_TEST_OBJS)
	rm -f $(TEST_EXES)
	rm -rf $(BUILD_DIR)/tests