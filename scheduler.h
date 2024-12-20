#ifndef SCHEDULER_H
#define SCHEDULER_H
#include "graph.h"

//function declarations
int calc_heuristics(NodeList* graph);
void select_nodes(NodeList* ready, uint32_t n_ready, Node* nop_node);
int scheduler(NodeList* graph);
#endif



