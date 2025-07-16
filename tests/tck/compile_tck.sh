#!/bin/bash

# Compile openCypher TCK test runner

echo "Compiling openCypher TCK Test Runner..."

# Create build directory
mkdir -p ../../build/tck

# Compile the TCK runner
gcc -std=c89 -Wall -Wextra -pedantic \
    -I../../include \
    -I../../src \
    tck_runner.c \
    -lsqlite3 \
    -o ../../build/tck/tck_runner

if [ $? -eq 0 ]; then
    echo "✅ TCK runner compiled successfully"
    echo "Run with: ./build/tck/tck_runner"
else
    echo "❌ TCK runner compilation failed"
    exit 1
fi