#!/usr/bin/env python3
import re
import sys

def fix_test_file(filename, setup_func, teardown_func):
    with open(filename, 'r') as f:
        content = f.read()
    
    # Find all test functions
    test_pattern = r'(void test[A-Za-z0-9_]+\(void\)\{)'
    
    # For each test function, check if it has setup/teardown
    lines = content.split('\n')
    new_lines = []
    i = 0
    
    while i < len(lines):
        line = lines[i]
        
        # Check if this is a test function
        if re.match(r'void test[A-Za-z0-9_]+\(void\)\{', line):
            new_lines.append(line)
            i += 1
            
            # Look for the first non-declaration line
            found_setup = False
            brace_count = 1
            func_lines = []
            
            while i < len(lines) and brace_count > 0:
                line = lines[i]
                
                # Count braces
                brace_count += line.count('{') - line.count('}')
                
                # Check if setup is already there
                if setup_func in line:
                    found_setup = True
                
                # If this is the first real code line and no setup, add it
                if not found_setup and not line.strip().startswith('//') and not re.match(r'^\s*(char|int|double|void)\s+.*[;=]', line.strip()) and line.strip() and line.strip() != '{':
                    # Add setup before this line
                    indent = '  '
                    new_lines.append(f'{indent}{setup_func}();')
                    found_setup = True
                
                func_lines.append(line)
                i += 1
            
            # Add all function lines except the last }
            for j, fline in enumerate(func_lines[:-1]):
                new_lines.append(fline)
            
            # Add teardown before the closing brace if not already there
            if teardown_func not in '\n'.join(func_lines):
                new_lines.append(f'  {teardown_func}();')
            
            # Add the closing brace
            if func_lines:
                new_lines.append(func_lines[-1])
        else:
            new_lines.append(line)
            i += 1
    
    # Write back
    with open(filename, 'w') as f:
        f.write('\n'.join(new_lines))

# Fix each test file
files_to_fix = [
    ('/shared/sqlite-graph/tests/test_traversal.c', 'setupTraversalTests', 'teardownTraversalTests'),
    ('/shared/sqlite-graph/tests/test_algorithms.c', 'setupAlgorithmTests', 'teardownAlgorithmTests'),
    ('/shared/sqlite-graph/tests/test_schema.c', 'setupSchemaTests', 'teardownSchemaTests'),
    ('/shared/sqlite-graph/tests/test_performance.c', 'setupPerformanceTests', 'teardownPerformanceTests'),
]

for filename, setup, teardown in files_to_fix:
    print(f"Fixing {filename}...")
    fix_test_file(filename, setup, teardown)

print("Done!")