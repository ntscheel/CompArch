#include <stdio.h>
#include <stdlib.h>
#define NUMMEM 65536
#define NUMREGS 8


typedef struct stateStruct {
    int pc;
    int mem[NUMMEM];
    int reg[NUMREGS];
    int numMemory;
} stateType;

void printState(stateType *statePtr);
void print_stats(int n_instrs);
int convertNum(int num);

int main(int argc, char *argv[]){
    int numInstructs = 0;
    stateType st;
    st.pc = 0;
    st.numMemory = 0;
    for(int k=0;k<NUMREGS;k++){
        st.reg[k] = 0;
    }

    if (argc == 1){
        printf("Error: Missing argument.\n");
        return -1;
    }
    else if (argc > 2){
        printf("Error: Too many arguments.\n");
        return -1;
    }
    else{ //Valid argument
        char const* const in_file = argv[1];
        FILE* file = fopen(in_file, "r");

        if(file != NULL) {
            int cur_cmd = 0;
            fscanf (file, "%d", &cur_cmd);
            while (!feof(file)){
                st.mem[st.numMemory] = cur_cmd;
                st.numMemory++;
                fscanf (file, "%d", &cur_cmd);
            }
            if(fscanf (file, "%d", &cur_cmd)){
                st.mem[st.numMemory] = cur_cmd;
                st.numMemory++;
            }
            fclose(file);

            int mask_111 = 7;
            int instruction = 0;
            st.pc = 0;
            while(st.pc < st.numMemory){ //Begin loop through all of mem until halt is hit
                numInstructs++;
                instruction = st.mem[st.pc];
                printState(&st);

                //Mask command into separate vars
                int op = instruction >> 22;
                int regA = (instruction>>19) & mask_111;
                int regB = (instruction>>16) & mask_111;
                int dest = instruction & mask_111;
                int offset = instruction & (65535);
                if(offset > 32767){
                    offset = convertNum(offset);
                }
                //printf("op: %d, regA: %d, regB: %d, dest: %d, offset: %d\n",op,regA,regB,dest,offset);

                if(op<2){ //r type
                    if(op == 0){ //add
                        st.reg[dest] = st.reg[regA] + st.reg[regB];
                    }else{ //nand
                        st.reg[dest] = ~(st.reg[regA] & st.reg[regB]);
                    }
                }else if(op<5){ //I type
                    if(op==2){ //lw
                        st.reg[regA] = st.mem[(st.reg[regB] + offset)];

                    }else if(op==3){ //sw
                        st.mem[(st.reg[regB] + offset)] = st.reg[regA];
                    }else{ //beq
                        if(st.reg[regA] == st.reg[regB]){
                            st.pc = st.pc + offset;
                        }
                    }
                }else if(op<6){ //J type
                    //must be jalr
                    st.reg[regA] = st.pc;
                    st.pc = offset;
                }else{ //O type
                    if(op==6){ //halt
                        print_stats(numInstructs);
                        return 0;
                    }else{ //noop
                        //Do nothing
                    }
                }
                st.pc++;
            }//end loop through st.mem
            print_stats(numInstructs);
        }else{
            printf("Error: file '%s' could not be read.\n", in_file);
            return -1;
        }
    }
    return 0;
}

//Begin helper functions

void printState(stateType *statePtr){
    int i;
    printf("\n@@@\nstate:\n");
    printf("\tpc %d\n", statePtr->pc);
    printf("\tmemory:\n");
    for(i = 0; i < statePtr->numMemory; i++){
        printf("\t\tmem[%d]=%d\n", i, statePtr->mem[i]);
    }
    printf("\tregisters:\n");
    for(i = 0; i < NUMREGS; i++){
        printf("\t\treg[%d]=%d\n", i, statePtr->reg[i]);
    }
    printf("end state\n");
}

void print_stats(int n_instrs){
    printf("INSTRUCTIONS: %d\n",n_instrs);
}

int convertNum(int num){
    if(num & (1<<15)){
        num -= (1<<16);
    }
    return num;
}