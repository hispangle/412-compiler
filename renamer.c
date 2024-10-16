#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "constants.h"

extern struct IR* head;
struct IR** VRtoDef;

//conducts the renaming
//returns 0 on success
//TODO: change n_ops to uint32_t (here and in parse)
//returns -1 on failure
int rename_registers(int32_t n_ops, uint32_t* maxVR, uint32_t* maxlive_ptr){
    //gets the max SR
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
    //ASSUMES VR MAX IS 2^32 - 2 (valid bc n_ops is int32_t) ((later not necessarily, but edge case of completely unique defs for 2^32 - 1 lines))
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

    //vectors desired for allocation //desired for more complex rematerialization
    VRtoDef = malloc(sizeof(struct IR*) * n_ops * 3);

    if(VRtoDef == NULL){
        return -1;
    }

    //initialize to NULL
    for(int i = 0; i < n_ops * 3; i++){
        VRtoDef[i] = NULL;
    }

    //initialize ints
    uint32_t VRName = 0;
    uint32_t nlive = 0;
    uint32_t maxlive = 0;
    
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

                //save the VR def
                VRtoDef[def_arg->VR] = node;

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

                    //saves location of this last use
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

                    //saves location of this last use
                    use_arg1->isLU = 1;

                    //VR live above its last use
                    nlive++;

                    //checks for maxlive on next one
                    // if(nlive > maxlive){
                    //     maxlive = nlive;
                    // }
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

                    //saves location of this last use
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

                //save the VR def
                VRtoDef[def_arg->VR] = node;

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

                //save the VR def
                VRtoDef[def_arg->VR] = node;

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

                    //saves location of this last use
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

                    //saves location of this last use
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






    //if there is an undefed use
    //double check if this check is correct
    //TODO: make this
    if(nlive != 0){ 
        //redo maxLive
        //maxlive = 0;

        //TODO: later redo maxlive

        // node = head->prev;
        // while(int32_t index = n_ops - 1; index >= 0; index--){
        //     //different number of arguments for different op codes
        //     struct argument* def_arg = &(node->arg3);
        //     struct argument* use_arg1 = &(node->arg1);
        //     struct argument* use_arg2 = &(node->arg2);

        //     uint32_t defVR = def_arg->VR;
        //     uint32_t use1VR = use_arg1->VR;
        //     uint32_t use2VR = use_arg2->VR;

        //     switch(node->opcode){
        //         case load:
        //         case store:
        //         case loadI:
        //         case add:
        //         case sub:
        //         case mult:
        //         case lshift:
        //         case rshift:
        //         case output:
        //         case nop:
        //         default:
        //             break;
        //     }
            
        //     node = node->prev;
        // }

    }


    //pass values
    *maxVR = VRName;
    *maxlive_ptr = maxlive;
    return 0;
}



//allocates physical registers to the ILOC block
//inputs-
//  k: number of physical registers, assumed to have 3 <= k <= 64
//  n_ops: number of operations in the ILOC block
//returns-
//  success: 0
//  failure: -1
int allocate(int k, int32_t n_ops){
    //rename first
    uint32_t maxVR = 0;
    uint32_t maxlive = 0;
    if(rename_registers(n_ops, &maxVR, &maxlive) == -1){
        return -1;
    }

    ////creates maps that are always used
    //make usable PR stack
    uint8_t* PR_stack = malloc(sizeof(uint8_t) * k);
    uint8_t PR_stack_size = k;

    //make PR to VR
    uint32_t* PRtoVR = malloc(sizeof(uint32_t) * k);

    //make VR to PR
    uint8_t* VRtoPR = malloc(sizeof(uint8_t) * maxVR);

    //null checks
    if(PR_stack == NULL || PRtoVR == NULL || VRtoPR == NULL){
        return -1;
    }

    //initialize PR indexed maps
    for(int i = 0; i < k; i++){
        PR_stack[i] = k - 1 - i; //descending order
        PRtoVR[i] = UINT32_MAX; //invalid
    }

    //initialize VR indexed maps
    for(int i = 0; i < maxVR; i++){
        VRtoPR[i] = 65; //k <= 64
    }

    //declares maps used in maxlive > k cases
    uint32_t* VRtoSpill;
    uint32_t spill_loc = 32768;
    uint8_t* VRNeedsRestore;

    uint32_t* PRtoNU;

    uint8_t* PR_queue; //removables array
    uint8_t* PR_queue_start; //actually used as pointer not array
    uint8_t* PR_queue_end; //actually used as pointer not array
    uint8_t PR_queue_size;

    //initialize if maxlive > k
    if(maxlive > k){
        //reserve a register
        PR_stack_size--; 
        PR_stack++;

        //initialize
        VRtoSpill = malloc(sizeof(uint32_t) * maxVR);
        VRNeedsRestore = malloc(sizeof(uint8_t) * maxVR);
        PRtoNU = malloc(sizeof(uint32_t) * k);
        PR_queue = malloc(sizeof(uint8_t) * k);

        PR_queue_start = PR_queue;
        PR_queue_end = PR_queue;

        //null checks
        if(VRtoSpill == NULL || PRtoNU == NULL || PR_queue == NULL || VRNeedsRestore == NULL){
            return -1;
        }

    }

    //go thru all ops in order
    struct IR* node = head->next;
    while(node != head){
        

        struct argument* use1 = NULL;
        struct argument* use2 = NULL;
        struct argument* def = NULL;

        switch(node->opcode){
            case load:
                use1 = &(node->arg1);
                def = &(node->arg3);
                break;
            case store:
                use1 = &(node->arg1);
                use2 = &(node->arg3);
                break;
            case loadI:
                //propagate constant
                use1 = &(node->arg1);
                use1->PR = use1->VR;
                use1 = NULL;

                def = &(node->arg3);
                break;
            case add:
            case sub:
            case mult:
            case lshift:
            case rshift:
                use1 = &(node->arg1);
                use2 = &(node->arg2);
                def = &(node->arg3);
                break;
            case output:
                //propagate constant
                use1 = &(node->arg1);
                use1->PR = use1->VR;
                use1 = NULL;
                break;
            case nop:
                break;
            default:
                break;
        
        }

        //declare vars
        uint8_t PR;
        uint32_t VR;

        //set PR
        //use1
        uint8_t PRUsed = 65;
        if(use1 != NULL){
            VR = use1->VR;
            PR = VRtoPR[VR];
            int save = 1;

                
            //no PR assigned
            if(PR == 65){
                if(maxlive > k){ //spillable getPR
                    //check if undefed use
                    if(use1->NU == 0){
                        //insert loadI 0 into reserve
                        struct IR* newLoadI= malloc(sizeof(struct IR));
                        newLoadI->opcode = loadI;
                        (&(newLoadI->arg1))->PR = 0;
                        (&(newLoadI->arg3))->PR = k - 1;

                        //insert into ILOC
                        struct IR* prev = node->prev;
                        prev->next = newLoadI;
                        node->prev = newLoadI;
                        newLoadI->next = node;
                        newLoadI->prev = prev;

                        save = 0;
                        PR = k - 1;
                    } else if(PR_stack_size > 0){ //grab from stack if possible
                        PR_stack_size--;
                        PR = PR_stack[PR_stack_size];
                        PRtoNU[PR] = use1->NU;
                    } else if(PR_queue_start != PR_queue_end){ //grab from queue if possible
                        PR = *PR_queue_start;
                        PR_queue_start += 1;

                        //maintain mod of queue
                        if(PR_queue_start >= PR_queue + k){ //make sure this math and check is correct
                            PR_queue_start = PR_queue;
                        } 

                        //remove old value
                        uint32_t VR_old = PRtoVR[PR];
                        VRtoPR[VR_old] = 65;
                        PRtoNU[PR] = use1->NU;
                    } else { //spill
                        uint8_t maxPR = 0;
                        uint32_t maxNU = 0;
                        for(int i = 0; i < k - 1; i++){
                            if(PRtoNU[i] > maxNU){
                                maxPR = i;
                                maxNU = PRtoNU[i];
                            }
                        }

                        PR = maxPR; //pick PR to spill (PR with max PRtoNU) //will be more complex in the future
                        //insert the loadI and store //WILL CHANGE LATER DO THIS FOR NOW
                        //                           //can delay inserting loadI and store if rematable rn
                        //                           //must store location of spill to be able to spill here if not rematable later
                        //                           //struct IR* VRtoSpillInsert, where loadI and store inserted as VRto...[VR]->next
                        //                           //on spill insertion, set VRto...[VR] to NULL

                        //create loadI
                        struct IR* newLoadI= malloc(sizeof(struct IR));
                        newLoadI->opcode = loadI;
                        (&(newLoadI->arg1))->PR = spill_loc;
                        (&(newLoadI->arg3))->PR = k - 1;

                        //create store
                        struct IR* newStore = malloc(sizeof(struct IR));
                        newStore->opcode = store;
                        (&(newStore->arg1))->PR = PR;
                        (&(newStore->arg3))->PR = k - 1;

                        //insert into ILOC
                        struct IR* prev = node->prev;
                        prev->next = newLoadI;
                        node->prev = newStore;
                        newLoadI->next = newStore;
                        newLoadI->prev = prev;
                        newStore->next = node;
                        newStore->prev = newLoadI;


                        //remove old values
                        uint32_t VR_old = PRtoVR[PR];
                        VRtoSpill[VR_old] = spill_loc;
                        spill_loc += 4;
                        
                        VRNeedsRestore[VR_old] = 1;
                        VRtoPR[VR_old] = 65;
                        PRtoNU[PR] = use1->NU;
                    }




                } else { //nonspillable getPR
                    PR_stack_size--;
                    PR = PR_stack[PR_stack_size];
                }

                //if reserve register wasnt used
                if(save){
                    //write VR and PR maps
                    VRtoPR[VR] = PR;
                    PRtoVR[PR] = VR;
                }
                
            }

            //check if restore needed
            if(maxlive > k && VRNeedsRestore[VR]){
                //reset restore
                VRNeedsRestore[VR] = 0;

                //create loadI
                        struct IR* newLoadI= malloc(sizeof(struct IR));
                        newLoadI->opcode = loadI;
                        (&(newLoadI->arg1))->PR = VRtoSpill[VR];
                        (&(newLoadI->arg3))->PR = k - 1;

                        //create store
                        struct IR* newLoad = malloc(sizeof(struct IR));
                        newLoad->opcode = load;
                        (&(newLoad->arg1))->PR = k - 1;
                        (&(newLoad->arg3))->PR = PR;

                        //insert into ILOC
                        struct IR* prev = node->prev;
                        prev->next = newLoadI;
                        node->prev = newLoad;
                        newLoadI->next = newLoad;
                        newLoadI->prev = prev;
                        newLoad->next = node;
                        newLoad->prev = newLoadI;
            }
            use1->PR = PR;
            PRUsed = PR;
        }

        


        //use2
        if(use2 != NULL){
            VR = use2->VR;
            PR = VRtoPR[VR];
            int save = 1;
            
            //no PR assigned
            if(PR == 65){
                if(maxlive > k){ //spillable getPR
                    //check if undefed use
                    if(use2->NU == 0){
                        //insert loadI 0 into reserve
                        struct IR* newLoadI= malloc(sizeof(struct IR));
                        newLoadI->opcode = loadI;
                        (&(newLoadI->arg1))->PR = 0;
                        (&(newLoadI->arg3))->PR = k - 1;

                        //insert into ILOC
                        struct IR* prev = node->prev;
                        prev->next = newLoadI;
                        node->prev = newLoadI;
                        newLoadI->next = node;
                        newLoadI->prev = prev;

                        save = 0;
                        PR = k - 1;
                    } else if(PR_stack_size > 0){ //grab from stack if possible
                        PR_stack_size--;
                        PR = PR_stack[PR_stack_size];
                        PRtoNU[PR] = use2->NU;
                    } else if(PR_queue_start != PR_queue_end){ //grab from queue if possible
                        PR = *PR_queue_start;
                        PR_queue_start += 1;

                        //maintain mod of queue
                        if(PR_queue_start >= PR_queue + k){ //make sure this math and check is correct
                            PR_queue_start = PR_queue;
                        } 

                        //remove old value
                        uint32_t VR_old = PRtoVR[PR];
                        VRtoPR[VR_old] = 65;
                        PRtoNU[PR] = use2->NU;
                    } else { //spill
                        uint8_t maxPR = 0;
                        uint32_t maxNU = 0;
                        for(int i = 0; i < k - 1; i++){
                            if(PRtoNU[i] > maxNU && i != PRUsed){
                                maxPR = i;
                                maxNU = PRtoNU[i];
                            }
                        }
                        PR = maxPR; //pick PR to spill (PR with max PRtoNU) //will be more complex in the future
                        
                        //insert the loadI and store //WILL CHANGE LATER DO THIS FOR NOW
                        //                           //can delay inserting loadI and store if rematable rn
                        //                           //must store location of spill to be able to spill here if not rematable later
                        //                           //struct IR* VRtoSpillInsert, where loadI and store inserted as VRto...[VR]->next
                        //                           //on spill insertion, set VRto...[VR] to NULL

                        //create loadI
                        struct IR* newLoadI= malloc(sizeof(struct IR));
                        newLoadI->opcode = loadI;
                        (&(newLoadI->arg1))->PR = spill_loc;
                        (&(newLoadI->arg3))->PR = k - 1;

                        //create store
                        struct IR* newStore = malloc(sizeof(struct IR));
                        newStore->opcode = store;
                        (&(newStore->arg1))->PR = PR;
                        (&(newStore->arg3))->PR = k - 1;

                        //insert into ILOC
                        struct IR* prev = node->prev;
                        prev->next = newLoadI;
                        node->prev = newStore;
                        newLoadI->next = newStore;
                        newLoadI->prev = prev;
                        newStore->next = node;
                        newStore->prev = newLoadI;


                        //remove old values
                        uint32_t VR_old = PRtoVR[PR];
                        VRtoSpill[VR_old] = spill_loc;
                        spill_loc += 4;

                        VRNeedsRestore[VR_old] = 1;
                        VRtoPR[VR_old] = 65;
                        PRtoNU[PR] = use2->NU;
                    }
                } else { //nonspillable getPR
                    PR_stack_size--;
                    PR = PR_stack[PR_stack_size];
                }

                if(save){
                    //write VR and PR maps
                    VRtoPR[VR] = PR;
                    PRtoVR[PR] = VR;
                }

            }

                        //check if restore needed
            if(maxlive > k && VRNeedsRestore[VR]){
                //reset restore
                VRNeedsRestore[VR] = 0;
                //create loadI
                        struct IR* newLoadI= malloc(sizeof(struct IR));
                        newLoadI->opcode = loadI;
                        (&(newLoadI->arg1))->PR = VRtoSpill[VR];
                        (&(newLoadI->arg3))->PR = k - 1;

                        //create store
                        struct IR* newLoad = malloc(sizeof(struct IR));
                        newLoad->opcode = load;
                        (&(newLoad->arg1))->PR = k - 1;
                        (&(newLoad->arg3))->PR = PR;

                        //insert into ILOC
                        struct IR* prev = node->prev;
                        prev->next = newLoadI;
                        node->prev = newLoad;
                        newLoadI->next = newLoad;
                        newLoadI->prev = prev;
                        newLoad->next = node;
                        newLoad->prev = newLoadI;
            }
            use2->PR = PR;
        }


        //free if last use
        //use1
        if(use1 != NULL){
            
            if(use1->isLU){
                VR = use1->VR;
                PR = use1->PR;
                if(maxlive > k){ //spillable free
                    PR_stack[PR_stack_size] = PR;
                    PR_stack_size++;
                    PRtoVR[PR] = UINT32_MAX;
                    VRtoPR[VR] = 65;
                    // *PR_queue_end = PR;
                    // PR_queue_end++;
                    // if(PR_queue_end >= PR_queue + k){ //check for correctness
                    //     PR_queue_end = PR_queue;
                    // } 
                } else { //nonspillable free
                    PR_stack[PR_stack_size] = PR;
                    PR_stack_size++;
                    PRtoVR[PR] = UINT32_MAX;
                    VRtoPR[VR] = 65;
                }
            }
        }

        //use2
        if(use2 != NULL){
            VR = use2->VR;
            PR = use2->PR;
            if(use2->isLU){
                if(maxlive > k){ //spillable free
                    PR_stack[PR_stack_size] = PR;
                    PR_stack_size++;
                    PRtoVR[PR] = UINT32_MAX;
                    VRtoPR[VR] = 65;
                    // *PR_queue_end = PR;
                    // PR_queue_end++;
                    // if(PR_queue_end >= PR_queue + k){ //check for correctness
                    //     PR_queue_end = PR_queue;
                    // } 
                } else { //nonspillable free
                    PR_stack[PR_stack_size] = PR;
                    PR_stack_size++;
                    PRtoVR[PR] = UINT32_MAX;
                    VRtoPR[VR] = 65;
                }
            }
        }


        //def
        if(def != NULL){
            VR = def->VR;
            int save = 1;

            if(maxlive > k){ //spillable getPR
                    //check if unused def
                    if(def->NU == 0){
                        //insert loadI 0 into reserve
                        struct IR* newLoadI= malloc(sizeof(struct IR));
                        newLoadI->opcode = loadI;
                        (&(newLoadI->arg1))->PR = 0;
                        (&(newLoadI->arg3))->PR = k - 1;

                        //insert into ILOC
                        struct IR* prev = node->prev;
                        prev->next = newLoadI;
                        node->prev = newLoadI;
                        newLoadI->next = node;
                        newLoadI->prev = prev;

                        save = 0;
                        PR = k - 1;
                    } else if(PR_stack_size > 0){ //grab from stack if possible
                        PR_stack_size--;
                        PR = PR_stack[PR_stack_size];
                        PRtoNU[PR] = def->NU;
                    } else if(PR_queue_start != PR_queue_end){ //grab from queue if possible
                        PR = *PR_queue_start;
                        PR_queue_start += 1;

                        //maintain mod of queue
                        if(PR_queue_start >= PR_queue + k){ //make sure this math and check is correct
                            PR_queue_start = PR_queue;
                        } 

                        //remove old value
                        uint32_t VR_old = PRtoVR[PR];
                        VRtoPR[VR_old] = 65;
                        PRtoNU[PR] = def->NU;
                    } else { //spill
                        uint8_t maxPR = 0;
                        uint32_t maxNU = 0;
                        for(int i = 0; i < k - 1; i++){
                            if(PRtoNU[i] > maxNU){
                                maxPR = i;
                                maxNU = PRtoNU[i];
                            }
                        }
                        PR = maxPR; //pick PR to spill (PR with max PRtoNU) //will be more complex in the future
                        
                        //insert the loadI and store //WILL CHANGE LATER DO THIS FOR NOW
                        //                           //can delay inserting loadI and store if rematable rn
                        //                           //must store location of spill to be able to spill here if not rematable later
                        //                           //struct IR* VRtoSpillInsert, where loadI and store inserted as VRto...[VR]->next
                        //                           //on spill insertion, set VRto...[VR] to NULL

                        //create loadI
                        struct IR* newLoadI= malloc(sizeof(struct IR));
                        newLoadI->opcode = loadI;
                        (&(newLoadI->arg1))->PR = spill_loc;
                        (&(newLoadI->arg3))->PR = k - 1;

                        //create store
                        struct IR* newStore = malloc(sizeof(struct IR));
                        newStore->opcode = store;
                        (&(newStore->arg1))->PR = PR;
                        (&(newStore->arg3))->PR = k - 1;

                        //insert into ILOC
                        struct IR* prev = node->prev;
                        prev->next = newLoadI;
                        node->prev = newStore;
                        newLoadI->next = newStore;
                        newLoadI->prev = prev;
                        newStore->next = node;
                        newStore->prev = newLoadI;


                        //remove old values
                        uint32_t VR_old = PRtoVR[PR];
                        VRtoSpill[VR_old] = spill_loc;
                        spill_loc += 4;

                        VRNeedsRestore[VR_old] = 1;
                        VRtoPR[VR_old] = 65;
                        PRtoNU[PR] = def->NU;
                    }
            } else { //nonspillable getPR
                PR_stack_size--;
                PR = PR_stack[PR_stack_size];
            }

            if(save){
                //write VR and PR maps
                VRtoPR[VR] = PR;
                PRtoVR[PR] = VR;
            }

            //check if restore needed
            if(maxlive > k && VRNeedsRestore[VR]){
                //reset restore
                VRNeedsRestore[VR] = 0;
                
                //create loadI
                        struct IR* newLoadI= malloc(sizeof(struct IR));
                        newLoadI->opcode = loadI;
                        (&(newLoadI->arg1))->PR = VRtoSpill[VR];
                        (&(newLoadI->arg3))->PR = k - 1;

                        //create store
                        struct IR* newLoad = malloc(sizeof(struct IR));
                        newLoad->opcode = load;
                        (&(newLoad->arg1))->PR = k - 1;
                        (&(newLoad->arg3))->PR = PR;

                        //insert into ILOC
                        struct IR* prev = node->prev;
                        prev->next = newLoadI;
                        node->prev = newLoad;
                        newLoadI->next = newLoad;
                        newLoadI->prev = prev;
                        newLoad->next = node;
                        newLoad->prev = newLoadI;
            }

            def->PR = PR;
        }



        node = node->next;

        // //check maps
        // for(int i = 0; i < maxVR; i++){
        //     PR = VRtoPR[i];
        //     if(PR != 65){
        //         VR = PRtoVR[PR];
        //         if(VR != i){
        //             printf("YELL %i\n", i);
        //         }
        //     }
        //     int rest = VRNeedsRestore[i];
        //     if(rest){
        //         printf("Needs restoration: %i\n", i);
        //     }

        //     int spill = VRtoSpill[i];
        //     if(spill){
        //         printf("%i spilled at %i\n", i, spill);
        //     }
        // }
    }
    

}



        //PR = VRtoPR
        //if PR == 65
        //  if(maxlive > k)
        //      spillable getPR
        //      restore if VRtoSpill != 0
        //      if PR = k-1 do not PR to VR and VR to PR
        //  else
        //      nonspillable getPR
        //  PR = x
        //  VRtoPR = PR
        //  PRtoVR = u.VR
        //u.PR = PR

        //if u.isLU
        //  if(maxlive > k)
        //      spillable freePR/removePR
        //  else
        //      nonspillable freePR

        //get defs
        //if(maxlive > k)
        //  spillable getPR
        //else
        //  nonspillable getPR
        //PR = x
        //VRtoPR = PR
        //PRtoVR = d.VR
        //d.PR = PR






        //////spillable getPR
        //if unused def return k-1 //check using arg.NU
        //if undefed insert loadI 0 k-1 (once per operation) return k-1 //check using VRtoDef
        //if PR_stack_size > 0 //stuff in PR stack
        //  PR_stack_size--
        //  PR = PR_stack[PR_stack_size]
        //  PRtoNU[PR] = arg.NU
        //  return PR
        //
        //if PR_queue_start != PR_queue_end //stuff in removal queue
        //  PR = PR_queue_start
        //  PR_queue_start += 1
        //  if PR_queue_start >= PR_queue + k //make sure this math and check is correct //maintains queue in malloced space
        //      PR_queue_start = PR_queue
        //  VR_old = PRtoVR[PR]
        //  VRtoPR[VR_old] = 65
        //  PRtoNU[PR] = arg.NU
        //  return PR
        //  
        ////must need a spill
        //PR_spill = pick PR to spill (PR with max PRtoNU) //will be more complex in the future
        //VRtoSpill = spill_loc
        //spill_loc += 4
        //insert the loadI and store //WILL CHANGE LATER DO THIS FOR NOW
        //                           //can delay inserting loadI and store if rematable rn
        //                           //must store location of spill to be able to spill here if not rematable later
        //                           //struct IR* VRtoSpillInsert, where loadI and store inserted as VRto...[VR]->next
        //                           //on spill insertion, set VRto...[VR] to NULL
        //VR_old = PRtoVR[PR]
        //VRtoPR[VR_old] = 65
        //PRtoNU[PR] = arg.NU


        //restore
        //insert the loadI and load //WILL CHANGE LATER DO THIS FOR NOW
        //                          //check if rematable rn
        //                          //remat if yes
        //                          //insert spill at VRto...[VR]->next if no
        //                          //set VRto...[VR] to NULL


        //////spillable freePR
        //*PR_queue_end = PR
        //PR_queue_end++
        //if PR_queue_end >= PR_queue + k //check for correctness
        //  PR_queue_end = PR_queue
        


        //////nonspillable getPR
        //PR_stack_size--
        //PR = PR_stack[PR_stack_size]


        //////nonspillable freePR
        //PR_stack[PR_stack_size] = PR
        //stack_size++
        //PRtoVR[PR] = UINT32_MAX
        //VRtoPR = 65