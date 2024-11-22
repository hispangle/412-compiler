#ifndef GRAPH_H
#define GRAPH_H
#include <stdbool.h>
#include "constants.h"

//typing for compilation
typedef struct _Node Node;
typedef struct _NodeList NodeList;
typedef struct _Child Child;

typedef enum {
    ZERO,
    ONE,
    BOTH
} F_Unit;

//Node struct for dependency graph
struct _Node{
    //original instruction
    IR* op;

    //the functional unit this node can be processed on
    F_Unit unit;

    //completed boolean (unsure if used). currently used for print
    bool complete;

    //node number. used for print
    uint32_t num;

    //latency of operation
    uint8_t latency;

    //remaining cycles
    uint8_t remaining_cycles;

    //subgraph critical path sum of latencies
    uint32_t latency_cost;

    //longest path of children
    uint32_t len_longest_path;

    //final heuristic of the node
    uint32_t heuristic;

    // memory location used in a memop
    uint32_t* mem_loc;

    //n_parents and n_ready for early release
    uint32_t n_ready;
    uint32_t n_parents;
    NodeList* parents;

    //children
    uint32_t n_children;
    Child* children; //dummy head
};


//linked list of nodes
//adheres to List type
struct _NodeList{
    NodeList* next;
    NodeList* prev;
    Node* node;
};

//types of edges
typedef enum {
    def,
    serial,
    conflict, 
} EdgeType;

//linked list of children
//adheres to List type
struct _Child{
    Child* next;
    Child* prev;
    Node* node;
    EdgeType edge;
    uint32_t register_cause;
};
#endif