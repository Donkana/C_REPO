/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

#include "coldfire.h"

/* OR instruction */

/* Format 
   
 15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
| 1 | 0 | 0 | 0 | Register  |  OPmode   |  EA Mode  |EA Register|
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+

*/
int ORTime[2][8]={{1, 3, 3, 3, 3, 4, 3, 1},
	          {-1, 3, 3, 3, 3, 4, 3, -1}};

TRACER_DEFAULT_CHANNEL(i_or);

#define INSTRUCTION_5ARGS(OPCODE, CODE2, REGISTER, CODE1, EAMODE, EAREGISTER) \
    ((OPCODE << 27) | (CODE2 << 23) | (REGISTER << 20) | (CODE1 << 17) | (EAMODE << 14) | (EAREGISTER << 11))

// INSTRUCTION_5ARGS(OR,
// 	unsigned Code1,4,
// 	unsigned Register,3,
// 	unsigned OPMode,3,
// 	unsigned EAMode,3,
// 	unsigned EARegister,3);

static void execute(void)
{
	struct _Address Source,Destination;
	u32 Result, SValue, DValue;
	OR_Instr Instr;
	Memory_RetrWordFromPC(&Instr.Code);

	if(Instr.Bits.OPMode==2) { /* <EA>y | Dx */
		TRACE("<EA>y | Dx -> Dx\n");
		if(Instr.Bits.EAMode==1) {
			ERR("May not specify Ax for source");
			return;
		}
		TRACE("Source:\n");
		if(!EA_GetFromPC(&Source, 32, Instr.Bits.EAMode, Instr.Bits.EARegister)) return;
		TRACE("Destination:\n");
		if(!EA_GetFromPC(&Destination, 32, 0, Instr.Bits.Register)) return;
	} else if (Instr.Bits.OPMode==6) { /* Dy | <EA>x -> <EA>x */
		if(Instr.Bits.EAMode==0) {
			ERR("May not specify Dx for destination when source is Dx");
			return;
		} else if (Instr.Bits.EAMode==1) {
			ERR("May not specify Ax for destination when source is Dx");
			return;
		} else if (Instr.Bits.EAMode==7 && Instr.Bits.EARegister==4) {
			ERR("May not specify Immediate Addressing for destination");
			return;
		}
		TRACE("Source:\n");
		if(!EA_GetFromPC(&Source, 32, 0, Instr.Bits.Register)) return;
		TRACE("Destination:\n");
		if(!EA_GetFromPC(&Destination, 32, Instr.Bits.EAMode, Instr.Bits.EARegister)) return;
	} else {
		ERR("Unknown OPMode %d", Instr.Bits.OPMode);
		return;
	}

	EA_GetValue(&SValue, &Source);
	EA_GetValue(&DValue, &Destination);

	Result = SValue | DValue;
	TRACE("0x%08lx | 0x%08lx = 0x%08lx\n", SValue, DValue, Result);

	TRACE("Storing Result:\n");
	EA_PutValue(&Destination, Result);
	
	/* Set the status register
	 *  X - not affected 
	 *  N - set it MSB or result is 1
	 *  Z - set if result is zero
	 *  V,C always cleared
	 */ 
	memory_core.sr.n = ((s32)Result < 0) ? SR_N : 0;
	memory_core.sr.z = (Result == 0) ? SR_Z : 0;
	memory_core.sr.v = 0;
	memory_core.sr.c = 0;

	TRACE("Done\n");
	
	if (Instr.Bits.OPMode==2)
	  cycle(ORTime[0][cycle_EA(Instr.Bits.EARegister,Instr.Bits.EAMode)]);
	else cycle(ORTime[1][cycle_EA(Instr.Bits.EARegister,Instr.Bits.EAMode)]);

	
	return;
}

static s32 disassemble(char *Instruction, char *Arg1, char *Arg2)
{
	OR_Instr Instr;
	Memory_RetrWordFromPC(&Instr.Code);
	sprintf(Instruction, "OR.L");
	if(Instr.Bits.OPMode==2) { /* <EA>y | Dx */
		Addressing_Print(32, Instr.Bits.EAMode, Instr.Bits.EARegister, Arg1);
		Addressing_Print(32, 0, Instr.Bits.Register, Arg2);	
	} else {
		Addressing_Print(32, 0, Instr.Bits.Register, Arg1);
		Addressing_Print(32, Instr.Bits.EAMode, Instr.Bits.EARegister, Arg2);	
	}
	return 0;
}

s32 or_5206_register(void)
{
	instruction_register(0x8000, 0xF000, &execute, &disassemble);
	return 1;
}
