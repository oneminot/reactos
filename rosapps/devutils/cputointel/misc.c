
/* only for getting the pe struct */
#include <windows.h>
#include <winnt.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "misc.h"
#include "ARM/ARM.h"
#include "m68k/m68k.h"
#include "PPC/PPC.h"


/* retun 
 * 0 = Ok 
 * 1 = unimplemt 
 * 2 = Unkonwn Opcode 
 * 3 = can not open read file
 * 4 = can not open write file
 * 5 = can not seek to end of read file
 * 6 = can not get the file size of the read file
 * 7 = read file size is Zero
 * 8 = can not alloc memory
 * 9 = can not read file
 *-------------------------
 * type 0 : auto
 * type 1 : bin
 * type 2 : exe/dll/sys
 */



/* Conveting bit array to a int byte */
CPU_UNINT ConvertBitToByte(CPU_BYTE *bit)
{
    CPU_UNINT Byte = 0;
    CPU_UNINT t;
    CPU_UNINT size = 15;

    for(t=size;t>0;t--)
    {
        if (bit[size-t] != 2) 
            Byte = Byte + (bit[size-t]<<t);
    }
    return Byte;
}

/* Conveting bit array mask to a int byte mask */
CPU_UNINT GetMaskByte(CPU_BYTE *bit)
{
    CPU_UNINT MaskByte = 0;
    CPU_UNINT t;
    CPU_UNINT size = 15;

    for(t=size;t>0;t--)
    {
        if (bit[size-t] == 2) 
        {            
            MaskByte = MaskByte + ( (bit[size-t]-1) <<t);
        }
    }
    return MaskByte;
}

/* Conveting bit array to a int byte */
CPU_UNINT ConvertBitToByte32(CPU_BYTE *bit)
{
    CPU_UNINT Byte = 0;
    CPU_UNINT t;
    CPU_UNINT size = 31;

    for(t=size;t>0;t--)
    {
        if (bit[size-t] != 2) 
            Byte = Byte + (bit[size-t]<<t);
    }
    return Byte;
}

/* Conveting bit array mask to a int byte mask */
CPU_UNINT GetMaskByte32(CPU_BYTE *bit)
{
    CPU_UNINT MaskByte = 0;
    CPU_UNINT t;
    CPU_UNINT size = 31;

    for(t=size;t>0;t--)
    {
        if (bit[size-t] == 2) 
        {            
            MaskByte = MaskByte + ( (bit[size-t]-1) <<t);
        }
    }
    return MaskByte;
}



// add no carry






