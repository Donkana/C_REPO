#include <stdio.h>

#include "coldfire.h"

void cycle(u32 number)
{
    extern struct _board_data board_data;
    if(number < 1) return; /* ignore 0 and -1 times since they are invalid */
    board_data.cycle_count += number;
    return;
}

/* Lookup table for getting the right offset in the timing tables
 * in each instruction */
static s16 cycle_EA_lookup[] = { 0, 0, 1, 2, 3, 4, 5, 6, 6, 4, 5, 7 };

int cycle_EA(s16 reg, s16 mode)
{
    if(mode == 7) return cycle_EA_lookup[reg+7];
    return cycle_EA_lookup[mode];
}

#ifdef explanation_of_what_the_above_routine_is_doing
int MoveCycle_EA (int Register, int Mode);    /* (These two are the same) */
int Cycle_EA (int Register, int Mode)
{    
    switch (Mode)    /* Return value 0-7 for destination addressing. */
    {
        case 0:  return sDRD;  /* Dy */
        case 1:  return sDRD;  /* Ay */
        case 2:  return sARI;  /* (Ay) */
        case 3:  return sARIPO;  /* (Ay)+ */
        case 4:  return sARIPR;  /* -(Ay) */
        case 5:  return sARID;  /* (d16,Ay) */
        case 6:  return sARIIB;  /* (d8,An,Xi) */
        case 7: switch (Register) 
            {
                case 0:  return sAS; /* word addressing */
                case 1:  return sAS; /* s32 addressing */
                case 2:  return sARID; /* (d16,PC) */
                case 3:  return sARIIB; /* (d8,PC,Xi) */
                case 4:  return sIM; /* (d8,PC,Xi) */
            }
        break;
        
    }
    /* This can cause a segfault, this returns an offset, returning -1 is bad (DG) */
    return -1;  /* Shouldn't get here */
}
#endif
