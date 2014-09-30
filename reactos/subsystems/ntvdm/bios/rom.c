/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            rom.c
 * PURPOSE:         ROM Support Functions
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "emulator.h"
#include "cpu/callback.h"
#include "utils.h"

#include "rom.h"

/* PRIVATE FUNCTIONS **********************************************************/

static HANDLE
OpenRomFile(IN  PCSTR  RomFileName,
            OUT PULONG RomSize OPTIONAL)
{
    HANDLE hRomFile;
    ULONG  ulRomSize = 0;

    /* Open the ROM image file */
    hRomFile = FileOpen(RomFileName, &ulRomSize);

    /* If we failed, bail out */
    if (hRomFile == NULL) return NULL;

    /*
     * The size of the ROM image file is at most 256kB. For instance,
     * the SeaBIOS image, which includes also expansion ROMs inside it,
     * covers the range C000:0000 to F000:FFFF .
     */
    if (ulRomSize > 0x40000)
    {
        /* We failed, bail out */
        DPRINT1("ROM image size 0x%lx too large, expected at most 0x40000 (256kB)", ulRomSize);
        FileClose(hRomFile);
        return NULL;
    }

    /* Success, return file handle and size if needed */
    if (RomSize) *RomSize = ulRomSize;
    return hRomFile;
}

static BOOLEAN
LoadRomFileByHandle(IN  HANDLE RomFileHandle,
                    IN  PVOID  RomLocation,
                    IN  ULONG  RomSize,
                    OUT PULONG BytesRead)
{
    /*
     * The size of the ROM image file is at most 256kB. For instance,
     * the SeaBIOS image, which includes also expansion ROMs inside it,
     * covers the range C000:0000 to F000:FFFF .
     */
    if (RomSize > 0x40000)
    {
        DPRINT1("ROM image size 0x%lx too large, expected at most 0x40000 (256kB)", RomSize);
        return FALSE;
    }

    /* Attempt to load the ROM image file into memory */
    return FileLoadByHandle(RomFileHandle,
                            REAL_TO_PHYS(RomLocation),
                            RomSize,
                            BytesRead);
}

static UCHAR
ComputeChecksum(IN ULONG RomLocation,
                IN ULONG RomSize)
{
    ULONG RomLastAddress = RomLocation + RomSize;
    UCHAR Sum = 0x00;   // Using a UCHAR guarantees that we wrap at 0xFF i.e. we do a sum modulo 0x100.

    while (RomLocation < RomLastAddress)
    {
        Sum += *(PUCHAR)REAL_TO_PHYS(RomLocation);
        ++RomLocation;
    }

    return Sum;
}

static VOID
InitRomRange(IN PCALLBACK16 Context,
             IN ULONG Start,
             IN ULONG End,
             IN ULONG Increment)
{
    ULONG Address, AddressBoot;
    ULONG RomSize;
    UCHAR Checksum;

    for (Address = Start; Address < End; Address += Increment)
    {
        /* Does the ROM have a valid signature? */
        if (*(PUSHORT)REAL_TO_PHYS(Address) == OPTION_ROM_SIGNATURE)
        {
            /* Check the control sum of the ROM */

            /*
             * If this is an adapter ROM (Start: C8000, End: E0000), its
             * reported size is stored in byte 2 of the ROM.
             *
             * If this is an expansion ROM (Start: E0000, End: F0000),
             * its real length is 64kB.
             */
            RomSize = *(PUCHAR)REAL_TO_PHYS(Address + 2) * 512;
            if (Address >= 0xE0000) RomSize = 0x10000;

            Checksum = ComputeChecksum(Address, RomSize);
            if (Checksum == 0x00)
            {
                AddressBoot = Address + 3;
                DPRINT1("Going to run @ address 0x%p\n", AddressBoot);

                AddressBoot = MAKELONG((AddressBoot & 0xFFFF), (AddressBoot & 0xF0000) >> 4);
                // setDS((Address & 0xF0000) >> 4);
                setDS((Address & 0xFF000) >> 4);
                RunCallback16(Context, AddressBoot);
                // Call16((AddressBoot & 0xF0000) >> 4, (AddressBoot & 0xFFFF));

                DPRINT1("Rom @ address 0x%p initialized\n", Address);
            }
            else
            {
                DPRINT1("Rom @ address 0x%p has invalid checksum of 0x%02x\n", Address, Checksum);
            }
        }
    }
}

/* PUBLIC FUNCTIONS ***********************************************************/

BOOLEAN
LoadBios(IN  PCSTR  BiosFileName,
         OUT PVOID* BiosLocation OPTIONAL,
         OUT PULONG BiosSize     OPTIONAL)
{
    BOOLEAN Success;
    HANDLE  hBiosFile;
    ULONG   ulBiosSize = 0;
    PVOID   pBiosLocation;

    /* Open the BIOS image file */
    hBiosFile = OpenRomFile(BiosFileName, &ulBiosSize);

    /* If we failed, bail out */
    if (hBiosFile == NULL) return FALSE;

    /* BIOS location needs to be aligned on 32-bit boundary */
    // (PVOID)((ULONG_PTR)BaseAddress + ROM_AREA_END + 1 - ulBiosSize)
    pBiosLocation = MEM_ALIGN_DOWN(TO_LINEAR(0xF000, 0xFFFF) + 1 - ulBiosSize, sizeof(ULONG));

    /* Attempt to load the BIOS image file into memory */
    Success = LoadRomFileByHandle(hBiosFile,
                                  pBiosLocation,
                                  ulBiosSize,
                                  &ulBiosSize);
    DPRINT1("BIOS loading %s ; GetLastError() = %u\n", Success ? "succeeded" : "failed", GetLastError());

    /* Close the BIOS image file */
    FileClose(hBiosFile);

    /* In case of success, return BIOS location and size if needed */
    if (Success)
    {
        if (BiosLocation) *BiosLocation = pBiosLocation;
        if (BiosSize)     *BiosSize     = ulBiosSize;
    }

    return Success;
}

BOOLEAN
LoadRom(IN  PCSTR  RomFileName,
        IN  PVOID  RomLocation,
        OUT PULONG RomSize OPTIONAL)
{
    BOOLEAN Success;
    HANDLE  hRomFile;
    ULONG   ulRomSize = 0;

    /* Open the ROM image file */
    hRomFile = OpenRomFile(RomFileName, &ulRomSize);

    /* If we failed, bail out */
    if (hRomFile == NULL) return FALSE;

    /* Attempt to load the ROM image file into memory */
    Success = LoadRomFileByHandle(hRomFile,
                                  RomLocation,
                                  ulRomSize,
                                  &ulRomSize);
    DPRINT1("ROM loading %s ; GetLastError() = %u\n", Success ? "succeeded" : "failed", GetLastError());

    /* Close the ROM image file and return */
    FileClose(hRomFile);

    /* In case of success, return ROM size if needed */
    if (Success)
    {
        if (RomSize) *RomSize = ulRomSize;
    }

    return Success;
}

VOID
SearchAndInitRoms(IN PCALLBACK16 Context)
{
    /* Adapters ROMs -- Start: C8000, End: E0000, 2kB blocks */
    InitRomRange(Context, 0xC8000, 0xE0000, 0x0800);

    /* Expansion ROM -- Start: E0000, End: F0000, 64kB block */
    InitRomRange(Context, 0xE0000, 0xEFFFF, 0x10000);
}

/* EOF */
