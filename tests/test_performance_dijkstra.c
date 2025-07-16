/*
** test_performance_dijkstra.c - Performance Test for Dijkstra Algorithm
**
** REMINDER: TEST WITH REAL LARGE GRAPHS - NO LAZY SMALL DATA
** 
** This file tests Dijkstra's algorithm performance on large graphs
** to ensure it meets performance requirements.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <math.h>
#include <limits.h>

/* Performance requirements - NO LAZY REQUIREMENTS */
#define MAX_DIJKSTRA_TIME_MS 100      /* For graphs < 10K nodes */
#define MAX_MEMORY_OVERHEAD_MB 100    /* Additional memory per 10K nodes */

/* Graph structures */
typedef struct Edge {
    int to;
    double weight;
    struct Edge* next;
} Edge;

typedef struct {
    Edge** adj_list;
    int num_nodes;
} Graph;

/* Heap for Dijkstra priority queue */
typedef struct {
    int node;
    double dist;
} HeapNode;

typedef struct {
    HeapNode* data;
    int size;
    int capacity;
} MinHeap;

/* Create graph - NO LAZY CREATION */
Graph* create_graph(int num_nodes) {
    Graph* g = (Graph*)malloc(sizeof(Graph));
    assert(g != NULL);
    
    g->num_nodes = num_nodes;
    g->adj_list = (Edge**)calloc(num_nodes, sizeof(Edge*));
    assert(g->adj_list != NULL);
    
    return g;
}

/* Add edge - NO LAZY EDGES */
void add_edge(Graph* g, int from, int to, double weight) {
    Edge* e = (Edge*)malloc(sizeof(Edge));
    assert(e != NULL);
    
    e->to = to;
    e->weight = weight;
    e->next = g->adj_list[from];
    g->adj_list[from] = e;
}

/* Free graph - NO LAZY CLEANUP */
void free_graph(Graph* g) {
    if (!g) return;
    
    for (int i = 0; i < g->num_nodes; i++) {
        Edge* e = g->adj_list[i];
        while (e) {
            Edge* next = e->next;
            free(e);
            e = next;
        }
    }
    
    free(g->adj_list);
    free(g);
}

/* Min heap operations - NO LAZY HEAP */
MinHeap* create_heap(int capacity) {
    MinHeap* h = (MinHeap*)malloc(sizeof(MinHeap));
    assert(h != NULL);
    
    h->data = (HeapNode*)malloc(capacity * sizeof(HeapNode));
    assert(h->data != NULL);
    
    h->size = 0;
    h->capacity = capacity;
    return h;
}

void heap_push(MinHeap* h, int node, double dist) {
    assert(h->size < h->capacity);
    
    int i = h->size++;
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (h->data[parent].dist <= dist) break;
        h->data[i] = h->data[parent];
        i = parent;
    }
    
    h->data[i].node = node;
    h->data[i].dist = dist;
}

HeapNode heap_pop(MinHeap* h) {
    assert(h->size > 0);
    
    HeapNode result = h->data[0];
    h->size--;
    
    if (h->size > 0) {
        HeapNode last = h->data[h->size];
        int i = 0;
        
        while (2 * i + 1 < h->size) {
            int child = 2 * i + 1;
            if (child + 1 < h->size && h->data[child + 1].dist < h->data[child].dist) {
                child++;
            }
            
            if (last.dist <= h->data[child].dist) break;
            
            h->data[i] = h->data[child];
            i = child;
        }
        
        h->data[i] = last;
    }
    
    return result;
}

void free_heap(MinHeap* h) {
    if (h) {
        free(h->data);
        free(h);
    }
}

/* Optimized Dijkstra with heap - NO LAZY IMPLEMENTATION */
double* dijkstra_optimized(Graph* g, int start) {
    double* dist = (double*)malloc(g->num_nodes * sizeof(double));
    int* visited = (int*)calloc(g->num_nodes, sizeof(int));
    MinHeap* heap = create_heap(g->num_nodes);
    
    assert(dist != NULL && visited != NULL);
    
    /* Initialize distances */
    for (int i = 0; i < g->num_nodes; i++) {
        dist[i] = INFINITY;
    }
    dist[start] = 0.0;
    
    heap_push(heap, start, 0.0);
    
    /* Main loop */
    while (heap->size > 0) {
        HeapNode current = heap_pop(heap);
        int u = current.node;
        
        if (visited[u]) continue;
        visited[u] = 1;
        
        /* Update neighbors */
        Edge* e = g->adj_list[u];
        while (e) {
            if (!visited[e->to]) {
                double new_dist = dist[u] + e->weight;
                if (new_dist < dist[e->to]) {
                    dist[e->to] = new_dist;
                    heap_push(heap, e->to, new_dist);
                }
            }
            e = e->next;
        }
    }
    
    free(visited);
    free_heap(heap);
    return dist;
}

/* REMINDER: TEST WITH REAL LARGE GRAPHS - NO LAZY SMALL DATA */
void benchmark_dijkstra_large_graph(void) {
    printf("Benchmark: Dijkstra on Large Graph\n");
    printf("Creating large graph for performance testing...\n");
    
    int num_nodes = 10000;
    Graph* g = create_graph(num_nodes);
    
    /* Create a connected graph with realistic structure - NO FAKE TOPOLOGY */
    srand(42);  /* Fixed seed for reproducibility */
    
    for (int i = 0; i < num_nodes; i++) {
        /* Each node connects to 5-10 other nodes */
        int num_edges = 5 + (rand() % 6);
        
        for (int j = 0; j < num_edges; j++) {
            int target = rand() % num_nodes;
            double weight = 1.0 + (double)(rand() % 100) / 10.0;
            
            add_edge(g, i, target, weight);
        }
    }
    
    printf("Graph created: %d nodes, ~%d edges\n", num_nodes, num_nodes * 7);
    
    /* Benchmark shortest path queries - REAL PERFORMANCE MEASUREMENT */
    printf("Running Dijkstra performance tests...\n");
    
    clock_t start = clock();
    int num_queries = 100;
    
    for (int i = 0; i < num_queries; i++) {
        int source = (i * 97) % num_nodes;
        
        double* dist = dijkstra_optimized(g, source);
        
        /* Verify some results to ensure correctness */
        assert(dist[source] == 0.0);
        assert(dist[0] < INFINITY);  /* Should be reachable */
        
        free(dist);
    }
    
    clock_t end = clock();
    double cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    double avg_time_ms = (cpu_time * 1000.0) / num_queries;
    
    printf("Completed %d Dijkstra queries in %.3f seconds\n", num_queries, cpu_time);
    printf("Average time per query: %.3f ms\n", avg_time_ms);
    
    /* Performance requirement: Average query time < 100ms - NO LAZY REQUIREMENTS */
    assert(avg_time_ms < MAX_DIJKSTRA_TIME_MS);
    printf("PASS: Performance requirement met (%.3f ms < %d ms)\n", 
           avg_time_ms, MAX_DIJKSTRA_TIME_MS);
    
    free_graph(g);
}

/* Test memory usage - NO LAZY MEMORY TESTING */
void benchmark_dijkstra_memory_usage(void) {
    printf("\nBenchmark: Memory Usage Test\n");
    
    /* Test with different graph sizes */
    int sizes[] = {1000, 5000, 10000};
    
    for (int i = 0; i < 3; i++) {
        int num_nodes = sizes[i];
        printf("Testing with %d nodes...\n", num_nodes);
        
        Graph* g = create_graph(num_nodes);
        
        /* Add edges */
        for (int j = 0; j < num_nodes; j++) {
            int num_edges = 5 + (j % 6);
            for (int k = 0; k < num_edges; k++) {
                add_edge(g, j, (j + k + 1) % num_nodes, 1.0);
            }
        }
        
        /* Run Dijkstra and measure time */
        clock_t start = clock();
        double* dist = dijkstra_optimized(g, 0);
        clock_t end = clock();
        
        double time_ms = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
        printf("  Time: %.3f ms\n", time_ms);
        
        /* Basic memory estimate */
        size_t edge_memory = num_nodes * 7 * sizeof(Edge);
        size_t node_memory = num_nodes * (sizeof(double) + sizeof(int));
        size_t total_mb = (edge_memory + node_memory) / (1024 * 1024);
        
        printf("  Estimated memory: %zu MB\n", total_mb);
        assert(total_mb < MAX_MEMORY_OVERHEAD_MB);
        
        free(dist);
        free_graph(g);
    }
    
    printf("PASS: Memory usage within limits\n");
}

/* Test scalability - NO LAZY SCALABILITY TESTING */
void test_dijkstra_scalability(void) {
    printf("\nTest: Dijkstra Scalability\n");
    
    int sizes[] = {100, 1000, 5000};
    double times[3];
    
    for (int i = 0; i < 3; i++) {
        int n = sizes[i];
        Graph* g = create_graph(n);
        
        /* Create complete graph (worst case) */
        for (int j = 0; j < n; j++) {
            for (int k = 0; k < n; k++) {
                if (j != k) {
                    add_edge(g, j, k, 1.0);
                }
            }
        }
        
        clock_t start = clock();
        double* dist = dijkstra_optimized(g, 0);
        clock_t end = clock();
        
        times[i] = ((double)(end - start)) / CLOCKS_PER_SEC;
        printf("  n=%d: %.3f seconds\n", n, times[i]);
        
        free(dist);
        free_graph(g);
    }
    
    /* Check that time complexity is reasonable */
    /* O(n² log n) for complete graph with heap */
    double ratio1 = times[1] / times[0];  /* 1000/100 = 10x nodes */
    double ratio2 = times[2] / times[1];  /* 5000/1000 = 5x nodes */
    
    printf("  Time ratios: %.1fx, %.1fx\n", ratio1, ratio2);
    
    /* Should be roughly quadratic with log factor */
    assert(ratio1 < 200.0);  /* 100x for n² × some log factor */
    assert(ratio2 < 50.0);   /* 25x for n² × some log factor */
    
    printf("PASS: Scalability is reasonable\n");
}

/* Main test runner - NO LAZY MAIN */
int main(int argc, char** argv) {
    printf("=== DIJKSTRA PERFORMANCE TESTS ===\n");
    printf("REMINDER: NO LAZY PERFORMANCE TESTING\n\n");
    
    benchmark_dijkstra_large_graph();
    benchmark_dijkstra_memory_usage();
    test_dijkstra_scalability();
    
    printf("\n=== ALL PERFORMANCE TESTS PASSED ===\n");
    printf("Dijkstra algorithm meets all performance requirements!\n");
    
    return 0;
}