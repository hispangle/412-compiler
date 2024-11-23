#include <stdio.h>
#include "ir.h"

/*
 * Prints an individual line of ILOC code. Does not print new line afterwards.
 * The type of value printed depends on type.
 * 
 * Requires: IR* ir, the line to be printed. must be non null.
 *           Type type, the type of register to print.
 * Returns: nothing.
*/
void print_IR(IR* ir, Type type){
    // get the right value to print
    uint32_t val1;
    uint32_t val2;
    uint32_t val3;
    switch(type){
        case SR:
            val1 = ir->arg1.SR;
            val2 = ir->arg2.SR;
            val3 = ir->arg3.SR;
            break;
        case VR:
            val1 = ir->arg1.VR;
            val2 = ir->arg2.VR;
            val3 = ir->arg3.VR;
            break;
        case PR:
            val1 = ir->arg1.PR;
            val2 = ir->arg2.PR;
            val3 = ir->arg3.PR;
            break;
    }

    // print based on opcode
    switch(ir->opcode){
        case load:
        case store:
            printf("%s ", TOKEN_NAMES[ir->opcode]);
            printf("r%i => r%i", val1, val3);
            break;
        case loadI:
            printf("%s ", TOKEN_NAMES[ir->opcode]);
            printf("%i => r%i", val1, val3);
            break;
        case add:
        case sub:
        case mult:
        case lshift:
        case rshift:
            printf("%s ", TOKEN_NAMES[ir->opcode]);
            printf("r%i, r%i => r%i", val1, val2, val3);
            break;
        case output:
            printf("%s ", TOKEN_NAMES[ir->opcode]);
            printf("%i", val1);
            break;
        case nop:
            printf("nop");
            break;
        default:
            ;
    }

    return;
}

/*
 * Prints the sequence of ILOC commands given at head. The
 * information printed depends on the type given. 
 * 
 * Requires: IR* head, the head of the linked list of the IR representation. Must be non null.
 *           Type type, the kind of register to be printed.
 * Returns: Nothing.
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