#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "constants.h"


//global current class and char
static uint8_t class;
static char cur_char;

//global file (and buffer later)
static FILE* file;


//max of constants
static const uint32_t ONE = 1; //prevents overflow
static const uint32_t MAX_CONST = (ONE << 31) - 1;


//char class identifiers
//just for readability
//COMMA matches position in TOKEN_TYPES
enum classes{
    INVALID = 0,
    FSLASH,
    //COMMA = 2, //COMMA excluded, included already in token_types
    //EoF = 3 //EoF excluded, already in token_types
    NL = 4,
    CF,
    WHITESPACE,
    EQUALS,
    GT,
    NUMBER //NUMBER *must* be the last one for initialization to work
};


//dfa table
//outputs a state
//nstates x nclass
//ASSUMES NUMBER is last explicit type, a-z and I get individual classes\
//                                INV, '/', ',', '\n', '\r', WS, '=', '>', #, I, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z
int delta_char[50][NUMBER + 28];


//lookup array for chars
//defaults to 0, therefore 0 is an error/everything else class
//ASSUMES ASCII
uint8_t* CHAR_CLASS;

//number conversion
uint8_t ctoi[256];

//simple function
//CAN BE OPTIMIZED INTO DOUBLE BUFFER
char get_next_char(){
    return fgetc(file);
}

//moves to the next '\n'
void get_next_line(){
    while((cur_char = get_next_char()) != '\n'){
    }
}


//gets the next token
//right now is a dowhile for terminating cases like ,
//changed to normal while and go backwards 1 spot if unoptimal
struct token get_next_token(){
    uint8_t state = 1; //default is 1, 0 is error
    uint32_t num = 0;

    //follows table rep of dfa
    //needs to build numbers
    do{
        //builds number
        //if slow, pulled out for 1 if every token instead of an if every character
        //or build number every character if thats good enough
        //make sure size check is correct
        if(class == NUMBER){
            if((MAX_CONST - ctoi[cur_char]) / 10 < num){
                printf("Overflow!\n");
                break;
            }
            num = num * 10;
            num += ctoi[cur_char];
        }

        state = delta_char[state][class];
        cur_char = get_next_char();
        class = CHAR_CLASS[cur_char];
        //printf("class: %i, cur_char: %i, state: %i, num: %i\n", class, cur_char, state, num);
    } while (state < 47 && class > 6); //not a self terminating state (like , or // or an error) and next char not a terminating character (like , or \t) or an error


    //follow until whitespace is done
    while(CHAR_CLASS[cur_char] == WHITESPACE){
        cur_char = get_next_char();
    }

    //follow until new line if comment or error
    //change number to correct one with proper table
    //can be different with a double buffer or line reader
    if(state == 69 || state == 420){
        get_next_line();
    }

    //finish getting class
    class = CHAR_CLASS[cur_char];

    //TODO: rework tok making
    //classify based on state
    uint8_t type;
    uint32_t name;

    switch(state){
        case 0: //error
            type = ERROR;
            name = spelling;
            break;
        case 6: //store
            type = MEMOP;
            name = store;
            break;
        case 8:
            type = ARITHOP;
            name = sub;
            break;
        case 25:
            type = REGISTER;
            name = num;
            break;
        case 26:
            type = CONSTANT;
            name = num;
            break;
        case 47:
            type = EOL;
            name = eol;
            break;
        case 51: //eof
            type = EoF;
            name = eof;
            break;
        default: 
            type = MEMOP;
            name = load;
    }
    struct token tok = {type, name};
    return tok;
}


//does any initialization
//CAN BE MOVED IF NECESSARY
int setup(char* filename){
    //initialize CHAR_CLASS
    {
        //give space to array
        CHAR_CLASS = malloc(sizeof(uint8_t) * 257);

        //deal with EOF
        CHAR_CLASS[0] = EoF;

        //move over 1 spot
        CHAR_CLASS = CHAR_CLASS + 1;

        //numbers map to number
        for(uint8_t num = '0'; num <= '9'; num ++){
            CHAR_CLASS[num] = NUMBER;
            char temp[] = {num, '\0'};
            ctoi[num] = atoi(temp);
        }

        //capital I for loadI operation
        CHAR_CLASS['I'] = NUMBER + 1;

        //letters map to their own class
        //CAN BE MADE BETTER
        for(uint8_t letter = 'a'; letter <= 'z'; letter ++){
            CHAR_CLASS[letter] = NUMBER + 2 + letter - 'a'; //+2 instead of +1 because of I (really (NUMBER + 1) + 1 + (letter - 'a'))
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

    //open file
    file = fopen(filename, "r");
    if(file == NULL){
        printf("file %s unable to be opened.\n");
        return -1;
    }

    //initialize character states
    cur_char = get_next_char(); //here bc my function is a do while
    class = CHAR_CLASS[cur_char]; //also here bc do while

    //create dfa table
    //manual for early testing
    //automatic later
    {
        //err   0
        //all error

        //0     1
        delta_char[1][NUMBER + 2 + 's' - 'a'] = 2; //s
        delta_char[1][NUMBER + 2 + 'l' - 'a'] = 9; //l
        delta_char[1][NUMBER + 2 + 'r' - 'a'] = 19; //r
        delta_char[1][NUMBER] = 26; //#
        delta_char[1][NUMBER + 2 + 'm' - 'a'] = 27; //m
        delta_char[1][NUMBER + 2 + 'a' - 'a'] = 31; //a
        delta_char[1][NUMBER + 2 + 'n' - 'a'] = 34; //n
        delta_char[1][NUMBER + 2 + 'o' - 'a'] = 37; //o
        delta_char[1][WHITESPACE] = 43; //WHITESPACE
        delta_char[1][FSLASH] = 44; ///
        delta_char[1][EQUALS] = 45; //=
        delta_char[1][CF] = 46; //\r
        delta_char[1][NL] = 47; //\n
        delta_char[1][COMMA] = 50; //,
        delta_char[1][EoF] = 51; //EoF

        //s     2
        delta_char[2][NUMBER + 2 + 't' - 'a'] = 3;
        delta_char[3][NUMBER + 2 + 'u' - 'a'] = 6;
        
        //st    3
        delta_char[3][NUMBER + 2 + 'o' - 'a'] = 4;

        //sto   4
        delta_char[4][NUMBER + 2 + 'r' - 'a'] = 5;

        //stor  5
        delta_char[5][NUMBER + 2 + 'e' - 'a'] = 6; //store

        //store 6
        //all error
        
        //su    7
        delta_char[7][NUMBER + 2 + 'b' - 'a'] = 8; //sub

        //sub   8
        //all error

        //l     9
        delta_char[9][NUMBER + 2 + 'o' - 'a'] = 10; //lo
        delta_char[9][NUMBER + 2 + 's' - 'a'] = 14; //ls
        
        //lo    10
        delta_char[10][NUMBER + 2 + 'a' - 'a'] = 11; //loa

        //loa   11
        delta_char[11][NUMBER + 2 + 'd' - 'a'] = 12; //load

        //load  12
        delta_char[12][NUMBER + 1] = 13; //loadI

        //loadI 13
        //all error

        //ls    14
        delta_char[14][NUMBER + 2 + 'h' - 'a'] = 15; //lsh

        //lsh   15
        delta_char[15][NUMBER + 2 + 'i' - 'a'] = 16; //lshi

        //lshi  16
        delta_char[16][NUMBER + 2 + 'f' - 'a'] = 17; //lshif

        //lshif 17
        delta_char[17][NUMBER + 2 + 'h' - 'a'] = 18; //lshift

        //lshift18
        //all errors

        //r     19
        delta_char[19][NUMBER + 2 + 's' - 'a'] = 20; //rs
        delta_char[19][NUMBER] = 25; //r#

        //rs    20
        delta_char[20][NUMBER + 2 + 'h' - 'a'] = 21; //rsh

        //rsh   21
        delta_char[21][NUMBER + 2 + 'i' - 'a'] = 22; //rshi

        //rshi  22
        delta_char[22][NUMBER + 2 + 'f' - 'a'] = 23; //rshif

        //rshif 23
        delta_char[23][NUMBER + 2 + 't' - 'a'] = 24; //rshift

        //rshift24
        //all errors

        //r#    25
        delta_char[25][NUMBER] = 25; //r#

        //#     26
        delta_char[26][NUMBER] = 26; //#

        //m     27
        delta_char[27][NUMBER + 2 + 'u' - 'a'] = 28; //mu

        //mu    28
        delta_char[28][NUMBER + 2 + 'l' - 'a'] = 29; //mul

        //mul   29
        delta_char[29][NUMBER + 2 + 't' - 'a'] = 30; //mult

        //mult  30
        //all errors

        //a     31
        delta_char[31][NUMBER + 2 + 'd' - 'a'] = 32; //ad

        //ad    32
        delta_char[32][NUMBER + 2 + 'd' - 'a'] = 33; //add

        //add   33
        //all errors

        //n     34
        delta_char[34][NUMBER + 2 + 'o' - 'a'] = 35; //no

        //no    35
        delta_char[35][NUMBER + 2 + 'p' - 'a'] = 36; //nop

        //nop   36
        //all errors

        //o     37
        delta_char[37][NUMBER + 2 + 'u' - 'a'] = 38; //ou

        //ou    38
        delta_char[38][NUMBER + 2 + 't' - 'a'] = 39; //out

        //out   39
        delta_char[39][NUMBER + 2 + 'p' - 'a'] = 40; //outp

        //outp  40
        delta_char[40][NUMBER + 2 + 'u' - 'a'] = 41; //outpu

        //outpu 41
        delta_char[41][NUMBER + 2 + 't' - 'a'] = 42; //output

        //output42
        //all errors

        //WS    43
        //white space

        ///     44
        delta_char[44][FSLASH] = 48;

        //=     45
        delta_char[45][EQUALS] = 49;

        //\r    46
        delta_char[46][NL] = 47;

        //\n    47
        //terminator

        ////    48
        //terminator

        // =>   49
        //terminator

        //,     50
        //terminator

        //eof   51
        //terminator
    }

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
        //tok type needs to be checked for number name (CONST or REG)
        if(tok.type != CONSTANT && tok.type != REGISTER){
            printf("type: %i or %s, name: %i or %s\n\n", tok.type, TOKEN_TYPES[tok.type], tok.name, TOKEN_NAMES[tok.type][tok.name]);
        } else{
            printf("type: %i or %s, name: %i\n\n", tok.type, TOKEN_TYPES[tok.type], tok.name);
        }
        
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