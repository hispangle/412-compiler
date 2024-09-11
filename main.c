#include "scanner.c"
#include "parser.c"

extern int setup_scanner(char* filename);
extern struct token get_next_token();

//prints token
void print_token(struct token tok){
    //tok type needs to be checked for number name (CONST or REG)
    if(tok.type != CONSTANT && tok.type != REGISTER){
        printf("type: %i or %s, name: %i or %s\n", tok.type, TOKEN_TYPES[tok.type], tok.name, TOKEN_NAMES[tok.type][tok.name]);
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
    if(code = setup_scanner(filename)){
        return code;
    }

    //setup parser
    if(code = setup_parser(tok)){
        return code;
    }

    return 0;
}

//function that scans the program and displays tokens
int s(char* filename){
    printf("scan!\n");
    int code;
    if(code = setup_scanner(filename)){
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
    printf("parse!\n");

    //create tok_pointer
    struct token* tok = malloc(sizeof(struct token));

    int code;

    //set up scanner
    if(code = setup_scanner(filename)){
        return code;
    }

    //setup parser
    if(code = setup_parser(tok)){
        return code;
    }

    printf("setup finished!\n");

    int32_t n_ops = parse();

    print_IR();
    
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