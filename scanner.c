#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "constants.h"



//global current char
static char cur_char;
static FILE* file;


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

//number conversion
uint8_t ctoi[255];

//simple function
//CAN BE OPTIMIZED INTO DOUBLE BUFFER
char get_next_char(){
    return fgetc(file);
}

struct token get_next_token(){
    uint8_t class = 0;
    uint8_t state = 0;
    uint32_t num = 0;

    //follows table rep of dfa
    //needs to build numbers
    do{
        class = CHAR_CLASS[cur_char];

        //builds number
        //if slow, pulled out for 1 if every token instead of an if every character
        //or build number every character if thats good enough
        //make sure size check is correct
        if(class == NUMBER){
            num = num * 10;
            num += ctoi[cur_char];
        }

        state = delta_char[state][class];
        printf("class: %i, cur_char: %c, state: %i, num: %i\n", class, cur_char, state, num);
        cur_char = get_next_char();
    } while (state < 2); //not ',', '=>', whitespace, '//', EOL, EOF


    //follow until whitespace is done
    while(CHAR_CLASS[cur_char] == WHITESPACE){
        cur_char = get_next_char();
    }

    //follow until comment is done
    //change number to correct one with proper table
    //can be different with a double buffer or line reader
    if(state == 69){
        while(cur_char != '\n'){
            cur_char = get_next_char();
        }
    }

    //TODO: rework tok making
    struct token tok = {state, eof};
    return tok;
}


//does any initialization
//CAN BE MOVED IF NECESSARY
int setup(char* filename){
    //numbers map to number
    for(uint8_t num = '0'; num <= '9'; num ++){
        CHAR_CLASS[num] = NUMBER;
        char temp[] = {num, '\0'};
        ctoi[num] = atoi(temp);
        printf("num: %c, val: %i\n", num, ctoi[num]);
    }

    //capital I for loadI operation
    CHAR_CLASS['I'] = NUMBER + 1;

    //letters map to their own class
    //CAN BE MADE BETTER
    for(uint8_t letter = 'a'; letter <= 'z'; letter ++){
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

    //open file
    file = fopen(filename, "r");
    if(file == NULL){
        printf("file %s unable to be opened.\n");
        return -1;
    }

    cur_char = get_next_char(); //here bc my function is a do while
    return 0;
}

//function that displays help for commandline args
void help(){
    printf("help!\n");
}

//function that displays the internal representation of the program
int rep(char* filename){
    printf("rep!\n");

    int code;
    if(code = setup(filename)){
        return code;
    }

    return 0;
}

//function that scans the program and displays tokens
int scan(char* filename){
    printf("scan!\n");
    int code;
    if(code = setup(filename)){
        return code;
    }

    return 0;
}

//function that parses the program, builds the IR, and reports success or failure. The default
int parse(char* filename){
    printf("parse!\n");
    int code;
    if(code = setup(filename)){
        return code;
    }

    printf("setup finished!\n");
    struct token tok = get_next_token();
    while(tok.type != EoF){
        printf("type: %i or %s, name: %i or %s", tok.type, TOKEN_TYPES[tok.type], tok.name, TOKEN_NAMES[tok.name]);
        tok = get_next_token();
    }

    return 0;
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
    int code = 0;
    if(h){
        help();
    }
    else if(r && r < (argc - 1)){
        code = rep(argv[r + 1]);
    }
    else if(p && p < (argc - 1)){
        code = parse(argv[p + 1]);
    }
    else if(s && s < (argc - 1)){
        code = scan(argv[s + 1]);
    }
    else if(argc == 2){
        code = parse(argv[1]);
    }
    else{
        printf("Bad Arguments!\n");
        help();
    }

    return code;
}