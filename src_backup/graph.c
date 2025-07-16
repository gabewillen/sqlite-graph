/*
** SQLite Graph Database Extension - Main Entry Point
**
** This file contains the main extension initialization function and
** SQL function registrations. Follows SQLite extension patterns exactly.
**
** Compilation: gcc -shared -fPIC -I. graph.c -o graph.so
** Loading: .load ./graph
** Usage: CREATE VIRTUAL TABLE mygraph USING graph();
*/

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1
#include "graph.h"
#include "graph-vtab.h"
#include "cypher.h"

/*
** Forward declarations for SQL functions.
** These will be implemented as the extension develops.
*/
static void graphNodeAddFunc(sqlite3_context*, int, sqlite3_value**);
static void graphEdgeAddFunc(sqlite3_context*, int, sqlite3_value**);
static void graphCountNodesFunc(sqlite3_context*, int, sqlite3_value**);
static void graphCountEdgesFunc(sqlite3_context*, int, sqlite3_value**);
static void graphShortestPathFunc(sqlite3_context*, int, sqlite3_value**);
static void graphPageRankFunc(sqlite3_context*, int, sqlite3_value**);
static void graphDegreeCentralityFunc(sqlite3_context*, int, sqlite3_value**);
static void graphIsConnectedFunc(sqlite3_context*, int, sqlite3_value**);
static void graphDensityFunc(sqlite3_context*, int, sqlite3_value**);
static void graphBetweennessCentralityFunc(sqlite3_context*, int, sqlite3_value**);
static void graphClosenessCentralityFunc(sqlite3_context*, int, sqlite3_value**);
static void graphTopologicalSortFunc(sqlite3_context*, int, sqlite3_value**);
static void graphHasCycleFunc(sqlite3_context*, int, sqlite3_value**);
static void graphConnectedComponentsFunc(sqlite3_context*, int, sqlite3_value**);
static void graphStronglyConnectedComponentsFunc(sqlite3_context*, int, sqlite3_value**);

/* Table-valued function registration from graph-tvf.c */
extern int graphRegisterTVF(sqlite3 *pDb);

/*
** Extension initialization function.
** Called when SQLite loads the extension via .load or sqlite3_load_extension.
** 
** Memory allocation: Uses sqlite3_malloc() for any required structures.
** Error handling: Returns SQLITE_ERROR and sets *pzErrMsg on failure.
** Module registration: Registers virtual table module and SQL functions.
*/
#ifdef _WIN32
__declspec(dllexport)
#endif
int sqlite3_graph_init(
  sqlite3 *pDb,
  char **pzErrMsg,
  const sqlite3_api_routines *pApi
){
  int rc = SQLITE_OK;
  SQLITE_EXTENSION_INIT2(pApi);
  
  /* Register the graph virtual table module */
  rc = sqlite3_create_module(pDb, "graph", &graphModule, 0);
  if( rc!=SQLITE_OK ){
    *pzErrMsg = sqlite3_mprintf("Failed to register graph module: %s", 
                                sqlite3_errmsg(pDb));
    return rc;
  }
  
  /* Register graph utility functions */
  rc = sqlite3_create_function(pDb, "graph_node_add", 2, SQLITE_UTF8, 0,
                              graphNodeAddFunc, 0, 0);
  if( rc!=SQLITE_OK ){
    *pzErrMsg = sqlite3_mprintf("Failed to register graph_node_add: %s",
                                sqlite3_errmsg(pDb));
    return rc;
  }
  
  rc = sqlite3_create_function(pDb, "graph_edge_add", 4, SQLITE_UTF8, 0,
                              graphEdgeAddFunc, 0, 0);
  if( rc!=SQLITE_OK ){
    *pzErrMsg = sqlite3_mprintf("Failed to register graph_edge_add: %s",
                                sqlite3_errmsg(pDb));
    return rc;
  }
  
  rc = sqlite3_create_function(pDb, "graph_count_nodes", 0, SQLITE_UTF8, 0,
                              graphCountNodesFunc, 0, 0);
  if( rc!=SQLITE_OK ){
    *pzErrMsg = sqlite3_mprintf("Failed to register graph_count_nodes: %s",
                                sqlite3_errmsg(pDb));
    return rc;
  }
  
  rc = sqlite3_create_function(pDb, "graph_count_edges", 0, SQLITE_UTF8, 0,
                              graphCountEdgesFunc, 0, 0);
  if( rc!=SQLITE_OK ){
    *pzErrMsg = sqlite3_mprintf("Failed to register graph_count_edges: %s",
                                sqlite3_errmsg(pDb));
    return rc;
  }
  
  /* Register table-valued functions for traversal */
  rc = graphRegisterTVF(pDb);
  if( rc!=SQLITE_OK ){
    *pzErrMsg = sqlite3_mprintf("Failed to register table-valued functions: %s",
                                sqlite3_errmsg(pDb));
    return rc;
  }
  
  /* Register algorithm functions */
  rc = sqlite3_create_function(pDb, "graph_shortest_path", 2, SQLITE_UTF8, 0,
                              graphShortestPathFunc, 0, 0);
  if( rc!=SQLITE_OK ){
    *pzErrMsg = sqlite3_mprintf("Failed to register graph_shortest_path: %s",
                                sqlite3_errmsg(pDb));
    return rc;
  }
  
  rc = sqlite3_create_function(pDb, "graph_pagerank", -1, SQLITE_UTF8, 0,
                              graphPageRankFunc, 0, 0);
  if( rc!=SQLITE_OK ){
    *pzErrMsg = sqlite3_mprintf("Failed to register graph_pagerank: %s",
                                sqlite3_errmsg(pDb));
    return rc;
  }
  
  rc = sqlite3_create_function(pDb, "graph_degree_centrality", 1, SQLITE_UTF8, 0,
                              graphDegreeCentralityFunc, 0, 0);
  if( rc!=SQLITE_OK ){
    *pzErrMsg = sqlite3_mprintf("Failed to register graph_degree_centrality: %s",
                                sqlite3_errmsg(pDb));
    return rc;
  }
  
  rc = sqlite3_create_function(pDb, "graph_is_connected", 0, SQLITE_UTF8, 0,
                              graphIsConnectedFunc, 0, 0);
  if( rc!=SQLITE_OK ){
    *pzErrMsg = sqlite3_mprintf("Failed to register graph_is_connected: %s",
                                sqlite3_errmsg(pDb));
    return rc;
  }
  
  rc = sqlite3_create_function(pDb, "graph_density", 0, SQLITE_UTF8, 0,
                              graphDensityFunc, 0, 0);
  if( rc!=SQLITE_OK ){
    *pzErrMsg = sqlite3_mprintf("Failed to register graph_density: %s",
                                sqlite3_errmsg(pDb));
    return rc;
  }
  
  /* Register advanced algorithm functions */
  rc = sqlite3_create_function(pDb, "graph_betweenness_centrality", 0, SQLITE_UTF8, 0,
                              graphBetweennessCentralityFunc, 0, 0);
  if( rc!=SQLITE_OK ){
    *pzErrMsg = sqlite3_mprintf("Failed to register graph_betweenness_centrality: %s",
                                sqlite3_errmsg(pDb));
    return rc;
  }
  
  rc = sqlite3_create_function(pDb, "graph_closeness_centrality", 0, SQLITE_UTF8, 0,
                              graphClosenessCentralityFunc, 0, 0);
  if( rc!=SQLITE_OK ){
    *pzErrMsg = sqlite3_mprintf("Failed to register graph_closeness_centrality: %s",
                                sqlite3_errmsg(pDb));
    return rc;
  }
  
  rc = sqlite3_create_function(pDb, "graph_topological_sort", 0, SQLITE_UTF8, 0,
                              graphTopologicalSortFunc, 0, 0);
  if( rc!=SQLITE_OK ){
    *pzErrMsg = sqlite3_mprintf("Failed to register graph_topological_sort: %s",
                                sqlite3_errmsg(pDb));
    return rc;
  }
  
  rc = sqlite3_create_function(pDb, "graph_has_cycle", 0, SQLITE_UTF8, 0,
                              graphHasCycleFunc, 0, 0);
  if( rc!=SQLITE_OK ){
    *pzErrMsg = sqlite3_mprintf("Failed to register graph_has_cycle: %s",
                                sqlite3_errmsg(pDb));
    return rc;
  }
  
  rc = sqlite3_create_function(pDb, "graph_connected_components", 0, SQLITE_UTF8, 0,
                              graphConnectedComponentsFunc, 0, 0);
  if( rc!=SQLITE_OK ){
    *pzErrMsg = sqlite3_mprintf("Failed to register graph_connected_components: %s",
                                sqlite3_errmsg(pDb));
    return rc;
  }
  
  rc = sqlite3_create_function(pDb, "graph_strongly_connected_components", 0, SQLITE_UTF8, 0,
                              graphStronglyConnectedComponentsFunc, 0, 0);
  if( rc!=SQLITE_OK ){
    *pzErrMsg = sqlite3_mprintf("Failed to register graph_strongly_connected_components: %s",
                                sqlite3_errmsg(pDb));
    return rc;
  }
  
  /* Register Cypher language support functions */
  rc = cypherRegisterSqlFunctions(pDb);
  if( rc!=SQLITE_OK ){
    *pzErrMsg = sqlite3_mprintf("Failed to register Cypher SQL functions: %s",
                                sqlite3_errmsg(pDb));
    return rc;
  }
  
  /* Register Cypher planner functions */
  rc = cypherRegisterPlannerSqlFunctions(pDb);
  if( rc!=SQLITE_OK ){
    *pzErrMsg = sqlite3_mprintf("Failed to register Cypher planner functions: %s",
                                sqlite3_errmsg(pDb));
    return rc;
  }
  
  /* Register Cypher executor functions */
  rc = cypherRegisterExecutorSqlFunctions(pDb);
  if( rc!=SQLITE_OK ){
    *pzErrMsg = sqlite3_mprintf("Failed to register Cypher executor functions: %s",
                                sqlite3_errmsg(pDb));
    return rc;
  }
  
  /* Register Cypher write operation functions */
  rc = cypherRegisterWriteSqlFunctions(pDb);
  if( rc!=SQLITE_OK ){
    *pzErrMsg = sqlite3_mprintf("Failed to register Cypher write functions: %s",
                                sqlite3_errmsg(pDb));
    return rc;
  }
  
  return SQLITE_OK;
}

/*
** SQL function: graph_node_add(node_id, properties)
** Adds a node to the default graph virtual table.
** Usage: SELECT graph_node_add(1, '{"name": "Alice"}');
*/
static void graphNodeAddFunc(sqlite3_context *pCtx, int argc, 
                            sqlite3_value **argv){
  sqlite3_int64 iNodeId;
  const char *zProperties;
  
  /* Validate argument count */
  if( argc!=2 ){
    sqlite3_result_error(pCtx, "graph_node_add() requires 2 arguments", -1);
    return;
  }
  
  /* Extract arguments */
  iNodeId = sqlite3_value_int64(argv[0]);
  zProperties = (const char*)sqlite3_value_text(argv[1]);
  
  if( zProperties==0 ){
    zProperties = "{}";  /* Default empty JSON object */
  }
  
  /* TODO: Find default graph vtab and call graphAddNode */
  /* For now, return success */
  sqlite3_result_int(pCtx, SQLITE_OK);
}

/*
** SQL function: graph_edge_add(from_id, to_id, weight, properties)
** Adds an edge to the default graph virtual table.
** Usage: SELECT graph_edge_add(1, 2, 1.0, '{"type": "friend"}');
*/
static void graphEdgeAddFunc(sqlite3_context *pCtx, int argc,
                            sqlite3_value **argv){
  sqlite3_int64 iFromId, iToId;
  double rWeight;
  const char *zProperties;
  
  /* Validate argument count */
  if( argc!=4 ){
    sqlite3_result_error(pCtx, "graph_edge_add() requires 4 arguments", -1);
    return;
  }
  
  /* Extract arguments */
  iFromId = sqlite3_value_int64(argv[0]);
  iToId = sqlite3_value_int64(argv[1]);
  rWeight = sqlite3_value_double(argv[2]);
  zProperties = (const char*)sqlite3_value_text(argv[3]);
  
  if( zProperties==0 ){
    zProperties = "{}";  /* Default empty JSON object */
  }
  
  /* TODO: Find default graph vtab and call graphAddEdge */
  /* For now, return success */
  sqlite3_result_int(pCtx, SQLITE_OK);
}

/*
** SQL function: graph_count_nodes()
** Returns the number of nodes in the default graph.
** Usage: SELECT graph_count_nodes();
*/
static void graphCountNodesFunc(sqlite3_context *pCtx, int argc,
                               sqlite3_value **argv){
  /* Validate argument count */
  if( argc!=0 ){
    sqlite3_result_error(pCtx, "graph_count_nodes() takes no arguments", -1);
    return;
  }
  
  /* TODO: Find default graph vtab and return node count */
  /* For now, return 0 */
  sqlite3_result_int(pCtx, 0);
}

/*
** SQL function: graph_count_edges()
** Returns the number of edges in the default graph.
** Usage: SELECT graph_count_edges();
*/
static void graphCountEdgesFunc(sqlite3_context *pCtx, int argc,
                               sqlite3_value **argv){
  /* Validate argument count */
  if( argc!=0 ){
    sqlite3_result_error(pCtx, "graph_count_edges() takes no arguments", -1);
    return;
  }
  
  /* TODO: Find default graph vtab and return edge count */
  /* For now, return 0 */
  sqlite3_result_int(pCtx, 0);
}

/*
** SQL function: graph_shortest_path(start_id, end_id)
** Returns the shortest path between two nodes as JSON array.
** Usage: SELECT graph_shortest_path(1, 5);
*/
static void graphShortestPathFunc(sqlite3_context *pCtx, int argc,
                                 sqlite3_value **argv){
  sqlite3_int64 iStartId, iEndId;
  char *zPath = 0;
  double rDistance;
  int rc;
  
  /* Validate argument count */
  if( argc!=2 ){
    sqlite3_result_error(pCtx, "graph_shortest_path() requires 2 arguments", -1);
    return;
  }
  
  /* Extract arguments */
  iStartId = sqlite3_value_int64(argv[0]);
  iEndId = sqlite3_value_int64(argv[1]);
  
  /* TODO: Find default graph vtab and call graphDijkstra */
  /* For now, return empty path */
  sqlite3_result_text(pCtx, "[]", -1, SQLITE_STATIC);
}

/*
** SQL function: graph_pagerank(damping, max_iter, epsilon)
** Calculates PageRank scores for all nodes.
** Usage: SELECT graph_pagerank(0.85, 100, 0.0001);
*/
static void graphPageRankFunc(sqlite3_context *pCtx, int argc,
                             sqlite3_value **argv){
  double rDamping = 0.85;
  int nMaxIter = 100;
  double rEpsilon = 0.0001;
  char *zResults = 0;
  int rc;
  
  /* Parse optional arguments */
  if( argc>=1 ){
    rDamping = sqlite3_value_double(argv[0]);
  }
  if( argc>=2 ){
    nMaxIter = sqlite3_value_int(argv[1]);
  }
  if( argc>=3 ){
    rEpsilon = sqlite3_value_double(argv[2]);
  }
  
  /* Validate parameters */
  if( rDamping<0.0 || rDamping>1.0 ){
    sqlite3_result_error(pCtx, "Damping factor must be between 0 and 1", -1);
    return;
  }
  if( nMaxIter<1 ){
    sqlite3_result_error(pCtx, "Max iterations must be positive", -1);
    return;
  }
  if( rEpsilon<=0.0 ){
    sqlite3_result_error(pCtx, "Epsilon must be positive", -1);
    return;
  }
  
  /* TODO: Find default graph vtab and call graphPageRank */
  /* For now, return empty results */
  sqlite3_result_text(pCtx, "{}", -1, SQLITE_STATIC);
}

/*
** SQL function: graph_degree_centrality(node_id)
** Returns the degree centrality for a specific node.
** Usage: SELECT graph_degree_centrality(1);
*/
static void graphDegreeCentralityFunc(sqlite3_context *pCtx, int argc,
                                     sqlite3_value **argv){
  sqlite3_int64 iNodeId;
  double rCentrality;
  
  /* Validate argument count */
  if( argc!=1 ){
    sqlite3_result_error(pCtx, "graph_degree_centrality() requires 1 argument", -1);
    return;
  }
  
  /* Extract argument */
  iNodeId = sqlite3_value_int64(argv[0]);
  
  /* TODO: Find default graph vtab and call graphDegreeCentrality */
  /* For now, return 0.0 */
  sqlite3_result_double(pCtx, 0.0);
}

/*
** SQL function: graph_is_connected()
** Returns 1 if graph is connected, 0 otherwise.
** Usage: SELECT graph_is_connected();
*/
static void graphIsConnectedFunc(sqlite3_context *pCtx, int argc,
                                sqlite3_value **argv){
  int bConnected;
  
  /* Validate argument count */
  if( argc!=0 ){
    sqlite3_result_error(pCtx, "graph_is_connected() takes no arguments", -1);
    return;
  }
  
  /* TODO: Find default graph vtab and call graphIsConnected */
  /* For now, return 0 */
  sqlite3_result_int(pCtx, 0);
}

/*
** SQL function: graph_density()
** Returns the density of the graph.
** Usage: SELECT graph_density();
*/
static void graphDensityFunc(sqlite3_context *pCtx, int argc,
                            sqlite3_value **argv){
  double rDensity;
  
  /* Validate argument count */
  if( argc!=0 ){
    sqlite3_result_error(pCtx, "graph_density() takes no arguments", -1);
    return;
  }
  
  /* TODO: Find default graph vtab and call graphDensity */
  /* For now, return 0.0 */
  sqlite3_result_double(pCtx, 0.0);
}

/*
** SQL function: graph_betweenness_centrality()
** Calculates betweenness centrality for all nodes.
** Usage: SELECT graph_betweenness_centrality();
*/
static void graphBetweennessCentralityFunc(sqlite3_context *pCtx, int argc,
                                          sqlite3_value **argv){
  char *zResults = 0;
  int rc;
  
  /* Validate argument count */
  if( argc!=0 ){
    sqlite3_result_error(pCtx, "graph_betweenness_centrality() takes no arguments", -1);
    return;
  }
  
  /* TODO: Find default graph vtab and call graphBetweennessCentrality */
  /* For now, return empty results */
  sqlite3_result_text(pCtx, "{}", -1, SQLITE_STATIC);
}

/*
** SQL function: graph_closeness_centrality()
** Calculates closeness centrality for all nodes.
** Usage: SELECT graph_closeness_centrality();
*/
static void graphClosenessCentralityFunc(sqlite3_context *pCtx, int argc,
                                        sqlite3_value **argv){
  char *zResults = 0;
  int rc;
  
  /* Validate argument count */
  if( argc!=0 ){
    sqlite3_result_error(pCtx, "graph_closeness_centrality() takes no arguments", -1);
    return;
  }
  
  /* TODO: Find default graph vtab and call graphClosenessCentrality */
  /* For now, return empty results */
  sqlite3_result_text(pCtx, "{}", -1, SQLITE_STATIC);
}

/*
** SQL function: graph_topological_sort()
** Returns topological ordering of nodes.
** Usage: SELECT graph_topological_sort();
*/
static void graphTopologicalSortFunc(sqlite3_context *pCtx, int argc,
                                    sqlite3_value **argv){
  char *zOrder = 0;
  int rc;
  
  /* Validate argument count */
  if( argc!=0 ){
    sqlite3_result_error(pCtx, "graph_topological_sort() takes no arguments", -1);
    return;
  }
  
  /* TODO: Find default graph vtab and call graphTopologicalSort */
  /* For now, return empty array */
  sqlite3_result_text(pCtx, "[]", -1, SQLITE_STATIC);
}

/*
** SQL function: graph_has_cycle()
** Returns 1 if graph has cycles, 0 otherwise.
** Usage: SELECT graph_has_cycle();
*/
static void graphHasCycleFunc(sqlite3_context *pCtx, int argc,
                             sqlite3_value **argv){
  int bHasCycle;
  
  /* Validate argument count */
  if( argc!=0 ){
    sqlite3_result_error(pCtx, "graph_has_cycle() takes no arguments", -1);
    return;
  }
  
  /* TODO: Find default graph vtab and call graphHasCycle */
  /* For now, return 0 */
  sqlite3_result_int(pCtx, 0);
}

/*
** SQL function: graph_connected_components()
** Returns connected components as JSON object.
** Usage: SELECT graph_connected_components();
*/
static void graphConnectedComponentsFunc(sqlite3_context *pCtx, int argc,
                                        sqlite3_value **argv){
  char *zComponents = 0;
  int rc;
  
  /* Validate argument count */
  if( argc!=0 ){
    sqlite3_result_error(pCtx, "graph_connected_components() takes no arguments", -1);
    return;
  }
  
  /* TODO: Find default graph vtab and call graphConnectedComponents */
  /* For now, return empty object */
  sqlite3_result_text(pCtx, "{}", -1, SQLITE_STATIC);
}

/*
** SQL function: graph_strongly_connected_components()
** Returns strongly connected components as JSON array.
** Usage: SELECT graph_strongly_connected_components();
*/
static void graphStronglyConnectedComponentsFunc(sqlite3_context *pCtx, int argc,
                                                sqlite3_value **argv){
  char *zSCC = 0;
  int rc;
  
  /* Validate argument count */
  if( argc!=0 ){
    sqlite3_result_error(pCtx, "graph_strongly_connected_components() takes no arguments", -1);
    return;
  }
  
  /* TODO: Find default graph vtab and call graphStronglyConnectedComponents */
  /* For now, return empty array */
  sqlite3_result_text(pCtx, "[]", -1, SQLITE_STATIC);
}