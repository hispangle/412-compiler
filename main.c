#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "constants.h"

//externs
extern int setup_scanner(char* filename);
extern int setup_parser(struct token* filename);
extern struct token get_next_token();
extern int32_t parse();

//globals
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

//lexeme of tokens
//matches TOKEN_TYPES order
const char* TOKEN_NAMES[] = {
    "load", "store",
    "loadI",
    ",",
    "eof",
    "add", "sub", "mult", "lshift", "rshift",
    "output",
    "nop",
    "0",
    "0",
    "=>",
    "\\n",
    "Invalid spelling", "Overflow/Above Constant Limit", "Invalid Op", "Invalid Sentence"
};


//prints token
void print_token(struct token tok){
    //tok type needs to be checked for number name (CONST or REG)
    if(tok.type != CONSTANT && tok.type != REGISTER){
        printf("type: %i or %s, name: %i or %s\n", tok.type, TOKEN_TYPES[tok.type], tok.name, TOKEN_NAMES[tok.name]);
    } else{
        printf("type: %i or %s, name: %i\n", tok.type, TOKEN_TYPES[tok.type], tok.name);
    }
}


//function that displays help for commandline args
void h(){
    printf("help!\n");
}

//function that displays the internal representation of the program
int r(char* filename){
    printf("rep!\n");
    
    //create tok_pointer
    struct token* tok = malloc(sizeof(struct token));

    int code;
    if((code = setup_scanner(filename))){
        return code;
    }

    //setup parser
    if((code = setup_parser(tok))){
        return code;
    }

    return 0;
}

//function that scans the program and displays tokens
int s(char* filename){
    printf("scan!\n");
    int code;
    if((code = setup_scanner(filename))){
        return code;
    }

    printf("setup finished!\n");

    struct token tok = get_next_token();
    while(tok.type != EoF){
        //tok type needs to be checked for number name (CONST or REG)
        print_token(tok);
        tok = get_next_token();
    }

    print_token(tok);

    return 0;
}

//function that parses the program, builds the IR, and reports success or failure. The default
int p(char* filename){
    //create tok_pointer
    struct token* tok = malloc(sizeof(struct token));

    int code;
    //set up scanner
    if((code = setup_scanner(filename))){
        return code;
    }

    //setup parser
    if((code = setup_parser(tok))){
        return code;
    }

    //parse
    int32_t n_ops;
    if((n_ops = parse()) != -1){
        printf("Parse succeeded. Processed %i operations.\n", n_ops);
        // printf("IR:\n");
        // print_IR();
    }

    return 0;
}




int main(int argc, char* argv[]){
    //flags
    uint8_t h_flag = 0;
    uint8_t s_flag = 0;
    uint8_t r_flag = 0;
    uint8_t p_flag = 0;

    //check arguments
    for(int i = 1; i < argc; i += 2){
        char* word = argv[i];
        if(!strcmp(word, "-h")){
            h_flag = i;
            break;
        }
        else if(!strcmp(word, "-s")){
            s_flag = i;
        }
        else if(!strcmp(word, "-p")){
            p_flag = i;
        }
        else if(!strcmp(word, "-r")){
            r_flag = i;
        }
        else if(i == 1 && argc == 2){
            break;
        }
        else{
            printf("Bad arguments!\n");
            h_flag = 1;
            break;
        }
    }

    //process flags
    int code = 0;
    if(h_flag){
        h();
    }
    else if(r_flag && r_flag < (argc - 1)){
        code = r(argv[r_flag + 1]);
    }
    else if(p_flag && p_flag < (argc - 1)){
        code = p(argv[p_flag + 1]);
    }
    else if(s_flag && s_flag < (argc - 1)){
        code = s(argv[s_flag + 1]);
    }
    else if(argc == 2){
        code = p(argv[1]);
    }
    else{
        printf("Bad Arguments!\n");
        h();
    }

    return code;
}