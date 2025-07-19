# Main Makefile

# Compiler and flags
CC = gcc
CFLAGS = -I$(CURDIR)/include -I$(CURDIR)/src -I$(CURDIR)/_deps/sqlite-src -I$(CURDIR)/_deps/Unity-2.5.2/src -g -O0 -std=gnu99 -fPIC
LDFLAGS = -lm -ldl -lpthread

# Add the SQLite source to the CFLAGS
CFLAGS += -I$(CURDIR)/_deps/sqlite-src

# Directories
BUILD_DIR = build
SRC_DIR = src
TESTS_DIR = tests

.PHONY: all clean test rebuild deps


all: deps
	@mkdir -p $(BUILD_DIR)
	$(MAKE) -C $(SRC_DIR) CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)"
	$(MAKE) -C $(TESTS_DIR) CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)"

deps:
	$(MAKE) -C _deps

clean:
	$(MAKE) -C $(SRC_DIR) clean
	$(MAKE) -C $(TESTS_DIR) clean
	$(MAKE) -C _deps clean
	rm -rf $(BUILD_DIR)

test:
	$(MAKE) -C $(TESTS_DIR) test

rebuild: clean all
