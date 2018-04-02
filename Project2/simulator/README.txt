Project 1
Simulator
April Gross, and Nick Scheel
October 20, 2017
1 Simulator
	A. Set up
		(a) Check for correct number of arguments from command line
		(b) Initialize struct with registers 0 through 7 initialized to 0
		(c) Initialize memory to 65536
		(d) Initialize program counter (PC) to 0
		(e) Loop through file and store each line in memory
	B. Looping through lines in memory starting at 0
		(a) Print state
		(b) PC++
		(c) Extract opcode from instruction
		(d) For instruction type assign appropriate labels to appropriate section of bits
		(e) For each opcode, do operation
			i. Assign registers values as needed
		(f) \halt" is reached, print state and exit
2 Multiplication
	A. Store values in registers
	B. Loop through 14 times
		(a) Check if the specifc bit of the multiplier is a 1 or a 0
		(b) If a 1, add the mcand to the running total
		(c) Shift the mcand over the the left one
		(d) Reset registers if needed