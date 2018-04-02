#CISC340 Project 1 - Assembler
###Nick Scheel (sche0210) and April Gross (gros0128)

####How to Compile
Navigate to the directory containing the project files, then run `make`. Then run `./assembler` 
followed by one or two arguments: the first being the source file (containing machine code) and
the second being the destination file for the output (optional).  
To remove executable and other files created by the Makefile, run `make clean`.  
####Test Files
Test files can be found in the `test_cases` directory, and include the following: 
- check_other_ops.txt (checks sw, jalr, and nand)
- handout_code.txt (copy of the example code from the handout)
- label_bad_char.txt (tests invalid characters)
- label_is_op.txt (attempts to use an op as a label)
- label_len_over_6.txt (tests label length)
- label_numb_first.txt (attempts to use number as first character)
- label_two_same.txt (attempts to use duplicate labels)
- op_bad.txt (attempts to use invalid operation)