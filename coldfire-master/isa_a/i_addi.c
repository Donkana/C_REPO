/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

#include "coldfire.h"

/* Add Immediate (ADDI) instruction */

/* Format 
   
 15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
| 0 | 0 | 0 | 0 | 0 | 1 | 1 | 0 | 1 | 0 | 0 | 0 | 0 |  Register |
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+

*/

int ADDITime=1;  /* All the rest of the modes are not valid */

TRACER_DEFAULT_CHANNEL(i_addi);

#define INSTRUCTION_2ARGS(CODE1, REGISTER) \
    ((CODE1 << 23) | (REGISTER << 20))



static void execute(void)
{
	struct _Address Source,Destination;
	u32 Result, SValue, DValue;
	ADDI_Instr Instr;
	Memory_RetrWordFromPC(&Instr.Code);

	TRACE("Source:\n");
	if(!EA_GetFromPC(&Source, 32, 7, 4)) return;
	TRACE("Destination:\n");
	if(!EA_GetFromPC(&Destination, 32, 0, Instr.Bits.Register)) return;

	EA_GetValue(&SValue, &Source);
	EA_GetValue(&DValue, &Destination);

	Result = SValue + DValue;

	TRACE("0x%08lx + 0x%08lx = 0x%08lx\n", SValue, DValue, Result);

	/* Set the status register */
	SR_Set(I_ADDI, SValue, DValue, Result);

	TRACE("Storing Result:\n");
	EA_PutValue(&Destination, Result);

	TRACE("Done");
	
	cycle(ADDITime);
	
	return;
}

static s32 disassemble(char *Instruction, char *Arg1, char *Arg2) 
{
	ADDI_Instr Instr;
	Memory_RetrWordFromPC(&Instr.Code);
	sprintf(Instruction, "ADDI.L");
	Addressing_Print(32, 7, 4, Arg1);
	Addressing_Print(32, 0, Instr.Bits.Register, Arg2);
	return 0;
}

s32 addi_5206_register(void)
{
	instruction_register(0x0680, 0xFFF8, &execute, &disassemble);
	return 1;
}
