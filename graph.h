#ifndef GRAPH_H
#define GRAPH_H
#include <stdbool.h>
#include "ir.h"

//typedef declarations
typedef enum F_Unit F_Unit;
typedef enum EdgeType EdgeType;
typedef struct Node Node;
typedef struct NodeList NodeList;
typedef struct Child Child;
typedef struct GraphInfo GraphInfo;

//function declarations
inline static NodeList* new_list(void);
inline static Node* new_node(IR* op, uint32_t op_num, uint8_t latency, F_Unit unit);
inline static int add_new_child(Node* node, Node* parent, EdgeType edge, uint32_t register_cause);
inline static int add_new_parent(Node* node, Node* parent);
inline static int add_node_to_list(Node* node, NodeList* list);
inline static int add_use_dependency(Node* node, uint32_t VR, Node** VRtoDef);
inline static int add_memory_dependency(Node* node, NodeList* head, EdgeType edge);
inline static int add_memory_dependency_list(Node* node, NodeList* head);
inline static Node* find_last_memory_dependency(Node* node, NodeList* head);
NodeList* build_dependency_graph(IR* head, uint32_t maxVR);
void print_graph(NodeList* nodes);

//which functional unit(s) the node can run in
enum F_Unit{
    ZERO,
    ONE,
    BOTH
};

//types of edges
enum EdgeType {
    def,
    serial,
    conflict, 
};

//information to be used for heuristic
//found while traversing the graph
struct GraphInfo{

};

//Node struct for dependency graph
struct Node{
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
struct NodeList{
    NodeList* next;
    NodeList* prev;
    Node* node;
};

//linked list of children
//adheres to List type
struct Child{
    Child* next;
    Child* prev;
    Node* node;
    EdgeType edge;
    uint32_t register_cause;
};

#endif