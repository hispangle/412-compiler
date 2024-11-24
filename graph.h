#ifndef GRAPH_H
#define GRAPH_H
#include <stdbool.h>
#include "ir.h"
#include "list.h"

//define a max function
#define MAX(a, b) a > b ? a : b

//define the number of bytes accessible
#define MEM_MAX 32768

//typedef declarations
typedef enum F_Unit F_Unit;
typedef enum EdgeType EdgeType;
typedef struct Node Node;
typedef struct NodeList NodeList;
typedef struct Child Child;
typedef struct GraphInfo GraphInfo;

//function declarations
static inline NodeList* new_list(void);
static inline Node* new_node(IR* op, uint32_t op_num, uint8_t latency, F_Unit unit);
static inline int add_new_child(Node* node, Node* parent, EdgeType edge, uint32_t register_cause);
static inline int add_new_parent(Node* node, Node* parent);
static inline int add_node_to_list(Node* node, NodeList* list);
static inline int add_data_dependency(Node* node, uint32_t VR, Node** VRtoDef);
static inline int add_memory_dependency(Node* node, NodeList* head, EdgeType edge);
static inline int add_memory_dependency_list(Node* node, NodeList* head);
static inline Node* find_last_memory_dependency(Node* node, NodeList* head);
NodeList* build_dependency_graph(IR* head, uint32_t maxVR, uint32_t n_ops);
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
    uint32_t n_ready;
    uint32_t max_weight;
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

    //if the node qualifies for early release checks
    bool releasable;

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

//inline function declared in header
/*
 * Creates and initializes a new linked list of nodes.
 * Sets all fields of the dummy head. Node is NULL. 
 * 
 * Requires: 
 *      nothing.
 * 
 * Returns:
 *      NodeList*: the dummy head of the created list.
*/
static inline NodeList* new_list(){
    NodeList* list = malloc(sizeof(NodeList));
    if(list == NULL) return NULL;
    list->next = list;
    list->prev = list;
    list->node = NULL;
    return list;
}

/*
 * Adds the given node to the given node list.
 * Creates a new NodeList to house the node and adds that to the node list.
 * Does not modify node.
 * 
 * Requires:
 *      Node* node: the node to be added.
 *      NodeList*: the list to add to
 * 
 * Returns:
 *      0 on success
 *      -1 on failure
*/
static inline int add_node_to_list(Node* node, NodeList* list){
    //create NodeList to house node
    NodeList* next_list = malloc(sizeof(NodeList));
    if(next_list == NULL) return -1;
    next_list->node = node;

    //add to list
    add_to_list((List*) list, (List*) next_list);
    return 0;
}

#endif