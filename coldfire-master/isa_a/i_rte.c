/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

#include "coldfire.h"

/* Return from Exception (RTE)  */

/* Format 
   
 15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
| 0 | 1 | 0 | 0 | 1 | 1 | 1 | 0 | 0 | 1 | 1 | 1 | 0 | 1 | 0 | 1 |
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+

*/

int RTETime=8;

TRACER_DEFAULT_CHANNEL(i_rte);

#define INSTRUCTION_5ARGS(OPCODE, CODE2, REGISTER, CODE1, EAMODE, EAREGISTER) \
    ((OPCODE << 27) | (CODE2 << 23) | (REGISTER << 20) | (CODE1 << 17) | (EAMODE << 14) | (EAREGISTER << 11))

// INSTRUCTION_1ARG(RTE,
// 	unsigned Code1,16);

static void execute(void)
{
	u32 dummy;
	/* Read the instruction with out storing it.. we already know what it is */
	Memory_RetrWordFromPC(&dummy);

	exception_restore_from_stack_frame();

	TRACE("Done\n");

	cycle(RTETime);
	
	return;
}

static s32 disassemble(char *Instruction, char *Arg1, char *Arg2) 
{
	u32 dummy;
	Memory_RetrWordFromPC(&dummy);
	sprintf(Instruction, "RTE");
	Arg1[0]=0;
	Arg2[0]=0;
	return 0;
}


s32 rte_5206_register(void)
{
	instruction_register(0x4E73, 0xFFFF, &execute, &disassemble);
	return 1;
}
