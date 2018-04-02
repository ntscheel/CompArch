#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUMMEMORY 65536 /* maximum number of data words in memory */
#define NUMREGS 8 /* number of machine registers */

#define ADD 0
#define NAND 1
#define LW 2
#define SW 3
#define BEQ 4
#define JALR 5 /* JALR – not implemented in this project */
#define HALT 6
#define NOOP 7

#define NOOPINSTRUCTION 0x1c00000

typedef struct IFIDstruct{
    int instr;
    int pcPlus1;
} IFIDType;

typedef struct IDEXstruct{
    int instr;
    int pcPlus1;
    int readRegA;
    int readRegB;
    int offset;
} IDEXType;

typedef struct EXMEMstruct{
    int instr;
    int branchTarget;
    int aluResult;
    int readReg;
} EXMEMType;

typedef struct MEMWBstruct{
    int instr;
    int writeData;
} MEMWBType;

typedef struct WBENDstruct{
    int instr;
    int writeData;
} WBENDType;

typedef struct statestruct{
    int pc;
    int instrMem[NUMMEMORY];
    int dataMem[NUMMEMORY];
    int reg[NUMREGS];
    int numMemory;
    IFIDType IFID;
    IDEXType IDEX;
    EXMEMType EXMEM;
    MEMWBType MEMWB;
    WBENDType WBEND;
    int cycles; /* Number of cycles run so far */
    int fetched; /* Total number of instructions fetched */
    int retired; /* Total number of completed instructions */
    int branches; /* Total number of branches executed */
    int mispreds; /* Number of branch mispredictions*/
} stateType;

void printState(stateType *statePtr);
void print_stats(int n_instrs);
int signExtend(int num);
int field0(int instruction);
int field1(int instruction);
int field2(int instruction);
int opcode(int instruction);

void main(int argc, char *argv[]){
    stateType state, newState;

    //Initialize PC = 0 and all pipeline register instructions to NOOP
    state.pc = 0;
    state.numMemory = 0;
    state.fetched = 0;
    state.branches = 0;
    state.mispreds = 0;
    state.retired = 0;
    state.IFID.instr = NOOPINSTRUCTION;
    state.IDEX.instr = NOOPINSTRUCTION;
    state.EXMEM.instr = NOOPINSTRUCTION;
    state.MEMWB.instr = NOOPINSTRUCTION;
    state.WBEND.instr = NOOPINSTRUCTION;

    for(int k=0;k<NUMREGS;k++){
        state.reg[k] = 0;
    }

    if (argc == 1){
        printf("Error: Missing argument.\n");
        exit(-1);
    }
    else if (argc > 2){
        printf("Error: Too many arguments.\n");
        exit(-1);
    }
    else { //Valid argument
        char const *const in_file = argv[1];
        FILE *file = fopen(in_file, "r");

        if (file != NULL) {
            int cur_cmd = 0;
            fscanf(file, "%d", &cur_cmd);
            while (!feof(file)) {
                state.instrMem[state.numMemory] = cur_cmd;
                state.dataMem[state.numMemory] = cur_cmd;
                state.numMemory++;
                fscanf(file, "%d", &cur_cmd);
            }
            if (fscanf(file, "%d", &cur_cmd)) {
                state.instrMem[state.numMemory] = cur_cmd;
                state.dataMem[state.numMemory] = cur_cmd;
                state.numMemory++;
            }
            fclose(file);

            while (1) {
                printState(&state);
                /*
                 * //Step-through for testing
                 * printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
                 * getchar();
                 */


                /* check for halt */
                if (HALT == state.MEMWB.instr >> 22) {
                    printf("machine halted \n");
                    printf("total of %d cycles executed\n", state.cycles);
                    printf("total of %d instructions fetched\n", state.fetched);
                    printf("total of %d instructions retired\n", state.retired);
                    printf("total of %d branches executed\n", state.branches);
                    printf("total of %d branch mispredictions\n", state.mispreds);
                    exit(0);
                }
                newState = state;
                newState.cycles++;

/*------------------ IF stage ----------------- */
                newState.IFID.instr = state.instrMem[state.pc];
                newState.fetched++;
                newState.pc = state.pc + 1;
                newState.IFID.pcPlus1 = state.pc + 1;

/*------------------ ID stage ----------------- */
                newState.IDEX.instr = state.IFID.instr;
                newState.IDEX.pcPlus1 = state.IFID.pcPlus1;
                int IDregA = (newState.IDEX.instr >> 19) & (7);
                int IDregB = (newState.IDEX.instr >> 16) & (7);
                int IDoffset = newState.IDEX.instr & (65535);
                if (IDoffset > 32767) {
                    IDoffset = signExtend(IDoffset);
                }

                newState.IDEX.readRegA = state.reg[IDregA];
                newState.IDEX.readRegB = state.reg[IDregB];
                newState.IDEX.offset = IDoffset;

                /*
                 * If there is a LW followed by an instruction that needs that value,
                 * add a single stall (NOOP)
                 */
                int IDop = newState.IDEX.instr >> 22;
                if(IDop == LW){
                    int IFregA = (newState.IFID.instr >> 19) & (7);
                    int IFregB = (newState.IFID.instr >> 16) & (7);
                    if(IDregA == IFregA || IDregA == IFregB){
                        //Insert bubbles
                        newState.IFID.instr = NOOPINSTRUCTION;
                        newState.pc--;
                    }
                }


/*------------------ EX stage ----------------- */
                newState.EXMEM.instr = state.IDEX.instr;
                newState.EXMEM.branchTarget = state.IDEX.pcPlus1 + state.IDEX.offset;
                newState.EXMEM.readReg = state.IDEX.readRegA;

                int EXop = newState.EXMEM.instr >> 22;
                int EXregA = (newState.EXMEM.instr >> 19) & (7);
                int EXregB = (newState.EXMEM.instr >> 16) & (7);
                int EXregAval = state.IDEX.readRegA;
                int EXregBval = state.IDEX.readRegB;
                int EXoffset = state.IDEX.offset;

                //---------------------------------------------------------------
                //BEGIN DATA HAZARD CHECKING
                //Check instruction in WBEND
                int stateWBENDop = state.WBEND.instr >> 22;
                int stateWBENDdest = state.WBEND.instr & 7;
                if(EXop < JALR && stateWBENDop < SW){
                    if(stateWBENDop == LW || stateWBENDop == SW){
                        stateWBENDdest = (state.WBEND.instr >> 19) & 7;
                    }
                    if(stateWBENDdest == EXregA){
                        //printf("Data forwarding hazard hit on regA, %d in WBEND.\n", EXregA);
                        EXregAval = state.WBEND.writeData;
                    }
                    if(stateWBENDdest == EXregB){
                        //printf("Data forwarding hazard hit on regB, %d in WBEND.\n", EXregB);
                        EXregBval = state.WBEND.writeData;
                    }
                }
                //Check instruction in MEMWB
                int stateMEMWBop = state.MEMWB.instr >> 22;
                int stateMEMBWdest = state.MEMWB.instr & 7;
                if(EXop < JALR && stateMEMWBop < SW){
                    if(stateMEMWBop == LW || stateMEMWBop == SW){
                        stateMEMBWdest = (state.MEMWB.instr >> 19) & 7;
                    }
                    if(stateMEMBWdest == EXregA){
                        //printf("Data forwarding hazard hit on regA, %d in MEMWB.\n", EXregA);
                        EXregAval = state.MEMWB.writeData;
                    }
                    if(stateMEMBWdest == EXregB){
                        //printf("Data forwarding hazard hit on regB, %d in MEMWB.\n", EXregB);
                        EXregBval = state.MEMWB.writeData;
                    }
                }
                //Check instruction in EXMEM
                int stateEXMEMop = state.EXMEM.instr >> 22;
                int stateEXMEMdest = state.EXMEM.instr & 7;
                if(EXop < JALR && stateEXMEMop < SW){
                    if(stateEXMEMop == LW || stateEXMEMop == SW){
                        stateEXMEMdest = (state.EXMEM.instr >> 19) & 7;
                    }
                    if(stateEXMEMdest == EXregA){
                        //printf("Data forwarding hazard hit on regA, %d in EXMEM.\n", EXregA);
                        EXregAval = state.EXMEM.aluResult;
                    }
                    if(stateEXMEMdest == EXregB){
                        //printf("Data forwarding hazard hit on regB, %d in EXMEM.\n", EXregB);
                        EXregBval = state.EXMEM.aluResult;
                    }
                }

                //---------------------------------------------------------------


                // Perform operation
                if (EXop < LW) { //R type
                    if (EXop == ADD) {
                        newState.EXMEM.aluResult = EXregAval + EXregBval;
                    } else { //NAND
                        newState.EXMEM.aluResult = ~(EXregAval & EXregBval);
                    }
                } else if (EXop < JALR) { //I type
                    if (EXop == LW || EXop == SW) {
                        newState.EXMEM.readReg = EXregAval;
                        newState.EXMEM.aluResult = EXregBval + EXoffset;
                    } else { //BEQ
                        if(EXregAval == EXregBval){
                            newState.EXMEM.aluResult = 0;
                        }else{
                            newState.EXMEM.aluResult = EXregAval - EXregBval;
                        }
                    }
                } else { //O type & J Type
                    // Do nothing because op is JALR (which we don't support), HALT, or NOOP
                }

/*------------------ MEM stage ----------------- */
                newState.MEMWB.instr = state.EXMEM.instr;
                int MEMop = newState.MEMWB.instr >> 22;
                if (MEMop == LW) {
                    newState.MEMWB.writeData = state.dataMem[state.EXMEM.aluResult];
                }else{
                    newState.MEMWB.writeData = state.EXMEM.aluResult;
                }
                if (MEMop == SW) {
                    newState.dataMem[state.EXMEM.aluResult] = state.EXMEM.readReg;
                }if(MEMop == BEQ){
                    newState.branches++;
                    if(newState.MEMWB.writeData == 0){
                        newState.mispreds++;
                        newState.pc = state.EXMEM.branchTarget;

                        newState.IFID.instr = NOOPINSTRUCTION;
                        newState.IDEX.instr = NOOPINSTRUCTION;
                        newState.EXMEM.instr = NOOPINSTRUCTION;
                        newState.MEMWB.instr = NOOPINSTRUCTION;
                    }else{
                        //branch not taken
                    }
                }

/*------------------ WB stage ----------------- */
                newState.WBEND.instr = state.MEMWB.instr;
                newState.WBEND.writeData = state.MEMWB.writeData;
                int WBop = newState.WBEND.instr >> 22;
                if(WBop < LW){
                    int WBdest = newState.WBEND.instr & 7;
                    newState.reg[WBdest] = newState.WBEND.writeData;
                    //printf("Writing %d to register %d\n", newState.WBEND.writeData, WBdest);
                }
                if (WBop == LW) {
                    int WBregA = (newState.WBEND.instr >> 19) & (7);
                    newState.reg[WBregA] = newState.WBEND.writeData;
                    //printf("Loading %d to register %d\n", newState.WBEND.writeData, WBregA);
                }

                newState.retired++;
                state = newState; /* this is the last statement before the end of the loop.
                                     It marks the end of the cycle and updates the current
                                     state with the values calculated in this cycle
                                     – AKA “Clock Tick”. */
            }
        }
    }

}


int field0(int instruction){
    return( (instruction>>19) & 0x7);
}
int field1(int instruction){
    return( (instruction>>16) & 0x7);
}
int field2(int instruction){
    return(instruction & 0xFFFF);
}
int opcode(int instruction){
    return(instruction>>22);
}
void printInstruction(int instr) {
    char opcodeString[10];
    if (opcode(instr) == ADD) {
        strcpy(opcodeString, "add");
    } else if (opcode(instr) == NAND) {
        strcpy(opcodeString, "nand");
    } else if (opcode(instr) == LW) {
        strcpy(opcodeString, "lw");
    } else if (opcode(instr) == SW) {
        strcpy(opcodeString, "sw");
    } else if (opcode(instr) == BEQ) {
        strcpy(opcodeString, "beq");
    } else if (opcode(instr) == JALR) {
        strcpy(opcodeString, "jalr");
    } else if (opcode(instr) == HALT) {
        strcpy(opcodeString, "halt");
    } else if (opcode(instr) == NOOP) {
        strcpy(opcodeString, "noop");
    } else {
        strcpy(opcodeString, "data");
    }
    if(opcode(instr) == ADD || opcode(instr) == NAND){
        printf("%s %d %d %d\n", opcodeString, field2(instr), field0(instr), field1(instr));
    }else if(0 == strcmp(opcodeString, "data")){
        printf("%s %d\n", opcodeString, signExtend(field2(instr)));
    }else{
        printf("%s %d %d %d\n", opcodeString, field0(instr), field1(instr),
               signExtend(field2(instr)));
    }
}
void printState(stateType *statePtr){
    int i;
    printf("\n@@@\nstate before cycle %d starts\n", statePtr->cycles);
    printf("\tpc %d\n", statePtr->pc);
    printf("\tdata memory:\n");
    for (i=0; i<statePtr->numMemory; i++) {
        printf("\t\tdataMem[ %d ] %d\n", i, statePtr->dataMem[i]);
    }
    printf("\tregisters:\n");
    for (i=0; i<NUMREGS; i++) {
        printf("\t\treg[ %d ] %d\n", i, statePtr->reg[i]);
    }
    printf("\tIFID:\n");
    printf("\t\tinstruction ");
    printInstruction(statePtr->IFID.instr);
    printf("\t\tpcPlus1 %d\n", statePtr->IFID.pcPlus1);
    printf("\tIDEX:\n");
    printf("\t\tinstruction ");
    printInstruction(statePtr->IDEX.instr);
    printf("\t\tpcPlus1 %d\n", statePtr->IDEX.pcPlus1);
    printf("\t\treadRegA %d\n", statePtr->IDEX.readRegA);
    printf("\t\treadRegB %d\n", statePtr->IDEX.readRegB);
    printf("\t\toffset %d\n", statePtr->IDEX.offset);
    printf("\tEXMEM:\n");
    printf("\t\tinstruction ");
    printInstruction(statePtr->EXMEM.instr);
    printf("\t\tbranchTarget %d\n", statePtr->EXMEM.branchTarget);
    printf("\t\taluResult %d\n", statePtr->EXMEM.aluResult);
    printf("\t\treadReg %d\n", statePtr->EXMEM.readReg);
    printf("\tMEMWB:\n");
    printf("\t\tinstruction ");
    printInstruction(statePtr->MEMWB.instr);
    printf("\t\twriteData %d\n", statePtr->MEMWB.writeData);
    printf("\tWBEND:\n");
    printf("\t\tinstruction ");
    printInstruction(statePtr->WBEND.instr);
    printf("\t\twriteData %d\n", statePtr->WBEND.writeData);
}

int signExtend(int num){
    if(num & (1<<15)){
        num -= (1<<16);
    }
    return num;
}