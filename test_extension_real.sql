-- Test loading and using the SQLite Graph Extension
-- REMINDER: NO LAZY TESTING - TEST REAL EXTENSION FUNCTIONALITY

.echo on

-- Load the extension
.load ./graph

-- Test 1: Create a virtual table
CREATE VIRTUAL TABLE test_graph USING graph();

-- Test 2: Insert nodes
INSERT INTO test_graph (from_id, to_id, weight) VALUES (1, 2, 1.0);
INSERT INTO test_graph (from_id, to_id, weight) VALUES (2, 3, 2.0);
INSERT INTO test_graph (from_id, to_id, weight) VALUES (1, 3, 5.0);

-- Test 3: Query the graph
SELECT * FROM test_graph;

-- Test 4: Count edges
SELECT COUNT(*) as edge_count FROM test_graph;

-- Test 5: Find specific edges
SELECT * FROM test_graph WHERE from_id = 1;

-- Test 6: Check node existence
SELECT DISTINCT from_id FROM test_graph
UNION
SELECT DISTINCT to_id FROM test_graph
ORDER BY from_id;