/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

#include "coldfire.h"

/* Add quick (ADDQ) instruction */

/* Format 
   
 15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
| 0 | 1 | 0 | 1 |   Data    | 0 | 1 | 0 |  EA Mode  |EA Register|
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+

*/

int ADDQTime[]={1, 3, 3, 3, 3, 4, 3, -1};

TRACER_DEFAULT_CHANNEL(i_addq);

#define INSTRUCTION_5ARGS(OPCODE, CODE2, REGISTER, CODE1, EAMODE, EAREGISTER) \
    ((OPCODE << 27) | (CODE2 << 23) | (REGISTER << 20) | (CODE1 << 17) | (EAMODE << 14) | (EAREGISTER << 11))



static void execute(void)
{
	struct _Address Destination;
	u32 Result, SValue, DValue;
	ADDQ_Instr Instr;
	Memory_RetrWordFromPC(&Instr.Code);

	TRACE("Source: (pulling from immediate data=%d where {0=8})\n", Instr.Bits.Data);
	/* Instr.Data is the immedate data, 1-7 represents 1-7, 0 
		represents 8 */
	SValue=Instr.Bits.Data;
	if(SValue==0) SValue = 8;

	TRACE("Destination:\n");
	if(!EA_GetFromPC(&Destination, 32, Instr.Bits.EAMode, Instr.Bits.EARegister)) return;
	EA_GetValue(&DValue, &Destination);

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

	Result = SValue + DValue;

	TRACE("0x%08lx + 0x%08lx = 0x%08lx\n", SValue, DValue, Result);

	/* The condition codes are not affected when the destation is an 
		address register */

	if(Instr.Bits.EAMode!=1)
		SR_Set(I_ADDQ, SValue, DValue, Result);
	else
		; /* Destination is an address register */

	TRACE("Storing Result:\n");
	EA_PutValue(&Destination, Result);
	cycle(ADDQTime[cycle_EA(Instr.Bits.EARegister,Instr.Bits.EAMode)]);
	return;
}

static s32 disassemble(char *Instruction, char *Arg1, char *Arg2)
{
	ADDQ_Instr Instr;
	Memory_RetrWordFromPC(&Instr.Code);
	sprintf(Instruction, "ADDQ.L");
	sprintf(Arg1, "#0x%02X", (Instr.Bits.Data == 0) ? 8 : Instr.Bits.Data);
	Addressing_Print(32, Instr.Bits.EAMode, Instr.Bits.EARegister, Arg2);
 	return 0;
}

s32 addq_5206_register(void)
{
	instruction_register(0x5080, 0xF1C0, &execute, &disassemble);
	return 1;
}
