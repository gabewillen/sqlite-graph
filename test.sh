#!/bin/bash

# SQLite Graph Database Extension - Test Runner Convenience Script
# This script runs the complete test suite from the project root

echo "Running SQLite Graph Database Extension Test Suite..."
echo ""

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Run the organized test suite
exec "$SCRIPT_DIR/tests/run_all_tests.sh"