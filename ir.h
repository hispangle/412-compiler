#ifndef IR_H
#define IR_H
#include <stdint.h>
#include "tokens.h"

//typedef for compilation
typedef struct IR IR;
typedef enum Type Type;

//function declarations
void print_IR(IR* ir, Type type);
void print_IR_List(IR* head, Type type);

//enums
enum Type {
    SR,
    VR,
    PR
};

//how arguments are stored
typedef struct {
    uint32_t SR;
    uint32_t VR;
    uint32_t PR;
    uint32_t NU;
    uint8_t isLU;
} argument;


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

#endif