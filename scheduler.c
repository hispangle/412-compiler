#include <stdlib.h>
#include <stdint.h>
#include "constants.h"
#include "scheduler.h"


/*
 * Initializes a new Node
 * Creates a linked list head for children
 * 
 * 
*/
Node* new_node(uint32_t op_num){
    //malloc node
    Node* node = malloc(sizeof(Node));
    if(node == NULL) return NULL;
    
    //malloc child
    Child* child = malloc(sizeof(Child));
    if(child == NULL) return NULL;
    
    //set children
    node->first_child = child;
    node->last_child = child;

    //set complete to false
    node->complete = false;

    //give op_num
    node->num = op_num;

    //return initialized node
    return node;
}



//builds a dependency graph of a given IR
//assumes IR has already been renamed
//assumes VR usage
//currently assumes addresses are unknown
//success: returns a NodeList* of Nodes that are leaves in the dependency graph
//failure: returns NULL
NodeList* build_dependency_graph(IR* head, uint32_t maxVR){
    //create the leaves list
    NodeList* leaves = malloc(sizeof(NodeList)); //dummy head
    if(leaves == NULL) return NULL;

    //current end of list
    NodeList* last_leaf = leaves;


    //create the load list
    NodeList* load_list = malloc(sizeof(NodeList)); //dummy head
    if(load_list == NULL) return NULL;

    //current last load
    load_list->node = NULL; //make this explicit as a preventative measure against bugs
    NodeList* last_load = load_list;

    //keep track of n_loads
    uint32_t n_loads = 0;


    //create the output list
    NodeList* output_list = malloc(sizeof(NodeList));
    if(output_list == NULL) return NULL;

    //current last output
    output_list->node = NULL; //preventative bug squash
    NodeList* last_output = output_list;

    //keep track of n_outputs
    uint32_t n_outputs = 0;


    //create the store list
    NodeList* store_list = malloc(sizeof(NodeList));
    if(store_list == NULL) return NULL;

    //current last store
    store_list->node = NULL; //preventative bug squash
    NodeList* last_store = store_list;

    //keep track of n_outputs
    uint32_t n_stores = 0;


    //create a VR to DEF map
    Node** VRtoDef = malloc(sizeof(Node*) * (maxVR + 1));
    if(VRtoDef == NULL) return NULL;


    //create a VR to Const map
    uint32_t** VRtoConst = malloc(sizeof(uint32_t*) * (maxVR + 1));
    if(VRtoConst == NULL) return NULL;


    //keep track of op_num
    uint32_t op_num = 0;


    //loop thru operations first to last
    IR* op = head->next;
    while(op != head){
        //skip nops
        if(op->opcode == nop){
            op = op->next;
            continue;
        }

        //make new node
        Node* node = new_node(op_num);
        if(node == NULL) return NULL;
        node->op = op;

        //collect VRs
        uint32_t VR_1 = op->arg1.VR;
        uint32_t VR_2 = op->arg2.VR;
        uint32_t VR_3 = op->arg3.VR;

        //definitions
        Node* parent_1;
        Node* parent_2;
        Child* child;

        uint32_t* mem;
        NodeList* current_store;
        NodeList* current_output;

        switch(op->opcode){
            case load:
                //latency
                node->latency = 6;


                //expected number of parents
                node->n_parents = 2;


                //use dependencies
                //use 1
                parent_1 = VRtoDef[VR_1];
                node->mem_loc = VRtoConst[VR_1];
                node->def_1 = parent_1;

                //add node to children of parent
                // if(parent_1 != NULL){
                    //create child
                    child = malloc(sizeof(Child));
                    if(child == NULL) return NULL;
                    child->node = node;
                    child->edge = def;

                    //extend child list
                    parent_1->last_child->next = child;
                    parent_1->last_child = child;
                // }

                // //undefed use
                // else {
                //     node->n_parents--;
                // }


                //def
                //no dependencies
                //assign to VRtoDef
                VRtoDef[VR_3] = node;
                

                //memory dependencies
                //find last store that has same or unknown address
                if(node->mem_loc != NULL){
                    current_store = last_store;
                    while(current_store->node != NULL){
                        mem = current_store->node->mem_loc;

                        //unknown or same address
                        if(mem == NULL || *mem == *(node->mem_loc)){
                            node->last_store = current_store->node;
                            break;
                        }

                        current_store = current_store->prev;
                    }
                } else {
                    node->last_store = last_store->node;
                }
                
                

                //make children
                if(node->last_store == NULL){
                    node->n_parents--;
                } else {
                    //make child
                    child = malloc(sizeof(Child));
                    if(child == NULL) return NULL;
                    child->node = node;
                    child->edge = conflict;

                    //assign child
                    node->last_store->last_child->next = child;
                    node->last_store->last_child = child;
                }


                // //check if leaf (only possible on undefed use)
                // if(!node->n_parents){
                //     //create leaf
                //     NodeList* next_leaf = malloc(sizeof(NodeList));
                //     if(next_leaf == NULL) return NULL;
                //     next_leaf->node = node;

                //     //extend list
                //     last_leaf->next = next_leaf;
                //     last_leaf = next_leaf;
                // }


                //add to load_list
                NodeList* next_load = malloc(sizeof(NodeList));
                if(next_load == NULL) return NULL;
                next_load->node = node;

                last_load->next = next_load;
                next_load->prev = last_load;
                last_load = next_load;
                n_loads++;

                break;
            case store:
                //latency
                node->latency = 6;


                //expected number of parents //must add n loads found later
                node->n_parents = 4; //2 defs + store + output


                //process def restrictions
                //use 1
                parent_1 = VRtoDef[VR_1];
                node->def_1 = parent_1;

                //add node to children of parent
                // if(parent_1 != NULL){
                    //create child
                    child = malloc(sizeof(Child));
                    if(child == NULL) return NULL;
                    child->node = node;
                    child->edge = def;

                    //extend child list
                    parent_1->last_child->next = child;
                    parent_1->last_child = child;
                // }

                // //undefed use
                // else {
                //     node->n_parents--;
                // }


                //use 2
                parent_2 = VRtoDef[VR_3];
                node->mem_loc = VRtoConst[VR_3];
                node->def_2 = parent_2;

                //add node to children of parent
                // if(parent_2 != NULL){
                    //create child
                    child = malloc(sizeof(Child));
                    if(child == NULL) return NULL;
                    child->node = node;
                    child->edge = def;

                    //extend child list
                    parent_2->last_child->next = child;
                    parent_2->last_child = child;
                // }

                // //undefed use
                // if(parent_2 == NULL){
                //     node->n_parents--;
                // }


                //memory dependencies
                //find last store with same or unknown address
                if(node->mem_loc != NULL){
                    current_store = last_store;
                    while(current_store->node != NULL){
                        mem = current_store->node->mem_loc;

                        //unknown or same address
                        if(mem == NULL || *mem == *(node->mem_loc)){
                            node->last_store = current_store->node;
                            break;
                        }

                        current_store = current_store->prev;
                    }
                } else {
                    node->last_store = last_store->node;
                }

                //create edge if needed
                if(node->last_store == NULL){
                    node->n_parents--;
                } else {
                    //make child
                    child = malloc(sizeof(Child));
                    if(child == NULL) return NULL;
                    child->node = node;
                    child->edge = serial;

                    //assign child
                    node->last_store->last_child->next = child;
                    node->last_store->last_child = child;
                }


                //find all load nodes with the same or unknown memory address
                NodeList* load_store = malloc(sizeof(NodeList));
                if(load_store == NULL) return NULL;
                NodeList* last_load_store = load_store;
                uint32_t n_load_serial = 0;

                NodeList* load_node = load_list->next;
                for(uint32_t i = 0; i < n_loads; i++){
                    //if a def dependency already exists, do not make another child
                    if(load_node->node == parent_1 || load_node->node == parent_2){
                        load_node = load_node->next;
                        continue;
                    } 

                    //if memory is different, continue
                    mem = load_node->node->mem_loc;
                    if(mem != NULL && node->mem_loc != NULL && *mem != *(node->mem_loc)){
                        load_node == load_node->next;
                        continue;
                    }

                    //make child
                    child = malloc(sizeof(Child));
                    if(child == NULL) return NULL;
                    child->node = node;
                    child->edge = serial;

                    //update linked list
                    load_node->node->last_child->next = child;
                    load_node->node->last_child = child;

                    //add to load_store
                    last_load_store->next = load_node;
                    last_load_store = load_node;

                    //iterate
                    load_node = load_node->next;
                    n_load_serial++;
                }

                //connect all loads with same or unknown memory address
                node->all_loads = load_store;
                node->n_loads = n_load_serial;
                node->n_parents += n_load_serial;


                //find last output with same or unknown address
                if(node->mem_loc != NULL){
                    current_output = last_output;
                    while(current_output->node != NULL){
                        mem = current_output->node->mem_loc;

                        //unknown or same address
                        if(mem == NULL || *mem == *(node->mem_loc)){
                            node->last_output = current_output->node;
                            break;
                        }

                        current_output = current_output->prev;
                    }
                } else {
                    node->last_output = last_output->node;
                }

                //create edge if needed
                if(node->last_output == NULL){
                    node->n_parents--;
                } else {
                    //make child
                    child = malloc(sizeof(Child));
                    if(child == NULL) return NULL;
                    child->node = node;
                    child->edge = serial;

                    //assign child
                    node->last_output->last_child->next = child;
                    node->last_output->last_child = child;
                }


                // //check if leaf (only possible on undefed use)
                // if(!node->n_parents){
                //     //create leaf
                //     NodeList* next_leaf = malloc(sizeof(NodeList));
                //     if(next_leaf == NULL) return NULL;
                //     next_leaf->node = node;

                //     //extend list
                //     last_leaf->next = next_leaf;
                //     last_leaf = next_leaf;
                // }


                //add to store_list
                NodeList* next_store = malloc(sizeof(NodeList));
                if(next_store == NULL) return NULL;
                next_store->node = node;

                last_store->next = next_store;
                next_store->prev = last_store;
                last_store = next_store;
                n_stores++;

                break;
            case loadI:
                //always a leaf
                //def
                VRtoDef[VR_3] = node;
                VRtoConst[VR_3] = &(op->arg1.VR);

                //create leaf
                NodeList* next_leaf = malloc(sizeof(NodeList));
                if(next_leaf == NULL) return NULL;
                next_leaf->node = node;

                //extend list
                last_leaf->next = next_leaf;
                last_leaf = next_leaf;

                break;
            case add:
            case sub:
            case mult:
            case lshift:
            case rshift:
                //latency (3 for mult, 1 for everything else)
                node->latency = op->opcode == mult ? 3 : 1;


                //expected number of parents
                node->n_parents = 2;


                //process def restrictions
                //use 1
                parent_1 = VRtoDef[VR_1];
                node->def_1 = parent_1;
                uint32_t* const_1 = VRtoConst[VR_1];

                //add node to children of parent
                // if(parent_1 != NULL){
                    //create child
                    child = malloc(sizeof(Child));
                    if(child == NULL) return NULL;
                    child->node = node;
                    child->edge = def;

                    //extend child list
                    parent_1->last_child->next = child;
                    parent_1->last_child = child;
                // }

                // //undefed use
                // else {
                //     node->n_parents--;
                // }


                //use 2
                parent_2 = VRtoDef[VR_2];
                node->def_2 = parent_2;
                uint32_t* const_2 = VRtoConst[VR_2];


                //if both uses are constants, propagate constant
                if(const_1 != NULL && const_2 != NULL){
                    uint32_t* val = malloc(sizeof(uint32_t));
                    if(val == NULL) return NULL;

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


                //add node to children of parent
                // if(parent_2 != NULL){
                    //create child
                    child = malloc(sizeof(Child));
                    if(child == NULL) return NULL;
                    child->node = node;
                    child->edge = def;

                    //extend child list
                    parent_2->last_child->next = child;
                    parent_2->last_child = child;
                // }

                // //undefed use
                // if(parent_2 == NULL){
                //     node->n_parents--;
                // }


                //def
                //no dependencies
                //assign to VRtoDef
                VRtoDef[VR_3] = node;


                //no memory dependencies; 


                // //check if leaf (only possible on undefed use)
                // if(!node->n_parents){
                //     //create leaf
                //     NodeList* next_leaf = malloc(sizeof(NodeList));
                //     if(next_leaf == NULL) return NULL;
                //     next_leaf->node = node;

                //     //extend list
                //     last_leaf->next = next_leaf;
                //     last_leaf = next_leaf;
                // }

                break;
            case output:
                //latency
                node->latency = 1;


                //expected number of parents
                node->n_parents = 2; //output + store


                //no defs
                node->mem_loc = &(op->arg1.VR);


                //memory dependencies
                //find last store with same or unknown address
                if(node->mem_loc != NULL){
                    current_store = last_store;
                    while(current_store->node != NULL){
                        mem = current_store->node->mem_loc;

                        //unknown or same address
                        if(mem == NULL || *mem == *(node->mem_loc)){
                            node->last_store = current_store->node;
                            break;
                        }

                        current_store = current_store->prev;
                    }
                } else {
                    node->last_store = last_store->node;
                }

                //create edge if needed
                if(node->last_store == NULL){
                    node->n_parents--;
                } else {
                    //make child
                    child = malloc(sizeof(Child));
                    if(child == NULL) return NULL;
                    child->node = node;
                    child->edge = serial;

                    //assign child
                    node->last_store->last_child->next = child;
                    node->last_store->last_child = child;
                }


                //find latest output (always)
                node->last_output = last_output->node;

                //deal with parents
                if(last_output->node == NULL){
                    node->n_parents--;
                } else {
                    //make child
                    child = malloc(sizeof(Child));
                    if(child == NULL) return NULL;
                    child->node = node;
                    child->edge = serial;

                    //assign child
                    last_output->node->last_child->next = child;
                    last_output->node->last_child = child;
                }


                //check if leaf (only possible if no stores or outputs beforehand)
                if(!node->n_parents){
                    //create leaf
                    NodeList* next_leaf = malloc(sizeof(NodeList));
                    if(next_leaf == NULL) return NULL;
                    next_leaf->node = node;

                    //extend list
                    last_leaf->next = next_leaf;
                    last_leaf = next_leaf;
                }


                //add to output_list
                NodeList* next_output = malloc(sizeof(NodeList));
                if(next_output == NULL) return NULL;
                next_output->node = node;

                last_output->next = next_output;
                next_output->prev = last_output;
                last_output = next_output;
                n_outputs++;
        
                break;
            default:
                break;
        }

        op_num++;
        op = op->next;
    }

    return leaves;
}
