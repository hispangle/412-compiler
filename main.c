#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "constants.h"

//externs
extern int setup_scanner(char* filename);
extern token get_next_token();
extern void setup_parser(IR* head);
extern int32_t parse(IR* head, token* cur_tok, uint32_t* n_ops);
extern int rename_registers(uint32_t n_ops, IR* head, uint32_t* maxVR, uint32_t* maxlive_ptr);

//enums
typedef enum {
    SR,
    VR,
    PR
} Type;


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


void print_IR(IR* head, Type type){
    uint32_t val1;
    uint32_t val2;
    uint32_t val3;

    IR* node = head->next;
    while(node != head){
        switch(type){
            case SR:
                val1 = node->arg1.SR;
                val2 = node->arg2.SR;
                val3 = node->arg3.SR;
                break;
            case VR:
                val1 = node->arg1.VR;
                val2 = node->arg2.VR;
                val3 = node->arg3.VR;
                break;
            case PR:
                val1 = val1;
                val2 = val2;
                val3 = val3;
                break;
        }
        switch(node->opcode){
            case load:
            case store:
                printf("%s ", TOKEN_NAMES[node->opcode]);
                printf("r%i => r%i", val1, val3);
                break;
            case loadI:
                printf("%s ", TOKEN_NAMES[node->opcode]);
                printf("%i => r%i", val1, val3);
                break;
            case add:
            case sub:
            case mult:
            case lshift:
            case rshift:
                printf("%s ", TOKEN_NAMES[node->opcode]);
                printf("r%i, r%i => r%i", val1, val2, val3);
                break;
            case output:
                printf("%s ", TOKEN_NAMES[node->opcode]);
                printf("%i", val1);
                break;
            case nop:
            printf("nop");
                break;
            default:
                ;
        }
        printf("\n");
        node = node->next;
    }
    
}


//function that displays help for commandline args
void h(){
    printf("Help for 412alloc (412 Register Allocator):\n");
    printf("Command syntax:\n");
    printf("./412alloc [optional flags] <filename>\n");
    printf("\n");
    printf("\t-h: Displays this help text. Ignores any file given.\n");
}

int schedule(char* filename){
    //allocate
    token* tok = malloc(sizeof(token));
    if(tok == NULL) return -1;
        
    IR* head = malloc(sizeof(IR));
    if(head == NULL) return -1;

    uint32_t* n_ops = malloc(sizeof(uint32_t));
    if(n_ops == NULL) return -1;

    uint32_t* maxVR = malloc(sizeof(uint32_t));
    if(maxVR == NULL) return -1;

    uint32_t* maxlive = malloc(sizeof(uint32_t));
    if(maxlive == NULL) return -1;

    //setup
    if(setup_scanner(filename)) return -1;
    setup_parser(head);

    //parse
    if(parse(head, tok, n_ops)) return -1;

    //rename
    if(rename_registers(*n_ops, head, maxVR, maxlive)) return -1;

    //schedule
    print_IR(head, VR);
}





//TODO: clean up parse to not have an extern setup
//TODO: clean up scan and parse code (mainly variables and global things)
//TODO: determine if passing in head is better than global (ehhhhhhh)
int main(int argc, char* argv[]){
    //flags
    uint8_t h_flag = 0;
    uint8_t schedule_flag = 0;

    //check arguments
    for(int i = 1; i < argc; i += 2){
        char* word = argv[i];
        if(word[0] == '-'){
            if(strchr(word, 'h')){
                h_flag = i;
                break;
            }
        } else if(!strcmp(word, "schedule")){
            schedule_flag = i;
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
    } else if(schedule_flag && schedule_flag < argc - 1){
        code = schedule(argv[schedule_flag + 1]);
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