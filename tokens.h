#ifndef TOKENS_H
#define TOKENS_H
#include <stdint.h>

//typedef definitions
typedef struct token token;

//function definitions
token get_next_token(void);
token get_next_eol_token(void);

//how tokens are structured
struct token {
    uint8_t type; //anything in token_types, but declared as uint8_t for size
    uint32_t name; //anything in token_names (or constant of size below 2^31)
};

//enumerates index of each type
enum token_types{
    MEMOP,
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

//lexeme of tokens
const static char* TOKEN_NAMES[] = {
    "load", "store",
    "loadI",
    ",",
    "",
    "add", "sub", "mult", "lshift", "rshift",
    "output",
    "nop",
    "0",
    "0",
    "=>",
    "\\n",
    "Invalid spelling", "Overflow/Above Constant Limit", "Invalid Op", "Invalid Sentence"
};


//enumerates the index of each word
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