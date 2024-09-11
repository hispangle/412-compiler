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

//global token pointer but declared
struct token* cur_tok;

//keep track of line number and op num
uint32_t line_num = 1;
int32_t op_num = 1;

//flag for first error
uint8_t err_found = 0;

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
void print_error(){
    if(!err_found){
        printf("Parse found errors.\n");
        err_found = 1;
    }
    printf("ERROR:%i %s\n", line_num, TOKEN_NAMES[ERROR][cur_tok->name]);
}

//creates the internal representation
//returns num operations
//or -1 on error
int32_t parse(){
    //get first token
    *cur_tok = get_next_token();

    //normal loop
    while(cur_tok->type != EoF){
        // //print token for now
        // if(cur_tok->type != CONSTANT && cur_tok->type != REGISTER){
        //     printf("TOKEN FOUND: type: %i or %s, name: %i or %s\n", cur_tok->type, TOKEN_TYPES[cur_tok->type], cur_tok->name, TOKEN_NAMES[cur_tok->type][cur_tok->name]);
        // } else{
        //     printf("TOKEN FOUND: type: %i or %s, name: %i\n", cur_tok->type, TOKEN_TYPES[cur_tok->type], cur_tok->name);
        // }


        switch(cur_tok->type){
            case MEMOP:
                break;
            case LOADI:
                break;
            case ARITHOP:
                break;
            case OUTPUT:
                break;
            case NOP:
                switch((*cur_tok = get_next_token()).type){
                    case EOL:
                        //build IR
                        ; //fuck c
                        struct IR* ir = get_next_IR_loc();
                        ir->opcode = nop;
                        add_IR(ir);
                        printf("%i\n", nop);
                        printf("opcode: %i, sr1: %i, sr2: %i, sr3: %i\n", ir->opcode, ir->arg1.SR, ir->arg2.SR, ir->arg3.SR);
                        break;
                    case ERROR:
                        print_error();
                    default:
                        cur_tok->type = ERROR;
                        cur_tok->name = invalid_sentence;
                        print_error();
                }

                break;
            case EOL:
                line_num += 1;
                break;
            case ERROR:
                print_error();
                break;
            default:
                //must be not an op
                cur_tok->type = ERROR;
                cur_tok->name = invalid_op;
                print_error();

                //get next line
                *cur_tok = get_next_eol_token();
                line_num += 1;

                break; 
        }


        *cur_tok = get_next_token();
    }


    //error loop
    //only needed if parsing too slow with additional if

    //return -1 on error otherwise num ops
    return err_found ? -1 : op_num;
}

//prints ir
void print_IR(){
    int32_t n = 1;
    struct IR* ir = head->next;
    while(ir != head){
        printf("opcode: %i, sr1: %i, sr2: %i, sr3: %i\n", ir->opcode, ir->arg1.SR, ir->arg2.SR, ir->arg3.SR);
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