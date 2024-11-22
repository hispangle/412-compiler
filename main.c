#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include "constants.h"
#include "graph.h"

//externs
extern int setup_scanner(char* filename);
extern token get_next_token();
extern int32_t parse(IR** list, token* cur_tok, uint32_t* n_ops);
extern int rename_registers(uint32_t n_ops, IR* head, uint32_t* maxVR, uint32_t* maxlive_ptr);
extern NodeList* build_dependency_graph(IR* head, uint32_t maxVR);

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


/*
 * Prints an individual line of ILOC code. Does not print new line afterwards.
 * The type of value printed depends on type.
 * 
 * Requires: IR* ir, the line to be printed. must be non null.
 *           Type type, the type of register to print.
 * Returns: nothing.
*/
void print_IR(IR* ir, Type type){
    // get the right value to print
        uint32_t val1;
        uint32_t val2;
        uint32_t val3;
        switch(type){
            case SR:
                val1 = ir->arg1.SR;
                val2 = ir->arg2.SR;
                val3 = ir->arg3.SR;
                break;
            case VR:
                val1 = ir->arg1.VR;
                val2 = ir->arg2.VR;
                val3 = ir->arg3.VR;
                break;
            case PR:
                val1 = ir->arg1.PR;
                val2 = ir->arg2.PR;
                val3 = ir->arg3.PR;
                break;
        }

        // print based on opcode
        switch(ir->opcode){
            case load:
            case store:
                printf("%s ", TOKEN_NAMES[ir->opcode]);
                printf("r%i => r%i", val1, val3);
                break;
            case loadI:
                printf("%s ", TOKEN_NAMES[ir->opcode]);
                printf("%i => r%i", val1, val3);
                break;
            case add:
            case sub:
            case mult:
            case lshift:
            case rshift:
                printf("%s ", TOKEN_NAMES[ir->opcode]);
                printf("r%i, r%i => r%i", val1, val2, val3);
                break;
            case output:
                printf("%s ", TOKEN_NAMES[ir->opcode]);
                printf("%i", val1);
                break;
            case nop:
            printf("nop");
                break;
            default:
                ;
        }
}

/*
 * Prints the sequence of ILOC commands given at head. The
 * information printed depends on the type given. 
 * 
 * Requires: IR* head, the head of the linked list of the IR representation. Must be non null.
 *           Type type, the kind of register to be printed.
 * Returns: Nothing.
*/
void print_IR_List(IR* head, Type type){
    IR* node = head->next;
    while(node != head){
        print_IR(node, type);
        printf("\n");
        node = node->next;
    } 
}

/*
 * Prints all the graph edges between nodes and their children.
 * Then recursively prints edges from children and their children.
 * Changes node->complete to false for each node.
 * 
 * Requires: NodeList* nodes, the list of nodes whos edges to print. Must be non null.
 * Returns: Nothing.
*/
void print_graph_edges(NodeList* nodes){
    //print nodes
    NodeList* node = nodes->next;
    while(node != nodes){
        IR* op = node->node->op;

        //skip if node has been printed
        if(!node->node->complete){
            node = node->next;
            continue;
        }

        char* edge_types[3] = {"Data", "Serial", "Conflict"};

        //print edge with children
        Child* child = (Child*) node->node->children->next;
        while(child != node->node->children){
            if(child->edge == def){
                printf("\t%i->%i [label = \"%s VR%i\"];\n", node->node->num, child->node->num, edge_types[child->edge], child->register_cause);
            } else {
                printf("\t%i->%i [label = \"%s\"];\n", node->node->num, child->node->num, edge_types[child->edge]);
            }
            child = child->next;
        }

        //update print completeness
        node->node->complete = false;

        //print children
        
        print_graph_edges((NodeList*) (node->node->children));
        
        node = node->next;
    }
}

/*
 * Prints all the graph nodes in the list of nodes, in a format readable by graphviz.
 * Also prints the children of each node recursively.
 * Sets node->successful to true for each node printed.
 * 
 * Requires: NodeList* nodes, a list of nodes to print. Must be non null.
 * Returns: nothing.
*/
void print_graph_nodes(NodeList* nodes){
    //cycle thru nodes in the list
    NodeList* node = nodes->next;
    while(node != nodes){
        IR* op = node->node->op;

        //skip if node has been printed
        if(node->node->complete){
            node = node->next;
            continue;
        }

        //print node
        printf("\t%i [label=\"%i: ", node->node->num, node->node->num);
        print_IR(op, VR);
        printf("\"];\n");

        //indicate completeness of print
        node->node->complete = true;

        //print children
        print_graph_nodes((NodeList*) (node->node->children));

        node = node->next;
    }
}

/*
 * Prints the dependency graph given by the leaves in dot format, readable by graphviz.
 * Sets node->num for each node in the dependency graph.
 * 
 * Requires: NodeList* leaves, the list of leaves of the dependency graph. Must be non null.
 * Returns: nothing
*/
void print_graph(NodeList* leaves){
    printf("digraph DG{\n");
    print_graph_nodes(leaves);
    print_graph_edges(leaves);
    printf("}\n");
}

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
 * Requires: char* filename, the name of the file containing the ILOC program. must be non null.
 * Returns: -1 on failure
 *           0 on success
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
    if(rename_registers(*n_ops, head, maxVR, maxlive)) return -1;

    //print IR
    // print_IR_List(head, VR);

    //build dependency
    NodeList* leaves = build_dependency_graph(head, *maxVR);
    if(leaves == NULL) return -1;

    clock_t end = clock();

    //print dependency  
    print_graph(leaves);

    printf("length: %f\n", ((float) (end - start)) / CLOCKS_PER_SEC);
}


/*
 * Driver function for code. 
 * Currently looks for either a -h flag or a filename.
 * -h flag prints help function.
 * filename invokes the schedule function
 * Prints the help function if neither are found.
 * 
 * Requires: int argc, the number of arguments in argv.
 *           char* argv[], the arguments passed to the schedule executable.
 * 
 * Returns: 0 on success
 *          -1 on failure
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