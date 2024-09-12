#include <stdint.h>

//constants or structs to be used in multiple files
//can also be thought of as translations from int to string (factorizing)

//need to make sure no possibility of out of bounds / segfault

//MAKE MORE DESCRIPTIVE ERRORS

//how tokens are structured
struct token {
    uint8_t type; //anything in token_types, but declared as uint8_t for size
    uint32_t name; //anything in token_names (or constant of size below 2^31)
};

//how arguments are stored
struct argument {
    uint32_t SR;
    uint32_t VR;
    uint32_t PR;
    uint32_t NU;
};

//IR
struct IR {
    //required fields
    uint8_t opcode;
    struct argument arg1;
    struct argument arg2;
    struct argument arg3;

    //linked list
    struct IR* next;
    struct IR* prev;
};



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
