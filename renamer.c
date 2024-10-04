#include <stdlib.h>
#include <stdio.h>
#include "constants.h"

extern struct IR* head;
uint32_t neg1 = (-1);


//conducts the renaming
//returns the max virtual register name on success
//returns -1 on failure
int rename_registers(int32_t n_ops){
    uint32_t max_SR = 0;
    struct IR* node;
    for(node = head->next; node != head; node = node->next){    
        switch(node->opcode){
            case load:
            case store:
                if(node->arg1.SR > max_SR) max_SR = node->arg1.SR;
                if(node->arg3.SR > max_SR) max_SR = node->arg3.SR;
                break;
            case loadI:
                if(node->arg3.SR > max_SR) max_SR = node->arg3.SR;
                break;
            case add:
            case sub:
            case mult:
            case lshift:
            case rshift:
                if(node->arg1.SR > max_SR) max_SR = node->arg1.SR;
                if(node->arg2.SR > max_SR) max_SR = node->arg2.SR;
                if(node->arg3.SR > max_SR) max_SR = node->arg3.SR;
                break;
            case output:
            case nop:
            default:
                break;
        }
    }
    
    //initializes vectors
    //ASSUMES VR MAX IS 2^32 - 2 (valid bc n_ops is int32_t)
    //invalid = neg1 = 2^32 - 1
    uint32_t* SRtoVR = malloc(sizeof(uint32_t) * (max_SR + 1));
    uint32_t* LU = malloc(sizeof(uint32_t) * (max_SR + 1));

    //check null
    if(SRtoVR == NULL || LU == NULL){
        return -1;
    }
    
    //sets invalid
    for(uint32_t i = 0; i <= max_SR; i++){
        SRtoVR[i] = neg1;
        LU[i] = neg1;
    }

    //initialize ints
    uint32_t VRName = 0;
    
    //main OP loop
    //equivalent to while(node != head)
    //blocks are 0 indexed
    node = head->prev;
    for(int32_t index = n_ops - 1; index >= 0; index--){
        //different number of arguments for different op codes
        struct argument* def_arg = &(node->arg3);
        struct argument* use_arg1 = &(node->arg1);
        struct argument* use_arg2 = &(node->arg2);
        switch(node->opcode){
            case load:
                ///handle def
                //unused def
                if(SRtoVR[def_arg->SR] == neg1){
                    SRtoVR[def_arg->SR] = VRName;
                    VRName++;
                }

                //set VR and NU
                def_arg->VR = SRtoVR[def_arg->SR];
                def_arg->NU = LU[def_arg->SR];

                //kill
                SRtoVR[def_arg->SR] = neg1;
                LU[def_arg->SR] = neg1;


                ///handle use
                //last use
                if(SRtoVR[use_arg1->SR] == neg1){
                    SRtoVR[use_arg1->SR] = VRName;
                    VRName++;
                }

                //set VR and NU
                use_arg1->VR = SRtoVR[use_arg1->SR];
                use_arg1->NU = LU[use_arg1->SR];

                //set index of most recent usage
                LU[use_arg1->SR] = index;
                break;
            case store:
                ///handle use
                //last use
                if(SRtoVR[use_arg1->SR] == neg1){
                    SRtoVR[use_arg1->SR] = VRName;
                    VRName++;
                }

                //set VR and NU
                use_arg1->VR = SRtoVR[use_arg1->SR];
                use_arg1->NU = LU[use_arg1->SR];

                //set index of most recent usage
                LU[use_arg1->SR] = index;


                ///handle def (considered a use in store)
                //unused def
                if(SRtoVR[def_arg->SR] == neg1){
                    SRtoVR[def_arg->SR] = VRName;
                    VRName++;
                }

                //set VR and NU
                def_arg->VR = SRtoVR[def_arg->SR];
                def_arg->NU = LU[def_arg->SR];

                //set VR and NU
                def_arg->VR = SRtoVR[def_arg->SR];
                def_arg->NU = LU[def_arg->SR];

                //set index of most recent usage
                LU[def_arg->SR] = index;
                break;

            case loadI:
                //propagate the constant
                node->arg1.VR = node->arg1.SR;

                ///handle def
                //unused def
                if(SRtoVR[def_arg->SR] == neg1){
                    SRtoVR[def_arg->SR] = VRName;
                    VRName++;
                }

                //set VR and NU
                def_arg->VR = SRtoVR[def_arg->SR];
                def_arg->NU = LU[def_arg->SR];

                //kill
                SRtoVR[def_arg->SR] = neg1;
                LU[def_arg->SR] = neg1;
                break;

            case add:
            case sub:
            case mult:
            case lshift:
            case rshift:
                ///handle def
                //unused def
                if(SRtoVR[def_arg->SR] == neg1){
                    SRtoVR[def_arg->SR] = VRName;
                    VRName++;
                }

                //set VR and NU
                def_arg->VR = SRtoVR[def_arg->SR];
                def_arg->NU = LU[def_arg->SR];

                //kill
                SRtoVR[def_arg->SR] = neg1;
                LU[def_arg->SR] = neg1;


                ///handle use1
                //last use
                if(SRtoVR[use_arg1->SR] == neg1){
                    SRtoVR[use_arg1->SR] = VRName;
                    VRName++;
                }

                //set VR and NU
                use_arg1->VR = SRtoVR[use_arg1->SR];
                use_arg1->NU = LU[use_arg1->SR];

                //set index of most recent usage
                LU[use_arg1->SR] = index;

                ///handle use2
                //last use
                if(SRtoVR[use_arg2->SR] == neg1){
                    SRtoVR[use_arg2->SR] = VRName;
                    VRName++;
                }

                //set VR and NU
                use_arg2->VR = SRtoVR[use_arg2->SR];
                use_arg2->NU = LU[use_arg2->SR];

                //set index of most recent usage
                LU[use_arg2->SR] = index;
                break;

            case output:
                //propagate the constant
                node->arg1.VR = node->arg1.SR;
                break;

            case nop:
                break;

            default:
                break;
        }
        
        node = node->prev;
    }

    return VRName;
}



