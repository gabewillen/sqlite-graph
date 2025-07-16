# SQLite Graph Extension - Phase 2 Requirements Plan
## Cypher Write Operations (Weeks 5-8)

### 🎯 **Phase 2 Executive Summary**

Phase 2 transforms the SQLite Graph Extension from a read-only system into a full CRUD-capable graph database. This phase implements all Cypher write operations (CREATE, MERGE, SET, DELETE) with complete transaction support and rollback capabilities.

**Critical Success Metrics:**
- ✅ All write operations functional and tested
- ✅ Zero TODO/FIXME markers in write operation code
- ✅ Complete transaction rollback support
- ✅ 95%+ test coverage for all write operations
- ✅ Performance targets: < 10ms for single node/edge operations

### 🚨 **MANDATORY @SELF_REVIEW.md COMPLIANCE**

**CRITICAL AI BEHAVIOR REQUIREMENT:** This plan is designed to prevent shortcuts and ensure production quality. The implementing AI agent MUST reference @SELF_REVIEW.md continuously and CANNOT declare any task complete until:

1. **ALL 8 verification commands pass** (Lines 369-400 in @SELF_REVIEW.md)
2. **ALL self-assessment questions answered "YES"** (Lines 498-522 in @SELF_REVIEW.md)
3. **ALL success criteria checkboxes verified** (Lines 530-556 in @SELF_REVIEW.md)
4. **Specific evidence provided** for each verification command execution

**Zero Tolerance Policy:** If any verification command fails, work MUST continue until it passes. No rationalizations, no "good enough" shortcuts, no incomplete implementations.

**Reference Points for Continuous Compliance:**
- @SELF_REVIEW.md Section 1: Implementation Completeness (Lines 28-122)
- @SELF_REVIEW.md Section 2: Error Handling Completeness (Lines 124-203)
- @SELF_REVIEW.md Section 3: Testing Thoroughness (Lines 205-286)
- @SELF_REVIEW.md Section 4: Thread Safety Verification (Lines 289-316)
- @SELF_REVIEW.md Section 5: Security Thoroughness (Lines 319-356)

---

## 📋 **Week 5: CREATE Operations Implementation**

### **5.1 Node Creation Foundation (Days 1-2)**

**Primary Target:** Replace TODO markers in cypher-write.c:303 with complete node creation implementation.

**@SELF_REVIEW.md Section 1.1 Compliance (Lines 32-59):**
- MUST eliminate ALL TODO/FIXME markers in node creation code
- CANNOT leave ANY placeholder return values (no "return 1", "return SQLITE_OK" without logic)
- MUST implement complete function behavior exactly as function name promises
- VERIFY with: `grep -r "TODO\|FIXME" src/cypher/cypher-write.c` returns ZERO results

**@SELF_REVIEW.md Section 2.1 Requirements (Lines 128-163):**
- MUST handle every malloc() failure with SQLITE_NOMEM
- MUST validate ALL input parameters against NULL, empty strings, oversized inputs
- MUST implement specific error codes (not generic SQLITE_ERROR)
- MUST trace every allocated resource to its corresponding free()

**Implementation Requirements:**
1. **Input Validation** - Apply @SELF_REVIEW.md Section 2.2 (Lines 165-177):
   - Validate pCtx and pOp parameters against NULL
   - Validate label names for length limits and valid characters
   - Validate JSON properties format and size constraints
   - Validate variable names against reserved words and length limits

2. **Node ID Generation:**
   - Generate cryptographically secure unique node IDs
   - Handle ID collision scenarios with proper error codes
   - Implement proper integer overflow protection

3. **Property Management:**
   - Parse JSON properties with complete error handling
   - Validate property types and size limits
   - Implement property serialization/deserialization

4. **Storage Integration:**
   - Atomic node creation with rollback support
   - Update all relevant indexes
   - Handle storage layer failures gracefully

5. **Variable Binding:**
   - Update execution context with new node variable
   - Handle variable name conflicts
   - Implement proper memory management for variable storage

**@SELF_REVIEW.md Section 3.1 Testing Requirements (Lines 209-243):**
- Test normal operation with valid inputs
- Test EVERY error condition (NULL inputs, malloc failures, storage failures)
- Test boundary conditions (empty labels, maximum sizes)
- Test security scenarios (malicious JSON, oversized inputs)
- VERIFY with coverage tools: 95% line coverage minimum

**@SELF_REVIEW.md Section 5.1 Security Requirements (Lines 323-355):**
- Sanitize ALL JSON input against injection attacks
- Protect ALL string operations against buffer overflow
- Validate property names against format string attacks
- Implement resource limits to prevent DoS

### **5.2 Relationship Creation (Days 3-4)**

**Primary Target:** Replace TODO in cypher-write.c:398 with complete relationship creation.

**@SELF_REVIEW.md Section 1.2 Function Verification (Lines 60-104):**
- THINK: Does cypherCreateRelationship() do exactly what its name promises?
- THINK: Are ALL parameters (pCtx, pOp, node IDs, type, properties) validated?
- THINK: Do ALL code paths return meaningful, correct values?
- CONTINUE UNTIL: Every edge case is handled gracefully

**Implementation Requirements:**
1. **Node Existence Validation:**
   - Verify from/to nodes exist in graph storage
   - Handle non-existent node scenarios with specific errors
   - Check node accessibility permissions

2. **Relationship Type Validation:**
   - Validate type string against length limits and valid characters
   - Check for reserved relationship type names
   - Apply @SELF_REVIEW.md Section 2.2 string validation (Lines 169-176)

3. **Weight and Properties:**
   - Validate weight values for numeric range and overflow
   - Parse and validate JSON properties completely
   - Handle property size and complexity limits

4. **Atomic Creation:**
   - Generate unique relationship ID with collision handling
   - Create relationship with complete rollback logging
   - Update bidirectional indexes atomically

**@SELF_REVIEW.md Section 2.3 Resource Management (Lines 179-201):**
- MUST trace every malloc() to its corresponding free()
- MUST handle errors before free() with proper cleanup
- VERIFY manually that no code paths skip resource cleanup

### **5.3 Variable Binding System (Days 5-6)**

**Primary Target:** Complete execution context variable management for created entities.

**@SELF_REVIEW.md Section 4.1 Thread Safety (Lines 293-307):**
- AUDIT: Variable storage for global state that could cause race conditions
- VERIFY: `grep -n "static.*=" src/cypher/cypher-execution-context.c | grep -v const` returns no unsafe state
- IMPLEMENT: Proper synchronization for shared variable data

**Implementation Requirements:**
1. **Variable Table Management:**
   - Dynamic expansion with proper overflow checking
   - Memory allocation failure handling
   - Variable name conflict resolution

2. **Variable Binding Operations:**
   - Deep copy semantics for complex values
   - Proper cleanup on binding failures
   - Support for all Cypher value types

3. **Variable Lookup Performance:**
   - Efficient search algorithms for large variable tables
   - Cache-friendly data structures
   - @SELF_REVIEW.md Section 3.3 performance validation (Lines 265-285)

### **5.4 Comprehensive Testing Framework (Days 7)**

**@SELF_REVIEW.md Section 3.1 Zero Tolerance Testing (Lines 209-243):**

**MANDATORY Test Categories:**
1. **Normal Operation Tests:**
   - Valid node creation with labels and properties
   - Valid relationship creation between existing nodes
   - Variable binding for created entities

2. **Error Condition Tests:**
   - NULL parameter handling for ALL functions
   - malloc() failure simulation for ALL allocations
   - Storage layer failure simulation
   - Invalid JSON property format testing

3. **Boundary Condition Tests:**
   - Empty label arrays and property objects
   - Maximum size inputs (labels, properties, variable names)
   - Minimum and maximum numeric values

4. **Security Tests:**
   - Malicious JSON injection attempts
   - Buffer overflow payloads in string inputs
   - Format string attack patterns
   - Resource exhaustion scenarios

**@SELF_REVIEW.md Section 3.2 Coverage Verification (Lines 246-263):**
- EXECUTE: `gcov src/cypher/*.c && gcovr --fail-under-line=95 --fail-under-branch=90`
- CONTINUE UNTIL: Line coverage ≥95%, branch coverage ≥90%, function coverage 100%
- DO NOT rationalize why lower coverage is "acceptable"

---

## 📋 **Week 6: MERGE Operations Implementation**

### **6.1 Node Matching Engine (Days 8-9)**

**Primary Target:** Implement efficient node matching for MERGE operations (cypher-write.c:434).

**@SELF_REVIEW.md Section 1.1 Placeholder Elimination (Lines 32-58):**
- REPLACE ALL TODO markers in cypherFindMatchingNode()
- ELIMINATE hardcoded test return values
- IMPLEMENT complete matching logic with performance optimization

**Implementation Requirements:**
1. **Label-Based Filtering:**
   - Use label indexes for initial candidate selection
   - Handle multiple label intersection efficiently
   - Apply @SELF_REVIEW.md Section 3.3 performance requirements (Lines 265-285)

2. **Property Matching:**
   - Deep property comparison for complex values
   - Handle partial property matches vs exact matches
   - Implement efficient property filtering algorithms

3. **Search Strategy Optimization:**
   - Index utilization planning
   - Early termination for unique matches
   - Memory-efficient candidate management

**@SELF_REVIEW.md Section 2.1 Error Handling (Lines 128-163):**
- Handle label index lookup failures
- Manage memory allocation failures in candidate arrays
- Implement specific error codes for match failure scenarios

### **6.2 MERGE Implementation with ON CREATE/ON MATCH (Days 10-11)**

**Primary Target:** Complete MERGE logic with conditional property application.

**@SELF_REVIEW.md Section 1.2 Function Completeness (Lines 60-104):**
- THINK: Does cypherMergeNode() handle both match and create scenarios correctly?
- THINK: Are ON CREATE and ON MATCH properties applied appropriately?
- THINK: Does the function handle all edge cases (existing vs new nodes)?

**Implementation Requirements:**
1. **Match Detection:**
   - Accurate node matching using cypherFindMatchingNode()
   - Handle ambiguous match scenarios
   - Validate match criteria completeness

2. **ON MATCH Processing:**
   - Apply property updates only when node exists
   - Merge existing and new properties correctly
   - Handle property update failures with rollback

3. **ON CREATE Processing:**
   - Combine match criteria with ON CREATE properties
   - Generate new node with merged property set
   - Handle creation failures with complete cleanup

4. **Result Binding:**
   - Bind result node to specified variable
   - Handle variable binding failures
   - Update execution context atomically

**@SELF_REVIEW.md Section 5.1 Security Requirements (Lines 323-355):**
- Sanitize all property inputs against injection
- Validate merged property sizes against DoS attacks
- Protect property merging logic against malicious inputs

---

## 📋 **Week 7: SET/DELETE Operations Implementation**

### **7.1 Property Update System (Days 12-13)**

**Primary Target:** Replace TODO in cypher-write.c:494 with complete property update system.

**@SELF_REVIEW.md Section 2.3 Resource Management (Lines 179-201):**
- MUST store old property values for rollback before modification
- MUST handle partial update failures with complete rollback
- VERIFY every property allocation has corresponding cleanup

**Implementation Requirements:**
1. **Property Value Storage:**
   - Backup old values before modification for rollback
   - Handle NULL/non-existent property cases
   - Implement atomic property updates

2. **Index Maintenance:**
   - Update property indexes consistently
   - Handle index update failures with rollback
   - Maintain index consistency during errors

3. **Value Type Conversion:**
   - Handle type changes in property updates
   - Validate new value types against constraints
   - Implement proper type coercion rules

**@SELF_REVIEW.md Section 4.2 Thread Safety (Lines 308-315):**
- THINK: What happens if two threads modify the same property simultaneously?
- IMPLEMENT: Proper locking for property update operations
- VERIFY: No race conditions in property index updates

### **7.2 DELETE Operations with DETACH Support (Days 14-15)**

**Primary Target:** Replace TODO in cypher-write.c:505 with comprehensive deletion logic.

**@SELF_REVIEW.md Section 1.2 Complete Implementation (Lines 60-104):**
- THINK: Does cypherDelete() handle both node and relationship deletion?
- THINK: Does DETACH DELETE properly remove connected relationships first?
- CONTINUE UNTIL: All deletion scenarios are handled correctly

**Implementation Requirements:**
1. **Node Deletion Logic:**
   - Verify node exists before deletion attempt
   - Check relationship constraints for non-DETACH deletion
   - Handle foreign key constraint violations

2. **DETACH DELETE Processing:**
   - Find ALL connected relationships efficiently
   - Delete relationships in proper order
   - Handle cascading deletion failures

3. **Relationship Deletion:**
   - Remove relationship from both node adjacency lists
   - Update relationship indexes atomically
   - Handle bidirectional relationship cleanup

4. **Rollback Data Storage:**
   - Store complete entity data for potential rollback
   - Handle storage failures during backup
   - Implement efficient backup data structures

**@SELF_REVIEW.md Section 3.1 Comprehensive Testing (Lines 209-243):**
- Test normal node deletion without relationships
- Test DETACH DELETE with multiple relationship types
- Test deletion failure scenarios and rollback
- Test orphaned relationship detection

---

## 📋 **Week 8: Transaction Management & Rollback**

### **8.1 Complete Rollback System (Days 16-17)**

**Primary Target:** Replace TODO in cypher-write.c:534 with comprehensive rollback system.

**@SELF_REVIEW.md Section 2.1 Every Error Scenario (Lines 128-163):**
- THINK: What happens if rollback operations themselves fail?
- THINK: How to handle partial rollback scenarios?
- CONTINUE UNTIL: Every rollback error case is handled

**Implementation Requirements:**
1. **Operation Log Management:**
   - Maintain complete operation history for rollback
   - Handle log storage failures gracefully
   - Implement efficient log traversal algorithms

2. **Reverse Operation Execution:**
   - Implement inverse operations for all write types
   - Handle dependent operation rollback ordering
   - Manage rollback operation failures

3. **State Restoration:**
   - Restore entity data from backup storage
   - Rebuild indexes during rollback
   - Verify data consistency after rollback

**@SELF_REVIEW.md Section 2.3 Resource Cleanup (Lines 179-201):**
- MUST free all rollback log memory after completion
- MUST handle cleanup failures without memory leaks
- VERIFY every backup allocation is properly freed

### **8.2 Nested Transaction Support (Days 18-19)**

**Primary Target:** Implement savepoint and nested transaction functionality.

**Implementation Requirements:**
1. **Savepoint Management:**
   - Create named savepoints with unique identification
   - Maintain savepoint stack for nested transactions
   - Handle savepoint creation failures

2. **Partial Rollback:**
   - Rollback to specific savepoint without affecting outer transactions
   - Maintain operation log segmentation
   - Handle complex nested rollback scenarios

3. **Variable State Management:**
   - Save and restore execution context variables
   - Handle variable binding state across savepoints
   - Implement efficient variable state storage

### **8.3 Error Recovery Mechanisms (Days 20)**

**Primary Target:** Complete error recovery and transaction failure handling.

**@SELF_REVIEW.md Section 1.1 Zero Shortcuts (Lines 50-58):**
- CANNOT leave any "TODO: implement error recovery later" comments
- MUST implement complete error handling for ALL scenarios
- VERIFY no hardcoded error responses remain

**Implementation Requirements:**
1. **Error Classification:**
   - Categorize errors by severity and recovery strategy
   - Implement appropriate recovery actions for each error type
   - Handle unrecoverable error scenarios gracefully

2. **Context State Management:**
   - Mark transaction context as failed for unrecoverable errors
   - Prevent further operations on failed contexts
   - Implement context cleanup for failed transactions

3. **Recovery Strategy Implementation:**
   - Retry logic for transient errors (SQLITE_BUSY)
   - Immediate rollback for critical errors (SQLITE_NOMEM, SQLITE_CORRUPT)
   - Partial rollback for constraint violations

---

## 🔍 **Continuous @SELF_REVIEW.md Validation Throughout Phase 2**

### **Daily Anti-Laziness Protocol**

**MANDATORY EXECUTION - NO EXCEPTIONS:**

Every single day during Phase 2, the implementing AI agent MUST execute ALL 8 verification commands from @SELF_REVIEW.md (Lines 369-400) and provide specific evidence:

```bash
# Command 1: @SELF_REVIEW.md Line 371
grep -r "TODO\|FIXME\|NOTE\|XXX\|HACK\|STUB" src/ include/
# REQUIRED RESULT: Zero output - continue until achieved

# Command 2: @SELF_REVIEW.md Line 375
make clean && make CFLAGS="-Wall -Wextra -Werror -O2"
# REQUIRED RESULT: Clean build - fix all warnings

# Command 3: @SELF_REVIEW.md Line 379
make test
# REQUIRED RESULT: All tests pass - fix failures

# Command 4: @SELF_REVIEW.md Line 383
valgrind --tool=memcheck --leak-check=full --error-exitcode=1 ./test_suite
# REQUIRED RESULT: Zero leaks - fix all memory issues

# Command 5: @SELF_REVIEW.md Line 387
cppcheck --error-exitcode=1 --enable=all src/
# REQUIRED RESULT: Zero issues - fix all analysis warnings

# Command 6: @SELF_REVIEW.md Line 391
gcov src/*.c && gcovr --fail-under-line=95 --fail-under-branch=90
# REQUIRED RESULT: Meet coverage targets - add tests

# Command 7: @SELF_REVIEW.md Line 395
./run_performance_tests.sh
# REQUIRED RESULT: Meet performance targets - optimize

# Command 8: @SELF_REVIEW.md Line 399
helgrind ./test_concurrent
# REQUIRED RESULT: No race conditions - fix thread safety
```

### **Anti-Rationalization Enforcement**

**@SELF_REVIEW.md Behavioral Rules (Lines 570-610) - MANDATORY COMPLIANCE:**

**DO NOT RATIONALIZE:**
- "This is good enough for now" → CONTINUE until perfect
- "I'll fix this later" → FIX immediately before proceeding  
- "This probably won't cause problems" → TEST and VERIFY
- "The user won't notice this shortcut" → ELIMINATE all shortcuts
- "This edge case is unlikely" → HANDLE all edge cases

**DO NOT SKIP:**
- Error handling because "it's tedious" → @SELF_REVIEW.md Section 2 requires ALL errors handled
- Testing because "it obviously works" → @SELF_REVIEW.md Section 3 requires comprehensive testing
- Input validation because "users will behave" → @SELF_REVIEW.md Section 5 requires security validation
- Performance testing because "it feels fast" → @SELF_REVIEW.md Section 3.3 requires benchmarking

### **Weekly Quality Gates with @SELF_REVIEW.md Evidence**

**End of Week 5 - CREATE Operations:**
- [ ] @SELF_REVIEW.md Section 1.1: Zero TODO markers verified with grep command
- [ ] @SELF_REVIEW.md Section 2.1: All malloc() failures handled with specific evidence
- [ ] @SELF_REVIEW.md Section 3.2: 95%+ test coverage achieved with gcovr report
- [ ] @SELF_REVIEW.md Section 5.1: Security validation passed with cppcheck clean

**End of Week 6 - MERGE Operations:**
- [ ] @SELF_REVIEW.md Section 1.2: All functions do exactly what names promise
- [ ] @SELF_REVIEW.md Section 3.1: Every error condition tested with specific test count
- [ ] @SELF_REVIEW.md Section 4.1: Thread safety verified with helgrind clean report
- [ ] @SELF_REVIEW.md Section 3.3: Performance targets met with benchmark evidence

**End of Week 7 - SET/DELETE Operations:**
- [ ] @SELF_REVIEW.md Section 2.3: Every allocation traced to free with manual verification
- [ ] @SELF_REVIEW.md Section 1.3: No hardcoded test values remain
- [ ] @SELF_REVIEW.md verification commands 1-8: ALL pass with documented evidence

**End of Week 8 - Transaction Management:**
- [ ] @SELF_REVIEW.md Section 1: Implementation completeness 100% verified
- [ ] @SELF_REVIEW.md Section 2: Error handling completeness 100% verified
- [ ] @SELF_REVIEW.md Section 3: Testing thoroughness 100% verified
- [ ] @SELF_REVIEW.md Section 4: Thread safety 100% verified
- [ ] @SELF_REVIEW.md Section 5: Security thoroughness 100% verified

### **Final Phase 2 Acceptance Criteria**

**@SELF_REVIEW.md Success Criteria (Lines 526-566) - ALL MUST BE VERIFIED:**

**Implementation Verification (@SELF_REVIEW.md Lines 530-536):**
- [ ] ✅ Zero TODO/FIXME/STUB markers anywhere in write operation code
- [ ] ✅ Every function completely implements its intended behavior
- [ ] ✅ No hardcoded test values or placeholders remain
- [ ] ✅ All error conditions handled with specific error codes
- [ ] ✅ All inputs validated against invalid/malicious data

**Testing Verification (@SELF_REVIEW.md Lines 537-542):**
- [ ] ✅ Every function has comprehensive test coverage
- [ ] ✅ Line coverage ≥95%, branch coverage ≥90%, function coverage 100%
- [ ] ✅ All performance benchmarks pass
- [ ] ✅ Memory leak testing shows zero leaks
- [ ] ✅ Thread safety testing shows no race conditions

**Quality Verification (@SELF_REVIEW.md Lines 544-549):**
- [ ] ✅ Code builds with -Wall -Wextra -Werror without warnings
- [ ] ✅ Static analysis (cppcheck) reports no issues
- [ ] ✅ All verification commands pass without errors
- [ ] ✅ Documentation exists for all public functions
- [ ] ✅ Code follows consistent style and patterns

**Security Verification (@SELF_REVIEW.md Lines 551-556):**
- [ ] ✅ All inputs sanitized against SQL injection
- [ ] ✅ All string operations protected against buffer overflow
- [ ] ✅ No format string vulnerabilities
- [ ] ✅ Resource limits implemented to prevent DoS
- [ ] ✅ Proper privilege restrictions in place

### **Final @SELF_REVIEW.md Self-Check (Lines 558-566)**

**Before declaring Phase 2 complete, the AI agent must answer "YES" to ALL:**
- Would I trust this code in a production system?
- Would I be proud to show this code to an expert reviewer?
- Have I truly finished everything, or am I just tired?
- Am I confident this code won't cause problems in production?

**IF ANY ANSWER IS "NO": CONTINUE WORKING UNTIL ALL ANSWERS ARE "YES"**

### **Critical Success Factors**

1. **@SELF_REVIEW.md Compliance:** Zero tolerance for shortcuts, complete adherence to all sections
2. **Verification-Driven Development:** Every feature must pass all 8 verification commands
3. **Evidence-Based Quality:** Specific evidence required for each quality claim
4. **Anti-Laziness Enforcement:** Continuous reference to @SELF_REVIEW.md prevents premature completion
5. **Production Readiness:** Code must meet production quality standards, not just "working" standards

**Phase 2 transforms the SQLite Graph Extension from a prototype into a production-ready write-capable graph database through unwavering adherence to @SELF_REVIEW.md quality standards.**