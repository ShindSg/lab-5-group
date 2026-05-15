#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "graph.h"
#include "../lab3/comparators.h"


static int HashLongLong(const void *key) {
    if (!key) return 0;
    
    unsigned long long x = (unsigned long long)*(const long long *)key;

    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;

    return (int)x;
}


Graph *createGraph() {
    Graph *graph = malloc(sizeof(*graph));
    if (!graph) {
        return NULL;
    }

    graph->nodes = createVector(sizeof(Node));
    if (!graph->nodes) {
        free(graph);
        return NULL;
    }

    graph->id_to_idx = createHashTable(sizeof(long long), sizeof(size_t));
    if (!graph->id_to_idx) {
        vectorFree(graph->nodes);
        free(graph);
        return NULL;
    }

    return graph;
}

int appendNode(Graph *graph,
                long long node_id,
                double latitude,
                double longitude) {
    if (!graph) {
        return -1;
    }

    Node node;
    node.node_id = node_id;
    node.latitude = latitude;
    node.longitude = longitude;

    node.edges = createVector(sizeof(Edge));
    if (!node.edges) {
        return -1;
    }

    size_t idx = graph->nodes->size;
    if (appendVectorItem(graph->nodes, &node) != 0) {
        vectorFree(node.edges);
        return -1;
    }

    setItemHashTable(graph->id_to_idx,
                    &node_id,
                    &idx,
                    HashLongLong,
                    longLongEquals);

    return 0;
}

Node *getNode(Graph *graph,
                size_t idx) {
    if (!graph || !graph->nodes) {
        return NULL;
    }

    return (Node*)getVectorItem(graph->nodes, idx);
}

int getNodeIndexById(Graph *graph,
                     long long node_id,
                     size_t *out_idx) {
    if (!graph || !out_idx || !graph->id_to_idx) {
        return -1;
    }

    void *value = getItemHashTable(graph->id_to_idx,
                                    &node_id,
                                    HashLongLong,
                                    longLongEquals);
    if (!value) {
        return -1;
    }

    *out_idx = *(size_t*)value;

    return 0;
}

int appendEdge(Graph *graph,
                size_t idx_from,
                size_t idx_to,
                double length,
                const char *name) {
    if (!graph) {
        return -1;
    }

    Node *from = getNode(graph, idx_from);
    if (!from) {
        return -1;
    }

    Edge edge;
    edge.idx_to = idx_to;
    edge.length = length;
    
    const char *safe_name = name ? name : "";
    snprintf(edge.name, sizeof(edge.name), "%s", safe_name);

    if (appendVectorItem(from->edges, &edge) != 0) {
        return -1;
    }

    return 0;
}

int loadNodes(Graph *graph, const char *path) {
    if (!graph || !path) {
        return -1;
    }

    FILE *file = fopen(path, "r");
    if (!file) {
        return -1;
    }

    char line[256];

    if (!fgets(line, sizeof(line), file)) {
        fclose(file);
        return -1;
    }

    while (fgets(line, sizeof(line), file)) {
        long long node_id;
        double lat, lon;

        if (sscanf(line, "%lld,%lf,%lf", &node_id, &lat, &lon) != 3) {
            continue;
        }

        if (appendNode(graph, node_id, lat, lon) != 0) {
            fclose(file);
            return -1;
        }
    }

    fclose(file);

    return 0;
}

int loadEdges(Graph *graph, const char *path) {
    if (!graph || !path) {
        return -1;
    }

    FILE *file = fopen(path, "r");
    if (!file) {
        return -1;
    }

    char line[512];
    if (!fgets(line, sizeof(line), file)) {
        fclose(file);
        return -1;
    }

    while (fgets(line, sizeof(line), file)) {
        long long from_id, to_id;
        double length;
        char oneway_str[16] = "";
        char name[256] = "";

        if (sscanf(line, "%lld,%lld,%lf,%15[^,],%255[^\n]",
                &from_id, &to_id, &length, oneway_str, name) < 4) {
            continue;
        }

        // bool oneway = (strcmp(oneway_str, "True") == 0);

        size_t idx_from, idx_to;

        if (getNodeIndexById(graph, from_id, &idx_from) != 0) continue;
        if (getNodeIndexById(graph, to_id, &idx_to) != 0) continue;

        if (appendEdge(graph, idx_from, idx_to, length, name) != 0) {
            continue;
        }

        // if (!oneway) {
        //     appendEdge(graph, idx_to, idx_from, length, name);
        // }
    }

    fclose(file);

    return 0;

}

void freeGraph(Graph *graph) {
    if (!graph) {
        return;
    }

    if (graph->nodes) {
        for (size_t i = 0; i < graph->nodes->size; i++) {
            Node *node = getNode(graph, i);
            if (node && node->edges) {
                vectorFree(node->edges);
            }
        }
        
        vectorFree(graph->nodes);
    }

    
    freeHashTable(graph->id_to_idx);
    free(graph);
}

int findClosestNode(Graph *graph,
                    double latitude,
                    double longitude,
                    size_t *out_idx) {
    if (!graph || !graph->nodes || !out_idx) {
        return -1;
    }

    double min_dist = DBL_MAX;
    size_t best_idx = (size_t)-1;

    for (size_t i = 0; i < graph->nodes->size; i++) {
        Node *node = getNode(graph, i);
        if (!node) continue;

        double dlat = node->latitude - latitude;
        double dlon = node->longitude - longitude;

        double dist = dlat * dlat + dlon * dlon;

        if (dist < min_dist) {
            min_dist = dist;
            best_idx = i;
        }
    }

    if (best_idx == (size_t)-1) {
        return -1;
    }

    *out_idx = best_idx;

    return 0;
}

static int minDistIdx(double *dist,
                bool *visited,
                size_t graph_size,
                size_t *out_idx) {
    if (!dist || !visited || !out_idx) {
        return -1;
    }

    double min_dist = DBL_MAX;
    size_t min_idx = (size_t)-1;

    for (size_t i = 0; i < graph_size; i++) {
        if (!visited[i] &&  dist[i] < min_dist) {
            min_dist = dist[i];
            min_idx = i;
        }
    }

    if (min_idx == (size_t)-1) {
        return -1;
    }

    *out_idx = min_idx;

    return 0;
}

int dijkstra(Graph* graph,
            size_t start_idx,
            size_t end_idx,
            Vector *parent) {
    if (!graph || !parent || !graph->nodes) {
        return -1;
    }

    size_t graph_size = graph->nodes->size;

    if (start_idx >= graph_size ||
            end_idx >= graph_size ||
            parent->size != graph_size) {
        return -1;
    }

    double *dist = malloc(graph_size * sizeof(double));
    if (!dist) {
        return -1;
    }

    for (size_t i = 0; i < graph_size; i++) {
        dist[i] = DBL_MAX;
    }
    dist[start_idx] = 0;

    bool *visited = calloc(graph_size, sizeof(bool));
    if (!visited) {
        free(dist);
        return -1;
    }

    for (size_t i = 0; i < graph_size; i++) {
        size_t min_idx;

        if (minDistIdx(dist, visited, graph_size, &min_idx) != 0) {
            break;
        }

        visited[min_idx] = true;
        if (min_idx == end_idx) break;

        Node *node = getNode(graph, min_idx);
        if (!node) {
            free(dist);
            free(visited);
            return -1;
        }

        for (size_t j = 0; j < node->edges->size; j++) {
            Edge *edge = getVectorItem(node->edges, j);
            if (!edge) {
                free(dist);
                free(visited);
                return -1;
            }

            size_t neighbor_idx = edge->idx_to;
            double neighbor_length = edge->length;

            if (visited[neighbor_idx]) continue;

            double new_dist = dist[min_idx] + neighbor_length;

            if (new_dist < dist[neighbor_idx]) {
                dist[neighbor_idx] = new_dist;
                
                if (setVectorItem(parent, neighbor_idx, &min_idx) != 0) {
                    free(dist);
                    free(visited);
                    return -1;
                }
            }
        }
    }
    
    if (dist[end_idx] == DBL_MAX) {
        free(dist);
        free(visited);
        return -1;
    }

    free(dist);
    free(visited);

    return 0;
}

int restorePath(size_t start_idx,
                size_t end_idx,
                Vector *parent,
                Vector *path) {
    if (!parent || !path) {
        return -1;
    }

    Vector *reversed = createVector(sizeof(size_t));
    if (!reversed) {
        return -1;
    }

    size_t current = end_idx;

    while (true) {
        if (appendVectorItem(reversed, &current) != 0) {
            vectorFree(reversed);
            return -1;
        }

        if (current == start_idx) {
            break;
        }

        size_t *prev = getVectorItem(parent, current);
        if (!prev || *prev == (size_t)-1) {
            vectorFree(reversed);
            return -1;
        }

        current = *prev;
    }

    for (size_t i = reversed->size; i > 0; i--) {
        size_t *idx = getVectorItem(reversed, i - 1);

        if (!idx || appendVectorItem(path, idx) != 0) {
            vectorFree(reversed);
            return -1;
        }
    }

    vectorFree(reversed);

    return 0;
}

int readInput(const char *path,
              double *lat_start,
              double *lon_start,
              double *lat_end,
              double *lon_end) {
    if (!path || !lat_start || !lon_start || !lat_end || !lon_end) {
        return -1;
    }

    FILE *file = fopen(path, "r");
    if (!file) {
        return -1;
    }

    int ok1 = fscanf(file, "%lf %lf", lat_start, lon_start);
    int ok2 = fscanf(file, "%lf %lf", lat_end, lon_end);

    fclose(file);

    if (ok1 != 2 || ok2 != 2) {
        return -1;
    }

    return 0;
}

static void clearFile(const char *path) {
    FILE *f = fopen(path, "w");

    if (f) fclose(f);
}

int writeOutput(const char *path,
                Graph *graph,
                Vector *path_nodes) {
    if (!path || !graph || !path_nodes) {
        return -1;
    }

    FILE *file = fopen(path, "w");
    if (!file) {
        return -1;
    }

    for (size_t i = 0; i < path_nodes->size; i++) {
        size_t *idx = getVectorItem(path_nodes, i);

        if (!idx) {
            fclose(file);
            clearFile(path);
            return -1;
        }

        Node *node = getNode(graph, *idx);
        if (!node) {
            fclose(file);
            clearFile(path);
            return -1;
        }

        if (fprintf(file, "%.6lf %.6lf\n", node->latitude, node->longitude) < 0) {
            fclose(file);
            clearFile(path);
            return -1;
        }
    }

    if (fclose(file) != 0) {
        clearFile(path);
        return -1;
    }

    return 0;
}

// int main(int argc, char *argv[]) {
//     if (argc != 4) return 1;

//     Graph *graph = createGraph();
//     if (!graph) return 1;

//     char nodes_path[256], edges_path[256];
//     snprintf(nodes_path, sizeof(nodes_path), "%s/nodes.csv", argv[1]);
//     snprintf(edges_path, sizeof(edges_path), "%s/edges.csv", argv[1]);

//     if (loadNodes(graph, nodes_path) != 0) return 1;
//     if (loadEdges(graph, edges_path) != 0) return 1;

//     double lat1, lon1, lat2, lon2;
//     if (readInput(argv[2], &lat1, &lon1, &lat2, &lon2) != 0) return 1;

//     size_t start_idx, end_idx;
//     if (findClosestNode(graph, lat1, lon1, &start_idx) != 0) return 1;
//     if (findClosestNode(graph, lat2, lon2, &end_idx) != 0) return 1;

//     Vector *parent = createVector(sizeof(size_t));
//     for (size_t i = 0; i < graph->nodes->size; i++) {
//         size_t p = (size_t)-1;
//         appendVectorItem(parent, &p);
//     }

//     Vector *path = createVector(sizeof(size_t));

//     if (dijkstra(graph, start_idx, end_idx, parent) == 0) {
//         restorePath(start_idx, end_idx, parent, path);
//     }

//     writeOutput(argv[3], graph, path);

//     vectorFree(path);
//     vectorFree(parent);
//     freeGraph(graph);

//     return 0;
// }