/*
* Copyright 2018-2024 Redis Labs Ltd. and Contributors
*
* This file is available under the Redis Source Available License 2.0,
* applying the same terms and conditions as the Redis Source Available License 2.0.
* You may not use this file except in compliance with the Redis Source Available License 2.0.
*
* A copy of the Redis Source Available License 2.0 is available at
* https://redis.io/rsal/Redis-Source-Available-License-2.0/
*
* The Redis Source Available License 2.0 is a copy-left license that requires any
* derivative work to be made available under the same terms and conditions.
* 
* See the file LICENSE for more details.
*/

#include "unity.h"
#include "cypher/cypher-ast.h"

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

void test_ast_creation_and_destruction() {
    CypherAst *pNode = cypherAstCreate(CYPHER_AST_QUERY, 1, 1);
    TEST_ASSERT_NOT_NULL(pNode);
    TEST_ASSERT_EQUAL(CYPHER_AST_QUERY, pNode->type);
    TEST_ASSERT_EQUAL(0, pNode->nChildren);
    TEST_ASSERT_NULL(pNode->zValue);
    cypherAstDestroy(pNode);
}

void test_ast_child_management() {
    CypherAst *pParent = cypherAstCreate(CYPHER_AST_QUERY, 1, 1);
    CypherAst *pChild1 = cypherAstCreate(CYPHER_AST_MATCH, 2, 1);
    CypherAst *pChild2 = cypherAstCreate(CYPHER_AST_RETURN, 3, 1);

    cypherAstAddChild(pParent, pChild1);
    cypherAstAddChild(pParent, pChild2);

    TEST_ASSERT_EQUAL(2, cypherAstGetChildCount(pParent));
    TEST_ASSERT_EQUAL_PTR(pChild1, cypherAstGetChild(pParent, 0));
    TEST_ASSERT_EQUAL_PTR(pChild2, cypherAstGetChild(pParent, 1));

    cypherAstDestroy(pParent);
}

void test_ast_value_management() {
    CypherAst *pNode = cypherAstCreateIdentifier("n", 1, 1);
    TEST_ASSERT_EQUAL_STRING("n", cypherAstGetValue(pNode));

    cypherAstSetValue(pNode, "m");
    TEST_ASSERT_EQUAL_STRING("m", cypherAstGetValue(pNode));

    cypherAstSetValue(pNode, NULL);
    TEST_ASSERT_NULL(cypherAstGetValue(pNode));

    cypherAstDestroy(pNode);
}

void test_ast_create_literal() {
    CypherAst *pNode = cypherAstCreateLiteral("123", 1, 1);
    TEST_ASSERT_NOT_NULL(pNode);
    TEST_ASSERT_EQUAL(CYPHER_AST_LITERAL, pNode->type);
    TEST_ASSERT_EQUAL_STRING("123", cypherAstGetValue(pNode));
    cypherAstDestroy(pNode);
}

void test_ast_create_binary_op() {
    CypherAst *pLeft = cypherAstCreateIdentifier("a", 1, 1);
    CypherAst *pRight = cypherAstCreateIdentifier("b", 1, 5);
    CypherAst *pNode = cypherAstCreateBinaryOp("+", pLeft, pRight, 1, 3);
    TEST_ASSERT_NOT_NULL(pNode);
    TEST_ASSERT_EQUAL(CYPHER_AST_BINARY_OP, pNode->type);
    TEST_ASSERT_EQUAL_STRING("+", cypherAstGetValue(pNode));
    TEST_ASSERT_EQUAL(2, cypherAstGetChildCount(pNode));
    TEST_ASSERT_EQUAL_PTR(pLeft, cypherAstGetChild(pNode, 0));
    TEST_ASSERT_EQUAL_PTR(pRight, cypherAstGetChild(pNode, 1));
    cypherAstDestroy(pNode);
}

void test_ast_create_property() {
    CypherAst *pObj = cypherAstCreateIdentifier("n", 1, 1);
    CypherAst *pNode = cypherAstCreateProperty(pObj, "name", 1, 3);
    TEST_ASSERT_NOT_NULL(pNode);
    TEST_ASSERT_EQUAL(CYPHER_AST_PROPERTY, pNode->type);
    TEST_ASSERT_EQUAL_STRING("name", cypherAstGetValue(pNode));
    TEST_ASSERT_EQUAL(1, cypherAstGetChildCount(pNode));
    TEST_ASSERT_EQUAL_PTR(pObj, cypherAstGetChild(pNode, 0));
    cypherAstDestroy(pNode);
}

void test_ast_get_child_invalid() {
    CypherAst *pNode = cypherAstCreate(CYPHER_AST_QUERY, 1, 1);
    TEST_ASSERT_NULL(cypherAstGetChild(pNode, 0));
    TEST_ASSERT_NULL(cypherAstGetChild(pNode, -1));
    TEST_ASSERT_NULL(cypherAstGetChild(NULL, 0));
    cypherAstDestroy(pNode);
}

void test_ast_get_child_count_null() {
    TEST_ASSERT_EQUAL(0, cypherAstGetChildCount(NULL));
}

void test_ast_get_value_null() {
    TEST_ASSERT_NULL(cypherAstGetValue(NULL));
}

void test_ast_node_type_name() {
    TEST_ASSERT_EQUAL_STRING("QUERY", cypherAstNodeTypeName(CYPHER_AST_QUERY));
    TEST_ASSERT_EQUAL_STRING("UNKNOWN", cypherAstNodeTypeName((CypherAstNodeType)999));
}

void test_ast_node_type_name_all() {
    TEST_ASSERT_EQUAL_STRING("QUERY", cypherAstNodeTypeName(CYPHER_AST_QUERY));
    TEST_ASSERT_EQUAL_STRING("SINGLE_QUERY", cypherAstNodeTypeName(CYPHER_AST_SINGLE_QUERY));
    TEST_ASSERT_EQUAL_STRING("UNION", cypherAstNodeTypeName(CYPHER_AST_UNION));
    TEST_ASSERT_EQUAL_STRING("MATCH", cypherAstNodeTypeName(CYPHER_AST_MATCH));
    TEST_ASSERT_EQUAL_STRING("OPTIONAL_MATCH", cypherAstNodeTypeName(CYPHER_AST_OPTIONAL_MATCH));
    TEST_ASSERT_EQUAL_STRING("WHERE", cypherAstNodeTypeName(CYPHER_AST_WHERE));
    TEST_ASSERT_EQUAL_STRING("RETURN", cypherAstNodeTypeName(CYPHER_AST_RETURN));
    TEST_ASSERT_EQUAL_STRING("PROJECTION_LIST", cypherAstNodeTypeName(CYPHER_AST_PROJECTION_LIST));
    TEST_ASSERT_EQUAL_STRING("PROJECTION_ITEM", cypherAstNodeTypeName(CYPHER_AST_PROJECTION_ITEM));
    TEST_ASSERT_EQUAL_STRING("ORDER_BY", cypherAstNodeTypeName(CYPHER_AST_ORDER_BY));
    TEST_ASSERT_EQUAL_STRING("SORT_LIST", cypherAstNodeTypeName(CYPHER_AST_SORT_LIST));
    TEST_ASSERT_EQUAL_STRING("SORT_ITEM", cypherAstNodeTypeName(CYPHER_AST_SORT_ITEM));
    TEST_ASSERT_EQUAL_STRING("LIMIT", cypherAstNodeTypeName(CYPHER_AST_LIMIT));
    TEST_ASSERT_EQUAL_STRING("SKIP", cypherAstNodeTypeName(CYPHER_AST_SKIP));
    TEST_ASSERT_EQUAL_STRING("PATTERN", cypherAstNodeTypeName(CYPHER_AST_PATTERN));
    TEST_ASSERT_EQUAL_STRING("NODE_PATTERN", cypherAstNodeTypeName(CYPHER_AST_NODE_PATTERN));
    TEST_ASSERT_EQUAL_STRING("REL_PATTERN", cypherAstNodeTypeName(CYPHER_AST_REL_PATTERN));
    TEST_ASSERT_EQUAL_STRING("LABELS", cypherAstNodeTypeName(CYPHER_AST_LABELS));
    TEST_ASSERT_EQUAL_STRING("PATH", cypherAstNodeTypeName(CYPHER_AST_PATH));
    TEST_ASSERT_EQUAL_STRING("IDENTIFIER", cypherAstNodeTypeName(CYPHER_AST_IDENTIFIER));
    TEST_ASSERT_EQUAL_STRING("LITERAL", cypherAstNodeTypeName(CYPHER_AST_LITERAL));
    TEST_ASSERT_EQUAL_STRING("UNARY_OP", cypherAstNodeTypeName(CYPHER_AST_UNARY_OP));
    TEST_ASSERT_EQUAL_STRING("BINARY_OP", cypherAstNodeTypeName(CYPHER_AST_BINARY_OP));
    TEST_ASSERT_EQUAL_STRING("PROPERTY", cypherAstNodeTypeName(CYPHER_AST_PROPERTY));
    TEST_ASSERT_EQUAL_STRING("MAP", cypherAstNodeTypeName(CYPHER_AST_MAP));
    TEST_ASSERT_EQUAL_STRING("LIST", cypherAstNodeTypeName(CYPHER_AST_LIST));
    TEST_ASSERT_EQUAL_STRING("FUNCTION_CALL", cypherAstNodeTypeName(CYPHER_AST_FUNCTION_CALL));
    TEST_ASSERT_EQUAL_STRING("CASE", cypherAstNodeTypeName(CYPHER_AST_CASE));
    TEST_ASSERT_EQUAL_STRING("COUNT", cypherAstNodeTypeName(CYPHER_AST_COUNT));
}

void test_ast_print() {
    // Redirigir stdout a un buffer para verificar la salida
    // Esto es más complejo y puede requerir una utilidad de prueba específica.
    // Por ahora, solo llamamos a la función para asegurarnos de que no falle.
    CypherAst *pNode = cypherAstCreate(CYPHER_AST_QUERY, 1, 1);
    cypherAstPrint(pNode, 0);
    cypherAstDestroy(pNode);
}


int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ast_creation_and_destruction);
    RUN_TEST(test_ast_child_management);
    RUN_TEST(test_ast_value_management);
    RUN_TEST(test_ast_create_literal);
    RUN_TEST(test_ast_create_binary_op);
    RUN_TEST(test_ast_create_property);
    RUN_TEST(test_ast_get_child_invalid);
    RUN_TEST(test_ast_get_child_count_null);
    RUN_TEST(test_ast_get_value_null);
    RUN_TEST(test_ast_node_type_name);
    RUN_TEST(test_ast_node_type_name_all);
    RUN_TEST(test_ast_print);
    return UNITY_END();
}
