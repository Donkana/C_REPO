/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

#include "coldfire.h"

/* Trap instruction */

/* Format 
   
 15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
| 0 | 1 | 0 | 0 | 1 | 1 | 1 | 0 | 0 | 1 | 0 | 0 |    Vector     |
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+

*/

int TRAPTime=15;

TRACER_DEFAULT_CHANNEL(i_trap);

#define INSTRUCTION_5ARGS(OPCODE, CODE2, REGISTER, CODE1, EAMODE, EAREGISTER) \
    ((OPCODE << 27) | (CODE2 << 23) | (REGISTER << 20) | (CODE1 << 17) | (EAMODE << 14) | (EAREGISTER << 11))

// INSTRUCTION_2ARGS(TRAP, 
// 		unsigned Code1,12, 
// 		unsigned Vector,4);


static void execute(void)
{
	TRAP_Instr Instr;
	Memory_RetrWordFromPC(&Instr.Code);

	TRACE("Vector=0x%02x\n", Instr.Bits.Vector);

	exception_do_exception(32+Instr.Bits.Vector);
	TRACE("Done\n");
	
	cycle(TRAPTime);
	
	return;
}

static s32 disassemble(char *Instruction, char *Arg1, char *Arg2)
{
	TRAP_Instr Instr;
	Memory_RetrWordFromPC(&Instr.Code);
	sprintf(Instruction, "TRAP");
	sprintf(Arg1, "#0x%02X", Instr.Bits.Vector);
	Arg2[0]=0;
	return 0;
}

s32 trap_5206_register(void)
{
	instruction_register(0x4E40, 0xFFF0, &execute, &disassemble);
	return 1;
}
