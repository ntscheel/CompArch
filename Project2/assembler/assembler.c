#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Authors: Nick Scheel (sche0210) and April Gross (gros0128)
 *
 */

struct Label
{
    int location;
    char *label;
};

typedef enum {
    STR2INT_SUCCESS,
    STR2INT_OVERFLOW,
    STR2INT_UNDERFLOW,
    STR2INT_INCONVERTIBLE
} str2int_errno;

char isOp(char *token);
int validate_label(char *chars, int len,struct Label labeltable[], int numlabels);
int process_arg(char *arg, int arg_len, struct Label labeltable[], int numlabels, int is_imm, int *error, int counter);
str2int_errno str2int(int *out, char *s, int base);


/*
 * TODO: 
 * - makefile
 * - check if label exists before adding to table, error if duplicate
 * - make sure all declared variables are used
 * 
 */


int main(int argc, char *argv[]){
    char *token = NULL; //Used to read in current token when reading through line
    char *token2 = NULL; //Used to check if a label is an op
    char *op0, *arg1, *arg2, *arg3;
    int op0_int, arg1_int, arg2_int, arg3_int;
    int command;
    int counter = 0;  //holds the address of the machine code instruction
    int opType; //Char to represent the type of op; Used in 2nd pass
    int token_len; //Length of current token, used to see if label is too long
    int error = 0; //Roving error that can be passed and switched to non-zero if an error occurs

    int regA_mask, regB_mask, regC_mask, offset_mask;

    //Hard-coded bit-shifted masks for each op
    int add_mask=0<<22, nand_mask=1<<22;
    int lw_mask=2<<22, sw_mask=3<<22;
    int beq_mask=4<<22, jalr_mask=5<<22;
    int halt_mask=6<<22, noop_mask=7<<22;

    // Allows us to store a single look-up table instead of a 2d one
    int labelbuffer = 20;
    struct Label *labeltable = malloc(labelbuffer*sizeof(struct Label));
    int numlabels = 0; //number of labels encountered during assembly.

    //Create table and butter vars for out output
    int *program;

    // ------------------------------------------------------------------


    if (argc < 2){
        printf("Missing argument.\n");
        return -1;
    }
    else{ // Arguments received, store first extra arg as the input file
        char const* const in_file = argv[1];


        FILE* file = fopen(in_file, "r");
        if(file != NULL){
            char line[1000]; //Max line size 1000 characters. Might want to make this dynamic if possible

            /*
             * ~~~~~~~~ FIRST PASS ~~~~~~~~
             */
            while (fgets(line, sizeof(line), file)) { //Loop through file line-by-line
                token = strtok(line,"\n\t\r "); //Create token for the line splitting on white space
                token2 = strtok(NULL,"\n\t\r ");
                token_len = strlen(token);
                if(isOp(token) == ' ' ){ //is label, not op
                    if(validate_label(token, token_len, labeltable, numlabels) != -1){
                        if(numlabels == labelbuffer-1){
                            struct Label *temptable = realloc(labeltable, (labelbuffer*2)*sizeof(struct Label));
                            if(temptable != NULL){
                                labeltable = temptable;
                            }else{
                                free(labeltable);
                                printf("Error allocating memory for label table.\n");
                                return -1;
                            }
                        }
                        labeltable[numlabels].location = counter;

                        op0 = (char*)malloc(sizeof(token));
                        strcpy(op0,token);
                        labeltable[numlabels].label=op0;

                        numlabels++;
                    }else{
                        printf("Invalid label: '%s'\n",token);
                        return -1;
                    }
                }
                else if(token2 != NULL && isOp(token2) != ' ' ){
                    printf("Label Error: label cannot be valid op - '%s'\n",token2);
                    return -1;
                }
                counter++;
            }
            // ~~~~~END FIRST PASS~~~~~

            program = malloc(counter*sizeof(int)); //Initialize program array to length of counter
            counter = 0;  //Reset counter to 0 for second pass
            rewind(file); //Reset input file for second pass

            /*
             * ~~~~~~~~ SECOND PASS ~~~~~~~~
             */
            while (fgets(line, sizeof(line), file)) { //Loop through file line-by-line
                command = 0;
                token = strtok(line,"\n\t\r ");
                if(isspace((int)line[0]) == 0){
                    //line begins with label, so skip the first field
                    token = strtok(NULL,"\n\t\r ");
                }

                //Find out what type the op is
                char type = (char)isOp(token);

                if(type == 'r'){
                    //Include: add and nand
                    int arg_len;
                    op0 = token;

                    arg1 = token = strtok(NULL,"\n\t\r ");
                    arg_len = strlen(arg1);
                    arg1_int = process_arg(arg1,arg_len,labeltable,numlabels,0,&error,0);

                    arg2 = token = strtok(NULL,"\n\t\r ");
                    arg_len = strlen(arg2);
                    arg2_int = process_arg(arg2,arg_len,labeltable,numlabels,0,&error,0);

                    arg3 = token = strtok(NULL,"\n\t\r ");
                    arg_len = strlen(arg3);
                    arg3_int = process_arg(arg3,arg_len,labeltable,numlabels,0,&error,0);

                    arg1_int = arg1_int;       //destReg
                    arg2_int = arg2_int << 19; //regA
                    arg3_int = arg3_int << 16; //regB

                    if(strcmp(op0,"add")==0){
                        //op is add
                        command = command | add_mask;
                    }else{
                        //op is nand
                        command = command | nand_mask;
                    }
                    command = command | arg1_int;
                    command = command | arg2_int;
                    command = command | arg3_int;

                }
                else if(type == 'i'){
                    //Include: lw, sw, and beq
                    int arg_len;
                    op0 = token;

                    arg1 = token = strtok(NULL,"\n\t\r ");
                    arg_len = strlen(arg1);
                    arg1_int = process_arg(arg1,arg_len,labeltable,numlabels,0,&error,0);

                    arg2 = token = strtok(NULL,"\n\t\r ");
                    arg_len = strlen(arg2);
                    arg2_int = process_arg(arg2,arg_len,labeltable,numlabels,0,&error,0);

                    arg3 = token = strtok(NULL,"\n\t\r ");
                    arg_len = strlen(arg3);
                    if(strcmp(op0,"beq") == 0) {
                        arg3_int = process_arg(arg3, arg_len, labeltable, numlabels, 1, &error, counter + 1);
                    }else {
                        arg3_int = process_arg(arg3, arg_len, labeltable, numlabels, 1, &error, 0);
                    }
                    if( (arg3_int<-32769) || (arg3_int>32768)){
                        printf("Invalid immediate value: %d \n",arg3_int);
                        return -1;
                    }
                    arg1_int = arg1_int << 19;         //regA
                    arg2_int = arg2_int << 16;         //regB

                    command = command | arg1_int;
                    command = command | arg2_int;
                    if(arg3_int < 0){
                        arg3_int = arg3_int ^ ~(65535);
                        command = command | arg3_int;
                    }else{
                        command = command | arg3_int;
                    }

                    if(strcmp(op0,"beq") == 0){
                        // op is beq
                        command = command | beq_mask;

                    }else{// is lw or sw
                        if(strcmp(op0,"lw")==0){
                            //op is lw
                            command = command | lw_mask;
                        }else{
                            //op is sw
                            command = command | sw_mask;
                        }
                    }
                }
                else if(type == 'j'){
                    //Includes jalr
                    int arg_len;
                    op0 = token;

                    arg1 = token = strtok(NULL,"\n\t\r ");
                    arg_len = strlen(arg1);
                    arg1_int = process_arg(arg1,arg_len,labeltable,numlabels,0,&error,0);

                    arg2 = token = strtok(NULL,"\n\t\r ");
                    arg_len = strlen(arg2);
                    arg2_int = process_arg(arg2,arg_len,labeltable,numlabels,0,&error,0);

                    arg1_int = arg1_int << 19; //regA
                    arg2_int = arg2_int << 16; //regB
                    command = command | jalr_mask;
                    command = command | arg1_int;
                    command = command | arg2_int;

                }
                else if(type == 'o'){
                    //Includes halt and noop
                    op0 = token;
                    if(strcmp(op0,"noop") == 0){
                        command = command | noop_mask;
                    }else{
                        command = command | halt_mask;
                    }
                }
                else if(type == 'f'){
                    int arg_len;
                    arg1 = token = strtok(NULL,"\n\t\r ");
                    arg_len = strlen(arg1);
                    arg1_int = process_arg(arg1,arg_len,labeltable,numlabels,0,&error,0);
                    command = arg1_int;

                }else{
                    printf("Error: unknown operation  '%s'\n",token);
                    return -1;
                }//End op else/ifs

                if(error != 0){//Error is set in process_arg() on invalid registers
                    printf("Error: invalid register received.\n");
                    return -1;
                }

                program[counter] = command;
                counter++; //increment system counter
            }// ~~~~~ END SECOND PASS ~~~~~~

            fclose(file); //Close input file

            if (argc == 3){ //Both input and output file given, save filename for specified output file
                char const* const out_file = argv[2];
                FILE *file = fopen(out_file, "w");
                if (file == NULL){
                    printf("Error: unable to open file: %s\n",argv[2]);
                    return -1;
                }
                else{
                    //Print results into file
                    for(int i = 0; i < counter; i++){
                        fprintf(file,"%d",program[i]);
                        if(i+1 < counter){//Only print newline when there is a following command
                            fprintf(file,"\n");
                        }
                    }
                }
                fclose(file);
            }else{
                //Print results into terminal
                for(int i = 0; i < counter; i++){
                    printf("%d\n",program[i]);
                }
            }


        }
        else{ //If incorrect filename given, print error
            printf("Error: unable to read file: %s\n",argv[1]);
            fclose(file);
            return -1;
        }

    }
    return 0;
}


/*
 * This method reads in a token and determines if it is one of 8 pre-determined operations
 * Returns char ' ' if not a valid op
 */
char isOp(char *token){
    char isOp = ' ';
    if(strcmp(token, "add") == 0 || strcmp(token, "nand") == 0){
        isOp = 'r';
    }
    if(strcmp(token, "lw") == 0 || strcmp(token, "sw") == 0 || strcmp(token, "beq") == 0){
        isOp = 'i';
    }
    if(strcmp(token, "jalr") == 0){
        isOp = 'j';
    }
    if(strcmp(token, "halt") == 0 || strcmp(token, "noop") == 0){
        isOp = 'o';
    }
    if(strcmp(token, ".fill") == 0){
        isOp = 'f';
    }
    return isOp;
}

/*
 * Determines if a label is valid or not
 */
int validate_label(char *chars, int len,struct Label labeltable[], int numlabels){
    if(isOp(chars) != ' '){
        printf("Label Error: label cannot be an operation - ");
        return -1;
    }
    if(len > 6){
        printf("Label Error: label length exceeds 6 characters - ");
        return -1;
    }
    int first = (int)chars[0];
    if((first<65) || (first>90 && first<97) || (first>122)){
        printf("Label Error: first letter is not a valid character - ");
        return -1;
    }
    for(int i=0; i < numlabels; i++){
        if(strcmp(chars, labeltable[i].label) == 0){
            printf("Label Error: contains duplicate label - ");
            return -1;
        }
    }
    for (int i=1; i<6; i++){
        if(chars[i]=='\0'){
            //end of label
            return 0;
        }

        int letter=(int)chars[i];
        if( (letter<48) || (letter>57 && letter<65) || (letter>90 && letter<97) || (letter>122)){
            //not a valid char
            printf("Label Error: label contains invalid char - ");
            return -1;
        }
    }

    return 0;
}

/*
 * Process argument. For registers, is_imm = 0, for immediate values, is_imm = 1
 */
int process_arg(char *arg, int arg_len, struct Label labeltable[], int numlabels, int is_imm, int *error, int counter){
    int ret = -1, k;
    if((int)arg[0] >= 47 && (int)arg[0] <= 57){
        if(str2int(&k, arg, 10) == STR2INT_SUCCESS){
            ret = k;
            if(is_imm == 0 && (ret < 0 || ret > 7)){
                *error = -1;
            }
        }else{
            printf("Could not convert string to int: '%s'\n",arg);
        }
    }else{
        ret = -1;
        for(int i = 0; i < numlabels; i++) {
            if (strcmp(arg, labeltable[i].label) == 0) {
                ret = labeltable[i].location;
                ret = ret - counter;
                break;
            }
        }
    }
    return ret;
}

/*
 * Converts string of numbers to an int. Taken from:
 * https://stackoverflow.com/questions/7021725/converting-string-to-integer-c
 */
str2int_errno str2int(int *out, char *s, int base) {
    char *end;
    if (s[0] == '\0' || isspace((unsigned char) s[0]))
        return STR2INT_INCONVERTIBLE;
    errno = 0;
    long l = strtol(s, &end, base);
    /* Both checks are needed because INT_MAX == LONG_MAX is possible. */
    if (l > INT_MAX || (errno == ERANGE && l == LONG_MAX))
        return STR2INT_OVERFLOW;
    if (l < INT_MIN || (errno == ERANGE && l == LONG_MIN))
        return STR2INT_UNDERFLOW;
    if (*end != '\0')
        return STR2INT_INCONVERTIBLE;
    *out = l;
    return STR2INT_SUCCESS;
}