BUILD_DIR = ../build
CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -fPIC
LDFLAGS = -lm

# External Dependencies
SQLITE_VERSION = 3450100
SQLITE_TARBALL = sqlite-autoconf-$(SQLITE_VERSION).tar.gz
SQLITE_URL = https://www.sqlite.org/2024/$(SQLITE_TARBALL)
SQLITE_DIR = ../_deps/sqlite-src

UNITY_VERSION = v2.5.2
UNITY_ZIP = $(UNITY_VERSION).zip
UNITY_URL = https://github.com/ThrowTheSwitch/Unity/archive/refs/tags/$(UNITY_ZIP)
UNITY_DIR = ../_deps/Unity-$(shell echo $(UNITY_VERSION) | sed 's/^v//')

# Source files for the shared library
SRCS_SHARED = \
	graph-advanced.c \
	graph-algo.c \
	graph-bulk.c \
	graph-cache.c \
	graph-compress.c \
	graph-enhanced.c \
	graph-json.c \
	graph-parallel.c \
	graph-performance.c \
	graph-schema.c \
	graph-traverse.c \
	graph-tvf.c \
	graph-util.c \
	graph-vtab.c \
	graph.c \
	cypher/cypher-ast.c \
	cypher/cypher-lexer.c \
	cypher/cypher-parser.c

# Source files for the static library (includes all shared sources plus more)
SRCS_STATIC = \
	$(SRCS_SHARED) \
	cypher/cypher-sql.c \
	cypher/cypher-logical-plan.c \
	cypher/cypher-physical-plan.c \
	cypher/cypher-planner.c \
	cypher/cypher-planner-sql.c \
	cypher/cypher-execution-context.c \
	cypher/cypher-iterators.c \
	cypher/cypher-executor.c \
	cypher/cypher-executor-sql.c \
	cypher/cypher-expressions.c \
	cypher/cypher-write.c \
	cypher/cypher-write-sql.c \
	cypher/cypher-json.c \
	cypher/cypher-storage.c \
	graph-benchmark.c \
	graph-cache.c \
	graph-compress.c \
	graph-bulk.c

# Object files for shared library
OBJS_SHARED = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS_SHARED))

# Object files for static library
OBJS_STATIC = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS_STATIC))

# SQLite object file
SQLITE_OBJ = $(BUILD_DIR)/sqlite3.o

# Unity library
UNITY_LIB = $(BUILD_DIR)/unity.a

# Include directories
INCLUDES = -I../include -I$(SQLITE_DIR) -I$(UNITY_DIR)/src

.PHONY: all clean deps

all: deps $(BUILD_DIR)/libgraph.so $(BUILD_DIR)/libgraph.a

deps: $(SQLITE_DIR) $(UNITY_DIR) $(SQLITE_OBJ) $(UNITY_LIB)

$(SQLITE_DIR):
	mkdir -p ../_deps
	curl -L $(SQLITE_URL) -o ../_deps/$(SQLITE_TARBALL)
	tar -xzf ../_deps/$(SQLITE_TARBALL) -C ../_deps/
	mv ../_deps/sqlite-autoconf-$(SQLITE_VERSION) $(SQLITE_DIR)

$(UNITY_DIR):
	mkdir -p ../_deps
	curl -L $(UNITY_URL) -o ../_deps/$(UNITY_ZIP)
	unzip ../_deps/$(UNITY_ZIP) -d ../_deps/
	mv ../_deps/Unity-$(UNITY_VERSION) $(UNITY_DIR)

# Rule to compile SQLite
$(SQLITE_OBJ): $(SQLITE_DIR)/sqlite3.c
	$(CC) $(CFLAGS) $(INCLUDES) -Wno-implicit-fallthrough -c $< -o $@

# Rule to build Unity
$(UNITY_LIB): $(UNITY_DIR)/src/unity.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $(UNITY_DIR)/src/unity.c -o $(BUILD_DIR)/unity.o
	$(AR) rcs $@ $(BUILD_DIR)/unity.o

# Generic rule for compiling C files
$(BUILD_DIR)/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Shared library
$(BUILD_DIR)/libgraph.so: $(OBJS_SHARED) $(SQLITE_OBJ)
	$(CC) $(OBJS_SHARED) $(SQLITE_OBJ) -shared -o $@ $(LDFLAGS)

# Static library
$(BUILD_DIR)/libgraph.a: $(OBJS_STATIC) $(SQLITE_OBJ) $(UNITY_LIB)
	$(AR) rcs $@ $(OBJS_STATIC) $(SQLITE_OBJ) $(BUILD_DIR)/unity.o

clean:
	rm -f $(OBJS_SHARED) $(OBJS_STATIC) $(SQLITE_OBJ) $(BUILD_DIR)/unity.o
	rm -f $(BUILD_DIR)/libgraph.so $(BUILD_DIR)/libgraph.a $(UNITY_LIB)
	rm -rf $(BUILD_DIR)/cypher
	rm -rf ../_deps
