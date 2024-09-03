//constants or structs to be used in multiple files
//change to header file


//how tokens are structured
struct token {
    uint8_t type; //anything in token_types, but declared as uint8_t for size
    uint32_t name; //anything in token_names (or constant of size below 2^31)
};





//token type
//matches with TOKEN_NAMES
const char* TOKEN_TYPES[] = {
    "MEMOP",
    "LOADI",
    "COMMA",
    "EOF",
    "ARITHOP",
    "OUTPUT",
    "NOP",
    "CONSTANT",
    "REGISTER",
    "INTO",
    "EOL",
    "ERROR"
};

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



//lexeme of tokens
//matches TOKEN_TYPES order
const char* TOKEN_NAMES[][5] = {
    {"load", "store"},
    {"loadI"},
    {","},
    {""},
    {"add", "sub", "mult", "lshift", "rshift"},
    {"output"},
    {"nop"},
    {},
    {},
    {"=>"},
    {"\\n"},
    {"Invalid spelling", "Overflow/Above Constant Limit"}
};

//enumerates the index of each word
//just for readability
//matches with the list above
enum token_names{
    load = 0,
    store,
    loadI = 0,
    comma = 0,
    eof = 0,
    add = 0,
    sub,
    mult,
    lshift,
    rshift,
    output = 0,
    nop = 0,
    into = 0,
    eol = 0,
    spelling = 0,
    overflow
};