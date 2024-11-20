#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "constants.h"

//conducts the renaming
//returns 0 on success
//returns -1 on failure
int rename_registers(uint32_t n_ops, IR* head, uint32_t* maxVR, uint32_t* maxlive_ptr){
    //gets the max SR
    uint32_t max_SR = 0;
    IR* node;
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
    //ASSUMES VR MAX IS 2^32 - 2
    //invalid = UINT32_MAX
    //linked list if want to not use max_SR to initialize??
    //maybe other method
    //LU IS LATEST USE NOT LAST USE*********** VIRTUALLY NEXT USE
    uint32_t* SRtoVR = malloc(sizeof(uint32_t) * (max_SR + 1));
    uint32_t* LU = malloc(sizeof(uint32_t) * (max_SR + 1));

    //check null
    if(SRtoVR == NULL || LU == NULL){
        return -1;
    }
    
    //sets invalid
    for(uint32_t i = 0; i <= max_SR; i++){
        SRtoVR[i] = UINT32_MAX;
        LU[i] = UINT32_MAX;
    }

    //initialize ints
    uint32_t VRName = 0;
    uint32_t nlive = 0;
    uint32_t maxlive = 0;
    
    //main OP loop
    //equivalent to while(node != head)
    //blocks are 0 indexed
    node = head->prev;
    for(uint32_t i = n_ops; i > 0; i--){
        uint32_t index = i - 1;
        //different number of arguments for different op codes
        argument* def_arg = &(node->arg3);
        argument* use_arg1 = &(node->arg1);
        argument* use_arg2 = &(node->arg2);
        switch(node->opcode){
            case load:
                ///handle def
                //unused def
                if(SRtoVR[def_arg->SR] == UINT32_MAX){
                    //creates VR
                    SRtoVR[def_arg->SR] = VRName;
                    VRName++;

                    //nlive blips
                    nlive++;

                    //checks for maxlive
                    if(nlive > maxlive){
                        maxlive = nlive;
                    }
                }

                //set VR and NU
                def_arg->VR = SRtoVR[def_arg->SR];
                def_arg->NU = LU[def_arg->SR];

                //reduce nlive bc not used above a def
                nlive--;

                //kill
                SRtoVR[def_arg->SR] = UINT32_MAX;
                LU[def_arg->SR] = UINT32_MAX;


                ///handle use
                //last use
                if(SRtoVR[use_arg1->SR] == UINT32_MAX){
                    //creates VR
                    SRtoVR[use_arg1->SR] = VRName;
                    VRName++;

                    //updateMapss location of this last use
                    use_arg1->isLU = 1;

                    //VR live above its last use
                    nlive++;

                    //no need to check if its the max, there was a nlive-- before
                    // if(nlive > maxlive){
                    //     maxlive = nlive;
                    // }
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
                if(SRtoVR[use_arg1->SR] == UINT32_MAX){
                    //creates VR
                    SRtoVR[use_arg1->SR] = VRName;
                    VRName++;

                    //updateMapss location of this last use
                    use_arg1->isLU = 1;

                    //VR live above its last use
                    nlive++;

                    //checks if max
                    if(nlive > maxlive){
                        maxlive = nlive;
                    }
                }

                //set VR and NU
                use_arg1->VR = SRtoVR[use_arg1->SR];
                use_arg1->NU = LU[use_arg1->SR];

                //set index of most recent usage
                LU[use_arg1->SR] = index;


                ///handle def (considered a use in store)
                //last use
                if(SRtoVR[def_arg->SR] == UINT32_MAX){
                    //creates VR
                    SRtoVR[def_arg->SR] = VRName;
                    VRName++;

                    //updateMapss location of this last use
                    def_arg->isLU = 1;

                    //VR live above its last use
                    nlive++;

                    //checks for maxlive
                    if(nlive > maxlive){
                        maxlive = nlive;
                    }
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
                if(SRtoVR[def_arg->SR] == UINT32_MAX){
                    //creates VR
                    SRtoVR[def_arg->SR] = VRName;
                    VRName++;

                    //nlive blips
                    nlive++;

                    //checks for maxlive
                    if(nlive > maxlive){
                        maxlive = nlive;
                    }
                }



                //set VR and NU
                def_arg->VR = SRtoVR[def_arg->SR];
                def_arg->NU = LU[def_arg->SR];

                //reduce nlive bc not used above a def
                nlive--;

                //kill
                SRtoVR[def_arg->SR] = UINT32_MAX;
                LU[def_arg->SR] = UINT32_MAX;
                break;

            case add:
            case sub:
            case mult:
            case lshift:
            case rshift:
                ///handle def
                //unused def
                if(SRtoVR[def_arg->SR] == UINT32_MAX){
                    //creates VR
                    SRtoVR[def_arg->SR] = VRName;
                    VRName++;

                    //nlive blips
                    nlive++;

                    //checks for maxlive
                    if(nlive > maxlive){
                        maxlive = nlive;
                    }
                }


                //set VR and NU
                def_arg->VR = SRtoVR[def_arg->SR];
                def_arg->NU = LU[def_arg->SR];

                //reduce nlive bc not used above a def
                nlive--;

                //kill
                SRtoVR[def_arg->SR] = UINT32_MAX;
                LU[def_arg->SR] = UINT32_MAX;


                ///handle use1
                //last use
                if(SRtoVR[use_arg1->SR] == UINT32_MAX){
                    //creates VR
                    SRtoVR[use_arg1->SR] = VRName;
                    VRName++;

                    //updateMapss location of this last use
                    use_arg1->isLU = 1;

                    //VR live above its last use
                    nlive++;

                    //no need to check if its the max, there was a nlive-- before
                    // if(nlive > maxlive){
                    //     maxlive = nlive;
                    // }
                }

                //set VR and NU
                use_arg1->VR = SRtoVR[use_arg1->SR];
                use_arg1->NU = LU[use_arg1->SR];

                //set index of most recent usage
                LU[use_arg1->SR] = index;

                ///handle use2
                //last use
                if(SRtoVR[use_arg2->SR] == UINT32_MAX){
                    //creates VR
                    SRtoVR[use_arg2->SR] = VRName;
                    VRName++;

                    //updateMapss location of this last use
                    use_arg2->isLU = 1;

                    //VR live above its last use
                    nlive++;

                    //need to check if its the second last use
                    if(nlive > maxlive){
                        maxlive = nlive;
                    }
                }

                //set VR and NU
                //TODO: wrong?
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

    //pass values
    *maxVR = VRName;
    *maxlive_ptr = maxlive;
    return 0;
}
