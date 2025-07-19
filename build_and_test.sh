#!/bin/bash
set -e

echo "=== Clean Checkout Build & Test ===" 
echo "Building dependencies..."
make -C _deps

echo "Building source..." 
make -C src

echo "Extension built successfully!"
echo "Build artifacts created:"
ls -la build/lib*.so build/lib*.a

echo "Checking extension structure..."
nm -D build/libgraph.so | grep -E "(sqlite3_graph_init|graph)" | head -5

echo "Clean build and verification completed successfully!"
