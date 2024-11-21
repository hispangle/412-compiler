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

        switch(op->opcode){
            case load:
                //latency
                node->latency = 6;


                //expected number of parents
                node->n_parents = 2;


                //use dependencies
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


                //def
                //no dependencies
                //assign to VRtoDef
                VRtoDef[VR_3] = node;
                

                //memory dependencies
                node->last_store = last_store->node;
                if(last_store->node == NULL){
                    node->n_parents--;
                } else {
                    //make child
                    child = malloc(sizeof(Child));
                    if(child == NULL) return NULL;
                    child->node = node;
                    child->edge = conflict;

                    //assign child
                    last_store->node->last_child->next = child;
                    last_store->node->last_child = child;
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


                //expected number of parents
                node->n_parents = 3 + n_loads + n_outputs;


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
                node->last_store = last_store->node;

                //create edge if needed
                if(last_store->node == NULL){
                    node->n_parents--;
                } else {
                    //make child
                    child = malloc(sizeof(Child));
                    if(child == NULL) return NULL;
                    child->node = node;
                    child->edge = serial;

                    //assign child
                    last_store->node->last_child->next = child;
                    last_store->node->last_child = child;
                }

                //connect all loads with same or unknown memory address
                node->all_loads = load_list;
                node->n_loads = n_loads;

                //add node to children list of all loads
                NodeList* load_node = load_list->next;
                for(int i = 0; i < n_loads; i++){
                    //if a def dependency already exists, reduce the parent count and do not make another child
                    if(load_node->node == parent_1 || load_node->node == parent_2){
                        node->n_parents--;
                        load_node = load_node->next;
                        continue;
                    } 

                    child = malloc(sizeof(Child));
                    if(child == NULL) return NULL;
                    child->node = node;
                    child->edge = serial;

                    //update linked list
                    load_node->node->last_child->next = child;
                    load_node->node->last_child = child;
                    load_node = load_node->next;
                }

                //connect last output

                //find last output that is same or unknown memory address
                node->last_output = last_output->node;

                //make the edge
                if(last_output->node == NULL){
                    node->n_parents--;
                } else {
                    //make child
                    child = malloc(sizeof(Child));
                    if(child == NULL) return NULL;
                    child->node = node;
                    child->edge = serial;

                    last_output->node->last_child->next = child;
                    last_output->node->last_child = child;
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
                node->n_parents = 2;


                //no uses
                //no defs


                //memory dependencies

                //find last_store with same or unknown memory address
                node->last_store = last_store->node;

                //edge relations
                if(last_store->node == NULL){
                    node->n_parents--;
                } else {
                    //make child
                    child = malloc(sizeof(Child));
                    if(child == NULL) return NULL;
                    child->node = node;
                    child->edge = conflict;

                    //assign child
                    last_store->node->last_child->next = child;
                    last_store->node->last_child = child;
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
