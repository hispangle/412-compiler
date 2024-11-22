#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "constants.h"
#include "graph.h"
#include "list.h"

//function declarations
inline static Node* new_node(IR* op, uint32_t op_num, uint8_t latency, F_Unit unit, uint32_t n_parents);
inline static int add_new_child(Node* node, Node* parent, EdgeType edge, uint32_t register_cause);
inline static int add_use_dependency(Node* node, uint32_t VR, Node** VRtoDef, uint8_t n_use);
inline static int add_memory_dependency(Node* node, Node** parent_location, NodeList* last, EdgeType edge);
inline static Node* find_last_memory_dependency(Node* node, NodeList* last);
NodeList* build_dependency_graph(IR* head, uint32_t maxVR);


/*
*/
inline static NodeList* new_list(){
    NodeList* list = malloc(sizeof(NodeList));
    if(list == NULL) return NULL;
    list->next = list;
    list->prev = list;
    return list;
}


/*
 * Initializes a new Node
 * Creates a linked list head for children
 * Assigns given op_num to node.
 * 
 * Requires: IR* op, the operation represented by this node. 
 *           uint32_t op_num, the op num to assign to node.
 *           uint8_t latency, the latency of processing this node.
 *           F_Unit unit, the functional unit(s) that can process this node.
 *           uint32_t n_parents, the expected number of parents this node will have.
 * 
 * Returns: Node* node on success, the new node created.
 *          NULL on failure.
*/
inline static Node* new_node(IR* op, uint32_t op_num, uint8_t latency, F_Unit unit, uint32_t n_parents){
    //malloc node
    Node* node = calloc(1, sizeof(Node));
    if(node == NULL) return NULL;
    
    //calloc child
    Child* child = malloc(sizeof(Child));
    if(child == NULL) return NULL;
    child->next = child;
    child->prev = child;
    
    //set children
    node->first_child = child;

    //set fields to arguments
    node->op = op;
    node->num = op_num;
    node->latency = latency;
    node->unit = unit;
    node->n_parents = n_parents;

    //return initialized node
    return node;
}

/*
 * Initializes a new Child. 
 * Adds child to parent's child list.
 * Assigns NULL to linked list fields.
 * Assigns passed arguments to appropriate fields.
 * 
 * Requires: Node* node, the node associated with the child.
 *           Node* parent, the parent of the child being created. Must be non null.
 *           EdgeType edge, the type of edge between this child and its parent.
 *           uint32_t register cause, the register that caused the node to be a child, if applicable (data/def edge).
 * 
 * Returns: 0 on success.
 *          -1 on failure.
*/
inline static int add_new_child(Node* node, Node* parent, EdgeType edge, uint32_t register_cause){
    //create Child
    Child* child = malloc(sizeof(Child));
    if(child == NULL) return -1;

    //assign fields
    child->next = NULL;
    child->prev = NULL;
    child->node = node;
    child->edge = edge;
    child->register_cause = register_cause;

    //add to parent
    add_circularly_doubly((List*) parent->first_child, (List*) child);
    parent->n_children++;

    return 0;
}

/*
*/
inline static int add_node_to_list(NodeList* list, Node* node){
    //create NodeList to house node
    NodeList* next_list = calloc(1, sizeof(NodeList));
    if(next_list == NULL) return -1;
    next_list->node = node;

    //add to list
    add_circularly_doubly((List*) list, (List*) next_list);

    return 0;
}


/*
*/

inline static int add_use_dependency(Node* node, uint32_t VR, Node** VRtoDef, uint8_t n_use){
    //find and add parent
    Node* parent = VRtoDef[VR];
    if(n_use == 1) node->def_1 = parent;
    else           node->def_2 = parent;    

    //create and add child
    if(add_new_child(node, parent, def, VR)) return -1;
    return 0;
}

/*
*/
inline static int add_memory_dependency(Node* node, Node** parent_location, NodeList* head, EdgeType edge){
    *parent_location = find_last_memory_dependency(node, head);

    //make children
    if(*parent_location == NULL) node->n_parents--;
    else if(add_new_child(node, *parent_location, conflict, 0)) return -1;
    return 0;
}


/*
*/
inline static Node* find_last_memory_dependency(Node* node, NodeList* head){
    //if memory address known, skip different addresses
    if(node->mem_loc != NULL){
        //loop backwards in list
        NodeList* current = head->prev;
        while(current != head){
            uint32_t* mem = current->node->mem_loc;

            //unknown or same address
            if(mem == NULL || *mem == *(node->mem_loc)){
                return current->node;
            }

            current = current->prev;
        }
    } 
    //otherwise return the latest
    else {
        return head->prev->node;
    }

    return NULL;
}


/*
 * Builds a dependency graph of a given IR. Assumes IR has been renamed.
 * Assumes VRs are used. Uses constant propagation to determine if an address
 * is known or not. 
 * Performs graph simplification as it is building the graph, built in to the algorithm.
 * Assumes store does not need all outputs, only the latest due to transitivity.
 * 
 * Requires: IR* head, the dummy head of a list of IRs. must be non null. VRs must be correct.
 *           uint32_t maxVR, the max virtual register number used in the IR.
 * Returns: NodeList* leaves on success, the list of leaves of the dependency graph.
 *          NULL on failure.
*/
NodeList* build_dependency_graph(IR* head, uint32_t maxVR){
    //create the leaves list
    NodeList* leaves = new_list(); //dummy head
    if(leaves == NULL) return NULL;

    //create the load list
    NodeList* load_list = new_list(); //dummy head
    if(load_list == NULL) return NULL;

    //create the output list
    NodeList* output_list = new_list(); //dummy head
    if(output_list == NULL) return NULL;

    //create the store list
    NodeList* store_list = new_list(); //dummy head
    if(store_list == NULL) return NULL;

    //create a VR to DEF map
    Node** VRtoDef = calloc((maxVR + 1), sizeof(Node*));
    if(VRtoDef == NULL) return NULL;

    //create a VR to Const map
    uint32_t** VRtoConst = calloc((maxVR + 1), sizeof(uint32_t*));
    if(VRtoConst == NULL) return NULL;

    //definitions
    Node* node;
    NodeList* current_store;
    NodeList* current_output;

    //loop thru operations first to last
    uint32_t op_num = 0;
    IR* op = head->next;
    while(op != head){
        //skip nops
        if(op->opcode == nop){
            op = op->next;
            continue;
        }

        //collect VRs
        uint32_t VR_1 = op->arg1.VR;
        uint32_t VR_2 = op->arg2.VR;
        uint32_t VR_3 = op->arg3.VR;

        switch(op->opcode){
            case load:
                //create new node
                node = new_node(op, op_num, 6, ZERO, 2); //def + store parents
                if(node == NULL) return NULL;

                //use dependency
                if(add_use_dependency(node, VR_1, VRtoDef, 1)) return NULL;

                //add memory location
                node->mem_loc = VRtoConst[VR_1];

                //definition
                //assign to VRtoDef
                VRtoDef[VR_3] = node;
                
                
                //memory dependencies
                //store->load (RAW) conflict
                if(add_memory_dependency(node, &(node->last_store), store_list, conflict)) return NULL;

                //add to load_list
                if(add_node_to_list(load_list, node)) return NULL;

                break;
            case store:
                //create new node
                node = new_node(op, op_num, 6, ZERO, 4); //2 defs + store + output
                if(node == NULL) return NULL;

                //use dependencies
                //use 1
                if(add_use_dependency(node, VR_1, VRtoDef, 1)) return NULL;

                //use 2
                if(add_use_dependency(node, VR_3, VRtoDef, 2)) return NULL;

                //add memory location
                node->mem_loc = VRtoConst[VR_3];

                //memory dependencies
                //store->store (WAW) serialization
                if(add_memory_dependency(node, &(node->last_store), store_list, serial)) return NULL;

                //find all load nodes with the same or unknown memory address
                NodeList* load_store = calloc(1, sizeof(NodeList));
                if(load_store == NULL) return NULL;
                NodeList* last_load_store = load_store;
                uint32_t n_load_serial = 0;

                NodeList* load_node = load_list->next;
                while(load_node != load_list){
                    //if a def dependency already exists, do not make another child
                    if(load_node->node == node->def_1 || load_node->node == node->def_2){
                        load_node = load_node->next;
                        continue;
                    } 

                    //if memory is different, continue
                    uint32_t* mem = load_node->node->mem_loc;
                    if(mem != NULL && node->mem_loc != NULL && *mem != *(node->mem_loc)){
                        load_node == load_node->next;
                        continue;
                    }

                    //make child
                    Child* child = calloc(1, sizeof(Child));
                    if(child == NULL) return NULL;
                    child->node = node;
                    child->edge = serial;

                    //update linked list
                    load_node->node->last_child->next = child;
                    load_node->node->last_child = child;
                    load_node->node->n_children++;

                    //add to load_store
                    last_load_store->next = load_node;
                    last_load_store = load_node;
                    n_load_serial++;

                    //iterate
                    load_node = load_node->next;
                }

                //connect all loads with same or unknown memory address
                node->all_loads = load_store;
                node->n_loads = n_load_serial;
                node->n_parents += n_load_serial;

                //output->store (WAR) serialization
                if(add_memory_dependency(node, &(node->last_output), output_list, serial)) return NULL;

                //add to store_list
                if(add_node_to_list(store_list, node)) return NULL;

                break;
            case loadI:
                //create new node
                node = new_node(op, op_num, 1, BOTH, 0); //always a leaf
                if(node == NULL) return NULL;

                //definition
                VRtoDef[VR_3] = node;

                //constant
                VRtoConst[VR_3] = &(op->arg1.VR);

                //create leaf
                if(add_node_to_list(leaves, node)) return NULL;
                
                break;
            case add:
            case sub:
            case mult:
            case lshift:
            case rshift:
                //create new node //latency (3 for mult, 1 for everything else) //mult can only be processed on unit one
                node = new_node(op, op_num, op->opcode == mult ? 3 : 1, op->opcode == mult ? ONE : BOTH, 2); //2 defs
                if(node == NULL) return NULL;

                //use dependencies
                //use 1
                if(add_use_dependency(node, VR_1, VRtoDef, 1)) return NULL;

                //use 2
                if(add_use_dependency(node, VR_2, VRtoDef, 2)) return NULL;

                //if both uses are constants, propagate constant
                uint32_t* const_1 = VRtoConst[VR_1];
                uint32_t* const_2 = VRtoConst[VR_2];
                if(const_1 != NULL && const_2 != NULL){
                    uint32_t* val = malloc(sizeof(uint32_t));
                    if(val == NULL) return NULL;

                    //evaluate constant
                    switch(op->opcode){
                        case add:
                            *val = *const_1 + *const_2;
                            break;
                        case sub:
                            *val = *const_1 - *const_2;
                            break;
                        case mult:
                            *val = *const_1 * *const_2;
                            break;
                        case lshift:
                            *val = *const_1 << *const_2;
                            break;
                        case rshift:
                            *val = *const_1 >> *const_2;
                            break;
                    }

                    VRtoConst[VR_3] = val;
                }


                //definition
                //assign to VRtoDef
                VRtoDef[VR_3] = node;

                //no memory dependencies

                break;
            case output:
                //create new node
                node = new_node(op, op_num, 1, ZERO, 2); //store + output
                if(node == NULL) return NULL;

                //memory dependencies
                //output->output (deterministic) serialization
                if(add_memory_dependency(node, &(node->last_output), output_list, serial)) return NULL;

                //memory location
                node->mem_loc = &(op->arg1.VR);

                //store->output (RAW) serialization
                if(add_memory_dependency(node, &(node->last_store), store_list, serial)) return NULL;

                //check if leaf (only possible if no stores or outputs beforehand)
                if(!node->n_parents){
                    //create leaf
                    add_node_to_list(leaves, node);
                }


                //add to output_list
                add_node_to_list(output_list, node);
        
                break;
            default:
                break;
        }

        op_num++;
        op = op->next;
    }

    //free at finish
    free(VRtoConst);
    free(VRtoDef);
    free(load_list);
    free(output_list);
    free(store_list);
    return leaves;
}


void calc_remaining_latency(NodeList* leaves){
    
}