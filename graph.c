#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "ir.h"
#include "graph.h"
#include "list.h"
#include "string.h"


/*
*/
static inline bool check_zero(FreeVars free_vars, int32_t* zero_array, uint32_t max_VR){
    if(free_vars.offset){
        return false;
    }
    if(memcmp(free_vars.counts, zero_array, max_VR + 1)){
        return false;
    }
    return true;
}


/*
*/
static inline int evaluate(uint32_t** VRtoConst, FreeVars* VRtoFreeVars, uint32_t* zero_array, 
                            uint8_t opcode, uint32_t VR_1, uint32_t VR_2, uint32_t VR_3, uint32_t max_VR){
    uint32_t* const_1 = VRtoConst[VR_1];
    uint32_t* const_2 = VRtoConst[VR_2];
    if(const_1 != NULL && const_2 != NULL){
        uint32_t* val = malloc(sizeof(uint32_t));
        if(val == NULL) return -1;

        //evaluate constant
        switch(opcode){
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
        return 0;
    }

    //evaluate free vars
    FreeVars free_vars_1 = VRtoFreeVars[VR_1];
    FreeVars free_vars_2 = VRtoFreeVars[VR_2];

    //if either are invalid, this too must be invalid
    if(free_vars_1.invalid || free_vars_2.invalid){
        VRtoFreeVars[VR_3].invalid = true;
        return 0;
    }

    
    //create counts
    int32_t* counts = calloc(max_VR + 1, sizeof(int32_t));
    if(counts == NULL) return -1;
    VRtoFreeVars[VR_3].counts = counts;

    //if neither are constants, combine the two free_vars
    if(const_1 == NULL && const_2 == NULL){
        switch(opcode){
            case add:
                for(uint32_t i = 0; i < max_VR + 1; i++){
                    counts[i] = free_vars_1.counts[i] + free_vars_2.counts[i];
                }
                VRtoFreeVars[VR_3].offset = free_vars_1.offset + free_vars_2.offset;
                break;
            case sub:
                for(uint32_t i = 0; i < max_VR + 1; i++){
                    counts[i] = free_vars_1.counts[i] - free_vars_2.counts[i];
                }
                VRtoFreeVars[VR_3].offset = free_vars_1.offset - free_vars_2.offset;
                break;
            //only cases with at least 1 constant can be evaluated
            case mult:
                VRtoFreeVars[VR_3].invalid = true;
                break;
            //only cases with at least 1 constant can be evaluated
            case lshift:
                VRtoFreeVars[VR_3].invalid = true;
                break;
            //only cases with at least 1 constant can be evaluated
            case rshift:
                VRtoFreeVars[VR_3].invalid = true;
                break;
            default:
                break;

        }

        //check if this evaluated to 0
        if(check_zero(VRtoFreeVars[VR_3], zero_array, max_VR)){
            uint32_t* zero = calloc(1, sizeof(uint32_t));
            if(zero == NULL) return -1;
            VRtoConst[VR_3] = zero;
        }

        return 0;
    }

    //one of the consts must exist
    //force constant to be second
    if(const_1 != NULL){
        free_vars_1 = free_vars_2;
        const_2 = const_1;
    }

    //freevar (op) const case
    switch(opcode){
        case add:
            VRtoFreeVars[VR_3] = free_vars_1;
            VRtoFreeVars[VR_3].offset += *const_2;
            break;
        case sub:
            //check if const was first or second slot
            //constant second slot
            if(const_1 == NULL){
                VRtoFreeVars[VR_3] = free_vars_1;
                VRtoFreeVars[VR_3].offset -= *const_2;
                break;
            }

            //constant first slot
            for(uint32_t i = 0; i < max_VR + 1; i++){
                counts[i] = -free_vars_1.counts[i];
            }
            VRtoFreeVars[VR_3].offset = *const_2 - free_vars_1.offset;
            break;
        case mult:
            for(uint32_t i = 0; i < max_VR + 1; i++){
                counts[i] = free_vars_1.counts[i] * *const_2;
            }
            VRtoFreeVars[VR_3].offset *= *const_2;
            break;
        case lshift:
        case rshift:
            //if constant is not 0, cannot do special cases
            if(!*const_2){
                VRtoFreeVars[VR_3].invalid = true;
                break;
            }

            //check which special case to do
            //0 second slot
            if(const_1 == NULL){
                VRtoFreeVars[VR_3] = free_vars_1;
                break;
            }   

            //0 second slot
            uint32_t* zero = calloc(1, sizeof(uint32_t));
            if(zero == NULL) return -1;
            VRtoConst[VR_3] = zero;
            return 0;
            break;
        default:
            break;

        //check if this evaluated to 0
        if(check_zero(VRtoFreeVars[VR_3], zero_array, max_VR)){
            uint32_t* zero = calloc(1, sizeof(uint32_t));
            if(zero == NULL) return -1;
            VRtoConst[VR_3] = zero;
        }
    }

    return 0;
}


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
static inline Node* new_node(IR* op, uint32_t op_num, uint8_t latency, F_Unit unit, uint32_t max_VR){
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
    node->heuristic = UINT32_MAX - op_num;

    //zero fields
    node->mem_loc = NULL;
    node->n_children = 0;
    node->n_parents = 0;
    node->n_ready = 0;
    node->releasable = false;
    node->free_vars = NULL;
    
    //do free vars
    // int32_t* counts = calloc(max_VR + 1, sizeof(int32_t));
    // if(counts == NULL) return NULL;
    // node->free_vars.counts = counts;
    // node->free_vars.invalid = false;
    // node->free_vars.offset = 0;

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
static inline int add_new_child(Node* node, Node* parent, EdgeType edge, uint32_t register_cause){
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
static inline int add_new_parent(Node* node, Node* parent){
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
static inline int add_data_dependency(Node* node, uint32_t VR, Node** VRtoDef){
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
static inline int add_memory_dependency(Node* node, NodeList* head, EdgeType edge, uint32_t max_VR){
    Node* parent = find_last_memory_dependency(node, head, max_VR);

    //make children if parent exists
    if(parent != NULL){
        if(add_new_parent(node, parent)) return -1;
        if(add_new_child(node, parent, edge, 0)) return -1;
    } 
    return 0;
}


/*
 * Assumes node has a memory location
*/
static inline int add_memory_dependency_unknowns(Node* node, NodeList* head, EdgeType edge){
    uint32_t mem_loc = *(node->mem_loc);
    uint8_t added = 0;
    Node* item_node;
    NodeList* item = head->next;
    while(item != head){
        item_node = item->node;
        //add unknowns until the same address is found
        if(item_node->mem_loc == NULL){
            if(add_new_parent(node, item_node)) return -1;
            if(add_new_child(node, item_node, edge, 0)) return -1;
            added = 1;
        } else if(mem_loc == *(item_node->mem_loc)){
            //add the node with the same address if there were no unknowns
            if(!added){
                if(add_new_parent(node, item_node)) return -1;
                if(add_new_child(node, item_node, edge, 0)) return -1;
            }
            return 0;
        }


        item = item->next;
    }
    return 0;
}


/*
 * Assumes node has no memory location
*/
static inline int add_memory_dependency_latest(Node* node, NodeList* head, EdgeType edge, uint32_t max_VR){
    uint8_t* address_accounted = calloc(MEM_MAX / 4, sizeof(uint8_t));
    if(address_accounted == NULL) return -1;

    //loop until an item with unknown address is found
    Node* item_node;
    uint32_t n_parents = 0;
    NodeList* item = head->prev;
    while(item != head){
        item_node = item->node;

        //if item is unknown, finish
        if(item_node->mem_loc == NULL){
            //if different type of unknown, continue
            if(!same_location(node, item_node, max_VR)){
                item = item->prev;
                continue;
            }

            //if there were no previous nodes, add this as a parent
            if(!n_parents){
                if(add_new_parent(node, item_node)) return -1;
                if(add_new_child(node, item_node, edge, 0)) return -1;
            }
            return 0;
        }

        //add memory dependency
        //check if memory location is tracked
        //if this address chain has not been included add to parents
        if(*(item_node->mem_loc) >= MEM_MAX){
            if(add_new_parent(node, item_node)) return -1;
            if(add_new_child(node, item_node, edge, 0)) return -1;
            n_parents++;
        } else if(!address_accounted[*(item_node->mem_loc) / 4]){
            if(add_new_parent(node, item_node)) return -1;
            if(add_new_child(node, item_node, edge, 0)) return -1;
            n_parents++;
            address_accounted[*(item_node->mem_loc) / 4] = 1;
        }
        
        item = item->prev;
    }

    free(address_accounted);
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
static inline int add_memory_dependency_list(Node* node, NodeList* head, uint32_t max_VR){
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
        if(!same_location(node, list_element->node, max_VR)){
            list_element = list_element->next;
            continue;
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
*/
static inline bool same_location(Node* first, Node* second, uint32_t max_VR){
    //first location is unknown
    if(first->mem_loc == NULL){
        //second location is known
        if(second->mem_loc != NULL){
            return true;
        }

        //second location unknown
        //if counts array different, might be different
        if(memcmp(first->free_vars->counts, second->free_vars->counts, max_VR + 1)){
            return false;
        }

        //if offsets different, might be different
        if(first->free_vars->offset != second->free_vars->offset){
            return false;
        }

        //must be the same
        return true;
    }

    //first must be known
    if(second->mem_loc == NULL){
        return true;
    }

    //check locations
    if(*first->mem_loc != *second->mem_loc){
        return false;
    }

    //must be the same
    return true;
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
static inline Node* find_last_memory_dependency(Node* node, NodeList* head, uint32_t max_VR){
    //if memory address known, skip different addresses
    if(node->mem_loc != NULL){
        //loop backwards in list
        NodeList* current = head->prev;
        while(current != head){
            uint32_t* mem = current->node->mem_loc;

            if(same_location(node, current->node, max_VR)){
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
NodeList* build_dependency_graph(IR* head, uint32_t max_VR, uint32_t n_ops){
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
    Node** VRtoDef = calloc((max_VR + 1), sizeof(Node*));
    if(VRtoDef == NULL) return NULL;

    //create a VR to Const map
    uint32_t** VRtoConst = calloc((max_VR + 1), sizeof(uint32_t*));
    if(VRtoConst == NULL) return NULL;

    //create a VR to FreeVars map
    FreeVars* VRtoFreeVars = calloc((max_VR + 1), sizeof(FreeVars));
    if(VRtoFreeVars == NULL) return NULL;

    //create a zero array
    int32_t* zero = calloc(max_VR + 1, sizeof(int32_t));
    if(zero == NULL) return NULL;

    //loop thru operations first to last
    Node* node;
    IR* op = head->next;
    for(uint32_t op_num = 0; op_num < n_ops; op_num++){
        // printf("op num: %i\n", op_num);
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
                node = new_node(op, op_num, 6, ZERO, max_VR); 
                if(node == NULL) return NULL;

                //use dependency
                if(add_data_dependency(node, VR_1, VRtoDef)) return NULL;

                //add memory location
                node->mem_loc = VRtoConst[VR_1];
                node->free_vars = &(VRtoFreeVars[VR_1]);

                //definition
                //assign to VRtoDef
                VRtoDef[VR_3] = node;

                //create a free var for VR_3 if memory contents unknown (currently always)
                int32_t* counts = calloc(max_VR + 1, sizeof(int32_t));
                if(counts == NULL) return NULL;
                counts[VR_3] = 1;
                VRtoFreeVars[VR_3].counts = counts;
                
                //memory dependencies
                //store->load (RAW) conflict
                //check if this is unknown
                if(node->mem_loc == NULL){
                    if(add_memory_dependency_latest(node, store_list, conflict, max_VR)) return NULL;
                } else {
                    if(add_memory_dependency_unknowns(node, store_list, conflict)) return NULL;
                }
                

                //add to load_list
                if(add_node_to_list(node, load_list)) return NULL;

                //can release children early
                node->releasable = true;

                break;
            case store:
                //create new node
                node = new_node(op, op_num, 6, ZERO, max_VR);
                if(node == NULL) return NULL;

                //use dependencies
                //use 1
                if(add_data_dependency(node, VR_1, VRtoDef)) return NULL;
                
                //use 2
                if(add_data_dependency(node, VR_3, VRtoDef)) return NULL;

                //add memory location
                node->mem_loc = VRtoConst[VR_3];
                node->free_vars = &(VRtoFreeVars[VR_3]);

                //memory dependencies
                //store->store (WAW) serialization
                //check if this is unknown
                if(node->mem_loc == NULL){
                    if(add_memory_dependency_latest(node, store_list, serial, max_VR)) return NULL;
                } else {
                    if(add_memory_dependency_unknowns(node, store_list, serial)) return NULL;
                }

                //load->store (WAR) serializations
                if(add_memory_dependency_list(node, load_list, max_VR)) return NULL;

                //output->store (WAR) serialization
                if(add_memory_dependency(node, output_list, serial, max_VR)) return NULL;

                //add to store_list
                if(add_node_to_list(node, store_list)) return NULL;

                //can release children early
                node->releasable = true;

                break;
            case loadI:
                //create new node
                node = new_node(op, op_num, 1, BOTH, max_VR);
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
                node = new_node(op, op_num, op->opcode == mult ? 3 : 1, op->opcode == mult ? ONE : BOTH, max_VR);
                if(node == NULL) return NULL;

                //use dependencies
                //use 1
                if(add_data_dependency(node, VR_1, VRtoDef)) return NULL;

                //use 2
                if(add_data_dependency(node, VR_2, VRtoDef)) return NULL;

                //evaluate constants and free vars
                if(evaluate(VRtoConst, VRtoFreeVars, zero, op->opcode, VR_1, VR_2, VR_3, max_VR)) return NULL;

                //definition
                //assign to VRtoDef
                VRtoDef[VR_3] = node;

                //no memory dependencies

                break;
            case output:
                //create new node
                node = new_node(op, op_num, 1, BOTH, max_VR); 
                if(node == NULL) return NULL;

                //memory dependencies
                //output->output (deterministic) serialization
                if(add_memory_dependency(node, output_list, serial, max_VR)) return NULL;

                //memory location
                node->mem_loc = &(op->arg1.VR);

                //store->output (RAW) conflict
                if(add_memory_dependency(node, store_list, conflict, max_VR)) return NULL;

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
        // printf("; n_par = %i; n_ready = %i; n_child = %i; heuristic: %i", node->n_parents, node->n_ready, node->n_children, node->heuristic);
        printf("\"];\n");
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