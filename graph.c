#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "ir.h"
#include "graph.h"
#include "list.h"


/*
 * Initializes a new Node.
 * Creates a linked list head for children and parents.
 * Assigns given arguments to node.
 * 
 * Requires: 
 *      op: the operation represented by this node. 
 *      op_num: the op num to assign to node.
 *      latency: the latency of processing this node.
 *      unit: the functional unit(s) that can process this node.
 * 
 * Returns: 
 *      Node* node on success, the new node created.
 *      NULL on failure.
*/
inline static Node* new_node(IR* op, uint32_t op_num, uint8_t latency, F_Unit unit){
    //malloc node
    Node* node = malloc(sizeof(Node));
    if(node == NULL) return NULL;
    
    //malloc child
    Child* child = malloc(sizeof(Child));
    if(child == NULL) return NULL;
    child->next = child;
    child->prev = child;
    child->node = NULL;
    
    //set children
    node->children = child;

    //make parents list
    NodeList* parents = new_list();
    if(parents == NULL) return NULL;
    node->parents = parents;

    //set fields
    node->op = op;
    node->num = op_num;
    node->latency = latency;
    node->remaining_cycles = latency;
    node->unit = unit;
    node->heuristic = 1;

    //zero fields
    node->mem_loc = NULL;
    node->n_children = 0;
    node->n_parents = 0;
    node->n_ready = 0;
    node->releasable = false;

    //return initialized node
    return node;
}

/*
 * Initializes a new Child. 
 * Adds child to parent's child list.
 * Assigns NULL to linked list fields.
 * Assigns passed arguments to appropriate fields.
 * 
 * Requires: 
 *      Node* node: the node associated with the child.
 *      Node* parent: the parent of the child being created. Must be non null.
 *      EdgeType edge: the type of edge between this child and its parent.
 *      uint32_t register cause: the register that caused the node to be a child, if applicable (data/def edge).
 * 
 * Returns: 
 *      0 on success.
 *      -1 on failure.
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
    add_to_list((List*) parent->children, (List*) child);
    parent->n_children++;

    return 0;
}

/*
*/
inline static int add_new_parent(Node* node, Node* parent){
    if(add_node_to_list(parent, node->parents)) return -1;
    node->n_parents++;
    return 0;
}

/*
 * Adds a data dependency to node from the node that defines the VR. 
 * Does not add dependency if it has already been made by a different call with 
 * the same node and VR. 
 * Modifies node and its parent.
 * 
 * Requires:
 *      Node* node: the node to make the dependency for.
 *      uint32_t VR: the VR whose definition to search for.
 *      Node** VRtoDef: the array that maps VRs to their definitions.
 * 
 * Returns:
 *      0 on success
 *      -1 on failure
*/
inline static int add_data_dependency(Node* node, uint32_t VR, Node** VRtoDef){
    //find and add parent
    Node* parent = VRtoDef[VR];
    // if(parent == NULL) return -1; //only possible on undefed use

    //check if double def
    if(parent == node->parents->next->node) return 0;

    //otherwise proceed as normal
    if(add_new_parent(node, parent)) return -1;

    //create and add child
    if(add_new_child(node, parent, def, VR)) return -1;

    return 0;
}

/*
 * Adds a memory dependency (edge) between the node and the last node in the list 
 * given by head that has the same memory access as node, or an unknown memory access.
 * Edge type is given by edge. 
 * Modifies node and some element of the list given at head.
 * 
 * Requires:
 *      Node* node: the node looking for a memory dependency
 *      NodeList* head: the dummy head of the list to search for the memory dependency
 *      EdgeType edge: the type of edge to create
 * 
 * Returns:
 *      0 on success
 *      -1 on failure
*/
inline static int add_memory_dependency(Node* node, NodeList* head, EdgeType edge){
    Node* parent = find_last_memory_dependency(node, head);

    //make children if parent exists
    if(parent != NULL){
        if(add_new_parent(node, parent)) return -1;
        if(add_new_child(node, parent, edge, 0)) return -1;
    } 
    return 0;
}

/*
 * Creates a memory dependency (edge) between the given node and all nodes in the list that access the 
 * same memory address as the node (including unknown). Does not duplicate existing edges with 
 * definitions. Currently assumes the edge is a serial edge.
 * Modifies node and an element of the list given by head.
 * 
 * Requires:
 *      Node* node: the node for edges are looked. Must be non null.
 *      NodeList* head: the head of the list that is being searched. Must be non null.
 *      
 * Returns: 
 *      0 on success
 *      -1 on failure
*/
inline static int add_memory_dependency_list(Node* node, NodeList* head){
    //get def dependencies (always the first two)
    Node* def_parent_1 = node->parents->next->node;
    Node* def_parent_2 = node->parents->next->next->node;

    //go thru all elements in list for dependencies
    NodeList* list_element = head->next;
    while(list_element != head){
        //if a def dependency already exists, do not make another child
        if(list_element->node == def_parent_1 || list_element->node == def_parent_2){
            list_element = list_element->next;
            continue;
        } 

        //if memory is different, continue
        uint32_t* mem = list_element->node->mem_loc;
        if(mem != NULL && node->mem_loc != NULL){
            if(*mem != *(node->mem_loc)){
                list_element = list_element->next;
                continue;
            }
        }

        //make child //assumes serial edge
        if(add_new_child(node, list_element->node, serial, 0)) return -1;

        //add to parents
        if(add_new_parent(node, list_element->node)) return -1;

        //iterate
        list_element = list_element->next;
    }

    return 0;
}

/*
 * Find the last node in the list given that has the same memory address as node or 
 * has an unknown memory address. Always gives the last node if node's memory address
 * is unknown.
 * Node with different addresses are not returned because they are 
 * independent of this node.
 * 
 * Requires:
 *      Node* node: the node whose address to compare to
 *      NodeList* head: the dummy head of the list of nodes to search for.
 * 
 * Returns:
 *      Node*: the node that satisfies conditions stated.
 *          Is be NULL if no node satisfies 
 *          the conditions. 
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
 * Requires: 
 *      IR* head: the dummy head of a list of IRs. must be non null.
 *      uint32_t maxVR: the max virtual register number used in the IR.
 *      uint32_t n_ops: the number of operations in the IR.
 * 
 * Returns: 
 *      NodeList*, the dummy head of the list of all nodes in the 
 *          dependency graph on success.
 *      NULL on failure.
*/
NodeList* build_dependency_graph(IR* head, uint32_t maxVR, uint32_t n_ops){
    //create the nodes list
    NodeList* node_list = new_list();
    if(node_list == NULL) return NULL;

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

    //loop thru operations first to last
    Node* node;
    IR* op = head->next;
    for(uint32_t op_num = 0; op_num < n_ops; op_num++){
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
                node = new_node(op, op_num, 6, ZERO); 
                if(node == NULL) return NULL;

                //use dependency
                if(add_data_dependency(node, VR_1, VRtoDef)) return NULL;

                //add memory location
                node->mem_loc = VRtoConst[VR_1];

                //definition
                //assign to VRtoDef
                VRtoDef[VR_3] = node;
                
                //memory dependencies
                //store->load (RAW) conflict
                if(add_memory_dependency(node, store_list, conflict)) return NULL;

                //add to load_list
                if(add_node_to_list(node, load_list)) return NULL;

                //can release children early
                node->releasable = true;

                break;
            case store:
                //create new node
                node = new_node(op, op_num, 6, ZERO);
                if(node == NULL) return NULL;

                //use dependencies
                //use 1
                if(add_data_dependency(node, VR_1, VRtoDef)) return NULL;
                
                //use 2
                if(add_data_dependency(node, VR_3, VRtoDef)) return NULL;

                //add memory location
                node->mem_loc = VRtoConst[VR_3];

                //memory dependencies
                //store->store (WAW) serialization
                if(add_memory_dependency(node, store_list, serial)) return NULL;

                //load->store (WAR) serializations
                if(add_memory_dependency_list(node, load_list)) return NULL;

                //output->store (WAR) serialization
                if(add_memory_dependency(node, output_list, serial)) return NULL;

                //add to store_list
                if(add_node_to_list(node, store_list)) return NULL;
                
                //can release children early
                node->releasable = true;

                break;
            case loadI:
                //create new node
                node = new_node(op, op_num, 1, BOTH);
                if(node == NULL) return NULL;

                //definition
                VRtoDef[VR_3] = node;

                //constant
                VRtoConst[VR_3] = &(op->arg1.VR);

                break;
            case add:
            case sub:
            case mult:
            case lshift:
            case rshift:
                //create new node //latency (3 for mult, 1 for everything else) //mult can only be processed on unit one
                node = new_node(op, op_num, op->opcode == mult ? 3 : 1, op->opcode == mult ? ONE : BOTH);
                if(node == NULL) return NULL;

                //use dependencies
                //use 1
                if(add_data_dependency(node, VR_1, VRtoDef)) return NULL;

                //use 2
                if(add_data_dependency(node, VR_2, VRtoDef)) return NULL;

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
                node = new_node(op, op_num, 1, BOTH); 
                if(node == NULL) return NULL;

                //memory dependencies
                //output->output (deterministic) serialization
                if(add_memory_dependency(node, output_list, serial)) return NULL;

                //memory location
                node->mem_loc = &(op->arg1.VR);

                //store->output (RAW) conflict
                if(add_memory_dependency(node, store_list, conflict)) return NULL;

                //add to output_list
                add_node_to_list(node, output_list);
                
                //can release children early
                node->releasable = true;
        
                break;
            default:
                break;
        }

        //add to node list
        if(add_node_to_list(node, node_list)) return NULL;

        //increment
        op = op->next;
    }

    //free at finish
    free(VRtoConst);
    free(VRtoDef);
    free(load_list);
    free(output_list);
    free(store_list);
    return node_list;
}


/*
*/
int calc_heuristics(NodeList* graph){
    //weight: nready but for children
    //load > store: if load +5?



    NodeList* explore = new_list();
    if(explore == NULL) return -1;

    //add roots to explore
    NodeList* item = graph->next;
    while(item != graph){
        //add if root
        if(item->node->n_children == 0){
            if(add_node_to_list(item->node, explore)) return -1;
        }

        item = item->next;
    }


    //go thru all nodes
    Node* node;
    NodeList* parent;
    while(explore->next != explore){
        NodeList* new_explore = new_list();
        if(new_explore == NULL) return -1;

        item = explore->next;
        while(item != explore){
            //set heuristic to be max_weight + latency (+ 5 if load)
            node = item->node;
            node->heuristic = node->graph_info.max_weight + node->op == load ? 5 : 0;

            //change all parents
            parent = node->parents->next;
            for(uint32_t i = 0; i < node->n_parents; i++){
                //change weight
                uint32_t* weight = &(parent->node->graph_info.max_weight);
                *weight = MAX(*weight, node->heuristic);

                //change n_ready
                parent->node->graph_info.n_ready++;

                //add to explore if all children explored
                if(parent->node->graph_info.n_ready == parent->node->n_children){
                    if(add_node_to_list(parent->node, new_explore)) return -1;
                }

                parent = parent->next;
            }


            item = item->next;
        }

        NodeList* temp = explore;
        explore = new_explore;
        free(temp);
    }

}


/*
 * Prints the dependency graph given by the nodes in dot format, readable by graphviz.
 * Does not modify the graph, or the nodes of the graph.
 * 
 * Requires: 
 *      NodeList* nodes, the list of nodes of the dependency graph. Must be non null.
 * 
 * Returns: 
 *      nothing
*/
void print_graph(NodeList* nodes){
    //print in dot format
    printf("digraph DG{\n");

    //print all graph nodes
    NodeList* item = nodes->next;
    while(item != nodes){
        //print node
        Node* node = item->node;
        printf("\t%i [label=\"%i: ", node->num, node->num);
        print_IR(node->op, VR);
        printf("; n_par = %i; n_ready = %i; n_child = %i\"];\n", node->n_parents, node->n_ready, node->n_children);
        item = item->next;
    }

    //define type strings
    char* edge_types[3] = {"Data", "Serial", "Conflict"};

    //print edges
    item = nodes->next;
    while(item != nodes){
        //print all edges for this node
        Node* node = item->node;

        Child* child = (Child*) node->children->next;
        for(uint32_t j = 0; j < node->n_children; j++){
            if(child->edge == def){
                printf("\t%i->%i [label = \"%s VR%i\"];\n", node->num, child->node->num, edge_types[child->edge], child->register_cause);
            } else {
                printf("\t%i->%i [label = \"%s\"];\n", node->num, child->node->num, edge_types[child->edge]);
            }
            child = child->next;
        }

        item = item->next;
    }

    //finish dot format
    printf("}\n");
}