#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "constants.c"



//global current char
char cur_char;


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
//outputs a state
//nstates x nclass
//ASSUMES NUMBER is last explicit type, a-z and I get individual classes
const int delta_char[][NUMBER + 28] = {{1}, {EoF}};


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
        printf("class: %i, cur_char: %c, state: %i\n", class, cur_char, state);
        cur_char = get_next_char();
    } while (state < 2);

    struct token tok = {state, eof};
    return tok;
}


//does any initialization
//CAN BE MOVED IF NECESSARY
void setup(char* filename){
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

//function that displays help for commandline args
void help(){
    printf("help!\n");
}

//function that displays the internal representation of the program
void rep(char* filename){
    printf("rep!\n");
    setup(filename);
}

//function that scans the program and displays tokens
void scan(char* filename){
    printf("scan!\n");
    setup(filename);   
}

//function that parses the program, builds the IR, and reports success or failure. The default
void parse(char* filename){
    printf("parse!\n");
    setup(filename);

    printf("setup finished!\n");
    struct token tok = get_next_token();
    while(tok.type != EoF){
        printf("type: %i or %s, name: %i or %s", tok.type, TOKEN_TYPES[tok.type], tok.name, TOKEN_NAMES[tok.name]);
        tok = get_next_token();
    }
}

int main(int argc, char* argv[]){
    //flags
    uint8_t h = 0;
    uint8_t s = 0;
    uint8_t r = 0;
    uint8_t p = 0;

    //check arguments
    for(int i = 1; i < argc; i += 2){
        char* word = argv[i];
        if(!strcmp(word, "-h")){
            h = i;
            break;
        }
        else if(!strcmp(word, "-s")){
            s = i;
        }
        else if(!strcmp(word, "-p")){
            p = i;
        }
        else if(!strcmp(word, "-r")){
            r = i;
        }
        else if(i == 1 && argc == 2){
            break;
        }
        else{
            printf("Bad arguments!\n");
            h = 1;
            break;
        }
    }

    //process flags
    if(h){
        help();
    }
    else if(r && r < (argc - 1)){
        rep(argv[r + 1]);
    }
    else if(p && p < (argc - 1)){
        parse(argv[p + 1]);
    }
    else if(s && s < (argc - 1)){
        scan(argv[s + 1]);
    }
    else if(argc == 2){
        parse(argv[1]);
    }
    else{
        printf("Bad Arguments!\n");
        help();
    }

    return 0;
}