#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "constants.h"

//need to make sure no possibility of out of bounds / segfault
//HANDLE WHITESPACE AT START OF THING (done)
//put class finding in get_char and get_line?

//have flag if eol already inserted before eof //prevents inf loop
uint8_t eol_inserted = 0;

//global token pointer
//struct token* cur_tok;

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
//COMMA and EoF match positions in TOKEN_TYPES
enum classes{
    OVERFLOW = 47,
    INVALID = 0,
    FSLASH,
    //COMMA = 2,
    //EoF = 3,
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
//ASSUMES NUMBER is last explicit type, a-z and I get individual classes
//OTHER, '/', ',', '\n', '\r', WS, '=', '>', #, I, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z
uint8_t delta_char[53][NUMBER + 28];


//lookup array for chars
//defaults to 0, therefore 0 is an error/everything else class
//ASSUMES ASCII
//malloced in setup, maybe initialize as array
uint8_t* CHAR_CLASS;


//simple function
//CAN BE OPTIMIZED INTO DOUBLE BUFFER
char get_next_char(){
    char c = fgetc(file);
    class = CHAR_CLASS[c];
    return c;
}

//moves to the next '\n'
void get_next_line(){
    while(cur_char != '\n' && cur_char != -1){
        cur_char = get_next_char();
    }
}

//skips all tokens until EOL
//useful for long lines
//maybe insert eol befor eof
//lightly tested
struct token get_next_eol_token(){
    get_next_line();
    cur_char = get_next_char();

    //follow until whitespace is done
    while(class == WHITESPACE){
        cur_char = get_next_char();
    }
    struct token tok = {EOL, eol};
    return tok;
}



//gets the next token
//ignores further characters in a line if there is an error
//right now is a dowhile
//changed to normal while and go backwards 1 spot if unoptimal
//return nothing and change contents of cur_tok if inefficient
struct token get_next_token(){
    uint8_t state = 1; //default is 1, 0 is error
    uint32_t num = 0;

    //follows table rep of dfa
    //needs to build numbers
    do{
        //printf("class: %i, cur_char: %c, state: %i, num: %i\n", class, cur_char, state, num);
        //builds number
        //if slow, pulled out for 1 if every token instead of an if every character
        //pull out 'r' as well
        //or build number every character if thats good enough
        //make sure size check is correct
        //make sure error is displayed correctly (size not spelling)
        if(class == NUMBER){
            if((MAX_CONST - (cur_char - '0')) / 10 < num){ //can be optimized with an array if needed
                state = OVERFLOW;
                break;
            }
            num = num * 10;
            num += (cur_char - '0');
        }

        state = delta_char[state][class];
        cur_char = get_next_char();
        //class = CHAR_CLASS[cur_char];
    } while (state < 47 && class > 7); //not a self terminating state (like , or // or an error) 
    //and next char not a terminating character (like , or \t) or an error


    //get next char if comment
    if(state == 44){
        state = delta_char[state][class];
        cur_char = get_next_char();
        if(state == 49){
            get_next_line();
            cur_char = get_next_char();
        }
    }

    //follow until new line or eof if comment, error, or overflow
    //return here
    //make sure to get next char and class after
    //change number to correct one with proper table
    //can be different with a double buffer or line reader
    if(state == 0 || state == 47){
        get_next_line();
    }

    //follow until whitespace is done
    while(class == WHITESPACE){
        cur_char = get_next_char();
    }

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
        case 8: //sub
            type = ARITHOP;
            name = sub;
            break;
        case 12: //load
            type = MEMOP;
            name = load;
            break;
        case 13: //loadI
            type = LOADI;
            name = loadI;
            break;
        case 18: //lshift
            type = ARITHOP;
            name = lshift;
            break;
        case 24: //rshift
            type = ARITHOP;
            name = rshift;
            break;
        case 25: //register
            type = REGISTER;
            name = num;
            break;
        case 26: //constant
            type = CONSTANT;
            name = num;
            break;
        case 30: //mult
            type = ARITHOP;
            name = mult;
            break;
        case 33: //add
            type = ARITHOP;
            name = add;
            break;
        case 36: //nop
            type = NOP;
            name = nop;
            break;
        case 42: //output
            type = OUTPUT;
            name = output;
            break;
        case 47: //overflow
            type = ERROR;
            name = overflow;
            break;
        case 48: //eol
        case 49: ////
            type = EOL;
            name = eol;
            break;
        case 50: //into
            type = INTO;
            name = into;
            break;
        case 51: //comma
            type = COMMA;
            name = comma;
            break;
        case 52: //eof
            //insert 1 eol before eof
            if(eol_inserted){
                type = EoF;
                name = eof;
            } else{
                type = EOL;
                name = eol;
                eol_inserted = 1;
            }
            
            break;
        default: 
            type = ERROR;
            name = spelling;
            get_next_line();
    }
    
    struct token tok = {type, name};
    return tok;
}



//does any initialization
//CAN BE MOVED IF NECESSARY
int setup_scanner(char* filename){
    //initialize CHAR_CLASS
    //maybe do array pointer stuff instead of malloc if slow?
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
        }

        //capital I for loadI operation
        CHAR_CLASS['I'] = NUMBER + 1;

        //letters map to their own class
        //CAN BE MADE BETTER
        for(uint8_t letter = 'a'; letter <= 'z'; letter ++){
            CHAR_CLASS[letter] = NUMBER + 2 + letter - 'a'; 
            //+2 instead of +1 because of I 
            //(really (NUMBER + 1) + 1 + (letter - 'a'))
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
        fprintf(stderr, "ERROR: file %s unable to be opened.\n", filename);
        return -1;
    }

    //initialize character states
    cur_char = get_next_char(); //here bc my function is a do while
    while(class == WHITESPACE){//ignore any initial whitespace
        cur_char = get_next_char();
    }
    //class = CHAR_CLASS[cur_char]; //also here bc do while

    //create dfa table
    //assumes whitespace after op necessary
    //if no assumption, endings are self terminating states
    //load and loadI special cases, maybe think about it idk its not necessary
    //bc d is not necessarily self terminating
    //I similar to /?
    //d self terminating in all cases except I
    //special I class?
    //not thinking about it more 
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
        delta_char[1][NL] = 48; //\n
        delta_char[1][COMMA] = 51; //,
        delta_char[1][EoF] = 52; //EoF

        //s     2
        delta_char[2][NUMBER + 2 + 't' - 'a'] = 3; //st
        delta_char[2][NUMBER + 2 + 'u' - 'a'] = 7; //su
        
        //st    3
        delta_char[3][NUMBER + 2 + 'o' - 'a'] = 4; //sto

        //sto   4
        delta_char[4][NUMBER + 2 + 'r' - 'a'] = 5; //stor

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
        delta_char[17][NUMBER + 2 + 't' - 'a'] = 18; //lshift

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
        delta_char[44][FSLASH] = 49; ////

        //=     45
        delta_char[45][GT] = 50; //=>

        //\r    46
        delta_char[46][NL] = 48; //\r\n

        //OVERFLOW  47
        delta_char[47][NUMBER] = 47; //OVERFLOW

        //\n    48
        //terminator

        ////    49
        //terminator

        // =>   50
        //terminator

        //,     51
        //terminator

        //eof   52
        //terminator
    }

    return 0;
}