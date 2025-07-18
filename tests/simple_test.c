#include <stdio.h>
#include <sqlite3.h>

int main() {
    sqlite3 *db;
    int rc = sqlite3_open(":memory:", &db);
    if (rc != SQLITE_OK) {
        printf("FAIL: Failed to open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }
    
    rc = sqlite3_enable_load_extension(db, 1);
    if (rc != SQLITE_OK) {
        printf("FAIL: Failed to enable load extension: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }
    
    char *err_msg = 0;
    rc = sqlite3_exec(db, "SELECT load_extension('./build/libgraph')", NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        printf("FAIL: Failed to load extension: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }
    
    // Test virtual table creation
    rc = sqlite3_exec(db, "CREATE VIRTUAL TABLE test_graph USING graph()", NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        printf("FAIL: Failed to create virtual table: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }
    
    // Test node addition
    rc = sqlite3_exec(db, "SELECT graph_node_add(1, '{\"name\": \"test\"}')", NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        printf("FAIL: Failed to add node: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }
    
    // Test edge addition
    rc = sqlite3_exec(db, "SELECT graph_edge_add(1, 2, 1.0, '{\"type\": \"test\"}')", NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        printf("FAIL: Failed to add edge: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }
    
    // Test node counting
    rc = sqlite3_exec(db, "SELECT graph_count_nodes()", NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        printf("FAIL: Failed to count nodes: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }
    
    // Test edge counting
    rc = sqlite3_exec(db, "SELECT graph_count_edges()", NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        printf("FAIL: Failed to count edges: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }
    
    // Test algorithm functions
    rc = sqlite3_exec(db, "SELECT graph_degree_centrality(1)", NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        printf("FAIL: Failed to compute degree centrality: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }
    
    rc = sqlite3_exec(db, "SELECT graph_is_connected()", NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        printf("FAIL: Failed to check connectivity: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }
    
    rc = sqlite3_exec(db, "SELECT graph_density()", NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        printf("FAIL: Failed to compute density: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }
    
    rc = sqlite3_exec(db, "SELECT graph_has_cycle()", NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        printf("FAIL: Failed to check for cycles: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }
    
    rc = sqlite3_exec(db, "SELECT graph_betweenness_centrality()", NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        printf("FAIL: Failed to compute betweenness centrality: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }
    
    rc = sqlite3_exec(db, "SELECT graph_closeness_centrality()", NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        printf("FAIL: Failed to compute closeness centrality: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }
    
    rc = sqlite3_exec(db, "SELECT graph_topological_sort()", NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        printf("FAIL: Failed to compute topological sort: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }
    
    rc = sqlite3_exec(db, "SELECT graph_connected_components()", NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        printf("FAIL: Failed to compute connected components: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }
    
    rc = sqlite3_exec(db, "SELECT graph_strongly_connected_components()", NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        printf("FAIL: Failed to compute strongly connected components: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }
    
    printf("PASS: All basic functionality tests passed\n");
    
    // Ensure proper cleanup
    sqlite3_close_v2(db);
    return 0;
}
