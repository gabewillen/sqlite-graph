# Makefile for SQLite Graph Extension Benchmarks

CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -O3 -DNDEBUG -fPIC -DSQLITE_CORE
INCLUDES = -I../include -I../src -I../build/_deps/sqlite3-src
LDFLAGS = ../build/libsqlite3_lib.a -lm -lpthread -ldl -lgcov
BUILD_DIR = ../build
EXTENSION_LIB = $(BUILD_DIR)/graph_extension.so

# Benchmark targets
BENCHMARKS = ldbc_benchmark performance_regression scalability_test memory_profiler

all: $(BENCHMARKS)

# Ensure the main extension is built first
$(EXTENSION_LIB):
	$(MAKE) -C .. all

# LDBC Social Network Benchmark
ldbc_benchmark: ldbc_benchmark.c $(EXTENSION_LIB)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LDFLAGS)

# Performance regression test suite
performance_regression: performance_regression.c $(EXTENSION_LIB)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LDFLAGS)

# Scalability testing
scalability_test: scalability_test.c $(EXTENSION_LIB)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LDFLAGS)

# Memory profiling tools
memory_profiler: memory_profiler.c $(EXTENSION_LIB)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LDFLAGS)

# Clean targets
clean:
	rm -f $(BENCHMARKS)
	rm -f *.db *.log *.json *.csv

# Run all benchmarks
run-all: all
	@echo "Running LDBC Benchmark..."
	./ldbc_benchmark
	@echo ""
	@echo "Running Performance Regression Tests..."
	./performance_regression
	@echo ""
	@echo "Running Scalability Tests..."
	./scalability_test
	@echo ""
	@echo "Running Memory Profiler..."
	./memory_profiler

# Run individual benchmarks
run-ldbc: ldbc_benchmark
	./ldbc_benchmark

run-perf: performance_regression
	./performance_regression

run-scale: scalability_test
	./scalability_test

run-memory: memory_profiler
	./memory_profiler

# Generate performance report
report: all
	@echo "Generating performance report..."
	@mkdir -p reports
	./ldbc_benchmark > reports/ldbc_results.txt 2>&1
	./performance_regression > reports/performance_results.txt 2>&1
	./scalability_test > reports/scalability_results.txt 2>&1
	./memory_profiler > reports/memory_results.txt 2>&1
	@echo "Reports generated in reports/ directory"

.PHONY: all clean run-all run-ldbc run-perf run-scale run-memory report