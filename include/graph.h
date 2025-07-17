/*
** SQLite Graph Database Extension - Core Definitions
**
** This file contains the core data structures and function declarations
** for the SQLite graph database extension. All structures follow SQLite
** naming conventions with Hungarian notation.
**
** Memory allocation: All functions use sqlite3_malloc()/sqlite3_free()
** Error handling: Functions return SQLite error codes (SQLITE_OK, etc.)
** Thread safety: Extension supports SQLite threading modes
*/
#ifndef GRAPH_H
#define GRAPH_H

#include "sqlite3.h"

/*
** SQLite-style testcase macro for coverage tracking.
** Marks boundary conditions and error paths for test coverage.
*/
#ifndef testcase
# define testcase(X)  /* No-op in release builds */
#endif

/*
** Enhanced graph node structure with label support for Cypher compatibility.
** Each node has a unique identifier, multiple labels, and optional JSON properties.
** Nodes are stored in a linked list for O(1) insertion with label indexing.
*/
typedef struct GraphNode GraphNode;
struct GraphNode {
  sqlite3_int64 iNodeId;  /* Unique node identifier */
  char **azLabels;        /* Array of node labels (Person, Company, etc.) */
  int nLabels;            /* Number of labels */
  char *zProperties;      /* JSON properties string (sqlite3_malloc'd) */
  GraphNode *pNext;       /* Next node in linked list */
  GraphNode *pLabelNext;  /* Next node with same label (for indexing) */
};

/*
** Enhanced graph edge structure with relationship type support for Cypher.
** Edges connect two nodes with type, optional weight and properties.
** Stored as linked list with adjacency list optimization.
*/
typedef struct GraphEdge GraphEdge;
struct GraphEdge {
  sqlite3_int64 iEdgeId;  /* Unique edge identifier */
  sqlite3_int64 iFromId;  /* Source node ID */
  sqlite3_int64 iToId;    /* Target node ID */
  char *zType;            /* Relationship type (KNOWS, WORKS_AT, etc.) */
  double rWeight;         /* Edge weight (default 1.0) */
  char *zProperties;      /* JSON properties string (sqlite3_malloc'd) */
  GraphEdge *pNext;       /* Next edge in linked list */
  GraphEdge *pFromNext;   /* Next edge from same source node */
  GraphEdge *pToNext;     /* Next edge to same target node */
};

/*
** Forward declarations for schema structures
*/
typedef struct CypherSchema CypherSchema;
typedef struct GraphIndex GraphIndex;
typedef struct GraphPropertySchema GraphPropertySchema;

/*
** Enhanced graph virtual table structure with schema and indexing support.
** All graph operations are performed through this interface.
** Memory management: Reference counted with proper cleanup.
*/
typedef struct GraphVtab GraphVtab;
struct GraphVtab {
  sqlite3_vtab base;      /* Base class - must be first */
  sqlite3 *pDb;           /* Database connection */
  char *zDbName;          /* Database name ("main", "temp", etc.) */
  char *zTableName;       /* Name of the virtual table */
  int nRef;               /* Reference count */
  GraphIndex *pLabelIndex;  /* Label-based node index */
  GraphIndex *pPropertyIndex; /* Property-based index */
  CypherSchema *pSchema;  /* Schema information for labels/types */
};

/* A global pointer to the graph virtual table. Not ideal, but simple. */
extern GraphVtab *pGraph;

/*
** Graph cursor structure for virtual table iteration.
** Subclass of sqlite3_vtab_cursor following SQLite patterns.
*/
typedef struct GraphCursor GraphCursor;
struct GraphCursor {
  sqlite3_vtab_cursor base;  /* Base class - must be first */
  GraphVtab *pVtab;         /* Pointer to virtual table */
  int iRowid;               /* Current row ID */
  sqlite3_stmt *pNodeStmt;  /* Statement for node iteration */
  sqlite3_stmt *pEdgeStmt;  /* Statement for edge iteration */
  int iIterMode;            /* 0=nodes, 1=edges */
};

/*
** Visited node structure for BFS
*/
typedef struct VisitedNode VisitedNode;
struct VisitedNode {
  sqlite3_int64 iNodeId;
  VisitedNode *pNext;
};

/*
** Depth info for BFS
*/
typedef struct GraphDepthInfo GraphDepthInfo;
struct GraphDepthInfo {
  sqlite3_int64 iNodeId;    /* Node ID */
  int nDepth;               /* Depth from start node */
  GraphDepthInfo *pNext;    /* Next in linked list */
};

/*
** Core storage function declarations.
** All functions return SQLite error codes and follow SQLite patterns.
*/


/*
** Add a node to the graph. Return SQLITE_OK on success.
** Memory allocation: Uses sqlite3_malloc() for internal structures.
** Error conditions: Returns SQLITE_NOMEM on OOM, SQLITE_CONSTRAINT on 
**                   duplicate ID.
** I/O errors: Returns SQLITE_IOERR on database write failure.
*/
int graphAddNode(GraphVtab *pVtab, sqlite3_int64 iNodeId, 
                 const char *zProperties);

/*
** Remove a node from the graph along with all connected edges.
** Return SQLITE_OK on success, SQLITE_NOTFOUND if node doesn't exist.
** Memory management: Frees all associated memory with sqlite3_free().
*/
int graphRemoveNode(GraphVtab *pVtab, sqlite3_int64 iNodeId);

/*
** Retrieve node properties. Caller must sqlite3_free() the returned string.
** Returns SQLITE_OK and sets *pzProperties to allocated string.
** Returns SQLITE_NOTFOUND if node doesn't exist.
** Memory allocation: Allocates new string with sqlite3_malloc().
*/
int graphGetNode(GraphVtab *pVtab, sqlite3_int64 iNodeId, 
                 char **pzProperties);

/*
** Add edge between two nodes. Return SQLITE_OK on success.
** Both nodes must exist before adding edge.
** Error conditions: SQLITE_CONSTRAINT if nodes don't exist.
** Note: This is the legacy interface - use graphAddEdgeWithType for new code.
*/
int graphAddEdge(GraphVtab *pVtab, sqlite3_int64 iFromId, 
                 sqlite3_int64 iToId, double rWeight, 
                 const char *zProperties);

/*
** Remove specific edge between two nodes.
** Returns SQLITE_OK on success, SQLITE_NOTFOUND if edge doesn't exist.
*/
int graphRemoveEdge(GraphVtab *pVtab, sqlite3_int64 iFromId, 
                    sqlite3_int64 iToId);

/*
** Update existing node properties.
** Returns SQLITE_OK on success, SQLITE_NOTFOUND if node doesn't exist.
** Memory management: Frees old properties, allocates new ones.
*/
int graphUpdateNode(GraphVtab *pVtab, sqlite3_int64 iNodeId, 
                    const char *zProperties);

/*
** Retrieve edge properties and weight.
** Caller must sqlite3_free() the returned properties string.
** Returns SQLITE_OK and sets outputs, SQLITE_NOTFOUND if edge doesn't exist.
*/
int graphGetEdge(GraphVtab *pVtab, sqlite3_int64 iFromId, 
                 sqlite3_int64 iToId, double *prWeight, 
                 char **pzProperties);

/*
** Utility functions for graph properties.
** These provide O(1) access to cached counts.
*/
int graphCountNodes(GraphVtab *pVtab);
int graphCountEdges(GraphVtab *pVtab);

/*
** Find a node by ID. Returns pointer to node or NULL if not found.
** Internal function for efficient node lookup.
*/
GraphNode *graphFindNode(GraphVtab *pVtab, sqlite3_int64 iNodeId);

/*
** Find an edge by source and target IDs.
** Returns pointer to edge or NULL if not found.
*/
GraphEdge *graphFindEdge(GraphVtab *pVtab, sqlite3_int64 iFromId, 
                         sqlite3_int64 iToId);

/*
** Graph traversal algorithms.
** These implement standard graph search with SQLite error handling.
*/

/*
** Depth-first search with cycle detection and depth limiting.
** Returns SQLITE_OK and sets *pzPath to JSON array of node IDs.
** Caller must sqlite3_free() the returned path string.
** nMaxDepth: Maximum depth to search (-1 for unlimited)
*/
int graphDFS(GraphVtab *pVtab, sqlite3_int64 iStartId, int nMaxDepth,
             char **pzPath);

/*
** Breadth-first search with level-order traversal.
** Returns SQLITE_OK and sets *pzPath to JSON array of node IDs.
** Caller must sqlite3_free() the returned path string.
** nMaxDepth: Maximum depth to search (-1 for unlimited)
*/
int graphBFS(GraphVtab *pVtab, sqlite3_int64 iStartId, int nMaxDepth,
             char **pzPath);

/*
** Graph algorithms.
** These implement advanced graph analysis algorithms.
*/

/*
** Dijkstra's shortest path algorithm.
** Returns SQLITE_OK and sets *pzPath to JSON array of node IDs.
** If prDistance is not NULL, sets it to the total path distance.
** iEndId: Target node (-1 for distances to all nodes)
*/
int graphDijkstra(GraphVtab *pVtab, sqlite3_int64 iStartId, 
                  sqlite3_int64 iEndId, char **pzPath, double *prDistance);

/*
** Shortest path for unweighted graphs using BFS.
** More efficient than Dijkstra for unweighted graphs.
*/
int graphShortestPathUnweighted(GraphVtab *pVtab, sqlite3_int64 iStartId,
                                sqlite3_int64 iEndId, char **pzPath);

/*
** PageRank algorithm implementation.
** Returns SQLITE_OK and sets *pzResults to JSON object with scores.
** rDamping: Damping factor (typically 0.85)
** nMaxIter: Maximum iterations
** rEpsilon: Convergence threshold
*/
int graphPageRank(GraphVtab *pVtab, double rDamping, int nMaxIter, 
                  double rEpsilon, char **pzResults);

/*
** Degree calculations.
*/
int graphInDegree(GraphVtab *pVtab, sqlite3_int64 iNodeId);
int graphOutDegree(GraphVtab *pVtab, sqlite3_int64 iNodeId);
int graphTotalDegree(GraphVtab *pVtab, sqlite3_int64 iNodeId);
double graphDegreeCentrality(GraphVtab *pVtab, sqlite3_int64 iNodeId,
                            int bDirected);

/*
** Graph properties.
*/
int graphIsConnected(GraphVtab *pVtab);
double graphDensity(GraphVtab *pVtab, int bDirected);

/*
** Advanced graph algorithms.
*/

/*
** Betweenness centrality using Brandes' algorithm.
** Returns SQLITE_OK and sets *pzResults to JSON object with scores.
** Algorithm complexity: O(V*E) for unweighted graphs.
*/
int graphBetweennessCentrality(GraphVtab *pVtab, char **pzResults);

/*
** Closeness centrality calculation.
** Returns SQLITE_OK and sets *pzResults to JSON object with scores.
** Closeness = (n-1) / sum of shortest path distances.
*/
int graphClosenessCentrality(GraphVtab *pVtab, char **pzResults);

/*
** Topological sort using Kahn's algorithm.
** Returns SQLITE_OK and sets *pzOrder to JSON array of node IDs.
** Returns SQLITE_CONSTRAINT if graph contains cycles.
*/
int graphTopologicalSort(GraphVtab *pVtab, char **pzOrder);

/*
** Check if directed graph has cycles.
** Returns 1 if cycles exist, 0 otherwise.
*/
int graphHasCycle(GraphVtab *pVtab);

/*
** Find connected components (for undirected view).
** Returns SQLITE_OK and sets *pzComponents to JSON object.
** Format: {"component_id": [node_ids...], ...}
*/
int graphConnectedComponents(GraphVtab *pVtab, char **pzComponents);

/*
** Find strongly connected components using Tarjan's algorithm.
** Returns SQLITE_OK and sets *pzSCC to JSON array of components.
** Each component is an array of node IDs.
*/
int graphStronglyConnectedComponents(GraphVtab *pVtab, char **pzSCC);

/*
** Enhanced data model structures for Cypher schema support.
*/

/*
** Cypher schema tracking for labels and relationship types.
** Maintains metadata about graph structure for optimization.
*/
struct CypherSchema {
  char **azNodeLabels;    /* Array of known node labels */
  int nNodeLabels;        /* Number of node labels */
  char **azRelTypes;      /* Array of known relationship types */
  int nRelTypes;          /* Number of relationship types */
  GraphPropertySchema *pPropSchema; /* Property schemas by label/type */
};

/*
** Index structures for efficient pattern matching.
** Hash-based indexing for O(1) label and property lookups.
*/
struct GraphIndex {
  char *zIndexName;       /* Index name */
  int iIndexType;         /* 0=label, 1=property, 2=composite */
  GraphNode **apNodes;    /* Hash table of nodes */
  int nBuckets;           /* Number of hash buckets */
  int nEntries;           /* Number of indexed entries */
};

/*
** Property schema information for type inference and optimization.
** Tracks property types and frequency for each label/relationship type.
*/
struct GraphPropertySchema {
  char *zLabelOrType;     /* Label or relationship type name */
  char **azProperties;    /* Array of property names */
  int *aiPropertyTypes;   /* Array of SQLite type codes */
  int nProperties;        /* Number of properties */
  GraphPropertySchema *pNext; /* Next schema in linked list */
};

/*
** Enhanced storage functions with label and type support.
** These extend the existing API with Cypher graph concepts.
*/

/*
** Add a node with labels to the graph.
** azLabels: Array of label strings (can be NULL for no labels)
** nLabels: Number of labels in array
*/
int graphAddNodeWithLabels(GraphVtab *pVtab, sqlite3_int64 iNodeId,
                           const char **azLabels, int nLabels,
                           const char *zProperties);

/*
** Add an edge with relationship type to the graph.
** zType: Relationship type string (can be NULL for untyped edge)
*/
int graphAddEdgeWithType(GraphVtab *pVtab, sqlite3_int64 iFromId,
                         sqlite3_int64 iToId, const char *zType,
                         double rWeight, const char *zProperties);

/*
** Set or update labels for an existing node.
** Replaces existing labels with new ones.
*/
int graphSetNodeLabels(GraphVtab *pVtab, sqlite3_int64 iNodeId,
                       const char **azLabels, int nLabels);

/*
** Add a single label to an existing node.
** Does not replace existing labels.
*/
int graphAddNodeLabel(GraphVtab *pVtab, sqlite3_int64 iNodeId,
                      const char *zLabel);

/*
** Remove a specific label from a node.
*/
int graphRemoveNodeLabel(GraphVtab *pVtab, sqlite3_int64 iNodeId,
                         const char *zLabel);

/*
** Get all labels for a node as a JSON array.
** Caller must sqlite3_free() the returned string.
*/
int graphGetNodeLabels(GraphVtab *pVtab, sqlite3_int64 iNodeId,
                       char **pzLabels);

/*
** Check if a node has a specific label.
** Returns 1 if node has label, 0 if not, -1 on error.
*/
int graphNodeHasLabel(GraphVtab *pVtab, sqlite3_int64 iNodeId,
                      const char *zLabel);

/*
** Schema management functions.
*/

/*
** Create or get schema structure for virtual table.
** Initializes schema tracking if not already present.
*/
int graphInitSchema(GraphVtab *pVtab);

/*
** Destroy schema structure and free all memory.
*/
void graphDestroySchema(CypherSchema *pSchema);

/*
** Register a new label in the schema.
** Returns SQLITE_OK if successful.
*/
int graphRegisterLabel(CypherSchema *pSchema, const char *zLabel);

/*
** Register a new relationship type in the schema.
*/
int graphRegisterRelationshipType(CypherSchema *pSchema, const char *zType);

/*
** Index management functions.
*/

/*
** Create a label-based index for fast node lookups.
** zLabel: Label to index (NULL for all labels)
*/
int graphCreateLabelIndex(GraphVtab *pVtab, const char *zLabel);

/*
** Create a property-based index for fast property lookups.
** zLabel: Label to index properties for (NULL for all nodes)
** zProperty: Property name to index
*/
int graphCreatePropertyIndex(GraphVtab *pVtab, const char *zLabel,
                             const char *zProperty);

/*
** Find nodes by label using index.
** Returns linked list of nodes with specified label.
*/
GraphNode *graphFindNodesByLabel(GraphVtab *pVtab, const char *zLabel);

/*
** Find edges by relationship type.
** Returns linked list of edges with specified type.
*/
GraphEdge *graphFindEdgesByType(GraphVtab *pVtab, const char *zType);

/*
** Utility functions for label and type operations.
*/

/*
** Hash function for label and property indexing.
** Simple but effective hash for string keys.
*/
unsigned int graphHashString(const char *zString);

/*
** Compare two label arrays for equality.
** Returns 1 if equal, 0 if different.
*/
int graphLabelsEqual(const char **azLabels1, int nLabels1,
                     const char **azLabels2, int nLabels2);

/*
** Copy label array with proper memory allocation.
** Caller must free returned array and strings.
*/
char **graphCopyLabels(const char **azLabels, int nLabels);

/*
** Free label array allocated by graphCopyLabels.
*/
void graphFreeLabels(char **azLabels, int nLabels);

/*
** Dynamic schema discovery and validation functions.
*/

/*
** Discover all labels and relationship types in the current graph.
** Updates the schema with found labels and types automatically.
*/
int graphDiscoverSchema(GraphVtab *pVtab);

/*
** Get schema information as JSON.
** Returns schema metadata including labels, types, and statistics.
** Caller must sqlite3_free() the returned string.
*/
int graphGetSchemaInfo(GraphVtab *pVtab, char **pzSchemaInfo);

/*
** Validate node labels against schema constraints.
** Returns SQLITE_OK if valid, SQLITE_CONSTRAINT if invalid.
*/
int graphValidateNodeLabels(GraphVtab *pVtab, const char **azLabels, int nLabels);

/*
** Validate relationship type against schema constraints.
** Returns SQLITE_OK if valid, SQLITE_CONSTRAINT if invalid.
*/
int graphValidateRelationshipType(GraphVtab *pVtab, const char *zType);

/*
** Rebuild all indexes after schema changes.
** This is a potentially expensive operation for large graphs.
*/
int graphRebuildIndexes(GraphVtab *pVtab);

#endif /* GRAPH_H */