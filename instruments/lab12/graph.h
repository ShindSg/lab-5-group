#ifndef GRAPH_H
#define GRAPH_H

#include "../lab3/vector/generic.h"
#include "../lab4/hash_table/generic.h"


typedef struct
{
    long long node_id;
    double latitude;
    double longitude;
    Vector *edges;
} Node;

typedef struct
{
    size_t idx_to;
    double length;
    char name[256];
} Edge;

typedef struct
{
    Vector *nodes;
    HashTable *id_to_idx;
} Graph;

//----------------------------------------------------

Graph *createGraph();

int appendNode(Graph *graph,
                long long node_id,
                double latitude,
                double longitude);

Node *getNode(Graph *graph,
                size_t idx);

int appendEdge(Graph *graph,
                size_t idx_from,
                size_t idx_to,
                double length,
                const char *name);

int getNodeIndexById(Graph *graph,
                     long long node_id,
                     size_t *out_idx);

int loadNodes(Graph *graph, const char *path);

int loadEdges(Graph *graph, const char *path);

void freeGraph(Graph *graph);

//----------------------------------------------------

int findClosestNode(Graph *graph,
                    double latitude,
                    double longitude,
                    size_t *out_idx);

int dijkstra(Graph* graph,
            size_t start_idx,
            size_t end_idx,
            Vector *parent);

int restorePath(size_t start_idx,
                size_t end_idx,
                Vector *parent,
                Vector *path);

//----------------------------------------------------

int readInput(const char *path,
              double *lat_start,
              double *lon_start,
              double *lat_end,
              double *lon_end);

int writeOutput(const char *path,
                Graph *graph,
                Vector *path_nodes);

#endif // GRAPH_H
