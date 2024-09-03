#include "scanner.c"

extern int setup(char* filename);
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
void help(){
    printf("help!\n");
}

//function that displays the internal representation of the program
int rep(char* filename){
    printf("rep!\n");

    int code;
    if(code = setup(filename)){
        return code;
    }

    return 0;
}

//function that scans the program and displays tokens
int scan(char* filename){
    printf("scan!\n");
    int code;
    if(code = setup(filename)){
        return code;
    }

    return 0;
}

//function that parses the program, builds the IR, and reports success or failure. The default
int parse(char* filename){
    printf("parse!\n");
    int code;
    if(code = setup(filename)){
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




int main(int argc, char* argv[]){
    //flags
    uint8_t h = 0;
    uint8_t s = 0;
    uint8_t r = 0;
    uint8_t p = 0;

    //check arguments
    for(int i = 1; i < argc; i += 2){
        char* word = argv[i];
        if(!strcmp(word, "-h")){
            h = i;
            break;
        }
        else if(!strcmp(word, "-s")){
            s = i;
        }
        else if(!strcmp(word, "-p")){
            p = i;
        }
        else if(!strcmp(word, "-r")){
            r = i;
        }
        else if(i == 1 && argc == 2){
            break;
        }
        else{
            printf("Bad arguments!\n");
            h = 1;
            break;
        }
    }

    //process flags
    int code = 0;
    if(h){
        help();
    }
    else if(r && r < (argc - 1)){
        code = rep(argv[r + 1]);
    }
    else if(p && p < (argc - 1)){
        code = parse(argv[p + 1]);
    }
    else if(s && s < (argc - 1)){
        code = scan(argv[s + 1]);
    }
    else if(argc == 2){
        code = parse(argv[1]);
    }
    else{
        printf("Bad Arguments!\n");
        help();
    }

    return code;
}