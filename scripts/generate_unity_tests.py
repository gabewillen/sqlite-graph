#!/usr/bin/env python3
"""
Generate Unity test files for TCK scenarios
Creates test files following the naming convention: test_<group>_<id>()
"""

import json
import os
from pathlib import Path

def sanitize_group_name(group_name):
    """Convert feature group name to valid C identifier."""
    return group_name.replace('-', '_').replace('.', '_')

def generate_test_file(group_name, scenarios):
    """Generate Unity test file for a feature group."""
    sanitized_name = sanitize_group_name(group_name)
    filename = f"test_tck_{sanitized_name}.c"
    
    content = f'''/*
 * Unity Test File: TCK {group_name}
 * Auto-generated from openCypher TCK scenarios
 * Feature Group: {group_name}
 */

#include "unity.h"
#include "../src/graph-extension.h"
#include "../src/cypher-extension.h"
#include "test_util.h"

void setUp(void) {{
    // Setup code for each test
}}

void tearDown(void) {{
    // Cleanup code for each test
}}

'''

    for scenario in scenarios:
        test_func_name = scenario['unity_test_function']
        scenario_id = scenario['scenario_id']
        
        content += f'''/*
 * TCK Scenario: {scenario_id}
 * Description: TODO - Add description from TCK
 * Cypher: {scenario['cypher_query'] or 'TODO - Add Cypher query'}
 * Expected: {scenario['expected_result'] or 'TODO - Add expected result'}
 * Status: {scenario['status']}
 */
void {test_func_name}(void) {{
    TEST_IGNORE_MESSAGE("TCK Scenario {scenario_id} - Not yet implemented");
    
    // TODO: Implement test for {scenario_id}
    // const char* query = "{scenario['cypher_query'] or 'TODO'}";
    // Expected result: {scenario['expected_result'] or 'TODO'}
}}

'''

    return filename, content

def main():
    # Load coverage data
    with open('coverage.json', 'r') as f:
        coverage_data = json.load(f)
    
    # Create tests directory if it doesn't exist
    tests_dir = Path('tests')
    tests_dir.mkdir(exist_ok=True)
    
    # Group scenarios by feature group
    scenarios_by_group = {}
    for scenario in coverage_data['scenarios']:
        group = scenario['feature_group']
        if group not in scenarios_by_group:
            scenarios_by_group[group] = []
        scenarios_by_group[group].append(scenario)
    
    # Generate test files
    generated_files = []
    for group_name, scenarios in scenarios_by_group.items():
        filename, content = generate_test_file(group_name, scenarios)
        filepath = tests_dir / filename
        
        # Only generate if file doesn't exist or if forced
        if not filepath.exists():
            with open(filepath, 'w') as f:
                f.write(content)
            generated_files.append(str(filepath))
            print(f"Generated: {filepath}")
        else:
            print(f"Skipped (exists): {filepath}")
    
    print(f"\\nGenerated {len(generated_files)} test files")
    return generated_files

if __name__ == '__main__':
    main()
