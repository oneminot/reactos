/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/armllb/envir.c
 * PURPOSE:         LLB Environment Variable Routines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#include "precomp.h"

ULONG LlbEnvHwPageSize;
ULONG LlbEnvHwMemStart;
ULONG LlbEnvHwMemSize;
ULONG LlbEnvRamDiskStart;
ULONG LlbEnvRamDiskSize;
CHAR LlbEnvCmdLine[256];

VOID
NTAPI
LlbEnvParseArguments(IN PATAG Arguments)
{
    PATAG Atag;
    
    /* Parse the ATAGs */
    Atag = Arguments;
    while (Atag->Hdr.Size)
    {
        /* Check tag type */
        switch (Atag->Hdr.Tag)
        {
            case ATAG_CORE:
            
                /* Save page size */
                LlbEnvHwPageSize = Atag->u.Core.PageSize;
                break;
                
            case ATAG_MEM:
            
                /* Save RAM start and size */
                LlbEnvHwMemStart = Atag->u.Mem.Start;
                LlbEnvHwMemSize = Atag->u.Mem.Size;
                break;
                
            case ATAG_INITRD2:
            
                /* Save RAMDISK start and size */
                LlbEnvRamDiskStart = Atag->u.InitRd2.Start;
                LlbEnvRamDiskSize = Atag->u.InitRd2.Size;
                break;
                
            case ATAG_CMDLINE:
            
                /* Save command line */
                strncpy(LlbEnvCmdLine,
                        Atag->u.CmdLine.CmdLine,
                        Atag->Hdr.Size * sizeof(ULONG));
                break;
                
            /* Nothing left to handle */
            case ATAG_NONE:
            default:
                return;
        }
        
        /* Next tag */
        Atag = (PATAG)((PULONG)Atag + Atag->Hdr.Size);
    }
    
    /* For debugging */
    DbgPrint("[BOOTROM] PageSize: %dKB RAM: %dMB CMDLINE: %s\n",
             LlbEnvHwPageSize / 1024, LlbEnvHwMemSize / 1024 / 1024, LlbEnvCmdLine);
}

VOID
NTAPI
LlbEnvGetMemoryInformation(IN PULONG Base,
                           IN PULONG Size)
{
    /* Return RAM information */
    *Base = LlbEnvHwMemStart;
    *Size = LlbEnvHwMemSize;
}

BOOLEAN
NTAPI
LlbEnvGetRamDiskInformation(IN PULONG Base,
                            IN PULONG Size)
{
    /* Do we have a ramdisk? */
    if (LlbEnvRamDiskSize == 0)
    {
        /* No */
        *Base = 0;
        *Size = 0;
        return FALSE;
    }
    
    /* Return ramdisk information */
    *Base = LlbEnvRamDiskStart;
    *Size = LlbEnvRamDiskSize;
    return TRUE;
}

PCHAR
NTAPI
LlbEnvRead(IN PCHAR ValueName)
{
    PCHAR ValueData;
    
    /* Search for the value name */
    ValueData = strstr(LlbEnvCmdLine, ValueName);
    if (ValueData) ValueData += strlen(ValueName) + 1;

    /* Return the data */
    return ValueData;
}

/* EOF */

