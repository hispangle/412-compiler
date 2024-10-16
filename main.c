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
extern int rename_registers(int32_t n_ops, uint32_t* maxvr, uint32_t* maxlive_ptr);
extern int allocate(int k, int32_t n_ops);
extern struct IR* head;


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
    printf("Help for 412alloc (412 Register Allocator):\n");
    printf("Command syntax:\n");
    printf("./412alloc [optional flags] <filename>\n");
    printf("\n");
    printf("\t<filename>: The file where the ILOC program is stored. 412fe will print an error if file cannot be accessed.\n");
    printf("\n");
    printf("Optional Flags:\n");
    printf("\tFlags are displayed in order of priority. Only one flag will be processed.\n");
    printf("\n");
    printf("\t-h: Displays this help text. Ignores any file given.\n");
    printf("\n");
    printf("\t-x: Parses the ILOC program given at filename, and displays the same program with virtual registers instead of given registers. \n\t\tPrints errors to stderr on failure.\n");
    printf("\n");
    printf("\t-r: Parses the ILOC program given at filename, and displays the Intermediate Representation. \n\t\tPrints errors to stderr upon error discovery.\n");
    printf("\n");
    printf("\t-p: Parses the ILOC program given at filename. Displays number of operations parsed on success. Prints errors to stderr on failure.\n");
    printf("\n");
    printf("\t-s: Scans the ILOC program given at filename. Prints all tokens found.\n");
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

    //hold line num
    uint32_t nline = 1;

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




int x(char* filename){
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
    int32_t n_ops = parse();
    if(n_ops == -1){
        return -1;
    }

    //rename registers
    uint32_t maxvr = 0;
    uint32_t maxlive = 0;
    if(rename_registers(n_ops, &maxvr, &maxlive) == -1){
        return -1;
    }
    
    //print valid ILOC
    struct IR* node = head->next;
    while(node != head){
        printf("%s ", TOKEN_NAMES[node->opcode]);

        switch(node->opcode){
            case load:
            case store:
                printf("r%i => r%i", node->arg1.VR, node->arg3.VR);
                break;
            case loadI:
                printf("%i => r%i", node->arg1.VR, node->arg3.VR);
                break;
            case add:
            case sub:
            case mult:
            case lshift:
            case rshift:
                printf("r%i, r%i => r%i", node->arg1.VR, node->arg2.VR, node->arg3.VR);
                break;
            case output:
                printf("%i", node->arg1.VR);
                break;
            case nop:
                break;
            default:
                ;
        }
        printf("\n");
        node = node->next;
    }


    return 0;
}


int k(uint32_t k_reg, char* filename){
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
    int32_t n_ops = parse();
    if(n_ops == -1){
        return -1;
    }

    if(allocate(k_reg, n_ops) == -1){
        return -1;
    }

    //print valid ILOC
    struct IR* node = head->next;
    while(node != head){
        printf("%s ", TOKEN_NAMES[node->opcode]);

        switch(node->opcode){
            case load:
            case store:
                printf("r%i => r%i", node->arg1.PR, node->arg3.PR);
                break;
            case loadI:
                printf("%i => r%i", node->arg1.PR, node->arg3.PR);
                break;
            case add:
            case sub:
            case mult:
            case lshift:
            case rshift:
                printf("r%i, r%i => r%i", node->arg1.PR, node->arg2.PR, node->arg3.PR);
                break;
            case output:
                printf("%i", node->arg1.PR);
                break;
            case nop:
                break;
            default:
                ;
        }
        printf("\n");
        node = node->next;
    }

}

//TODO: clean up parse to not have an extern setup
//TODO: clean up scan and parse code (mainly variables and global things)
//TODO: determine if passing in head is better than global (ehhhhhhh)
int main(int argc, char* argv[]){
    //flags
    uint8_t h_flag = 0;
    uint8_t s_flag = 0;
    uint8_t r_flag = 0;
    uint8_t p_flag = 0;
    uint8_t x_flag = 0;
    uint8_t k_flag = 0;

    //k
    uint32_t k_reg = 0;


    //check arguments
    for(int i = 1; i < argc; i += 2){
        char* word = argv[i];
        if(word[0] == '-'){
            if(strchr(word, 'x')){
                x_flag = i;
            }
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
            if(!k_flag){
                k_reg = atoi(word);
                if(k_reg >= 3 && k_reg <= 64){
                    k_flag = i;
                    continue;
                }
            }



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
    else if(k_flag && k_flag < (argc - 1)){
        code = k(k_reg, argv[k_flag + 1]);
    }
    else if(x_flag && x_flag < (argc - 1)){
        code = x(argv[x_flag + 1]);
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

























        // switch(node->opcode){
        //     case load:
        //     case store:
        //     case loadI:
        //     case add:
        //     case sub:
        //     case mult:
        //     case lshift:
        //     case rshift:
        //     case output:
        //     case nop:
        //     default:
        //         break;
        // }