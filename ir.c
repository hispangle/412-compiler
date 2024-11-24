#include <stdio.h>
#include "ir.h"

/*
 * Prints the sequence of ILOC commands given at head. The
 * information printed depends on the type given. 
 * 
 * Requires: 
 *      IR* head: the head of the linked list of the IR representation. Must be non null.
 *      Type type: the kind of register to be printed.
 * 
 * Returns: 
 *      Nothing.
*/
void print_IR_List(IR* head, Type type){
    IR* node = head->next;
    while(node != head){
        print_IR(node, type);
        printf("\n");
        node = node->next;
    } 
    return;
}