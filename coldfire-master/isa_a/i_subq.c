/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

#include "coldfire.h"

/* Subtract instruction */

/* Format 
   
 15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
| 0 | 1 | 0 | 1 |   Data    | 1 | 1 | 0 |  EA Mode  |EA Register|
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+

*/

int SUBQTime[]={1, 3, 3, 3, 3, 4, 3, -1};

TRACER_DEFAULT_CHANNEL(i_subq);

#define INSTRUCTION_5ARGS(OPCODE, CODE2, REGISTER, CODE1, EAMODE, EAREGISTER) \
    ((OPCODE << 27) | (CODE2 << 23) | (REGISTER << 20) | (CODE1 << 17) | (EAMODE << 14) | (EAREGISTER << 11))

// INSTRUCTION_5ARGS(SUBQ,
// 	unsigned Code2,4,
// 	unsigned Data,3,
// 	unsigned Code1,3,
// 	unsigned EAMode,3,
// 	unsigned EARegister,3);

static void execute(void)
{
	struct _Address Destination;
	u32 Result, SValue, DValue;
	SUBQ_Instr Instr;
	Memory_RetrWordFromPC(&Instr.Code);

	/* Instr.Data is the immedate data, 1-7 represents 1-7, 0 
		represents 8 */

	TRACE("Source: (Retrieving from instruction word) = 0x%02x where (0=8)\n", Instr.Bits.Data);
	SValue=Instr.Bits.Data;
	if(SValue==0) SValue = 8;

	TRACE("Destination:\n");
	if(!EA_GetFromPC(&Destination, 32, Instr.Bits.EAMode, Instr.Bits.EARegister)) return;

	if(Instr.Bits.EAMode==7) { 
		switch(Instr.Bits.EARegister) {
		case 2: 
			ERR("(d_16,PC) addressing mode invalid.");
			return;
		case 3: 
			ERR("(d_8,PC,Xn) addressing mode invalid.");
			return;
		case 4: 
			ERR("#<data> addressing mode invalid.");
			return;
		}
	}

	EA_GetValue(&DValue, &Destination);

	Result = DValue - SValue;

	TRACE("0x%08lx - 0x%08lx = 0x%08lx\n", DValue, SValue, Result);

	/* Set the status register, the condition codes are unaffected
		when subtracting from an address register */
	if(Instr.Bits.EAMode!=1)
		SR_Set(I_SUBQ, SValue, DValue, Result);
	else
		; /* Address register */

	TRACE("Storing Result:\n");
	EA_PutValue(&Destination, Result);

	TRACE("Done\n");
	
	cycle(SUBQTime[cycle_EA(Instr.Bits.EARegister,Instr.Bits.EAMode)]);
	
	return;
}

static s32 disassemble(char *Instruction, char *Arg1, char *Arg2)
{
	SUBQ_Instr Instr;
	Memory_RetrWordFromPC(&Instr.Code);
	sprintf(Instruction, "SUBQ.L");
	sprintf(Arg1, "#0x%02X", (char)(Instr.Bits.Data == 0) ? 8 : Instr.Bits.Data);
	Addressing_Print(32, Instr.Bits.EAMode, Instr.Bits.EARegister, Arg2);
 	return 0;
}

s32 subq_5206_register(void)
{
	instruction_register(0x5180, 0xF1C0, &execute, &disassemble);
	return 1;
}
