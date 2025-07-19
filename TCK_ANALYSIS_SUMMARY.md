# OpenCypher TCK Scenarios Analysis Summary

## Repository Information
- **Repository**: `opencypher/openCypher`
- **Commit Hash**: `7db2677dd3c6c87cc8d6b35fa7a32e5054ef6ebf`
- **Analysis Date**: 2025-07-19 15:05:39 UTC
- **Total Feature Files**: 220
- **Total Scenarios**: 1615

## Language Area Categorization

The TCK scenarios are organized into the following language areas based on their directory structure:

### Clauses (799 scenarios)
- **clauses.call**: 41 scenarios (11 runtime, 30 parse)
- **clauses.create**: 78 scenarios (34 runtime, 44 parse)  
- **clauses.delete**: 41 scenarios (19 runtime, 22 parse)
- **clauses.match**: 161 scenarios (72 runtime, 89 parse)
- **clauses.match-where**: 34 scenarios (19 runtime, 15 parse)
- **clauses.merge**: 75 scenarios (18 runtime, 57 parse)
- **clauses.remove**: 33 scenarios (33 runtime, 0 parse)
- **clauses.return**: 63 scenarios (9 runtime, 54 parse)
- **clauses.return-orderby**: 35 scenarios (16 runtime, 19 parse)
- **clauses.return-skip-limit**: 31 scenarios (3 runtime, 28 parse)
- **clauses.set**: 53 scenarios (42 runtime, 11 parse)
- **clauses.union**: 12 scenarios (0 runtime, 12 parse)
- **clauses.unwind**: 14 scenarios (14 runtime, 0 parse)
- **clauses.with**: 29 scenarios (13 runtime, 16 parse)
- **clauses.with-orderBy**: 99 scenarios (0 runtime, 99 parse)
- **clauses.with-skip-limit**: 9 scenarios (9 runtime, 0 parse)
- **clauses.with-where**: 19 scenarios (19 runtime, 0 parse)

### Expressions (786 scenarios)
- **expressions.aggregation**: 27 scenarios (27 runtime, 0 parse)
- **expressions.boolean**: 36 scenarios (8 runtime, 28 parse)
- **expressions.comparison**: 33 scenarios (16 runtime, 17 parse)
- **expressions.conditional**: 2 scenarios (2 runtime, 0 parse)
- **expressions.existentialSubqueries**: 10 scenarios (7 runtime, 3 parse)
- **expressions.graph**: 48 scenarios (16 runtime, 32 parse)
- **expressions.list**: 94 scenarios (35 runtime, 59 parse)
- **expressions.literals**: 131 scenarios (6 runtime, 125 parse)
- **expressions.map**: 19 scenarios (19 runtime, 0 parse)
- **expressions.mathematical**: 6 scenarios (5 runtime, 1 parse)
- **expressions.null**: 16 scenarios (16 runtime, 0 parse)
- **expressions.path**: 7 scenarios (4 runtime, 3 parse)
- **expressions.pattern**: 36 scenarios (11 runtime, 25 parse)
- **expressions.precedence**: 43 scenarios (43 runtime, 0 parse)
- **expressions.quantifier**: 100 scenarios (39 runtime, 61 parse)
- **expressions.string**: 32 scenarios (32 runtime, 0 parse)
- **expressions.temporal**: 89 scenarios (89 runtime, 0 parse)
- **expressions.typeConversion**: 29 scenarios (29 runtime, 0 parse)

### Use Cases (30 scenarios)
- **useCases.countingSubgraphMatches**: 11 scenarios (11 runtime, 0 parse)
- **useCases.triadicSelection**: 19 scenarios (19 runtime, 0 parse)

## Runtime vs Parse/Validate Classification

**Total Runtime Execution Required**: 794 scenarios (49.2%)
**Total Parse/Validate Only**: 821 scenarios (50.8%)

The scenarios have been classified into two categories:
- **Runtime Execution**: Scenarios that require a full Cypher runtime to execute queries and validate results
- **Parse/Validate Only**: Scenarios that can be validated through parsing and static analysis alone (primarily syntax error cases)

## Output Files

1. **tck_scenarios_manifest.json**: Complete structured data with all scenario information and statistics
2. **tck_scenarios_manifest.csv**: Tabular format suitable for spreadsheet analysis and further processing

## Reproducibility

This analysis was performed on commit `7db2677dd3c6c87cc8d6b35fa7a32e5054ef6ebf` from the `opencypher/openCypher` repository to ensure reproducibility of results.
