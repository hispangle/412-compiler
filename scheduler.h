#ifndef SCHEDULER_H
#define SCHEDULER_H
#include <stdbool.h>
#include "constants.h"

//typing for compilation
typedef struct _Node Node;
typedef struct _NodeList NodeList;
typedef struct _Child Child;

//Node struct for dependency graph
struct _Node{
    //original instruction
    IR* op;

    //completed boolean (unsure if used). currently used for print
    bool complete;

    //node number. used for print
    uint32_t num;

    //latency of operation
    uint8_t latency;

    // memory location used in a memop
    uint32_t* mem_loc;

    //n_parents and n_ready for early release
    uint32_t n_parents;
    uint32_t n_ready;

    //use dependencies
    Node* def_1;
    Node* def_2;

    //serial + conflict dependencies
    //load and output conflict; store serialization
    Node* last_store;

    //output and store serialization
    Node* last_output;

    //store serialization
    uint32_t n_loads;
    NodeList* all_loads;

    //children
    Child* first_child; //dummy head
    Child* last_child;
};


//linked list of nodes
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
//has an additional 'type' attribute to NodeList
struct _Child{
    Child* next;
    Child* prev;
    Node* node;
    EdgeType edge;
};
#endif