#!/usr/bin/env python3
import re

# Read the file
with open('/shared/sqlite-graph/tests/test_schema.c', 'r') as f:
    content = f.read()

# Find all test functions and add setup/teardown if missing
def fix_test_function(match):
    func_content = match.group(0)
    
    # Check if setup already exists
    if 'setupSchemaTests()' not in func_content:
        # Find the first line after variable declarations
        lines = func_content.split('\n')
        new_lines = []
        inserted_setup = False
        
        for i, line in enumerate(lines):
            new_lines.append(line)
            
            # Insert setup after opening brace and variable declarations
            if not inserted_setup and i > 0 and line.strip() and \
               not line.strip().startswith('//') and \
               not re.match(r'^\s*(int|char|const|double|void)\s+.*[;=]', line.strip()) and \
               line.strip() != '{':
                # Insert setup before this line
                new_lines.insert(-1, '  setupSchemaTests();')
                inserted_setup = True
        
        # If we didn't insert setup, add it after variable declarations
        if not inserted_setup:
            for i, line in enumerate(new_lines):
                if '{' in line:
                    # Find the last variable declaration
                    j = i + 1
                    while j < len(new_lines) and (new_lines[j].strip() == '' or 
                          re.match(r'^\s*(int|char|const|double|void|GraphNode|GraphEdge)\s+.*[;=]', new_lines[j].strip())):
                        j += 1
                    new_lines.insert(j, '  setupSchemaTests();')
                    break
        
        func_content = '\n'.join(new_lines)
    
    # Check if teardown exists
    if 'teardownSchemaTests()' not in func_content:
        # Add before the last closing brace
        lines = func_content.split('\n')
        for i in range(len(lines)-1, -1, -1):
            if lines[i].strip() == '}':
                lines.insert(i, '  teardownSchemaTests();')
                break
        func_content = '\n'.join(lines)
    
    return func_content

# Pattern to match test functions
pattern = r'void test[A-Za-z0-9_]+\(void\)\s*\{[^}]+\}'

# Fix all test functions
content = re.sub(pattern, fix_test_function, content, flags=re.DOTALL)

# Write back
with open('/shared/sqlite-graph/tests/test_schema.c', 'w') as f:
    f.write(content)

print("Fixed test_schema.c")