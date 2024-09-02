#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "constants.c"



//global current char
char cur_char = 'Z';


//char class identifiers
//just for readability
//COMMA matches position in TOKEN_TYPES
enum classes{
    ERROR = 0,
    FSLASH = 1,
    //COMMA = 2, //COMMA excluded, included already in token_types
    NL = 3,
    CF = 4,
    WHITESPACE = 5,
    EQUALS = 6,
    GT = 7,
    NUMBER = 8 //NUMBER *must* be the last one for initialization to work
};


//dfa table
//nstates x nclass
const int delta_char[][NUMBER + 28] = {{40}};


//lookup array for chars
//defaults to 0, therefore 0 is an error/everything else class
//ASSUMES ASCII
uint8_t CHAR_CLASS[255];


char get_next_char(){
    return 'C';
}

struct token get_next_token(){
    uint8_t class = 0;
    uint8_t state = 0;
    uint32_t num = 0;
    do{
        class = CHAR_CLASS[cur_char];
        state = delta_char[state][class];
        cur_char = get_next_char();
    } while (state < 39);

    printf("%i\n", CHAR_CLASS['0']);

    struct token tok = {0, 0};
    return tok;
}


//does any initialization
//CAN BE MOVED TO MAINS
void setup(){
    //numbers map to number
    for(uint8_t num = '0'; num < '9'; num ++){
        CHAR_CLASS[num] = NUMBER;
    }

    //capital I for loadI operation
    CHAR_CLASS['I'] = NUMBER + 1;

    //letters map to their own class
    //CAN BE MADE BETTER
    for(uint8_t letter = 'a'; letter < 'z'; letter ++){
        CHAR_CLASS[letter] = NUMBER + 2 + letter - 'a'; //+2 instead of +1 because of I
    }

    //special mappings
    CHAR_CLASS['/'] = FSLASH;
    CHAR_CLASS[','] = COMMA;
    CHAR_CLASS['\n'] = NL;
    CHAR_CLASS['\r'] = CF;
    CHAR_CLASS['\t'] = WHITESPACE;
    CHAR_CLASS[' '] = WHITESPACE;
    CHAR_CLASS['='] = EQUALS;
    CHAR_CLASS['>'] = GT;
}

int main(int argc, char* argv[]){

    for(int i = 1; i < argc; i++){
        char* word = argv[i];
        if(strcmp(word, "-h")){
            printf("help, %s", word);
        }
    }



    setup();





    get_next_token();
    return 0;
}