#ifndef CONSTANTS_H
#define CONSTANTS_H
#include <stdint.h>
//constants or structs to be used in multiple files

//how tokens are structured
typedef struct {
    uint8_t type; //anything in token_types, but declared as uint8_t for size
    uint32_t name; //anything in token_names (or constant of size below 2^31)
} token;

//how arguments are stored
typedef struct {
    uint32_t SR;
    uint32_t VR;
    uint32_t PR;
    uint32_t NU;
    uint8_t isLU;
} argument;

//typedef for compilation
typedef struct _IR IR;

//IR
struct _IR {
    //required fields
    uint8_t opcode;
    argument arg1;
    argument arg2;
    argument arg3;

    //linked list
    IR* next;
    IR* prev;
};


extern const char* TOKEN_NAMES[];
// //token type
// //matches with TOKEN_NAMES
// const char* TOKEN_TYPES[] = {
//     "MEMOP",
//     "LOADI",
//     "COMMA",
//     "EOF",
//     "ARITHOP",
//     "OUTPUT",
//     "NOP",
//     "CONSTANT",
//     "REGISTER",
//     "INTO",
//     "EOL",
//     "ERROR"
// };

//enumerates index of each type
//just for readability
enum token_types{
    MEMOP = 0,
    LOADI,
    COMMA,
    EoF, //compiler did not like EOF
    ARITHOP,
    OUTPUT,
    NOP,
    CONSTANT,
    REGISTER,
    INTO,
    EOL,
    ERROR
};


extern const char* TOKEN_TYPES[];
// //lexeme of tokens
// //matches TOKEN_TYPES order
// const char* TOKEN_NAMES[] = {
//     "load", "store",
//     "loadI",
//     ",",
//     "eof",
//     "add", "sub", "mult", "lshift", "rshift",
//     "output",
//     "nop",
//     "0",
//     "0",
//     "=>",
//     "\\n",
//     "Invalid spelling", "Overflow/Above Constant Limit", "Invalid Op", "Invalid Sentence"
// };

//enumerates the index of each word
//just for readability
//matches with the list above
enum token_names{
    load,
    store,
    loadI,
    comma,
    eof,
    add,
    sub,
    mult,
    lshift,
    rshift,
    output,
    nop,
    constant,
    reg, //register not liked
    into,
    eol,
    spelling,
    overflow,
    invalid_op,
    invalid_sentence
};

#endif