#include <stdlib.h>
#include <stdio.h>
#include "constants.h"

//change tok to global pointer in scanner
//get_tok changes contents but not pointer
//allows use of same pointer in parser

//i am able to completely decouple parser and scanner, but it is not particularly useful
//could have setup parser take pointer for tok
//could also have it take in functions for get_next_token, etc
//not really useful, parser is not going to have a different scanner

//groups of linked lists
//allocate heads, 
//then full groups as needed

//allocate space for IR at setup and in larger chunks after (exponential i think) (1k, 2k, 4k, 8k, etc)

//externs
extern struct token get_next_token();
extern struct token get_next_eol_token();
extern const char* TOKEN_NAMES[];

//global token pointer but declared
struct token* cur_tok;

//keep track of line number and op num
uint32_t line_num = 1;
int32_t op_num = 0;

//flag for first error
uint8_t err_found = 0;
uint8_t new_err = 0;

//keep track of head and last IR
struct IR* head;
struct IR* current;

//gets the next spot for IR
//mallocs space if none left
//for now is just malloc
struct IR* get_next_IR_loc(){
    return malloc(sizeof(struct IR));
}

//adds IR to linked list
void add_IR(struct IR* ir){
    ir->prev = current;
    ir->next = head;
    current->next = ir;
    current = ir;
}

//prints errors
//can be made inline
void print_error(uint32_t name){
    //new error found
    new_err = 1;

    //test if parse found errors at start or end
    //maybe not necessary if stderr
    if(!err_found){
        printf("Parse found errors.\n");
        err_found = 1;
    }
    fprintf(stderr, "ERROR %i: \t%s\n", line_num, TOKEN_NAMES[name]);
}

//creates the internal representation
//returns num operations
//or -1 on error
int32_t parse(){
    //get first token
    *cur_tok = get_next_token();
    struct IR* ir;
    //normal loop
    while(cur_tok->type != EoF && !err_found){
        switch(cur_tok->type){
            case MEMOP:
                ir = get_next_IR_loc();
                ir->opcode = cur_tok->name;
                switch((*cur_tok = get_next_token()).type){
                    case REGISTER:
                        ir->arg1.SR = cur_tok->name;
                        
                        switch((*cur_tok = get_next_token()).type){
                            case INTO:
                                switch((*cur_tok = get_next_token()).type){
                                    case REGISTER:
                                        ir->arg3.SR = cur_tok->name;
                                        switch((*cur_tok = get_next_token()).type){
                                            case EOL:
                                                add_IR(ir);
                                                break;
                                            case ERROR:
                                                print_error(cur_tok->name);
                                            default:
                                                print_error(invalid_sentence);
                                        }
                                        break;
                                    case ERROR:
                                        print_error(cur_tok->name);
                                    default:
                                        print_error(invalid_sentence);
                                }
                                break;
                            
                            case ERROR:
                                print_error(cur_tok->name);
                            default:
                                print_error(invalid_sentence);
                        }


                        break;

                    case ERROR:
                        print_error(cur_tok->name);
                    default:
                        print_error(invalid_sentence);
                }
                op_num += 1;
                break;
            case LOADI:
                ir = get_next_IR_loc();
                ir->opcode = cur_tok->name;
                switch((*cur_tok = get_next_token()).type){
                    case CONSTANT:
                        ir->arg1.SR = cur_tok->name;
                        switch((*cur_tok = get_next_token()).type){
                            case INTO:
                                switch((*cur_tok = get_next_token()).type){
                                    case REGISTER:
                                        ir->arg3.SR = cur_tok->name;
                                        switch((*cur_tok = get_next_token()).type){
                                            case EOL:
                                                add_IR(ir);
                                                break;
                                            case ERROR:
                                                print_error(cur_tok->name);
                                            default:
                                                print_error(invalid_sentence);
                                        }
                                        break;
                                    case ERROR:
                                        print_error(cur_tok->name);
                                    default:
                                        print_error(invalid_sentence);
                                }
                                break;
                            
                            case ERROR:
                                print_error(cur_tok->name);
                            default:
                                print_error(invalid_sentence);
                        }


                        break;

                    case ERROR:
                        print_error(cur_tok->name);
                    default:
                        print_error(invalid_sentence);
                }

                op_num += 1;
                break;
            case ARITHOP:
                ir = get_next_IR_loc();
                ir->opcode = cur_tok->name;
                switch((*cur_tok = get_next_token()).type){
                    case REGISTER:
                        ir->arg1.SR = cur_tok->name;
                        switch((*cur_tok = get_next_token()).type){
                            case COMMA:
                                switch((*cur_tok = get_next_token()).type){
                                    case REGISTER:
                                        ir->arg2.SR = cur_tok->name;
                                        switch((*cur_tok = get_next_token()).type){
                                            case INTO:
                                                switch((*cur_tok = get_next_token()).type){
                                                    case REGISTER:
                                                        ir->arg3.SR = cur_tok->name;
                                                        switch((*cur_tok = get_next_token()).type){
                                                            case EOL:
                                                                add_IR(ir);
                                                                break;
                                                            case ERROR:
                                                                print_error(cur_tok->name);
                                                            default:
                                                                print_error(invalid_sentence);
                                                        }
                                                        break;
                                                    case ERROR:
                                                        print_error(cur_tok->name);
                                                    default:
                                                        print_error(invalid_sentence);

                                                }
                                                
                                                break;
                                            case ERROR:
                                                print_error(cur_tok->name);
                                            default:
                                                print_error(invalid_sentence);
                                        }
                                        break;
                                    case ERROR:
                                        print_error(cur_tok->name);
                                    default:
                                        print_error(invalid_sentence);
                                }
                                break;
                            
                            case ERROR:
                                print_error(cur_tok->name);
                            default:
                                print_error(invalid_sentence);
                        }


                        break;

                    case ERROR:
                        print_error(cur_tok->name);
                    default:
                        print_error(invalid_sentence);
                }
                op_num += 1;
                break;
            case OUTPUT:
                ir = get_next_IR_loc();
                ir->opcode = cur_tok->name;
                switch((*cur_tok = get_next_token()).type){
                    case CONSTANT:
                        ir->arg1.SR = cur_tok->name;
                        switch((*cur_tok = get_next_token()).type){
                            case EOL:
                                add_IR(ir);
                                break;
                            case ERROR:
                                print_error(cur_tok->name);
                            default:
                                print_error(invalid_sentence);
                        }
                        break;
                    case ERROR:
                        print_error(cur_tok->name);
                    default:
                        print_error(invalid_sentence);
                }     
                        
                op_num += 1;
                break;
            case NOP:
                ir = get_next_IR_loc();
                ir->opcode = nop;
                switch((*cur_tok = get_next_token()).type){
                    case EOL:
                        //add IR
                        add_IR(ir);
                        //printf("%i\n", nop);
                        //printf("opcode: %i, sr1: %i, sr2: %i, sr3: %i\n", ir->opcode, ir->arg1.SR, ir->arg2.SR, ir->arg3.SR);
                        break;
                    case ERROR:
                        print_error(cur_tok->name);
                    default:
                        print_error(invalid_sentence);
                }
                op_num += 1;
                
                break;
            case EOL:
                break;
            case ERROR:
                print_error(cur_tok->name);
            default:
                //must be not an op
                print_error(invalid_op);
        }

        //go to new line if any errors were detected
        if(new_err){
            *cur_tok = get_next_eol_token();
            new_err = 0;
            continue;
        }

        line_num += 1;
        *cur_tok = get_next_token();
    }


    //error loop
    while(cur_tok->type != EoF){
        switch(cur_tok->type){
            case MEMOP:
                switch((*cur_tok = get_next_token()).type){
                    case REGISTER:
                        switch((*cur_tok = get_next_token()).type){
                            case INTO:
                                switch((*cur_tok = get_next_token()).type){
                                    case REGISTER:
                                        switch((*cur_tok = get_next_token()).type){
                                            case EOL:
                                                break;
                                            case ERROR:
                                                print_error(cur_tok->name);
                                            default:
                                                print_error(invalid_sentence);
                                        }
                                        break;
                                    case ERROR:
                                        print_error(cur_tok->name);
                                    default:
                                        print_error(invalid_sentence);
                                }
                                break;
                            case ERROR:
                                print_error(cur_tok->name);
                            default:
                                print_error(invalid_sentence);
                        }
                        break;
                    case ERROR:
                        print_error(cur_tok->name);
                    default:
                        print_error(invalid_sentence);
                }
                op_num += 1;
                break;
            case LOADI:
                switch((*cur_tok = get_next_token()).type){
                    case CONSTANT:
                        switch((*cur_tok = get_next_token()).type){
                            case INTO:
                                switch((*cur_tok = get_next_token()).type){
                                    case REGISTER:
                                        switch((*cur_tok = get_next_token()).type){
                                            case EOL:
                                                break;
                                            case ERROR:
                                                print_error(cur_tok->name);
                                            default:
                                                print_error(invalid_sentence);
                                        }
                                        break;
                                    case ERROR:
                                        print_error(cur_tok->name);
                                    default:
                                        print_error(invalid_sentence);
                                }
                                break;
                            case ERROR:
                                print_error(cur_tok->name);
                            default:
                                print_error(invalid_sentence);
                        }
                        break;
                    case ERROR:
                        print_error(cur_tok->name);
                    default:
                        print_error(invalid_sentence);
                }

                op_num += 1;
                break;
            case ARITHOP:
                switch((*cur_tok = get_next_token()).type){
                    case REGISTER:
                        switch((*cur_tok = get_next_token()).type){
                            case COMMA:
                                switch((*cur_tok = get_next_token()).type){
                                    case REGISTER:
                                        switch((*cur_tok = get_next_token()).type){
                                            case INTO:
                                                switch((*cur_tok = get_next_token()).type){
                                                    case REGISTER:
                                                        switch((*cur_tok = get_next_token()).type){
                                                            case EOL:
                                                                break;
                                                            case ERROR:
                                                                print_error(cur_tok->name);
                                                            default:
                                                                print_error(invalid_sentence);
                                                        }
                                                        break;
                                                    case ERROR:
                                                        print_error(cur_tok->name);
                                                    default:
                                                        print_error(invalid_sentence);

                                                }
                                                break;
                                            case ERROR:
                                                print_error(cur_tok->name);
                                            default:
                                                print_error(invalid_sentence);
                                        }
                                        break;
                                    case ERROR:
                                        print_error(cur_tok->name);
                                    default:
                                        print_error(invalid_sentence);
                                }
                                break;
                            case ERROR:
                                print_error(cur_tok->name);
                            default:
                                print_error(invalid_sentence);
                        }
                        break;
                    case ERROR:
                        print_error(cur_tok->name);
                    default:
                        print_error(invalid_sentence);
                }

                op_num += 1;
                break;
            case OUTPUT:
                switch((*cur_tok = get_next_token()).type){
                    case CONSTANT:
                        switch((*cur_tok = get_next_token()).type){
                            case EOL:
                                break;
                            case ERROR:
                                print_error(cur_tok->name);
                            default:
                                print_error(invalid_sentence);
                        }
                        break;
                    case ERROR:
                        print_error(cur_tok->name);
                    default:
                        print_error(invalid_sentence);
                }     
                        
                op_num += 1;
                break;
            case NOP:
                switch((*cur_tok = get_next_token()).type){
                    case EOL:
                        break;
                    case ERROR:
                        print_error(cur_tok->name);
                    default:
                        print_error(invalid_sentence);
                }

                op_num += 1;
                break;
            case EOL:
                break;
            case ERROR:
                print_error(cur_tok->name);
            default:
                //must be not an op
                print_error(invalid_op);

        }

        //go to new line if any errors were detected
        if(new_err){
            *cur_tok = get_next_eol_token();
            new_err = 0;
            continue;
        }

        //eol must have been reached
        line_num += 1;
        *cur_tok = get_next_token();
    }

    //return -1 on error otherwise num ops
    return err_found ? -1 : op_num;
}

//prints ir
void print_IR(){
    struct IR* ir = head->next;
    while(ir != head){
        //printf("opcode: %i or %s, sr1: %i, sr2: %i, sr3: %i\n", ir->opcode, TOKEN_NAMES[ir->opcode], ir->arg1.SR, ir->arg2.SR, ir->arg3.SR);
        ir = ir->next;
    }
}


//sets up parser
int setup_parser(struct token* tok_pointer){
    cur_tok = tok_pointer;

    //create empty head
    struct IR* ir = malloc(sizeof(struct IR));
    ir->next = ir;
    ir->prev = ir;

    //assign head and current
    head = ir;
    current = ir;

    return 0;
}