#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include "ir.h"
#include "tokens.h"
#include "graph.h"

//externs
extern int setup_scanner(char* filename);
extern int32_t parse(IR** list, token* cur_tok, uint32_t* n_ops);
extern int rename_registers(IR* head, uint32_t n_ops, uint32_t* maxVR, uint32_t* maxlive_ptr);


/*
 * Prints out all functionality of this program in a useful manner.
 * Program should terminate afterwards.
 *
 * Requires: nothing.
 * Returns: nothing.
*/
void h(){
    printf("Help for 412alloc (412 Register Allocator):\n");
    printf("Command syntax:\n");
    printf("./412alloc [optional flags] <filename>\n");
    printf("\n");
    printf("\t-h: Displays this help text. Ignores any file given.\n");
}



/*
 * Conducts the scheduling of an ILOC code.
 * If the file does not contain a valid ILOC program, then prints all errors found to stderr.
 * If the file has a valid ILOC program, it schedules the program to work units.
 * 
 * Requires: 
 *      char* filename: the name of the file containing the ILOC program. must be non null.
 * 
 * Returns: 
 *      0 on success
 *      -1 on failure
*/
int schedule(char* filename){
    clock_t start = clock();

    //allocate
    token* tok = malloc(sizeof(token));
    if(tok == NULL) return -1;
        
    IR** list = malloc(sizeof(IR*));
    if(list == NULL) return -1;

    uint32_t* n_ops = malloc(sizeof(uint32_t));
    if(n_ops == NULL) return -1;

    uint32_t* maxVR = malloc(sizeof(uint32_t));
    if(maxVR == NULL) return -1;

    uint32_t* maxlive = malloc(sizeof(uint32_t));
    if(maxlive == NULL) return -1;

    //setup
    if(setup_scanner(filename)) return -1;

    //parse
    if(parse(list, tok, n_ops)) return -1;
    IR* head = *list; //guaranteed to be non null

    //rename
    if(rename_registers(head, *n_ops, maxVR, maxlive)) return -1;

    //print IR
    // print_IR_List(head, VR);

    //build dependency
    NodeList* graph = build_dependency_graph(head, *maxVR, *n_ops);
    if(graph == NULL) return -1;

    clock_t end = clock();

    //print dependency  
    print_graph(graph);

    // printf("length: %f\n", ((float) (end - start)) / CLOCKS_PER_SEC);
}


/*
 * Driver function for code. 
 * Currently looks for either a -h flag or a filename.
 * -h flag prints help function.
 * filename invokes the schedule function
 * Prints the help function if neither are found.
 * 
 * Requires: 
 *      int argc: the number of arguments in argv.
 *      char* argv[]: the arguments passed to the schedule executable.
 * 
 * Returns: 
 *      0 on success
 *      -1 on failure
*/
int main(int argc, char* argv[]){
    //flags
    uint8_t h_flag = 0;

    //check arguments
    for(int i = 1; i < argc; i += 2){
        char* word = argv[i];
        if(word[0] == '-'){
            if(strchr(word, 'h')){
                h_flag = i;
            }
        } 
        break;
    }

    //process flags
    int code = 0;
    if(h_flag){
        h();
    } 
    else if(argc > 0){
        schedule(argv[1]);
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