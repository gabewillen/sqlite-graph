# SQLite Graph Extension Tests CMakeLists.txt

# Test executable
add_executable(test_graph
    test_main.c
    test_storage.c
    test_algorithms.c
    test_traversal.c
    test_virtual_table.c
    test_integration.c
    test_performance.c
    test_advanced.c
    test_schema.c
    test_cypher_parser.c
    test_cypher_planner.c
    test_cypher_executor.c
    test_performance_optimization.c
    test_performance_features.c
)

# Link with Unity test framework and graph library
target_link_libraries(test_graph
    unity
    graph_static
    sqlite3_lib
    m
    dl
    pthread
)

# Test with coverage
add_executable(test_graph_coverage
    test_main.c
    test_storage.c
    test_algorithms.c
    test_traversal.c
    test_virtual_table.c
    test_integration.c
    test_performance.c
    test_advanced.c
    test_schema.c
    test_cypher_parser.c
    test_cypher_planner.c
    test_cypher_executor.c
    test_performance_optimization.c
    test_performance_features.c
)

target_link_libraries(test_graph_coverage
    unity
    graph_static
    sqlite3_lib
    m
    dl
    pthread
)

target_compile_options(test_graph_coverage PRIVATE --coverage)
target_link_options(test_graph_coverage PRIVATE --coverage)

# Enable CTest
enable_testing()

# Main test suite
add_test(NAME graph_tests COMMAND test_graph)
add_test(NAME graph_coverage_tests COMMAND test_graph_coverage)

# COMPREHENSIVE INDIVIDUAL TESTS - NO LAZY TEST BUILDING
add_executable(test_extension_loading 
    test_extension_loading.c
    test_database_utils.c
)
target_link_libraries(test_extension_loading 
    unity 
    graph_static 
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
            sqlite3_lib
            m dl pthread
        )
        add_test(NAME ${test} COMMAND ${test})
    endif()
endforeach()

# Set test properties
set_tests_properties(graph_tests PROPERTIES 
    TIMEOUT 60
    LABELS "unit;integration"
)

# Add test labels for organization
set_tests_properties(test_cypher_basic test_planner_compile test_executor_compile 
    PROPERTIES LABELS "cypher")
set_tests_properties(test_write_simple test_merge_simple test_write_comprehensive 
    PROPERTIES LABELS "write_ops")
set_tests_properties(test_transaction_complete 
    PROPERTIES LABELS "transactions")
set_tests_properties(test_tck_basic 
    PROPERTIES LABELS "tck;compliance")