/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

#include "coldfire.h"

/* Add instruction */

/* Format 
   
 15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
| 1 | 1 | 0 | 1 | Register  |  OPmode   |  EA Mode  |EA Register|
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+

*/

int ADDTime[2][8] = {{1, 3, 3, 3, 3, 4, 3, 1},
                      {-1, 3, 3, 3, 3, 4, 3, -1}};

TRACER_DEFAULT_CHANNEL(i_add);

#define INSTRUCTION_5ARGS(OPCODE, CODE1, REGISTER, CODE1, EAMODE, EAREGISTER) \
    ((OPCODE << 27) | (CODE2 << 23) | (REGISTER << 20) | (CODE1 << 17) | (EAMODE << 14) | (EAREGISTER << 11))



static void execute(void)
{
    struct _Address Source, Destination;
    u32 Result, SValue, DValue;
    ADD_Instr Instr;

    Memory_RetrWordFromPC(&Instr.Code);

    if (Instr.Bits.OPMode == 2) { /* <EA>y + Dx */
        TRACE("<EA>y + Dx -> Dx\n");
        TRACE("Source:\n");
        if (!EA_GetFromPC(&Source, 32, Instr.Bits.EAMode, Instr.Bits.EARegister)) return;
        TRACE("Destination:\n");
        if (!EA_GetFromPC(&Destination, 32, 0, Instr.Bits.Register)) return;
    } else { /* Dy + <EA>x -> <EA>x */
        TRACE("Dy + <EA>x -> <EA>x\n");
        if (Instr.Bits.EAMode == 0) {
            ERR("May not specify Dx for destination when source is Dx");
            return;
        } else if (Instr.Bits.EAMode == 1) {
            ERR("May not specify Ax for destination when source is Dx");
            return;
        }
        TRACE("Source:\n");
        if (!EA_GetFromPC(&Source, 32, 0, Instr.Bits.Register)) return;
        TRACE("Destination:\n");
        if (!EA_GetFromPC(&Destination, 32, Instr.Bits.EAMode, Instr.Bits.EARegister)) return;
    }
    EA_GetValue(&SValue, &Source);
    EA_GetValue(&DValue, &Destination);

    Result = SValue + DValue;

    TRACE("0x%08lx + 0x%08lx = 0x%08lx\n", SValue, DValue, Result);

    SR_Set(I_ADD, SValue, DValue, Result);

    TRACE("Storing Result:\n");
    EA_PutValue(&Destination, Result);

    if (Instr.Bits.OPMode == 2)  /* <EA>y + Dx */
        cycle(ADDTime[0][cycle_EA(Instr.Bits.EARegister, Instr.Bits.EAMode)]);
    else 
        cycle(ADDTime[1][cycle_EA(Instr.Bits.EARegister, Instr.Bits.EAMode)]);

    return;
}

static s32 disassemble(char *Instruction, char *Arg1, char *Arg2)
{
    ADD_Instr Instr;
    Memory_RetrWordFromPC(&Instr.Code);

    sprintf(Instruction, "ADD.L");
    if (Instr.Bits.OPMode == 2) { /* <EA>y & Dx */
        Addressing_Print(32, Instr.Bits.EAMode, Instr.Bits.EARegister, Arg1);
        Addressing_Print(32, 0, Instr.Bits.Register, Arg2);
    } else {
        Addressing_Print(32, 0, Instr.Bits.Register, Arg1);
        Addressing_Print(32, Instr.Bits.EAMode, Instr.Bits.EARegister, Arg2);
    }
    return 0;
}

s32 add_5206_register(void)
{
    instruction_register(0xD000, 0xF000, &execute, &disassemble);
    return 1;
}
