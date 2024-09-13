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
extern struct IR* head;

//line num for s
uint32_t nline = 0;

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


//function that displays help for commandline args
void h(){
    printf("help!\n");
}

//function that displays the internal representation of the program
int r(char* filename){
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

    if(parse() == -1){
        return -1;
    }

    //print IR
    struct IR* ir = head->next;
    while(ir != head){
        switch(ir->opcode){
            case load:
            case store:
            case loadI:
                printf("%s\t [ sr %i ], [  ], [ sr %i ]\n", TOKEN_NAMES[ir->opcode], ir->arg1.SR, ir->arg3.SR);
                break;
            case output:
                printf("%s\t [ sr %i ], [  ], [  ]\n", TOKEN_NAMES[ir->opcode], ir->arg1.SR);
                break;
            case nop:
                printf("%s\t [  ], [  ], [  ]\n", TOKEN_NAMES[ir->opcode]);
                break;
            default:
                printf("%s\t [ sr %i ], [ sr %i ], [ sr %i ]\n", TOKEN_NAMES[ir->opcode], ir->arg1.SR, ir->arg2.SR, ir->arg3.SR);
                break;
        }

        ir = ir->next;
    }

    return 0;
}

//function that scans the program and displays tokens
int s(char* filename){
    int code;
    if((code = setup_scanner(filename))){
        return code;
    }

    //print tokens
    struct token tok;
    while(tok.type != EoF){
        //tok type needs to be checked for number name (CONST or REG)
        //printf("new tok\n");
        //printf("print! d\n");
        tok = get_next_token();
        
        //tok type needs to be checked for number name (CONST or REG)
        if(tok.type != CONSTANT && tok.type != REGISTER){
            printf("%i: < %s, \"%s\" >\n", nline, TOKEN_TYPES[tok.type], TOKEN_NAMES[tok.name]);
        } else{
            printf("%i: < %s, \"%s%i\" >\n", nline, TOKEN_TYPES[tok.type], tok.type == CONSTANT ? "" : "r", tok.name);
        }

        //increment line number
        if(tok.type == EOL){
            nline += 1;
        }
    }

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
        if(word[0] == '-'){
            if(strchr(word, 'h')){
                h_flag = i;
                break;
            }
            if(strchr(word, 's')){
                s_flag = i;
            }
            if(strchr(word, 'p')){
                p_flag = i;
            }
            if(strchr(word, 'r')){
                r_flag = i;
            }
        }
        else if(i == 1 && argc == 2){
            break;
        }
        else{
            fprintf(stderr, "Bad arguments!\n");
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
        fprintf(stderr, "ERROR: Bad Arguments!\n");
        h();
    }

    return code;
}