#ifndef GRAPH_H
#define GRAPH_H
#include <stdbool.h>
#include "ir.h"

//typing for compilation
typedef struct _Node Node;
typedef struct _NodeList NodeList;
typedef struct _Child Child;

//which functional unit(s) the node can run in
typedef enum {
    ZERO,
    ONE,
    BOTH
} F_Unit;

//types of edges
typedef enum {
    def,
    serial,
    conflict, 
} EdgeType;

//information to be used for heuristic
//found while traversing the graph
typedef struct{

} GraphInfo;

//Node struct for dependency graph
struct _Node{
    //original instruction
    IR* op;

    //the functional unit this node can be processed on
    F_Unit unit;

    //latency of operation
    uint8_t latency;

    //remaining cycles
    uint8_t remaining_cycles;

    //final heuristic of the node
    uint32_t heuristic;
    
    //info used for heuristic
    GraphInfo graph_info;

    // memory location used in a memop
    uint32_t* mem_loc;

    //parents list, and number of parents ready
    uint32_t n_ready;
    uint32_t n_parents;
    NodeList* parents;

    //children
    uint32_t n_children;
    Child* children; //dummy head

    //completed boolean. currently used for print
    bool complete;

    //node number. used for print
    uint32_t num;
};


//linked list of nodes
//adheres to List type
struct _NodeList{
    NodeList* next;
    NodeList* prev;
    Node* node;
};

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