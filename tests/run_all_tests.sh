#!/bin/bash

# SQLite Graph Database Extension - Complete Test Suite Runner
# This script runs all compilation and functional tests for the project

set -euo pipefail

# Default options
KEEP_GOING=false
OUTPUT_FORMAT="tap"  # Options: tap, junit
JUNIT_FILE="test-results.xml"

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --keep-going)
            KEEP_GOING=true
            shift
            ;;
        --output=*)
            OUTPUT_FORMAT="${1#*=}"
            shift
            ;;
        --junit-file=*)
            JUNIT_FILE="${1#*=}"
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [options]"
            echo ""
            echo "Options:"
            echo "  --keep-going        Continue running tests after failures"
            echo "  --output=FORMAT     Output format: tap or junit (default: tap)"
            echo "  --junit-file=FILE   JUnit XML output file (default: test-results.xml)"
            echo "  -h, --help          Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

echo "=============================================="
echo "SQLite Graph Database Extension - Test Suite"
echo "=============================================="
echo ""

# Change to tests directory to access Makefile
SCRIPT_DIR="$(dirname "$0")"
cd "$SCRIPT_DIR"

# Get ALL_TESTS from Makefile using the print-tests target
ALL_TESTS=""
if command -v make >/dev/null 2>&1 && [[ -f "Makefile" ]]; then
    # Use the print-tests target to get the test list
    ALL_TESTS=$(make -s print-tests 2>/dev/null || true)
fi

# Method 2: Extract from Makefile directly if Method 1 failed
if [[ -z "$ALL_TESTS" ]]; then
    echo "Warning: Could not get test list from 'make print-tests'. Using direct file discovery."
    if [[ -f "Makefile" ]]; then
        # Find all .c files that match test_* pattern, excluding utility files
        ALL_TESTS=$(find . -maxdepth 1 -name "test_*.c" -not -name "test_database_utils.c" -not -name "test_util.h" -exec basename {} .c \; | sort | tr '\n' ' ' || true)
    fi
fi

# Method 3: If still empty, use a minimal fallback based on common tests
if [[ -z "$ALL_TESTS" ]]; then
    echo "Warning: Could not determine test list from Makefile. Using fallback discovery."
    ALL_TESTS="test_cypher_basic test_planner_compile test_executor_compile test_write_simple test_merge_simple test_write_comprehensive test_transaction_complete test_tck_basic"
fi

# Remove any empty entries and convert to array
read -ra TEST_ARRAY <<< "$ALL_TESTS"

# Filter out empty entries
FILTERED_TESTS=()
for test in "${TEST_ARRAY[@]}"; do
    if [[ -n "$test" ]]; then
        FILTERED_TESTS+=("$test")
    fi
done

echo "Discovered tests from Makefile: ${FILTERED_TESTS[*]}"
echo ""

# Change to build/tests directory for test execution
if [[ ! -d "../build/tests" ]]; then
    echo "Error: ../build/tests directory not found. Please run 'make' first to build tests."
    exit 1
fi

cd "../build/tests"

# Initialize counters
TOTAL_TESTS=${#FILTERED_TESTS[@]}
PASSED_TESTS=0
FAILED_TESTS=0

# Arrays to store test results for output formatting
PASSED_TEST_NAMES=()
FAILED_TEST_NAMES=()
TEST_RESULTS=()
TEST_START_TIME=$(date +%s)

# Function to initialize output format
init_output() {
    case $OUTPUT_FORMAT in
        tap)
            echo "TAP version 13"
            echo "1..$TOTAL_TESTS"
            ;;
        junit)
            # JUnit XML will be written at the end
            ;;
    esac
}

# Function to output test result
output_test_result() {
    local test_name="$1"
    local passed="$2"
    local test_num="$3"
    
    case $OUTPUT_FORMAT in
        tap)
            if [[ "$passed" == "true" ]]; then
                echo "ok $test_num - $test_name"
            else
                echo "not ok $test_num - $test_name"
            fi
            ;;
        junit)
            # Results stored for final XML output
            ;;
    esac
}

# Function to finalize output
finalize_output() {
    case $OUTPUT_FORMAT in
        tap)
            # TAP output is already complete
            ;;
        junit)
            generate_junit_xml
            ;;
    esac
}

# Function to generate JUnit XML
generate_junit_xml() {
    local timestamp=$(date -Iseconds)
    local end_time=$(date +%s)
    local duration=$((end_time - TEST_START_TIME))
    
    cat > "$JUNIT_FILE" << XMLEOF
<?xml version="1.0" encoding="UTF-8"?>
<testsuites name="SQLite Graph Extension Tests" tests="$TOTAL_TESTS" failures="$FAILED_TESTS" time="$duration" timestamp="$timestamp">
  <testsuite name="AllTests" tests="$TOTAL_TESTS" failures="$FAILED_TESTS" time="$duration">
XMLEOF

    for i in "${!FILTERED_TESTS[@]}"; do
        local test_name="${FILTERED_TESTS[$i]}"
        local result="${TEST_RESULTS[$i]}"
        
        if [[ "$result" == "PASSED" ]]; then
            echo "    <testcase name=\"$test_name\" classname=\"SQLiteGraphTests\" time=\"0\"/>" >> "$JUNIT_FILE"
        else
            echo "    <testcase name=\"$test_name\" classname=\"SQLiteGraphTests\" time=\"0\">" >> "$JUNIT_FILE"
            echo "      <failure message=\"Test failed\">Test executable failed or not found</failure>" >> "$JUNIT_FILE"
            echo "    </testcase>" >> "$JUNIT_FILE"
        fi
    done
    
    cat >> "$JUNIT_FILE" << XMLEOF
  </testsuite>
</testsuites>
XMLEOF
    
    echo "JUnit XML report written to: $JUNIT_FILE"
}

# Function to run a test and track results
run_test() {
    local test_name="$1"
    local test_executable="$2"
    local test_num="$3"
    
    echo "Running: $test_name"
    echo "----------------------------------------"
    
    local test_passed=false
    
    if [ -f "$test_executable" ]; then
        if ./"$test_executable" 2>&1; then
            echo "✅ PASSED: $test_name"
            test_passed=true
            PASSED_TESTS=$((PASSED_TESTS + 1))
            PASSED_TEST_NAMES+=("$test_name")
            TEST_RESULTS+=("PASSED")
        else
            echo "❌ FAILED: $test_name"
            FAILED_TESTS=$((FAILED_TESTS + 1))
            FAILED_TEST_NAMES+=("$test_name")
            TEST_RESULTS+=("FAILED")
        fi
    else
        echo "❌ NOT FOUND: $test_executable"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        FAILED_TEST_NAMES+=("$test_name")
        TEST_RESULTS+=("FAILED")
    fi
    
    output_test_result "$test_name" "$test_passed" "$test_num"
    echo ""
    
    # Exit on first failure unless --keep-going is specified
    if [[ "$test_passed" == false && "$KEEP_GOING" == false ]]; then
        echo "❌ Test failed and --keep-going not specified. Exiting."
        finalize_output
        exit 1
    fi
}

# Initialize output format
init_output

# Run all tests
echo "Starting test execution..."
echo "Found ${#FILTERED_TESTS[@]} tests to run"
echo ""

test_counter=1
for test_name in "${FILTERED_TESTS[@]}"; do
    if [[ -n "$test_name" ]]; then
        run_test "$test_name" "$test_name" "$test_counter"
        ((test_counter++))
    fi
done

# Finalize output format
finalize_output

# Print final summary
echo "=============================================="
echo "TEST SUITE SUMMARY"
echo "=============================================="
echo "Total Tests:    $TOTAL_TESTS"
echo "Passed:         $PASSED_TESTS"
echo "Failed:         $FAILED_TESTS"

if [ $FAILED_TESTS -eq 0 ]; then
    echo "Success Rate:   100.0%"
    echo ""
    echo "🎉 ALL TESTS PASSED! 🎉"
    echo "The SQLite Graph Database Extension is working correctly."
    exit 0
else
    SUCCESS_RATE=$(echo "scale=1; $PASSED_TESTS * 100 / $TOTAL_TESTS" | bc -l 2>/dev/null || echo "N/A")
    echo "Success Rate:   ${SUCCESS_RATE}%"
    echo ""
    echo "⚠️  SOME TESTS FAILED ⚠️"
    echo "Please check the test output above for details."
    echo ""
    echo "Failed tests:"
    for failed_test in "${FAILED_TEST_NAMES[@]}"; do
        echo "  - $failed_test"
    done
    exit 1
fi
