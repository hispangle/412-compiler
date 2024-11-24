#include <stdlib.h>
#include <stdio.h>
#include "graph.h"
#include "list.h"


/*
*/
int early_release(NodeList* ready, uint32_t* n_ready, Node* node){
    Child* child = node->children->next;
    for(uint32_t i = 0; i < node->n_children; i++){
        //check for serialization edge
        if(child->edge == serial){
            //increase number of satisfied parents
            child->node->n_ready++;

            //if all parents satisfied, release child to ready set
            if(child->node->n_ready == child->node->n_parents){
                if(add_node_to_list(child->node, ready)) return -1;
                (*n_ready)++;
            }
        }
        child = child->next;
    }

    return 0;
}

/*
*/
int increase_cycle(NodeList* ready, uint32_t* n_ready, NodeList* active_set, uint8_t* n_active){
    //decrease remaining cycles
        uint8_t n_completed = 0;
        NodeList* active = active_set->next;
        for(uint8_t i = 0; i < *n_active; i++){
            Node* node = active->node;
            node->remaining_cycles--;

            //if node is complete update children
            if(node->remaining_cycles == 0){
                Child* child = node->children->next;
                for(uint32_t j = 0; j < node->n_children; j++){
                    //if serial edge ignore (already released with early release)
                    if(child->edge == serial){
                        child = child->next;
                        continue;
                    }

                    //increase number of parents satisfied
                    child->node->n_ready++;
                    
                    //if all parents satisfied, release child to ready set
                    if(child->node->n_ready == child->node->n_parents){
                        if(add_node_to_list(child->node, ready)) return -1;
                        (*n_ready)++;
                    }

                    child = child->next;
                }
                //keep count of completed operations, to reduce n_active after loop
                remove_from_list((List*) active);
                n_completed++;
            }
            active = active->next;
        }

        //reduce n_active_0
        *n_active -= n_completed;
        return 0;
}



/*
 * Selects the two best nodes from the ready set for execution. 
 * Removes the two nodes from the ready set. The first node must be
 * able to execute on functional unit 0 (the load/store unit). The
 * second node must be able to execute on functional unit 1 (the mult
 * unit). If there is no first or second node, inserts a NOP node for
 * execution.
 * Uses precalculated heuristics for execution.
 * 
 * Requires: 
 *      NodeList* ready: the list of nodes in the ready set. Must be non null. Must be circularly doubly linked.
 *      uint32_t n_ready: the number of nodes in the ready set.
 *      Node* nop_node: the node that will be inserted as NOP. 
 * 
 * Returns: 
 *      Node**, the list of 2 Nodes to be executed on success. Malloced. 
 *      NULL on failure.
*/
void select_nodes(Node** selection, NodeList* ready, uint32_t n_ready, Node* nop_node){
    //define nodes and heuristic
    NodeList* first = NULL;
    NodeList* second = NULL;

    uint32_t first_heuristic = 0;
    uint32_t second_heuristic = 0;
    uint32_t current_heuristic = 0;

    //look at each node in ready set
    Node* node;
    NodeList* node_list = ready->next;
    for(uint32_t i = 0; i < n_ready; i++){
        node = node_list->node;
        current_heuristic = node->heuristic;

        //select based on possible units
        switch(node->unit){
            case ZERO:
                if(current_heuristic > first_heuristic){
                    first_heuristic = current_heuristic;
                    first = node_list;
                }
                break;
            
            case ONE:
                if(current_heuristic > second_heuristic){
                    second_heuristic = current_heuristic;
                    second = node_list;
                }
                break;

            case BOTH:
                //compare with the min heuristic
                if(first_heuristic > second_heuristic){
                    if(current_heuristic > second_heuristic){
                        second_heuristic = current_heuristic;
                        second = node_list;
                    }
                } else {
                    if(current_heuristic > first_heuristic){
                        first_heuristic = current_heuristic;
                        first = node_list;
                    }
                }
        }

        node_list = node_list->next;
    }

    //remove selected nodes from ready set
    if(first != NULL) remove_from_list((List*) first);
    if(second != NULL) remove_from_list((List*) second);

    //select the mult node if second is NULL and unit is BOTH
    if(second == NULL && first != NULL){
        if(first->node->unit == BOTH){
            second = first;
            first = NULL;
        }
    }

    //replace nulls with nops and place in selection
    selection[0] = first == NULL ? nop_node : first->node;
    selection[1] = second == NULL ? nop_node : second->node;

    return;
}


/*
 * Schedules the lines given by the nodes in the graph to two functional
 * units. Unit 0 is the only unit that can run load and store. Unit 1
 * is the only unit that can run mult.
 * Prints the schedule as a 
 * [ILOC; ILOC] 
 * pair.
 * Modifies the nodes.
 * Respects the dependencies given by the graph.
 * 
 * Requires:
 *      NodeList* graph: the list of nodes that represents the graph.
 * 
 * Returns:
 *      0 on success
 *      -1 on failure
*/
int scheduler(NodeList* graph){
    //create the ready set
    NodeList* ready = new_list();
    if(ready == NULL) return -1;
    uint32_t n_ready = 0;

    //initialize ready set
    NodeList* item = graph->next;
    while(item != graph){
        //add item to ready set if no parents exist
        if(!item->node->n_parents){
            if(add_node_to_list(item->node, ready)) return -1;
            n_ready++;
        }

        item = item->next;
    }

    //active sets
    NodeList* active_0 = new_list();
    if(active_0 == NULL) return -1;

    NodeList* active_1 = new_list();
    if(active_1 == NULL) return -1;

    uint8_t n_active_0 = 0;
    uint8_t n_active_1 = 0;

    //selection space
    Node** selected = malloc(2 * sizeof(Node*));
    if(selected == NULL) return -1;

    //make singular NOP node
    IR* nop_line = malloc(sizeof(IR));
    if(nop_line == NULL) return -1;
    nop_line->opcode = nop;

    //Node must be calloced (or at least latency set so it doesnt accidentally pass the latency check)
    Node* nop_node = calloc(1, sizeof(Node));
    if(nop_node == NULL) return -1;
    nop_node->op = nop_line;

    //main loop, iterates while ready set and active set exist
    while(n_ready + n_active_0 + n_active_1){
        //select nodes to print
        select_nodes(selected, ready, n_ready, nop_node);

        //activate nodes
        if(selected[0] != nop_node){
            if(add_node_to_list(selected[0], active_0)) return -1;
            n_active_0++;
            n_ready--;
        }

        if(selected[1] != nop_node){
            if(add_node_to_list(selected[1], active_1)) return -1;
            n_active_1++;
            n_ready--;
        }
        
        //print scheduled cycle
        printf("[");
        print_IR(selected[0]->op, VR);
        printf("; ");
        print_IR(selected[1]->op, VR);
        printf("]\n");

        //cycle
        if(increase_cycle(ready, &n_ready, active_0, &n_active_0)) return -1;
        if(increase_cycle(ready, &n_ready, active_1, &n_active_1)) return -1;

        //early release
        if(selected[0]->releasable){
            if(early_release(ready, &n_ready, selected[0])) return -1;
        }

        if(selected[1]->releasable){
            if(early_release(ready, &n_ready, selected[1])) return -1;
        }
    }

    return 0;
}