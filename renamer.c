#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "constants.h"

extern struct IR* head;

//renamer variables
struct IR** VRtoDef;



uint8_t* isVRStoreUse2;
uint8_t* VRHasUse;
int updateMaps;

    //declares maps used in maxlive > k cases
    uint32_t* VRtoSpill;
    uint32_t spill_loc = 32768;

    struct IR** VRtoRemat;
    uint8_t* VRtoOldPR;

    uint32_t* PRtoNU;

    uint8_t* PR_queue; //removables array
    uint8_t* PR_queue_start; //actually used as pointer not array
    uint8_t* PR_queue_end; //actually used as pointer not array
    uint8_t PR_queue_size;

        ////creates maps that are always used
    //make usable PR stack
    uint8_t* PR_stack;
    uint8_t PR_stack_size;

    //make PR to VR
    uint32_t* PRtoVR;

    //make VR to PR
    uint8_t* VRtoPR;

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

                //updateMaps the VR def
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

                //updateMaps the VR def
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

                //updateMaps the VR def
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


    //realloc
    VRtoDef = realloc(VRtoDef, sizeof(struct IR*) * VRName);

    //pass values
    *maxVR = VRName;
    *maxlive_ptr = maxlive;
    return 0;
}

uint8_t isRematable(uint32_t VR, uint8_t* VRtoPR){
    struct IR* node = VRtoDef[VR];

    //NULL check
    if(node == NULL){
        return 0;
    }


    //declare vars used for while
    struct argument* use1 = NULL;
    struct argument* use2 = NULL;

        //grab the use and def args where applicable 
    switch(node->opcode){
            case load:
                use1 = &(node->arg1);
                //return 0; //UNDER INVESTIGATION
                //NONTRIVIAL CLEAN/DIRTY DISTINGUISHMENT
                //perhaps mark if VR is a use2 in a store op
                if(isVRStoreUse2[use1->VR]){
                    return 0;
                }
                break;
            case store:
                use1 = &(node->arg1);
                use2 = &(node->arg3);
                break;
            case loadI:
                //always rematable
                return 2;
                break;
            case add:
            case sub:
            case mult:
            case lshift:
            case rshift:
                use1 = &(node->arg1);
                use2 = &(node->arg2);
                break;
            case output:
                break;
            case nop:
                break;
            default:
                break;
        
    }

    //check if both uses are still held in PRs
    if(use1 != NULL && VRtoPR[use1->VR] == 65){
        return 0;
    }

    if(use2 != NULL && VRtoPR[use2->VR] == 65){
        return 0;
    }

    //if so it is rematable
    return 1;
}

//copies opcode and VR only, inserts correct PR
void copy(struct IR* new, struct IR* old, uint8_t* VRtoPR){
    //copy opcode
    new->opcode = old->opcode;

    //copy VR
    new->arg1.VR = old->arg1.VR;
    new->arg2.VR = old->arg2.VR;
    new->arg3.VR = old->arg3.VR;

    //get PR based on opcode
    switch(new->opcode){
        case loadI: 
            new->arg3.PR = VRtoPR[old->arg3.VR];
        case output:
            //propagate constant
            new->arg1.PR = old->arg1.VR;
            break;
        case add:
        case sub:
        case mult:
        case rshift:
        case lshift:
            new->arg2.PR = VRtoPR[old->arg2.VR];
            //printf("new arg2 PR: %i\n", new->arg2.PR);
        case load:
        case store:
            new->arg1.PR = VRtoPR[old->arg1.VR];
            //printf("new arg1 PR: %i\n", new->arg1.PR);
            new->arg3.PR = VRtoPR[old->arg3.VR];
            //printf("new arg3 PR: %i\n", new->arg3.PR);
        default:
            break;
    }
}

void spill(struct IR* prev, uint8_t PR, uint32_t VR, uint8_t k){
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
    struct IR* next = prev->next;
    prev->next = newLoadI;
    next->prev = newStore;
    newLoadI->next = newStore;
    newLoadI->prev = prev;
    newStore->next = next;
    newStore->prev = newLoadI;


    //set spill location
    VRtoSpill[VR] = spill_loc;
    spill_loc += 4;
}


void restore(struct IR* prev, uint8_t PR, uint32_t VR, uint8_t k, uint32_t index){
    struct IR* newLoadI= malloc(sizeof(struct IR));
    newLoadI->opcode = loadI;
    (&(newLoadI->arg1))->PR = VRtoSpill[VR];
    (&(newLoadI->arg3))->PR = k - 1;
                        
    //create store
    struct IR* newLoad = malloc(sizeof(struct IR));
    newLoad->opcode = load;
    (&(newLoad->arg1))->PR = k - 1;
    (&(newLoad->arg3))->PR = PR;
    PRtoNU[PR] = index; //should be next line, maybe keep index

    //insert into ILOC
    struct IR* next = prev->next;
    prev->next = newLoadI;
    next->prev = newLoad;
    newLoadI->next = newLoad;
    newLoadI->prev = prev;
    newLoad->next = next;
    newLoad->prev = newLoadI;
}

uint8_t get_a_PR(struct IR* node, struct argument* arg, uint32_t maxlive, uint8_t k, uint8_t markedPR){
    uint8_t PR;

    //assign PR with algorithm based on maxlive and k
    if(maxlive > k){ //spillable getPR
        //check if undefed use
        if(arg->NU == 0){
            //define loadI 0 => r(k-1)
            struct IR* newLoadI= malloc(sizeof(struct IR));
            newLoadI->opcode = loadI;
            (&(newLoadI->arg1))->PR = 0;
            (&(newLoadI->arg3))->PR = k - 1;

            //insert 0'ed value into ILOC
            struct IR* prev = node->prev;
            prev->next = newLoadI;
            node->prev = newLoadI;
            newLoadI->next = node;
            newLoadI->prev = prev;

            //do not update maps, reserve used
            updateMaps = 0;

            //return  the reserve PR
            PR = k - 1;
        } else if(PR_stack_size > 0){ //grab from stack if possible
            PR_stack_size--;
            PR = PR_stack[PR_stack_size];
            PRtoNU[PR] = arg->NU;
        } else if(PR_queue_start != PR_queue_end){ //grab from queue if possible
            PR = *PR_queue_start;
            PR_queue_start += 1;

            //maintain queue bounds
            if(PR_queue_start >= PR_queue + k){ //make sure this math and check is correct
                PR_queue_start = PR_queue;
            } 

            //remove old value, since queue is lazy removal
            uint32_t VR_old = PRtoVR[PR];
            VRtoPR[VR_old] = 65;
            PRtoNU[PR] = arg->NU;
        } else { //spill
            //values needed to PR selection
            uint8_t maxDirtyPR = 65;
            uint32_t maxDirtyNU = 0;

            uint8_t minRematPR = 65;
            uint32_t minRematNU = UINT32_MAX;

            uint8_t maxCleanPR = 65;
            uint32_t maxCleanNU = 0;

            uint8_t maxLoadIPR = 65;
            uint32_t maxLoadINU = 0;

            //choose which PR to spill
            for(int i = 0; i < k - 1; i++){
                //ignore markedPR
                if(i == markedPR){
                    continue;
                }

                //get farthest spill
                if(PRtoNU[i] > maxDirtyNU && VRtoSpill[PRtoVR[i]] == 0){
                    maxDirtyPR = i;
                    maxDirtyNU = PRtoNU[i];
                }

                //get farthest clean spill
                if(PRtoNU[i] > maxCleanNU && VRtoSpill[PRtoVR[i]] != 0){
                    maxCleanPR = i;
                    maxCleanNU = PRtoNU[i];
                }

                //get farthest loadI
                if(PRtoNU[i] > maxLoadINU && isRematable(PRtoVR[i], VRtoPR) == 2){
                    maxLoadIPR = i;
                    maxLoadINU = PRtoNU[i];
                }

                // //get farthest remat
                // if(PRtoNU[i] > maxRematNU && isRematable(PRtoVR[i], VRtoPR)){
                //     maxRematPR = i;
                //     maxRematNU = PRtoNU[i];
                // }


                //get closest remat
                if(PRtoNU[i] < minRematNU && isRematable(PRtoVR[i], VRtoPR)){
                    minRematNU = PRtoNU[i];
                    minRematPR = i;
                }
            }

            //spill furthest PR or closest rematerializable PR
            PR = maxDirtyPR == 65 ? maxCleanPR : maxDirtyPR;
            // if(minRematPR != 65){
            //     PR = minRematPR;
            // } 

            //get clean PR if more than 75% dirty PR
            if(maxDirtyNU - arg->NU < 4 / 3 * (maxCleanNU - arg->NU) && maxCleanPR != 65){
                PR = maxCleanPR;
            }

            //get remat if remat < 50% dirty PR
            if(maxDirtyNU - arg->NU > 2 * (minRematNU - arg->NU) && minRematPR != 65){
                PR = minRematPR;
            }

            //get loadI if loadI > 50% dirty PR
            if(maxDirtyNU - arg->NU < 2 * (maxLoadINU - arg->NU) && maxLoadIPR != 65){
                PR = maxLoadIPR;
            }

            //get VR that is currently sitting in the PR (due to laziness of queue)
            uint32_t VR_old = PRtoVR[PR];

            ///spill insertion
            //check if rematerializable
            if(minRematPR != 65){
                //save spill location in case remateralization does not work later
                VRtoRemat[VR_old] = node->prev;
                VRtoOldPR[VR_old] = PR;
            } else if(VRtoSpill[VR_old] == 0){ //check if value was already spilled
                //dirty spill
                spill(node->prev, PR, VR_old, k);
            }

            //remove old values
            VRtoPR[VR_old] = 65;
            PRtoNU[PR] = arg->NU;
        }
    } else { //nonspillable getPR
        //get PR from stack
        PR_stack_size--;
        PR = PR_stack[PR_stack_size];
    }

    return PR;
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

    //initialization
        ////creates maps that are always used
        //make usable PR stack
        PR_stack = malloc(sizeof(uint8_t) * k);
        PR_stack_size = k;

        //make PR to VR
        PRtoVR = malloc(sizeof(uint32_t) * k);

        //make VR to PR
        VRtoPR = malloc(sizeof(uint8_t) * maxVR);

        VRHasUse = malloc(sizeof(uint8_t) * maxVR);

        //null checks
        if(PR_stack == NULL || PRtoVR == NULL || VRtoPR == NULL || VRHasUse == NULL){
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

        //initialize if maxlive > k
        if(maxlive > k){
            //reserve a register
            PR_stack_size--; 
            PR_stack++;

            //initialize arrays
            VRtoSpill = malloc(sizeof(uint32_t) * maxVR);
            VRtoRemat = malloc(sizeof(struct IR*) * maxVR);
            VRtoOldPR = malloc(sizeof(uint8_t) * maxVR);
            isVRStoreUse2 = malloc(sizeof(uint8_t) * maxVR);
            PRtoNU = malloc(sizeof(uint32_t) * k);
            PR_queue = malloc(sizeof(uint8_t) * k);

            //initialize pointers
            PR_queue_start = PR_queue;
            PR_queue_end = PR_queue;

            //null checks
            if(VRtoSpill == NULL || VRtoRemat == NULL || VRtoOldPR == NULL || isVRStoreUse2 == NULL || PRtoNU == NULL || PR_queue == NULL){
                return -1;
            }

        }


        //declare vars used for while
        struct argument* use1;
        struct argument* use2;
        struct argument* def;

        //PR and VR of the arguments
        uint8_t PR;
        uint32_t VR;

        //Indicators used for decision making
        int VRNeedsPR;
        uint8_t markedPR;
    

    //printf("max: %i\n", maxlive);

    //go thru all ops in order
    uint32_t index = 0;
    struct IR* node = head->next;
    while(node != head){
        //set args to NULL
        use1 = NULL;
        use2 = NULL;
        def = NULL;

        //grab the use and def args where applicable 
        switch(node->opcode){
            case store:
                use1 = &(node->arg1);
                use2 = &(node->arg3);
                break;
            case loadI:
                def = &(node->arg3);
            case output:
                //propagate constant
                use1 = &(node->arg1);
                use1->PR = use1->VR;
                use1 = NULL;
                break;
            case add:
            case sub:
            case mult:
            case lshift:
            case rshift:
                use2 = &(node->arg2);
            case load:
                use1 = &(node->arg1);
                def = &(node->arg3);
                break;
            case nop:
                break;
            default:
                break;
        
        }


        //set marker that prevents use2 from spilling use1
        markedPR = 65; 

        ////set PRs
        //set PR for use1
        if(use1 != NULL){
            //sets indicators for actions later
            updateMaps = 1;
            VRNeedsPR = 0;

            //grabs VR and PR
            VR = use1->VR;
            PR = VRtoPR[VR];

            //no PR assigned
            if(PR == 65){
                //sets indicator
                VRNeedsPR = 1;

                //gets a pr
                PR = get_a_PR(node, use1, maxlive, k, markedPR);
            
                //update maps if reserve register wasnt used
                if(updateMaps){
                    //write VR and PR maps
                    VRtoPR[VR] = PR;
                    PRtoVR[PR] = VR;
                }
            }

            //check if restore needed
            if(maxlive > k && (VRtoSpill[VR] || VRtoRemat[VR] != NULL) && VRNeedsPR){
                //check remat
                if(VRtoRemat[VR] != NULL){
                    if(isRematable(VR, VRtoPR)){
                        //rematerialize
                        struct IR* newDef = malloc(sizeof(struct IR));
                        copy(newDef, VRtoDef[VR], VRtoPR);

                        //insert before node
                        node->prev->next = newDef;
                        newDef->prev = node->prev;
                        newDef->next = node;
                        node->prev = newDef;
                        
                        //remove the spill location
                        VRtoRemat[VR] = NULL;

                        //remove original if possible
                        if(newDef->opcode == loadI && !VRHasUse[VR]){
                            VRtoDef[VR]->next->prev = VRtoDef[VR]->prev;
                            VRtoDef[VR]->prev->next = VRtoDef[VR]->next;
                        }
                    } else { //must insert the spill
                        spill(VRtoRemat[VR], VRtoOldPR[VR], VR, k);
                        restore(node->prev, PR, VR, k, index);
                    }
                } else { //else do current restore
                    restore(node->prev, PR, VR, k, index);
                }
            }

            //indicate use
            VRHasUse[VR] = 1;

            //sets PR where needed
            use1->PR = PR;
            markedPR = PR;
        }

        //set PR for use2
        if(use2 != NULL){
            //sets indicators for actions later
            updateMaps = 1;
            VRNeedsPR = 0;

            //grabs VR and PR
            VR = use2->VR;
            PR = VRtoPR[VR];

                        //no PR assigned
            if(PR == 65){
                //sets indicator
                VRNeedsPR = 1;

                //gets a pr
                PR = get_a_PR(node, use1, maxlive, k, markedPR);
            
                //update maps if reserve register wasnt used
                if(updateMaps){
                    //write VR and PR maps
                    VRtoPR[VR] = PR;
                    PRtoVR[PR] = VR;
                }
            }

            //check if restore needed
            if(maxlive > k && (VRtoSpill[VR] || VRtoRemat[VR] != NULL) && VRNeedsPR){
                //check remat
                if(VRtoRemat[VR] != NULL){
                    if(isRematable(VR, VRtoPR)){
                        //rematerialize
                        //rematerialize
                        struct IR* newDef = malloc(sizeof(struct IR));
                        copy(newDef, VRtoDef[VR], VRtoPR);

                        //insert before node
                        node->prev->next = newDef;
                        newDef->prev = node->prev;
                        newDef->next = node;
                        node->prev = newDef;
                        
                        //remove the spill location
                        VRtoRemat[VR] = NULL;
                    } else { //must insert the spill
                        spill(VRtoRemat[VR], VRtoOldPR[VR], VR, k);
                        restore(node->prev, PR, VR, k, index);
                    }
                } else { //else do current restore
                    restore(node->prev, PR, VR, k, index);
                }
            }

            //save that VR was a Store use 2 for rematerialization purposes
            if(maxlive > k && node->opcode == store){
                isVRStoreUse2[VR] = 1;
            }

            //indicate use
            VRHasUse[VR] = 1;

            //sets PR
            use2->PR = PR;
        }


        ///free PR if last use
        //free use1
        if(use1 != NULL){
            //check for last use
            if(use1->isLU){
                //reobtain VR and PR
                VR = use1->VR;
                PR = use1->PR;

                //free algorithm based on maxlive and k
                if(maxlive > k){ //spillable free
                    //place PR in queue but do not remove its contents
                    *PR_queue_end = PR;
                    PR_queue_end++;
                    if(PR_queue_end >= PR_queue + k){ //check for correctness
                        PR_queue_end = PR_queue;
                    } 
                } else { //nonspillable free
                    //immediately place in stack again
                    PR_stack[PR_stack_size] = PR;
                    PR_stack_size++;
                    PRtoVR[PR] = UINT32_MAX;
                    VRtoPR[VR] = 65;
                }
            }
        }

        //free use2
        if(use2 != NULL){
            //check for last use
            if(use2->isLU){
                //reobtain VR and PR
                VR = use2->VR;
                PR = use2->PR;
                
                //free algorithm based on maxlive and k
                if(maxlive > k){ //spillable free
                    //place PR in queue but do not remove its contents
                    *PR_queue_end = PR;
                    PR_queue_end++;
                    if(PR_queue_end >= PR_queue + k){ //check for correctness
                        PR_queue_end = PR_queue;
                    } 
                } else { //nonspillable free
                    //immediately place in stack again
                    PR_stack[PR_stack_size] = PR;
                    PR_stack_size++;
                    PRtoVR[PR] = UINT32_MAX;
                    VRtoPR[VR] = 65;
                }
            }
        }


        //PRs do not need to be marked for def
        markedPR = 65;

        //get PR for def
        if(def != NULL){
            //sets indicators for actions later
            updateMaps = 1;

            //grabs VR and PR
            VR = def->VR;
            PR = VRtoPR[VR];

            //assign PR with algorithm based on maxlive and k
            PR = get_a_PR(node, def, maxlive, k, markedPR);

            //update maps if reserve register wasnt used
            if(updateMaps){
                //write VR and PR maps
                VRtoPR[VR] = PR;
                PRtoVR[PR] = VR;
            }

            //sets PR
            def->PR = PR;
        }

        //next operation
        node = node->next;
        index++;
    }
}