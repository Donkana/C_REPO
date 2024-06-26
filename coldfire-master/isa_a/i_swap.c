/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

#include "coldfire.h"

/* Swap register halves (SWAP) instruction */

/* Format 
   
 15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
| 0 | 1 | 0 | 0 | 1 | 0 | 0 | 0 | 0 | 1 | 0 | 0 | 0 |  Register |
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+

*/

int SWAPTime=1;

TRACER_DEFAULT_CHANNEL(i_swap);

#define INSTRUCTION_5ARGS(OPCODE, CODE2, REGISTER, CODE1, EAMODE, EAREGISTER) \
    ((OPCODE << 27) | (CODE2 << 23) | (REGISTER << 20) | (CODE1 << 17) | (EAMODE << 14) | (EAREGISTER << 11))

// INSTRUCTION_2ARGS(SWAP,
// 	unsigned Code1,13,
// 	unsigned Register,3);

static void execute(void)
{
	struct _Address Destination;
	u32 DValue;
	u32 Tmp;
	SWAP_Instr Instr;
	Memory_RetrWordFromPC(&Instr.Code);

	TRACE("Destination:\n");
	if(!EA_GetFromPC(&Destination, 32, 0, Instr.Bits.Register)) return;
	EA_GetValue(&DValue, &Destination);
	Tmp = (DValue << 16) & 0xFFFF0000;
	Tmp += (DValue >> 16) & 0x0000FFFF;
	
	TRACE("Storing result:\n");
	EA_PutValue(&Destination, Tmp);

	/* X - Not affected
	   N - Set if 32bits result is negative
	   Z - Set if 32bit result is zero
	   V - Always Cleared
	   C - Always Cleared
	*/
	memory_core.sr.n = ((s32)Tmp < 0) ? SR_N : 0;
	memory_core.sr.z = (Tmp == 0) ? SR_Z : 0;
	memory_core.sr.v = 0;
	memory_core.sr.c = 0;

	cycle(SWAPTime);
	
	return;
}

static s32 disassemble(char *Instruction, char *Arg1, char *Arg2)
{
	SWAP_Instr Instr;
	Memory_RetrWordFromPC(&Instr.Code);

	sprintf(Instruction, "SWAP");

	Addressing_Print(32, 0, Instr.Bits.Register, Arg1);
	Arg2[0]=0;

	return 0;
}

s32 swap_5206_register(void)
{
	instruction_register(0x4840, 0xFFF8, &execute, &disassemble);
	return 1;
}
