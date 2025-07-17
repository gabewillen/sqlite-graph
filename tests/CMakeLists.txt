# SQLite Graph Extension Tests CMakeLists.txt

# Enable CTest
enable_testing()

# Main test suite
# add_test(NAME graph_tests COMMAND test_graph)
# add_test(NAME graph_coverage_tests COMMAND test_graph_coverage)

# COMPREHENSIVE INDIVIDUAL TESTS - NO LAZY TEST BUILDING
add_executable(test_extension_loading 
    test_extension_loading.c
    test_database_utils.c
)
target_link_libraries(test_extension_loading 
    unity 
    graph_static 
    graph_test_util
    sqlite3_lib
    m dl pthread
)

add_executable(test_virtual_table_comprehensive
    test_virtual_table_comprehensive.c
    test_database_utils.c  
)
target_link_libraries(test_virtual_table_comprehensive
    unity
    graph_static 
    graph_test_util
    sqlite3_lib
    m dl pthread
)

# Add individual test executables for specific testing
# (These were previously handled by the Makefile)
set(INDIVIDUAL_TESTS
    test_cypher_basic
    test_planner_compile
    test_executor_compile
    test_write_simple
    test_merge_simple
    test_write_comprehensive
    test_transaction_complete
    test_tck_basic
)

foreach(test ${INDIVIDUAL_TESTS})
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${test}.c")
        add_executable(${test} ${test}.c)
        target_link_libraries(${test} 
            graph_static 
            graph_test_util
            sqlite3_lib
            m dl pthread
        )
        add_test(NAME ${test} COMMAND ${test})
    endif()
endforeach()

# Set test properties
# set_tests_properties(graph_tests PROPERTIES 
#     TIMEOUT 60
#     LABELS "unit;integration"
# )

# Add test labels for organization
set_tests_properties(test_cypher_basic test_planner_compile test_executor_compile 
    PROPERTIES LABELS "cypher")
set_tests_properties(test_write_simple test_merge_simple test_write_comprehensive 
    PROPERTIES LABELS "write_ops")
set_tests_properties(test_transaction_complete 
    PROPERTIES LABELS "transactions")
set_tests_properties(test_tck_basic 
    PROPERTIES LABELS "tck;compliance")

add_executable(test_cypher_lexer test_cypher_lexer.c)
target_link_libraries(test_cypher_lexer graph_static graph_test_util sqlite3_lib m dl pthread)
add_test(NAME test_cypher_lexer COMMAND test_cypher_lexer)

add_executable(test_cypher_parser test_cypher_parser.c)
target_link_libraries(test_cypher_parser graph_static unity graph_test_util sqlite3_lib m dl pthread)
add_test(NAME test_cypher_parser COMMAND test_cypher_parser)

add_executable(test_cypher_ast test_cypher_ast.c)
target_link_libraries(test_cypher_ast graph_static unity graph_test_util sqlite3_lib m dl pthread)
add_test(NAME test_cypher_ast COMMAND test_cypher_ast)
