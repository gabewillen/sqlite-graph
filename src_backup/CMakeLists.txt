# SQLite Graph Extension Source CMakeLists.txt

# Graph extension shared library
add_library(graph SHARED
    graph.c
    graph-vtab.c
    graph-algo.c
    graph-traverse.c
    graph-util.c
    graph-json.c
    graph-tvf.c
    graph-advanced.c
    graph-schema.c
    graph-enhanced.c
    cypher/cypher-lexer.c
    cypher/cypher-ast.c
    cypher/cypher-parser.c
    cypher/cypher-parser-generated.c
    cypher/cypher-sql.c
    cypher/cypher-logical-plan.c
    cypher/cypher-physical-plan.c
    cypher/cypher-planner.c
    cypher/cypher-planner-sql.c
    cypher/cypher-execution-context.c
    cypher/cypher-iterators.c
    cypher/cypher-executor.c
    cypher/cypher-executor-sql.c
    cypher/cypher-expressions.c
    cypher/cypher-write.c
    cypher/cypher-write-sql.c
    graph-performance.c
    graph-benchmark.c
    graph-parallel.c
    graph-cache.c
    graph-compress.c
    graph-bulk.c
)

# Include SQLite headers
target_include_directories(graph PRIVATE ${sqlite3_SOURCE_DIR})

# Graph extension static library for embedding
add_library(graph_static STATIC
    graph.c
    graph-vtab.c
    graph-algo.c
    graph-traverse.c
    graph-util.c
    graph-json.c
    graph-tvf.c
    graph-advanced.c
    graph-schema.c
    graph-enhanced.c
    cypher/cypher-lexer.c
    cypher/cypher-ast.c
    cypher/cypher-parser.c
    cypher/cypher-parser-generated.c
    cypher/cypher-sql.c
    cypher/cypher-logical-plan.c
    cypher/cypher-physical-plan.c
    cypher/cypher-planner.c
    cypher/cypher-planner-sql.c
    cypher/cypher-execution-context.c
    cypher/cypher-iterators.c
    cypher/cypher-executor.c
    cypher/cypher-executor-sql.c
    cypher/cypher-expressions.c
    cypher/cypher-write.c
    cypher/cypher-write-sql.c
    graph-performance.c
    graph-benchmark.c
    graph-parallel.c
    graph-cache.c
    graph-compress.c
    graph-bulk.c
)

# Include SQLite headers
target_include_directories(graph_static PRIVATE ${sqlite3_SOURCE_DIR})

# Set library properties
set_target_properties(graph PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    OUTPUT_NAME graph
)

set_target_properties(graph_static PROPERTIES
    OUTPUT_NAME graph
)

# Install targets
install(TARGETS graph graph_static
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(FILES ../include/graph.h ../include/graph-vtab.h
              ../include/cypher.h ../include/cypher-planner.h
              ../include/cypher-executor.h ../include/cypher-write.h
              ../include/cypher-expressions.h ../include/cypher-optimizer.h
              ../include/cypher-errors.h ../include/cypher-paths.h
              ../include/graph-performance.h
    DESTINATION include
)