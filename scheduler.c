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
Node* new_node(){
    //malloc node
    Node* node = malloc(sizeof(Node));
    if(node == NULL) return NULL;
    
    //malloc child
    NodeList* child = malloc(sizeof(NodeList));
    if(child == NULL) return NULL;
    
    //set children
    node->first_child = child;
    node->last_child = child;

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
    NodeList* output_list = malloc(sizeof(NodeList)); //dummy head
    if(output_list == NULL) return NULL;

    //current last output
    output_list->node = NULL; //make this explicit as a preventative measure against bugs
    NodeList* last_output = output_list;

    //keep track of n_outputs
    uint32_t n_outputs = 0;


    //keep the most recent store
    Node* last_store = NULL;


    //create a VR to DEF map
    Node** VRtoDef = malloc(sizeof(Node*) * (maxVR + 1));
    if(VRtoDef == NULL) return NULL;


    //loop thru operations first to last
    IR* op = head->next;
    while(op != head){
        //skip nops
        if(op->opcode == nop){
            op = op->next;
            continue;
        }

        //make new node
        Node* node = new_node();
        if(node == NULL) return NULL;
        node->op = op;

        //collect VRs
        uint32_t VR_1 = op->arg1.VR;
        uint32_t VR_2 = op->arg2.VR;
        uint32_t VR_3 = op->arg3.VR;

        //definitions
        Node* parent_1;
        Node* parent_2;
        NodeList* child;

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
                    child = malloc(sizeof(NodeList));
                    if(child == NULL) return NULL;
                    child->node = node;

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
                node->last_store = last_store;
                if(last_store == NULL){
                    node->n_parents--;
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
                    child;
                    child = malloc(sizeof(NodeList));
                    if(child == NULL) return NULL;
                    child->node = node;

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
                    child = malloc(sizeof(NodeList));
                    if(child == NULL) return NULL;
                    child->node = node;

                    //extend child list
                    parent_2->last_child->next = child;
                    parent_2->last_child = child;
                // }

                // //undefed use
                // if(parent_2 == NULL){
                //     node->n_parents--;
                // }


                //memory dependencies
                node->last_store = last_store;
                if(last_store == NULL){
                    node->n_parents--;
                }

                //connect all loads
                node->all_loads = load_list;
                node->n_loads = n_loads;

                //add node to children list of all loads
                NodeList* load_node = load_list->next;
                for(int i = 0; i < n_loads; i++){
                    child = malloc(sizeof(NodeList));
                    if(child == NULL) return NULL;
                    child->node = node;

                    load_node->node->last_child->next = child;
                    load_node->node->last_child = child;
                    load_node = load_node->next;
                }

                //connect all outputs
                node->all_outputs = output_list;
                node->n_outputs = n_outputs;

                //add node to children list of all outputs
                NodeList* output_node = output_list->next;
                for(int i = 0; i < n_outputs; i++){
                    child = malloc(sizeof(NodeList));
                    if(child == NULL) return NULL;
                    child->node = node;

                    output_node->node->last_child->next = child;
                    output_node->node->last_child = child;
                    output_node = output_node->next;
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
                    child = malloc(sizeof(NodeList));
                    if(child == NULL) return NULL;
                    child->node = node;

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
                    child = malloc(sizeof(NodeList));
                    if(child == NULL) return NULL;
                    child->node = node;

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
                node->last_store = last_store;
                if(last_store == NULL){
                    node->n_parents--;
                }

                node->last_output = last_output->node;
                if(last_output->node == NULL){
                    node->n_parents--;
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
                last_output = next_output;
                n_outputs++;
        
                break;
            default:
                break;
        }

        op = op->next;
    }

    return leaves;
}
