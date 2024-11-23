#ifndef SCHEDULER_H
#define SCHEDULER_H
#include "graph.h"

//function declarations
Node** select_nodes(NodeList* ready, uint32_t n_ready, Node* nop_node);

#endif



