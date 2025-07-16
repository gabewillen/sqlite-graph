CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99
LDFLAGS = -lm
BUILD_DIR = build

.PHONY: all clean test rebuild grammar src tests

all: $(BUILD_DIR) src tests

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

src:
	$(MAKE) -C src

tests: src
	$(MAKE) -C tests

clean:
	$(MAKE) -C src clean
	$(MAKE) -C tests clean
	rm -rf $(BUILD_DIR)

test: all
	$(MAKE) -C tests test

rebuild: clean all
