#include <stdlib.h>
#include <stdio.h>
#include "constants.h"

//change tok to global pointer in scanner
//get_tok changes contents but not pointer
//allows use of same pointer in parser

//groups of linked lists
//allocate heads, 
//then full groups as needed

//allocate space for IR at setup and in larger chunks after (exponential i think) (1k, 2k, 4k, 8k, etc)

//externs
extern token get_next_token();
extern token get_next_eol_token();

//keep track of line number and op num
uint32_t line_num = 1;
uint32_t op_num = 0;

//flag for first error
uint8_t err_found = 0;
uint8_t new_err = 0;

//keep track of head and last IR
IR* current;

//gets the next spot for IR
//mallocs space if none left
//for now is just malloc
IR* get_next_IR_loc(){
    return malloc(sizeof(IR));
}

//adds IR to linked list
void add_IR(IR* ir){
    ir->prev = current;
    current->next = ir;
    current = ir;
}



//prints errors
//can be made inline
void print_token_error(uint32_t name){
    //new error found
    new_err = 1;

    //sends error message at first error
    if(!err_found){
        printf("Parse found errors.\n");
        err_found = 1;
    }
    fprintf(stderr, "ERROR %i: \t%s\n", line_num, TOKEN_NAMES[name]);
}


//declare missing regs
const char* MISSING[] = {"constant", "first source register", "comma", "second source register",  "=>", "target register"};

//prints descriptive sentence erroring
void print_sentence_error(uint8_t missing, uint32_t opcode){
    //new error found
    new_err = 1;

    //sends error message at first error
    if(!err_found){
        printf("Parse found errors.\n");
        err_found = 1;
    }

    //missing (MISSING) in (opcode)
    fprintf(stderr, "ERROR %i: \tMissing %s in %s.\n", line_num, MISSING[missing], TOKEN_NAMES[opcode]);
}

//prints that an extra token was found
void print_eol_error(){
    //new error found
    new_err = 1;

    //sends error message at first error
    if(!err_found){
        printf("Parse found errors.\n");
        err_found = 1;
    }

    //extra token
    fprintf(stderr, "ERROR %i: Extra token at end of line.\n", line_num);
}

//creates the internal representation
//returns num operations
//or -1 on error
int32_t parse(IR* head, token* cur_tok, uint32_t* n_ops){
    //get first token
    *cur_tok = get_next_token();
    IR* ir;

    //normal loop
    while(cur_tok->type != EoF){
        switch(cur_tok->type){
            case MEMOP: //MEMOP
                ir = get_next_IR_loc();
                ir->opcode = cur_tok->name;
                switch((*cur_tok = get_next_token()).type){
                    case REGISTER: //MEMOP REG
                        ir->arg1.SR = cur_tok->name;
                        switch((*cur_tok = get_next_token()).type){
                            case INTO: //MEMOP REG INTO
                                switch((*cur_tok = get_next_token()).type){
                                    case REGISTER: //MEMOP REG INTO REG
                                        ir->arg3.SR = cur_tok->name;
                                        switch((*cur_tok = get_next_token()).type){
                                            case EOL: //MEMOP REG INTO REG EOL
                                                add_IR(ir);
                                                break;
                                            case ERROR:
                                                print_token_error(cur_tok->name);
                                            default:
                                                print_eol_error();
                                        }
                                        break;
                                    case ERROR:
                                        print_token_error(cur_tok->name);
                                    default:
                                        print_sentence_error(5, ir->opcode);
                                }
                                break;
                            case ERROR:
                                print_token_error(cur_tok->name);
                            default:
                                print_sentence_error(4, ir->opcode);
                        }
                        break;
                    case ERROR:
                        print_token_error(cur_tok->name);
                    default:
                        print_sentence_error(1, ir->opcode);
                }
                op_num += 1;
                break;
            case LOADI: //LOADI
                ir = get_next_IR_loc();
                ir->opcode = cur_tok->name;
                switch((*cur_tok = get_next_token()).type){
                    case CONSTANT: //LOADI CONST
                        ir->arg1.SR = cur_tok->name;
                        switch((*cur_tok = get_next_token()).type){
                            case INTO: //LOADI CONST INTO
                                switch((*cur_tok = get_next_token()).type){
                                    case REGISTER: //LOADI CONST INTO REG
                                        ir->arg3.SR = cur_tok->name;
                                        switch((*cur_tok = get_next_token()).type){
                                            case EOL: //LOADI CONST INTO REG EOL
                                                add_IR(ir);
                                                break;
                                            case ERROR:
                                                print_token_error(cur_tok->name);
                                            default:
                                                print_eol_error();
                                        }
                                        break;
                                    case ERROR:
                                        print_token_error(cur_tok->name);
                                    default:
                                        print_sentence_error(5, ir->opcode);
                                }
                                break;
                            case ERROR:
                                print_token_error(cur_tok->name);
                            default:
                                print_sentence_error(4, ir->opcode);
                        }
                        break;
                    case ERROR:
                        print_token_error(cur_tok->name);
                    default:
                        print_sentence_error(0, ir->opcode);
                }
                op_num += 1;
                break;
            case ARITHOP: //ARITHOP
                ir = get_next_IR_loc();
                ir->opcode = cur_tok->name;
                switch((*cur_tok = get_next_token()).type){
                    case REGISTER: //ARITHOP REG
                        ir->arg1.SR = cur_tok->name;
                        switch((*cur_tok = get_next_token()).type){
                            case COMMA: //ARITHOP REG COMMA
                                switch((*cur_tok = get_next_token()).type){
                                    case REGISTER: //ARITHOP REG COMMA REG
                                        ir->arg2.SR = cur_tok->name;
                                        switch((*cur_tok = get_next_token()).type){
                                            case INTO: //ARITHOP REG COMMA REG INTO
                                                switch((*cur_tok = get_next_token()).type){
                                                    case REGISTER: //ARITHOP REG COMMA REG INTO REG
                                                        ir->arg3.SR = cur_tok->name;
                                                        switch((*cur_tok = get_next_token()).type){
                                                            case EOL: //ARITHOP REG COMMA REG INTO REG EOL
                                                                add_IR(ir);
                                                                break;
                                                            case ERROR:
                                                                print_token_error(cur_tok->name);
                                                            default:
                                                                print_eol_error();
                                                        }
                                                        break;
                                                    case ERROR:
                                                        print_token_error(cur_tok->name);
                                                    default:
                                                        print_sentence_error(5, ir->opcode);
                                                }
                                                break;
                                            case ERROR:
                                                print_token_error(cur_tok->name);
                                            default:
                                                print_sentence_error(4, ir->opcode);
                                        }
                                        break;
                                    case ERROR:
                                        print_token_error(cur_tok->name);
                                    default:
                                        print_sentence_error(3, ir->opcode);
                                }
                                break;
                            case ERROR:
                                print_token_error(cur_tok->name);
                            default:
                                print_sentence_error(2, ir->opcode);
                        }
                        break;
                    case ERROR:
                        print_token_error(cur_tok->name);
                    default:
                        print_sentence_error(1, ir->opcode);
                }
                op_num += 1;
                break;
            case OUTPUT: //OUTPUT
                ir = get_next_IR_loc();
                ir->opcode = cur_tok->name;
                switch((*cur_tok = get_next_token()).type){
                    case CONSTANT: //OUTPUT CONST
                        ir->arg1.SR = cur_tok->name;
                        switch((*cur_tok = get_next_token()).type){
                            case EOL: //OUTPUT CONST EOL
                                add_IR(ir);
                                break;
                            case ERROR:
                                print_token_error(cur_tok->name);
                            default:
                                print_eol_error();
                        }
                        break;
                    case ERROR:
                        print_token_error(cur_tok->name);
                    default:
                        print_sentence_error(0, ir->opcode);
                }        
                op_num += 1;
                break;
            case NOP: //NOP
                ir = get_next_IR_loc();
                ir->opcode = nop;
                switch((*cur_tok = get_next_token()).type){
                    case EOL: //NOP EOL
                        add_IR(ir);
                        break;
                    case ERROR:
                        print_token_error(cur_tok->name);
                    default:
                        print_eol_error();
                }
                op_num += 1;
                break;
            case EOL: //EOL
                break;
            case ERROR:
                print_token_error(cur_tok->name);
            default:
                //must be not an op
                print_token_error(invalid_op);
        }

        //go to new line if any errors were detected
        if(new_err){
            *cur_tok = get_next_eol_token();
            new_err = 0;
            break;
        }

        //eol reached
        line_num += 1;
        *cur_tok = get_next_token();
    }


    //error loop
    uint32_t name;
    while(cur_tok->type != EoF){
        switch(cur_tok->type){
            case MEMOP: //MEMOP
                name = cur_tok->name;
                switch((*cur_tok = get_next_token()).type){
                    case REGISTER: //MEMOP REG
                        switch((*cur_tok = get_next_token()).type){
                            case INTO: //MEMOP REG INTO 
                                switch((*cur_tok = get_next_token()).type){
                                    case REGISTER: //MEMOP REG INTO REG
                                        switch((*cur_tok = get_next_token()).type){
                                            case EOL: //MEMOP REG INTO REG EOL
                                                break;
                                            case ERROR:
                                                print_token_error(cur_tok->name);
                                            default:
                                                print_eol_error();
                                        }
                                        break;
                                    case ERROR:
                                        print_token_error(cur_tok->name);
                                    default:
                                        print_sentence_error(5, name);
                                }
                                break;
                            case ERROR:
                                print_token_error(cur_tok->name);
                            default:
                                print_sentence_error(4, name);
                        }
                        break;
                    case ERROR:
                        print_token_error(cur_tok->name);
                    default:
                        print_sentence_error(1, name);
                }
                op_num += 1;
                break;
            case LOADI: //LOADI
                name = cur_tok->name;
                switch((*cur_tok = get_next_token()).type){
                    case CONSTANT: //LOADI CONST
                        switch((*cur_tok = get_next_token()).type){
                            case INTO: //LOADI CONST INTO
                                switch((*cur_tok = get_next_token()).type){
                                    case REGISTER: //LOADI CONST INTO REG
                                        switch((*cur_tok = get_next_token()).type){
                                            case EOL: //LOADI CONST INTO REG EOL
                                                break;
                                            case ERROR:
                                                print_token_error(cur_tok->name);
                                            default:
                                                print_eol_error();
                                        }
                                        break;
                                    case ERROR:
                                        print_token_error(cur_tok->name);
                                    default:
                                        print_sentence_error(5, name);
                                }
                                break;
                            case ERROR:
                                print_token_error(cur_tok->name);
                            default:
                                print_sentence_error(4, name);
                        }
                        break;
                    case ERROR:
                        print_token_error(cur_tok->name);
                    default:
                        print_sentence_error(0, name);
                }
                op_num += 1;
                break;
            case ARITHOP: //ARITHOP
                name = cur_tok->name;
                switch((*cur_tok = get_next_token()).type){
                    case REGISTER: //ARITHOP REG
                        switch((*cur_tok = get_next_token()).type){
                            case COMMA: //ARITHOP REG COMMA
                                switch((*cur_tok = get_next_token()).type){
                                    case REGISTER: //ARITHOP REG COMMA REG
                                        switch((*cur_tok = get_next_token()).type){
                                            case INTO: //ARITHOP REG COMMA REG INTO
                                                switch((*cur_tok = get_next_token()).type){
                                                    case REGISTER: //ARITHOP REG COMMA REG INTO REG
                                                        switch((*cur_tok = get_next_token()).type){
                                                            case EOL: //ARITHOP REG COMMA REG INTO REG EOL
                                                                break;
                                                            case ERROR:
                                                                print_token_error(cur_tok->name);
                                                            default:
                                                                print_eol_error();
                                                        }
                                                        break;
                                                    case ERROR:
                                                        print_token_error(cur_tok->name);
                                                    default:
                                                        print_sentence_error(5, name);
                                                }
                                                break;
                                            case ERROR:
                                                print_token_error(cur_tok->name);
                                            default:
                                                print_sentence_error(4, name);
                                        }
                                        break;
                                    case ERROR:
                                        print_token_error(cur_tok->name);
                                    default:
                                        print_sentence_error(3, name);
                                }
                                break;
                            case ERROR:
                                print_token_error(cur_tok->name);
                            default:
                                print_sentence_error(2, name);
                        }
                        break;
                    case ERROR:
                        print_token_error(cur_tok->name);
                    default:
                        print_sentence_error(1, name);
                }
                op_num += 1;
                break;
            case OUTPUT: //OUTPUT
                name = cur_tok->name;
                switch((*cur_tok = get_next_token()).type){
                    case CONSTANT: //OUTPUT CONST
                        switch((*cur_tok = get_next_token()).type){
                            case EOL: //OUTPUT CONST EOL
                                break;
                            case ERROR:
                                print_token_error(cur_tok->name);
                            default:
                                print_eol_error();
                        }
                        break;
                    case ERROR:
                        print_token_error(cur_tok->name);
                    default:
                        print_sentence_error(0, name);
                }             
                op_num += 1;
                break;
            case NOP: //NOP
                name = nop;
                switch((*cur_tok = get_next_token()).type){
                    case EOL: //NOP EOL
                        break;
                    case ERROR:
                        print_token_error(cur_tok->name);
                    default:
                        print_eol_error();
                }
                op_num += 1;
                break;
            case EOL: //EOL
                break;
            case ERROR:
                print_token_error(cur_tok->name);
            default:
                //must be not an op
                print_token_error(invalid_op);
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

    //set heads prev at the finale
    head->prev = current;
    current->next = head;

    //return -1 on error otherwise num ops
    *n_ops = op_num;
    return err_found ? -1 : 0;
}

void setup_parser(IR* head){
    head->next = head;
    head->prev = head;
    current = head;
}