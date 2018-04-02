#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#define NUMMEMORY 65536 /* maximum number of data words in memory */
#define NUMREGS 8 /* number of machine registers */

#define ADD 0
#define NAND 1
#define LW 2
#define SW 3
#define BEQ 4
#define JALR 5
#define HALT 6
#define NOOP 7

#define NOOPINSTRUCTION 0x1c00000
typedef struct BlockStruct {
    int tag;
    int valid;
    int dirty;
    int* words;
} block;
typedef struct stateStruct {
    int pc;
    int mem[NUMMEMORY];
    int reg[NUMREGS];
    int numMemory;
    block **cache;
    int **lruTable;
    int associativity;
    int blockSizeInWords;
    int numberOfSets;
} stateType;

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

/*
* Log the specifics of each cache action.
*
* address is the starting word address of the range of data being transferred.
* size is the size of the range of data being transferred.
* type specifies the source and destination of the data being transferred.
*
* cacheToProcessor: reading data from the cache to the processor
* processorToCache: writing data from the processor to the cache
* memoryToCache: reading data from the memory to the cache
* cacheToMemory: evicting cache data by writing it to the memory
* cacheToNowhere: evicting cache data by throwing it away
*/
enum actionType {cacheToProcessor, processorToCache, memoryToCache, cacheToMemory,
    cacheToNowhere};
void printAction(int address, int size, enum actionType type)
{
    printf("transferring word [%i-%i] ", address, address + size - 1);
    if (type == cacheToProcessor) {
        printf("from the cache to the processor\n");
    } else if (type == processorToCache) {
        printf("from the processor to the cache\n");
    } else if (type == memoryToCache) {
        printf("from the memory to the cache\n");
    } else if (type == cacheToMemory) {
        printf("from the cache to the memory\n");
    } else if (type == cacheToNowhere) {
        printf("from the cache to nowhere\n");
    }
}

int signExtend(int num){
    // convert a 16-bit number into a 32-bit integer
    if (num & (1<<15) ) {
        num -= (1<<16);
    }
    return num;
}

int isPowerOfTwo(int x)
{
    if((x != 0) && ((x & (x - 1)) == 0)){
        return 1;
    }else{
        return 0;
    }
}

void updateLRUTable(stateType* state, int set, int lru){
    int temp = state->lruTable[set][lru];
    for (int i = lru; i < state->associativity-1; i++) {
        state->lruTable[set][i] = state->lruTable[set][i+1];
    }
    state->lruTable[set][state->associativity-1] = temp;
}

void print_stats(int n_instrs){
    printf("INSTRUCTIONS: %d\n", n_instrs);
}

int cacheOps(stateType* state, int mem_loc, int toStore, int indic){
    //Calculate offsets to get cache stuff using pc
    int blockOffsetMask = state->blockSizeInWords - 1;
    int setOffsetMask = (state->numberOfSets - 1) << ((int) (log( (double)state->blockSizeInWords ) / log(2)));
    int tag = mem_loc >> (int) (log((double)(state->numberOfSets * state->blockSizeInWords))/log(2));
    //Set offset vars
    int blockOffset = mem_loc & blockOffsetMask;
    int setOffset = (mem_loc & setOffsetMask) >> ((int) (log( (double)state->blockSizeInWords ) / log(2)));
    int fixed_loc = mem_loc - (mem_loc % state->blockSizeInWords );

    //Check if instruction is in cache
    int cache_hit = 0;
    int cache_loc;
    for (int i = 0; i < state->associativity; i++) {
        if(state->cache[setOffset][i].valid == 1 && state->cache[setOffset][i].tag == tag){
            updateLRUTable(state, setOffset, i);

            if(indic == 1){
                printAction(mem_loc, 1, processorToCache);
                state->cache[setOffset][i].words[blockOffset] = toStore;
                state->cache[setOffset][i].dirty = 1;
            }

            cache_hit = 1;
            cache_loc = i;
            i = state->associativity;
        }
    }
    //If no hits, evict and add to cache
    if(cache_hit != 1) {
        int leastUsed = state->lruTable[setOffset][0];
        //If block that is going to be evicted is valid and dirty, write back to memory
        if (state->cache[setOffset][leastUsed].valid != 0 && state->cache[setOffset][leastUsed].dirty == 1) {
            int leastUsed_set = setOffset << ((int) (log( (double)state->blockSizeInWords ) / log(2)));
            int leastUsed_tag = state->cache[setOffset][leastUsed].tag << (int) (log((double)(state->numberOfSets * state->blockSizeInWords))/log(2));
            int leastUsed_loc = leastUsed_set | leastUsed_tag;
            printAction(leastUsed_loc, state->blockSizeInWords, cacheToMemory);
            for (int j = 0; j < state->blockSizeInWords; j++) {
                state->mem[fixed_loc + j] = state->cache[setOffset][leastUsed].words[j];
            }
        } else if (state->cache[setOffset][leastUsed].valid != 0 && state->cache[setOffset][leastUsed].dirty == 0) {
            printAction(fixed_loc, state->blockSizeInWords, cacheToNowhere);
        }
        //There is room in the cache
        state->cache[setOffset][leastUsed].valid = 1;
        state->cache[setOffset][leastUsed].tag = tag;

        printAction(fixed_loc, state->blockSizeInWords, memoryToCache);

        for (int j = 0; j < state->blockSizeInWords; j++) {
            state->cache[setOffset][leastUsed].words[j] = state->mem[fixed_loc + j];
        }
        updateLRUTable(state, setOffset, leastUsed);

        if(indic == 1){
            printAction(mem_loc, 1, processorToCache);
            state->cache[setOffset][leastUsed].words[blockOffset] = toStore;
            state->cache[setOffset][leastUsed].dirty = 1;
        }

        cache_loc = leastUsed;
    }

    if(indic == 0){
        printAction(mem_loc,1,cacheToProcessor);
    }


    return state->cache[setOffset][cache_loc].words[blockOffset];

}

void run(stateType* state){

    // Reused variables;
    int instr = 0;
    int regA = 0;
    int regB = 0;
    int offset = 0;
    int branchTarget = 0;
    int aluResult = 0;

    int total_instrs = 0;

    // Primary loop
    while(1){
        total_instrs++;

        // ~~~~~~~~~~ Instruction Fetch ~~~~~~~~~~

        //Get instruction from op
        instr = cacheOps(state, state->pc, 0, 0);

        /* check for halt */
        if (opcode(instr) == HALT) {
            printf("machine halted\n");
            break;
        }

        // Increment the PC
        state->pc = state->pc+1;
        // Set reg A and B
        regA = state->reg[field0(instr)];
        regB = state->reg[field1(instr)];
        // Set sign extended offset
        offset = signExtend(field2(instr));
        // Branch target gets set regardless of instruction
        branchTarget = state->pc + offset;

        // ADD
        if(opcode(instr) == ADD){
            // Add
            aluResult = regA + regB;
            // Save result
            state->reg[field2(instr)] = aluResult;
        }
            // NAND
        else if(opcode(instr) == NAND){
            // NAND
            aluResult = ~(regA & regB);
            // Save result
            state->reg[field2(instr)] = aluResult;
        }
            // LW or SW
        else if(opcode(instr) == LW || opcode(instr) == SW){
            // Calculate memory address
            aluResult = regB + offset;
            if(opcode(instr) == LW){
                // Load
                state->reg[field0(instr)] = cacheOps(state, aluResult, 0, 0);
            }else if(opcode(instr) == SW){
                // Store
                cacheOps(state, aluResult, regA, 1);
            }
        }
            // JALR
        else if(opcode(instr) == JALR){
            // Save pc+1 in regA
            state->reg[field0(instr)] = state->pc;
            //Jump to the address in regB;
            state->pc = state->reg[field1(instr)];
        }
            // BEQ
        else if(opcode(instr) == BEQ){
            // Calculate condition
            aluResult = (regA == regB);

            // ZD
            if(aluResult){
                // branch
                state->pc = branchTarget;
            }
        }
    } // While
}

int main(int argc, char** argv){

    /** Get command line arguments **/
    char* fname;
    char* fname_dup;
    char* token;
    int cacheSize;
    int blockSizeInWords;
    int numberOfSets;
    int associativity;

    if(argc == 1){
        fname = (char*)malloc(sizeof(char)*100);
        //Using a duplicate so that tokenizer doesn't break up fname
        fname_dup = (char*)malloc(sizeof(char)*100);
        printf("Enter the name of the machine code file to simulate: ");
        fgets(fname_dup, 100, stdin);
        fname[0] = '\0';
        strcat(fname, fname_dup);
        token = strtok(fname_dup, "."); //Tokenize the duplicate
        fname[strlen(fname)-1] = '\0'; // gobble up the \n with a \0
    }else if (argc == 2) {
        int strsize = strlen(argv[1]);
        fname = (char *) malloc(strsize);
        fname[0] = '\0';
        strcat(fname, argv[1]);
        token = strtok(argv[1], "."); //Tokenize the arg so it doesn't mess with our file path

    }else{
        printf("Please run this program correctly.\n");
        exit(-1);
    }
    //Tokenize the file path to get the other vars
    token = strtok(NULL, ".");
    if(token){
        blockSizeInWords = atoi(token); //atoi converts string to int
    }
    token = strtok(NULL, ".");
    if(token){
        numberOfSets = atoi(token);
    }
    token = strtok(NULL, ".");
    if(token){
        associativity = atoi(token);
    }
    //Set size
    cacheSize = blockSizeInWords * numberOfSets * associativity;

    printf("File path: %s \n", fname);
    printf("blockSizeInWords: %d; ", blockSizeInWords);
    printf("numberOfSets: %d; ", numberOfSets);
    printf("associativity: %d\n",associativity);
    printf("---------------------------------------------------------\n");



    //Check input for logical errors
    if(associativity < 1){
        printf("Error: associativity cannot be less than 1.\n");
        exit(-1);
    }
    if(associativity > blockSizeInWords){
        printf("Error: associativity cannot be greater than block size.\n");
        exit(-1);
    }
    if(isPowerOfTwo(blockSizeInWords) == 0){
        printf("Error: block size is not a power of 2.\n");
        exit(-1);
    }
    if(isPowerOfTwo(numberOfSets) == 0){
        printf("Error: number of sets is not a power of 2.\n");
        exit(-1);
    }
    if(isPowerOfTwo(associativity) == 0){
        printf("Error: associativity is not a power of 2.\n");
        exit(-1);
    }
    //Open file
    FILE *fp = fopen(fname, "r");
    if (fp == NULL) {
        printf("Cannot open file '%s' : %s\n", fname, strerror(errno));
        return -1;
    }
    //Count the number of lines by counting newline characters
    int line_count = 0;
    int c;
    while (EOF != (c=getc(fp))) {
        if ( c == '\n' ){
            line_count++;
        }
    }
    // reset fp to the beginning of the file
    rewind(fp);

    //Make temp block so that we know sizeof(block) correctly, accounting for blockSizeInWords
    int blockSize = sizeof(block) + (sizeof(int) * blockSizeInWords);
    //Allocate space equal to sizeof(stateType), accounting for cache size (m*n*sizeof(block))
    stateType* state = (stateType*)malloc(sizeof(stateType) + (numberOfSets * associativity * blockSize));

    state->pc = 0;
    state->associativity = associativity;
    state->numberOfSets = numberOfSets;
    state->blockSizeInWords = blockSizeInWords;
    memset(state->mem, 0, NUMMEMORY*sizeof(int));
    memset(state->reg, 0, NUMREGS*sizeof(int));
    //Init lru_2d array
    state->lruTable = malloc(sizeof(int) * numberOfSets * associativity); //Allocate space for
    for (int i = 0; i < numberOfSets; i++){
        state->lruTable[i] = malloc(sizeof(int) * associativity);
        for (int j = 0; j < associativity; j++){
            state->lruTable[i][j] = j; //set initial value to 0 -> #associativity
        }
    }
    //Init cache 2d array
    state->cache = malloc(numberOfSets * associativity * blockSize); //Allocate space for the cache
    for (int i = 0; i < numberOfSets; i++) {
        state->cache[i] = malloc(associativity * blockSize);
        for (int j = 0; j < associativity; j++) {
            state->cache[i][j].words = malloc(sizeof(int) * blockSizeInWords);
            state->cache[i][j].dirty = 0;
            state->cache[i][j].valid = 0;
            state->cache[i][j].tag = 0;
            for (int k = 0; k < blockSizeInWords; k++) {
                state->cache[i][j].words[k] = 0;
            }

        }
    }

    state->numMemory = line_count;

    char line[256];

    int i = 0;
    while (fgets(line, sizeof(line), fp)) {
        /* note that fgets doesn't strip the terminating \n, checking its
           presence would allow to handle lines longer that sizeof(line) */
        state->mem[i] = atoi(line);
        i++;
    }
    fclose(fp);
    /** Run the simulation **/
    run(state);

    free(state);
    free(fname);

}