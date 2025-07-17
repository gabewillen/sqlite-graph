# tests/Makefile

# Compiler and flags
CC = gcc
CFLAGS = -I../include -I../src -I../_deps/sqlite-src -I../_deps/Unity-2.5.2/src -g -O0 -std=gnu99
LDFLAGS = -L../build -lgraph_static -lgraph_test_util -lsqlite3_lib -lunity -lm -ldl -lpthread

# Directories
BUILD_DIR = ../build
OBJ_DIR = $(BUILD_DIR)/obj/tests
TEST_EXEC_NAME = test_runner
TEST_EXECUTABLE = $(BUILD_DIR)/$(TEST_EXEC_NAME)

# Test source files
TEST_SRCS = $(wildcard *.c)

# Object files
TEST_OBJS = $(patsubst %.c,$(OBJ_DIR)/%.o,$(TEST_SRCS))

.PHONY: all clean test rebuild

all: $(TEST_EXECUTABLE)

$(TEST_EXECUTABLE): $(TEST_OBJS)
	$(CC) $(TEST_OBJS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

test: rebuild
	@echo "Running all tests..."
	$(TEST_EXECUTABLE)

rebuild: clean all

clean:
	rm -f $(TEST_EXECUTABLE)
	rm -rf $(OBJ_DIR)