#ifndef IR_H
#define IR_H
#include <stdint.h>
#include "tokens.h"

//typedef declarations
typedef struct argument argument;
typedef struct IR IR;
typedef enum Type Type;

//function declarations
static inline void print_IR(IR* ir, Type type);
void print_IR_List(IR* head, Type type);

//enums
enum Type {
    SR,
    VR,
    PR
};

//how arguments are stored
struct argument {
    uint32_t SR;
    uint32_t VR;
    uint32_t PR;
    uint32_t NU;
    uint8_t isLU;
};


//IR
struct IR {
    //linked list
    IR* next;
    IR* prev;

    //required fields
    uint8_t opcode;
    argument arg1;
    argument arg2;
    argument arg3;
};


/*
 * Prints an individual line of ILOC code. Does not print new line afterwards.
 * The type of value printed depends on type.
 * 
 * Requires: 
 *      IR* ir: the line to be printed. must be non null.
 *      Type type: the type of register to print.
 * 
 * Returns: 
 *      nothing.
*/
static inline void print_IR(IR* ir, Type type){
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

#endif