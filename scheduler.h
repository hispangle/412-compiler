#ifndef SCHEDULER_H
#define SCHEDULER_H
#include <stdbool.h>
#include "constants.h"

//typing for compilation
typedef struct _Node Node;
typedef struct _NodeList NodeList;

//Node struct for dependency graph
struct _Node{
    //original instruction
    IR* op;

    //completed boolean (unsure if used)
    bool complete;

    //latency of operation
    uint8_t latency;

    //memory location used in a memop
    //uint32_t* mem_loc;

    //n_parents and n_ready for early release
    uint32_t n_parents;
    uint32_t n_ready;

    //use dependencies
    Node* def_1;
    Node* def_2;

    //serial + conflict dependencies
    //load and output conflict; store serialization
    Node* last_store;

    //output serialization
    Node* last_output;

    //store serialization
    uint32_t n_loads;
    NodeList* all_loads;

    //store serialization
    uint32_t n_outputs;
    NodeList* all_outputs;

    //children
    NodeList* first_child; //dummy head
    NodeList* last_child;
};

struct _NodeList{
    Node* node;
    NodeList* next;
};
#endif