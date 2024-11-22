#include <stdlib.h>
#include "graph.h"
#include "list.h"


/*
 * Selects the two best nodes from the ready set for execution. 
 * Removes the two nodes from the ready set. The first node must be
 * able to execute on functional unit 0 (the load/store unit). The
 * second node must be able to execute on functional unit 1 (the mult
 * unit). If there is no first or second node, inserts a NOP node for
 * execution.
 * Uses precalculated heuristics for execution.
 * 
 * Requires: NodeList* ready, the list of nodes in the ready set. Must be non null. Must be doubly linked.
 *           uint32_t n_ready, the number of nodes in the ready set.
 *           Node* nop_node, the node that will be inserted as NOP. 
 * Returns: Node**, the list of 2 Nodes to be executed. Malloced. 
 *          NULL on failure.
*/
Node** select_nodes(NodeList* ready, uint32_t n_ready, Node* nop_node){
    //create selection array
    Node** selection = malloc(2 * sizeof(Node*));
    if(selection == NULL) return NULL;


    //define nodes and heuristic
    Node* first;
    Node* second;

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
                    first = node;
                }
                break;
            
            case ONE:
                if(current_heuristic > second_heuristic){
                    second_heuristic = current_heuristic;
                    second = node;
                }
                break;

            case BOTH:
                //compare with the min heuristic
                if(first_heuristic > second_heuristic){
                    if(current_heuristic > second_heuristic){
                        second_heuristic = current_heuristic;
                        second = node;
                    }
                } else {
                    if(current_heuristic > first_heuristic){
                        first_heuristic = current_heuristic;
                        first = node;
                    }
                }
        }

        node_list = node_list->next;
    }


    //remove selected nodes from ready set
    remove_doubly((List*) first);
    remove_doubly((List*) second);
    

    //select the mult node if second is NULL and unit is BOTH
    if(second == NULL && first != NULL && first->unit == BOTH){
        second = first;
        first = nop_node;
    }

    //replace nulls with noops and place in selection
    *selection = first == NULL ? nop_node : first;
    *(selection + 1) = second == NULL ? nop_node : second;

    return selection;
}