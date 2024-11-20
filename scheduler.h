#ifndef SCHEDULER_H
#define SCHEDULER_H
#include "constants.h"

//typing for compilation
typedef struct _Node Node;
typedef struct _Child Child;

//Node struct for dependency graph
struct _Node{
    //original instruction
    IR* instruction;

    //use dependencies
    Node* def_1;
    Node* def_2;

    //serial + conflict dependencies
    Node* last_store;
    Node* last_load;
    Node* last_output;

    //children
    Child* children;
};

struct _Child{
    Node* node;
    Child* next;
    Child* prev;
};
#endif