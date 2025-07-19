# TCK Coverage Matrix and Mapping Rules

## Overview

This document defines the coverage matrix and mapping rules for openCypher TCK compliance testing using Unity framework.

## Naming Convention

### Unity Test Functions
- **Pattern**: `test_<group>_<id>()`
- **Group**: Feature group name with dots replaced by underscores
- **ID**: Zero-padded 3-digit scenario identifier

### Examples
- `expressions.mathematical` scenario 1 → `test_expressions_mathematical_001()`
- `clauses.match` scenario 42 → `test_clauses_match_042()`
- `useCases.triadicSelection` scenario 7 → `test_useCases_triadicSelection_007()`

## Feature Group Mapping

### Clauses (799 scenarios)
- `clauses.call` → `test_clauses_call_XXX()`
- `clauses.create` → `test_clauses_create_XXX()`
- `clauses.delete` → `test_clauses_delete_XXX()`
- `clauses.match` → `test_clauses_match_XXX()`
- `clauses.match-where` → `test_clauses_match_where_XXX()`
- `clauses.merge` → `test_clauses_merge_XXX()`
- `clauses.remove` → `test_clauses_remove_XXX()`
- `clauses.return` → `test_clauses_return_XXX()`
- `clauses.return-orderby` → `test_clauses_return_orderby_XXX()`
- `clauses.return-skip-limit` → `test_clauses_return_skip_limit_XXX()`
- `clauses.set` → `test_clauses_set_XXX()`
- `clauses.union` → `test_clauses_union_XXX()`
- `clauses.unwind` → `test_clauses_unwind_XXX()`
- `clauses.with` → `test_clauses_with_XXX()`
- `clauses.with-orderBy` → `test_clauses_with_orderBy_XXX()`
- `clauses.with-skip-limit` → `test_clauses_with_skip_limit_XXX()`
- `clauses.with-where` → `test_clauses_with_where_XXX()`

### Expressions (786 scenarios)
- `expressions.aggregation` → `test_expressions_aggregation_XXX()`
- `expressions.boolean` → `test_expressions_boolean_XXX()`
- `expressions.comparison` → `test_expressions_comparison_XXX()`
- `expressions.conditional` → `test_expressions_conditional_XXX()`
- `expressions.existentialSubqueries` → `test_expressions_existentialSubqueries_XXX()`
- `expressions.graph` → `test_expressions_graph_XXX()`
- `expressions.list` → `test_expressions_list_XXX()`
- `expressions.literals` → `test_expressions_literals_XXX()`
- `expressions.map` → `test_expressions_map_XXX()`
- `expressions.mathematical` → `test_expressions_mathematical_XXX()`
- `expressions.null` → `test_expressions_null_XXX()`
- `expressions.path` → `test_expressions_path_XXX()`
- `expressions.pattern` → `test_expressions_pattern_XXX()`
- `expressions.precedence` → `test_expressions_precedence_XXX()`
- `expressions.quantifier` → `test_expressions_quantifier_XXX()`
- `expressions.string` → `test_expressions_string_XXX()`
- `expressions.temporal` → `test_expressions_temporal_XXX()`
- `expressions.typeConversion` → `test_expressions_typeConversion_XXX()`

### Use Cases (30 scenarios)
- `useCases.countingSubgraphMatches` → `test_useCases_countingSubgraphMatches_XXX()`
- `useCases.triadicSelection` → `test_useCases_triadicSelection_XXX()`

## Coverage.json Structure

The `coverage.json` file maintains the following structure:

```json
{
  "commit_info": {
    "repository": "opencypher/openCypher",
    "commit_hash": "7db2677dd3c6c87cc8d6b35fa7a32e5054ef6ebf",
    "total_scenarios": 1615
  },
  "feature_groups": {
    "<group_name>": {
      "display_name": "<original.dot.notation>",
      "total_scenarios": <count>,
      "runtime_scenarios": <count>,
      "parse_only_scenarios": <count>,
      "unity_test_functions": ["test_group_001", ...]
    }
  },
  "scenarios": [
    {
      "scenario_id": "<group>_<id>",
      "feature_group": "<original.dot.notation>",
      "unity_test_function": "test_<group>_<id>",
      "cypher_query": "<cypher_text>",
      "expected_result": "<expected_result>",
      "status": "pass|fail|skip|pending",
      "comment": "<optional_comment>",
      "priority": "high|medium|low"
    }
  ],
  "statistics": {
    "total_scenarios": 1615,
    "passed": <count>,
    "failed": <count>,
    "skipped": <count>,
    "pending": <count>,
    "compliance_percentage": <percentage>
  }
}
```

## Status Values

- **pass**: Test executes successfully and produces expected results
- **fail**: Test executes but produces incorrect results
- **skip**: Test is intentionally skipped (e.g., feature not supported)
- **pending**: Test not yet implemented

## Priority Levels

- **high**: Core functionality tests (basic MATCH, CREATE, etc.)
- **medium**: Standard feature tests (most expressions, clauses)
- **low**: Advanced or edge case tests (complex temporal, advanced patterns)

## Compliance Calculation

**Formula**: `(Passed Scenarios / Total Scenarios) * 100`

Only scenarios with `status: "pass"` count toward compliance.

## Usage

### Update Compliance Percentage
```bash
python3 scripts/update_compliance.py
```

### Query Coverage Information
```bash
# Get total compliance
jq '.statistics.compliance_percentage' coverage.json

# Get failing tests
jq '.scenarios[] | select(.status == "fail") | .unity_test_function' coverage.json

# Get pending tests by priority
jq '.scenarios[] | select(.status == "pending" and .priority == "high") | .unity_test_function' coverage.json
```

## CI Integration

The compliance percentage from `coverage.json` should be surfaced in CI through:

1. **Test Reports**: Include compliance percentage in test output
2. **Badge Generation**: Update README badges with current percentage
3. **Trend Tracking**: Log compliance over time for regression detection
4. **Pull Request Checks**: Prevent regressions in compliance percentage

## Implementation Notes

- Each Unity test function should correspond to exactly one TCK scenario
- Test functions should be grouped in files by feature group (e.g., `test_clauses_match.c`)
- Use descriptive assertion messages that reference the original TCK scenario
- Include the original Cypher query and expected result as comments in test functions
