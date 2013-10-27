/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            bios.c
 * PURPOSE:         VDM BIOS
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "bios.h"
#include "emulator.h"
#include "vga.h"
#include "pic.h"
#include "ps2.h"
#include "timer.h"

#include "registers.h"

/* PRIVATE VARIABLES **********************************************************/

PBIOS_DATA_AREA Bda;
static BYTE BiosKeyboardMap[256];
static HANDLE BiosConsoleInput  = INVALID_HANDLE_VALUE;
static HANDLE BiosConsoleOutput = INVALID_HANDLE_VALUE;
static CONSOLE_SCREEN_BUFFER_INFO BiosSavedBufferInfo;

/*
 * VGA Register Configurations for BIOS Video Modes
 * The configurations come from DosBox.
 */
static BYTE VideoMode_40x25_text[] =
{
    /* Miscellaneous Register */
    0x67,

    /* Sequencer Registers */
    0x00, 0x08, 0x03, 0x00, 0x07,

    /* GC Registers */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0E, 0x0F, 0xFF,

    /* CRTC Registers */
    0x2D, 0x27, 0x28, 0x90, 0x2B, 0xA0, 0xBF, 0x1F, 0x00, 0x4F, 0x0D, 0x0E,
    0x00, 0x00, 0x00, 0x00, 0x9C, 0x8E, 0x8F, 0x14, 0x1F, 0x96, 0xB9, 0xA3,
    0xFF,

    /* AC Registers */
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07, 0x38, 0x39, 0x3A, 0x3B,
    0x3C, 0x3D, 0x3E, 0x3F, 0x0C, 0x00, 0x0F, 0x08, 0x00
};

static BYTE VideoMode_80x25_text[] =
{
    /* Miscellaneous Register */
    0x67,

    /* Sequencer Registers */
    0x00, 0x00, 0x03, 0x00, 0x07,

    /* GC Registers */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0E, 0x0F, 0xFF,

    /* CRTC Registers */
    0x5F, 0x4F, 0x50, 0x82, 0x55, 0x81, 0xBF, 0x1F, 0x00, 0x4F, 0x0D, 0x0E,
    0x00, 0x00, 0x00, 0x00, 0x9C, 0x8E, 0x8F, 0x28, 0x1F, 0x96, 0xB9, 0xA3,
    0xFF,

    /* AC Registers */
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07, 0x38, 0x39, 0x3A, 0x3B,
    0x3C, 0x3D, 0x3E, 0x3F, 0x0C, 0x00, 0x0F, 0x08, 0x00
};

static BYTE VideoMode_320x200_4color[] =
{
    /* Miscellaneous Register */
    0x63,

    /* Sequencer Registers */
    0x00, 0x09, 0x00, 0x00, 0x02,

    /* GC Registers */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x0F, 0x0F, 0xFF,

    /* CRTC Registers */
    0x2D, 0x27, 0x28, 0x90, 0x2B, 0x80, 0xBF, 0x1F, 0x00, 0xC1, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x9C, 0x8E, 0x8F, 0x14, 0x00, 0x96, 0xB9, 0xA2,
    0xFF,

    /* AC Registers */
    0x00, 0x13, 0x15, 0x17, 0x02, 0x04, 0x06, 0x07, 0x10, 0x11, 0x12, 0x13,
    0x14, 0x15, 0x16, 0x17, 0x01, 0x00, 0x0F, 0x00, 0x00
};

static BYTE VideoMode_640x200_2color[] =
{
    /* Miscellaneous Register */
    0x63,

    /* Sequencer Registers */
    0x00, 0x09, 0x0F, 0x00, 0x02,

    /* GC Registers */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0xFF,

    /* CRTC Registers */
    0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F, 0x00, 0xC1, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x9C, 0x8E, 0x8F, 0x28, 0x00, 0x96, 0xB9, 0xC2,
    0xFF,

    /* AC Registers */
    0x00, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17,
    0x17, 0x17, 0x17, 0x17, 0x01, 0x00, 0x01, 0x00, 0x00
};

static BYTE VideoMode_320x200_16color[] =
{
    /* Miscellaneous Register */
    0x63,

    /* Sequencer Registers */
    0x00, 0x09, 0x0F, 0x00, 0x02,

    /* GC Registers */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x0F, 0xFF,

    /* CRTC Registers */
    0x2D, 0x27, 0x28, 0x90, 0x2B, 0x80, 0xBF, 0x1F, 0x00, 0xC0, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x9C, 0x8E, 0x8F, 0x14, 0x00, 0x96, 0xB9, 0xE3,
    0xFF,

    /* AC Registers */
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x10, 0x11, 0x12, 0x13,
    0x14, 0x15, 0x16, 0x17, 0x01, 0x00, 0x0F, 0x00, 0x00
};

static BYTE VideoMode_640x200_16color[] =
{
    /* Miscellaneous Register */
    0x63,

    /* Sequencer Registers */
    0x00, 0x01, 0x0F, 0x00, 0x02,

    /* GC Registers */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x0F, 0xFF,

    /* CRTC Registers */
    0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F, 0x00, 0xC0, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x9C, 0x8E, 0x8F, 0x28, 0x00, 0x96, 0xB9, 0xE3,
    0xFF,

    /* AC Registers */
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x10, 0x11, 0x12, 0x13,
    0x14, 0x15, 0x16, 0x17, 0x01, 0x00, 0x0F, 0x00, 0x00
};

static BYTE VideoMode_640x350_16color[] =
{
    /* Miscellaneous Register */
    0xA3,

    /* Sequencer Registers */
    0x00, 0x01, 0x0F, 0x00, 0x02,

    /* GC Registers */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x0F, 0xFF,

    /* CRTC Registers */
    0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F, 0x00, 0x40, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x83, 0x85, 0x5D, 0x28, 0x0F, 0x63, 0xBA, 0xE3,
    0xFF,

    /* AC Registers */
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07, 0x38, 0x39, 0x3A, 0x3B,
    0x3C, 0x3D, 0x3E, 0x3F, 0x01, 0x00, 0x0F, 0x00, 0x00
};

static BYTE VideoMode_640x480_2color[] =
{
    /* Miscellaneous Register */
    0xE3,

    /* Sequencer Registers */
    0x00, 0x01, 0x0F, 0x00, 0x02,

    /* GC Registers */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x0F, 0xFF,

    /* CRTC Registers */
    0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0x0B, 0x3E, 0x00, 0x40, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xEA, 0x8C, 0xDF, 0x28, 0x00, 0xE7, 0x04, 0xC3,
    0xFF,

    /* AC Registers */
    0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
    0x3F, 0x3F, 0x3F, 0x3F, 0x01, 0x00, 0x0F, 0x00, 0x00
};

static BYTE VideoMode_640x480_16color[] =
{
    /* Miscellaneous Register */
    0xE3,

    /* Sequencer Registers */
    0x00, 0x01, 0x0F, 0x00, 0x02,

    /* GC Registers */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x0F, 0xFF,

    /* CRTC Registers */
    0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0x0B, 0x3E, 0x00, 0x40, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xEA, 0x8C, 0xDF, 0x28, 0x00, 0xE7, 0x04, 0xE3,
    0xFF,

    /* AC Registers */
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07, 0x38, 0x39, 0x3A, 0x3B,
    0x3C, 0x3D, 0x3E, 0x3F, 0x01, 0x00, 0x0F, 0x00, 0x00
};

static BYTE VideoMode_320x200_256color[] =
{
    /* Miscellaneous Register */
    0x63,

    /* Sequencer Registers */
    0x00, 0x01, 0x0F, 0x00, 0x0E,

    /* GC Registers */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F, 0xFF,

    /* CRTC Registers */
    0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F, 0x00, 0x41, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x9C, 0x8E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3,
    0xFF,

    /* AC Registers */
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
    0x0C, 0x0D, 0x0E, 0x0F, 0x41, 0x00, 0x0F, 0x00, 0x00
};

static LPBYTE VideoModes[] =
{
    VideoMode_40x25_text,       /* Mode 00h */
    VideoMode_40x25_text,       /* Mode 01h */
    VideoMode_80x25_text,       /* Mode 02h */
    VideoMode_80x25_text,       /* Mode 03h */
    VideoMode_320x200_4color,   /* Mode 04h */
    VideoMode_320x200_4color,   /* Mode 05h */
    VideoMode_640x200_2color,   /* Mode 06h */
    NULL,                       /* Mode 07h */
    NULL,                       /* Mode 08h */
    NULL,                       /* Mode 09h */
    NULL,                       /* Mode 0Ah */
    NULL,                       /* Mode 0Bh */
    NULL,                       /* Mode 0Ch */
    VideoMode_320x200_16color,  /* Mode 0Dh */
    VideoMode_640x200_16color,  /* Mode 0Eh */
    NULL,                       /* Mode 0Fh */
    VideoMode_640x350_16color,  /* Mode 10h */
    VideoMode_640x480_2color,   /* Mode 11h */
    VideoMode_640x480_16color,  /* Mode 12h */
    VideoMode_320x200_256color, /* Mode 13h */
};

/* PRIVATE FUNCTIONS **********************************************************/

static BOOLEAN BiosKbdBufferPush(WORD Data)
{
    /* Get the location of the element after the tail */
    WORD NextElement = Bda->KeybdBufferTail + 2;

    /* Wrap it around if it's at or beyond the end */
    if (NextElement >= Bda->KeybdBufferEnd) NextElement = Bda->KeybdBufferStart;

    /* If it's full, fail */
    if (NextElement == Bda->KeybdBufferHead) return FALSE;

    /* Put the value in the queue */
    *((LPWORD)((ULONG_PTR)Bda + Bda->KeybdBufferTail)) = Data;
    Bda->KeybdBufferTail += sizeof(WORD);

    /* Check if we are at, or have passed, the end of the buffer */
    if (Bda->KeybdBufferTail >= Bda->KeybdBufferEnd)
    {
        /* Return it to the beginning */
        Bda->KeybdBufferTail = Bda->KeybdBufferStart;
    }

    /* Return success */
    return TRUE;
}

static BOOLEAN BiosKbdBufferTop(LPWORD Data)
{
    /* If it's empty, fail */
    if (Bda->KeybdBufferHead == Bda->KeybdBufferTail) return FALSE;

    /* Otherwise, get the value and return success */
    *Data = *((LPWORD)((ULONG_PTR)Bda + Bda->KeybdBufferHead));

    return TRUE;
}

static BOOLEAN BiosKbdBufferPop(VOID)
{
    /* If it's empty, fail */
    if (Bda->KeybdBufferHead == Bda->KeybdBufferTail) return FALSE;

    /* Remove the value from the queue */
    Bda->KeybdBufferHead += sizeof(WORD);

    /* Check if we are at, or have passed, the end of the buffer */
    if (Bda->KeybdBufferHead >= Bda->KeybdBufferEnd)
    {
        /* Return it to the beginning */
        Bda->KeybdBufferHead = Bda->KeybdBufferStart;
    }

    /* Return success */
    return TRUE;
}

static VOID BiosReadWindow(LPWORD Buffer, SMALL_RECT Rectangle, BYTE Page)
{
    INT i, j;
    INT Counter = 0;
    WORD Character;
    DWORD VideoAddress = TO_LINEAR(TEXT_VIDEO_SEG, Page * Bda->VideoPageSize);

    for (i = Rectangle.Top; i <= Rectangle.Bottom; i++)
    {
        for (j = Rectangle.Left; j <= Rectangle.Right; j++)
        {
            /* Read from video memory */
            VgaReadMemory(VideoAddress + (i * Bda->ScreenColumns + j) * sizeof(WORD),
                          (LPVOID)&Character,
                          sizeof(WORD));

            /* Write the data to the buffer in row order */
            Buffer[Counter++] = Character;
        }
    }
}

static VOID BiosWriteWindow(LPWORD Buffer, SMALL_RECT Rectangle, BYTE Page)
{
    INT i, j;
    INT Counter = 0;
    WORD Character;
    DWORD VideoAddress = TO_LINEAR(TEXT_VIDEO_SEG, Page * Bda->VideoPageSize);

    for (i = Rectangle.Top; i <= Rectangle.Bottom; i++)
    {
        for (j = Rectangle.Left; j <= Rectangle.Right; j++)
        {
            Character = Buffer[Counter++];

            /* Write to video memory */
            VgaWriteMemory(VideoAddress + (i * Bda->ScreenColumns + j) * sizeof(WORD),
                           (LPVOID)&Character,
                           sizeof(WORD));
        }
    }
}

/* PUBLIC FUNCTIONS ***********************************************************/

BYTE BiosGetVideoMode(VOID)
{
    return Bda->VideoMode;
}

BOOLEAN BiosSetVideoMode(BYTE ModeNumber)
{
    INT i;
    COORD Resolution;
    LPBYTE Values = VideoModes[ModeNumber];

    if (Values == NULL) return FALSE;

    /* Write the misc register */
    VgaWritePort(VGA_MISC_WRITE, *(Values++));

    /* Write the sequencer registers */
    for (i = 0; i < VGA_SEQ_MAX_REG; i++)
    {
        VgaWritePort(VGA_SEQ_INDEX, i);
        VgaWritePort(VGA_SEQ_DATA, *(Values++));
    }

    /* Write the GC registers */
    for (i = 0; i < VGA_GC_MAX_REG; i++)
    {
        VgaWritePort(VGA_GC_INDEX, i);
        VgaWritePort(VGA_GC_DATA, *(Values++));
    }

    /* Write the CRTC registers */
    for (i = 0; i < VGA_CRTC_MAX_REG; i++)
    {
        VgaWritePort(VGA_CRTC_INDEX, i);
        VgaWritePort(VGA_CRTC_DATA, *(Values++));
    }

    /* Write the AC registers */
    for (i = 0; i < VGA_AC_MAX_REG; i++)
    {
        VgaWritePort(VGA_AC_INDEX, i);
        VgaWritePort(VGA_AC_WRITE, *(Values++));
    }

    /* Update the values in the BDA */
    Bda->VideoMode = ModeNumber;
    Bda->VideoPage = 0;
    Bda->VideoPageSize = BIOS_PAGE_SIZE;
    Bda->VideoPageOffset = 0;

    /* Get the character height */
    VgaWritePort(VGA_CRTC_INDEX, VGA_CRTC_MAX_SCAN_LINE_REG);
    Bda->CharacterHeight = 1 + (VgaReadPort(VGA_CRTC_DATA) & 0x1F);

    Resolution = VgaGetDisplayResolution();
    Bda->ScreenColumns = Resolution.X;
    Bda->ScreenRows = Resolution.Y - 1;

    return TRUE;
}

BOOLEAN BiosSetVideoPage(BYTE PageNumber)
{
    /* Check if the page exists */
    if (PageNumber >= BIOS_MAX_PAGES) return FALSE;

    /* Check if this is the same page */
    if (PageNumber == Bda->VideoPage) return TRUE;

    /* Set the values in the BDA */
    Bda->VideoPage = PageNumber;
    Bda->VideoPageSize = BIOS_PAGE_SIZE;
    Bda->VideoPageOffset = PageNumber * BIOS_PAGE_SIZE;

    /* Set the start address in the CRTC */
    VgaWritePort(VGA_CRTC_INDEX, VGA_CRTC_CURSOR_LOC_LOW_REG);
    VgaWritePort(VGA_CRTC_DATA , LOBYTE(Bda->VideoPageOffset));
    VgaWritePort(VGA_CRTC_INDEX, VGA_CRTC_CURSOR_LOC_HIGH_REG);
    VgaWritePort(VGA_CRTC_DATA , HIBYTE(Bda->VideoPageOffset));

    return TRUE;
}

BOOLEAN BiosInitialize(VOID)
{
    INT i;
    WORD Offset = 0;
    LPWORD IntVecTable = (LPWORD)((ULONG_PTR)BaseAddress);
    LPBYTE BiosCode = (LPBYTE)((ULONG_PTR)BaseAddress + TO_LINEAR(BIOS_SEGMENT, 0));

    /* Initialize the BDA */
    Bda = (PBIOS_DATA_AREA)((ULONG_PTR)BaseAddress + TO_LINEAR(BDA_SEGMENT, 0));
    Bda->EquipmentList = BIOS_EQUIPMENT_LIST;
    Bda->KeybdBufferStart = FIELD_OFFSET(BIOS_DATA_AREA, KeybdBuffer);
    Bda->KeybdBufferEnd = Bda->KeybdBufferStart + BIOS_KBD_BUFFER_SIZE * sizeof(WORD);

    /* Generate ISR stubs and fill the IVT */
    for (i = 0; i < 256; i++)
    {
        IntVecTable[i * 2] = Offset;
        IntVecTable[i * 2 + 1] = BIOS_SEGMENT;

        BiosCode[Offset++] = 0xFB; // sti

        BiosCode[Offset++] = 0x6A; // push i
        BiosCode[Offset++] = (BYTE)i;

        BiosCode[Offset++] = 0x6A; // push 0
        BiosCode[Offset++] = 0x00;

        BiosCode[Offset++] = 0xF8; // clc

        BiosCode[Offset++] = LOBYTE(EMULATOR_BOP); // BOP sequence
        BiosCode[Offset++] = HIBYTE(EMULATOR_BOP);
        BiosCode[Offset++] = LOBYTE(EMULATOR_INT_BOP);
        BiosCode[Offset++] = HIBYTE(EMULATOR_INT_BOP);

        BiosCode[Offset++] = 0x73; // jnc +3
        BiosCode[Offset++] = 0x03;

        // HACK: The following instruction should be HLT!
        BiosCode[Offset++] = 0x90; // nop

        BiosCode[Offset++] = 0xEB; // jmp -10
        BiosCode[Offset++] = 0xF6;

        BiosCode[Offset++] = 0x83; // add sp, 4
        BiosCode[Offset++] = 0xC4;
        BiosCode[Offset++] = 0x04;

        BiosCode[Offset++] = 0xCF; // iret
    }

    /* Get the input handle to the real console, and check for success */
    BiosConsoleInput = CreateFileW(L"CONIN$",
                                   GENERIC_READ | GENERIC_WRITE,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   NULL,
                                   OPEN_EXISTING,
                                   0,
                                   NULL);
    if (BiosConsoleInput == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    /* Get the output handle to the real console, and check for success */
    BiosConsoleOutput = CreateFileW(L"CONOUT$",
                                    GENERIC_READ | GENERIC_WRITE,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL,
                                    OPEN_EXISTING,
                                    0,
                                    NULL);
    if (BiosConsoleOutput == INVALID_HANDLE_VALUE)
    {
        CloseHandle(BiosConsoleInput);
        return FALSE;
    }

    /* Save the console screen buffer information */
    if (!GetConsoleScreenBufferInfo(BiosConsoleOutput, &BiosSavedBufferInfo))
    {
        CloseHandle(BiosConsoleOutput);
        CloseHandle(BiosConsoleInput);
        return FALSE;
    }

    /* Initialize VGA */
    if (!VgaInitialize(BiosConsoleOutput))
    {
        CloseHandle(BiosConsoleOutput);
        CloseHandle(BiosConsoleInput);
        return FALSE;
    }

    /* Update the cursor position */
    BiosSetCursorPosition(BiosSavedBufferInfo.dwCursorPosition.Y,
                          BiosSavedBufferInfo.dwCursorPosition.X,
                          0);

    /* Set the console input mode */
    SetConsoleMode(BiosConsoleInput, ENABLE_MOUSE_INPUT | ENABLE_PROCESSED_INPUT);

    /* Initialize the PIC */
    PicWriteCommand(PIC_MASTER_CMD, PIC_ICW1 | PIC_ICW1_ICW4);
    PicWriteCommand(PIC_SLAVE_CMD , PIC_ICW1 | PIC_ICW1_ICW4);

    /* Set the interrupt offsets */
    PicWriteData(PIC_MASTER_DATA, BIOS_PIC_MASTER_INT);
    PicWriteData(PIC_SLAVE_DATA , BIOS_PIC_SLAVE_INT);

    /* Tell the master PIC there is a slave at IRQ 2 */
    PicWriteData(PIC_MASTER_DATA, 1 << 2);
    PicWriteData(PIC_SLAVE_DATA , 2);

    /* Make sure the PIC is in 8086 mode */
    PicWriteData(PIC_MASTER_DATA, PIC_ICW4_8086);
    PicWriteData(PIC_SLAVE_DATA , PIC_ICW4_8086);

    /* Clear the masks for both PICs */
    PicWriteData(PIC_MASTER_DATA, 0x00);
    PicWriteData(PIC_SLAVE_DATA , 0x00);

    PitWriteCommand(0x34);
    PitWriteData(0, 0x00);
    PitWriteData(0, 0x00);

    return TRUE;
}

VOID BiosCleanup(VOID)
{
    /* Restore the old screen buffer */
    SetConsoleActiveScreenBuffer(BiosConsoleOutput);

    /* Restore the screen buffer size */
    SetConsoleScreenBufferSize(BiosConsoleOutput, BiosSavedBufferInfo.dwSize);

    /* Close the console handles */
    if (BiosConsoleOutput != INVALID_HANDLE_VALUE) CloseHandle(BiosConsoleOutput);
    if (BiosConsoleInput  != INVALID_HANDLE_VALUE) CloseHandle(BiosConsoleInput);
}

WORD BiosPeekCharacter(VOID)
{
    WORD CharacterData;
    
    /* Check if there is a key available */
    if (Bda->KeybdBufferHead == Bda->KeybdBufferTail) return 0xFFFF;

    /* Get the key from the queue, but don't remove it */
    BiosKbdBufferTop(&CharacterData);

    return CharacterData;
}

WORD BiosGetCharacter(VOID)
{
    WORD CharacterData = 0;

    /* Check if there is a key available */
    if (Bda->KeybdBufferHead != Bda->KeybdBufferTail)
    {
        /* Get the key from the queue, and remove it */
        BiosKbdBufferTop(&CharacterData);
        BiosKbdBufferPop();
    }
    else
    {
        /* Set the handler CF to repeat the BOP */
        EmulatorSetFlag(EMULATOR_FLAG_CF);
    }

    return CharacterData;
}

VOID BiosSetCursorPosition(BYTE Row, BYTE Column, BYTE Page)
{
    /* Make sure the selected video page is valid */
    if (Page >= BIOS_MAX_PAGES) return;

    /* Update the position in the BDA */
    Bda->CursorPosition[Page] = (Row << 8) | Column;

    /* Check if this is the current video page */
    if (Page == Bda->VideoPage)
    {
        WORD Offset = Row * Bda->ScreenColumns + Column;

        /* Modify the CRTC registers */
        VgaWritePort(VGA_CRTC_INDEX, VGA_CRTC_CURSOR_LOC_LOW_REG);
        VgaWritePort(VGA_CRTC_DATA , LOBYTE(Offset));
        VgaWritePort(VGA_CRTC_INDEX, VGA_CRTC_CURSOR_LOC_HIGH_REG);
        VgaWritePort(VGA_CRTC_DATA , HIBYTE(Offset));
    }
}

BOOLEAN BiosScrollWindow(INT Direction,
                         DWORD Amount,
                         SMALL_RECT Rectangle,
                         BYTE Page,
                         BYTE FillAttribute)
{
    DWORD i;
    LPWORD WindowData;
    DWORD WindowSize = (Rectangle.Bottom - Rectangle.Top + 1)
                       * (Rectangle.Right - Rectangle.Left + 1);

    /* Allocate a buffer for the window */
    WindowData = (LPWORD)HeapAlloc(GetProcessHeap(),
                                   HEAP_ZERO_MEMORY,
                                   WindowSize * sizeof(WORD));
    if (WindowData == NULL) return FALSE;

    /* Read the window data */
    BiosReadWindow(WindowData, Rectangle, Page);

    if (Amount == 0)
    {
        /* Fill the window */
        for (i = 0; i < WindowSize; i++)
        {
            WindowData[i] = ' ' | (FillAttribute << 8);
        }

        goto Done;
    }

    // TODO: Scroll the window!

Done:
    /* Write back the window data */
    BiosWriteWindow(WindowData, Rectangle, Page);

    /* Free the window buffer */
    HeapFree(GetProcessHeap(), 0, WindowData);

    return TRUE;
}

VOID BiosPrintCharacter(CHAR Character, BYTE Attribute, BYTE Page)
{
    WORD CharData = (Attribute << 8) | Character;
    BYTE Row, Column;

    /* Make sure the page exists */
    if (Page >= BIOS_MAX_PAGES) return;

    /* Get the cursor location */
    Row = HIBYTE(Bda->CursorPosition[Page]);
    Column = LOBYTE(Bda->CursorPosition[Page]);

    if (Character == '\a')
    {
        /* Bell control character */
        // NOTE: We may use what the terminal emulator offers to us...
        Beep(800, 200);
        return;
    }
    else if (Character == '\b')
    {
        /* Backspace control character */
        if (Column > 0)
        {
            Column--;
        }
        else if (Row > 0)
        {
            Column = Bda->ScreenColumns - 1;
            Row--;
        }

        /* Erase the existing character */
        CharData = (Attribute << 8) | ' ';
        VgaWriteMemory(TO_LINEAR(TEXT_VIDEO_SEG,
                       Page * Bda->VideoPageSize
                       + (Row * Bda->ScreenColumns + Column) * sizeof(WORD)),
                       (LPVOID)&CharData,
                       sizeof(WORD));
    }
    else if (Character == '\n')
    {
        /* Line Feed control character */
        Row++;
    }
    else if (Character == '\r')
    {
        /* Carriage Return control character */
        Column = 0;
    }
    else
    {
        /* Default character */

        /* Write the character */
        VgaWriteMemory(TO_LINEAR(TEXT_VIDEO_SEG,
                       Page * Bda->VideoPageSize
                       + (Row * Bda->ScreenColumns + Column) * sizeof(WORD)),
                       (LPVOID)&CharData,
                       sizeof(WORD));

        /* Advance the cursor */
        Column++;
    }

    /* Check if it passed the end of the row */
    if (Column >= Bda->ScreenColumns)
    {
        /* Return to the first column and go to the next line */
        Column = 0;
        Row++;
    }

    /* Scroll the screen up if needed */
    if (Row > Bda->ScreenRows)
    {
        /* The screen must be scrolled up */
        SMALL_RECT Rectangle = { 0, 0, Bda->ScreenColumns - 1, Bda->ScreenRows };

        BiosScrollWindow(SCROLL_DIRECTION_UP,
                         1,
                         Rectangle,
                         Page,
                         DEFAULT_ATTRIBUTE);
    }

    /* Set the cursor position */
    BiosSetCursorPosition(Row, Column, Page);
}

VOID BiosVideoService(LPWORD Stack)
{
    DWORD Eax = EmulatorGetRegister(EMULATOR_REG_AX);
    DWORD Ecx = EmulatorGetRegister(EMULATOR_REG_CX);
    DWORD Edx = EmulatorGetRegister(EMULATOR_REG_DX);
    DWORD Ebx = EmulatorGetRegister(EMULATOR_REG_BX);

    switch (HIBYTE(Eax))
    {
        /* Set Video Mode */
        case 0x00:
        {
            BiosSetVideoMode(LOBYTE(Eax));
            VgaClearMemory();
            break;
        }

        /* Set Text-Mode Cursor Shape */
        case 0x01:
        {
            /* Update the BDA */
            Bda->CursorStartLine = HIBYTE(Ecx);
            Bda->CursorEndLine = LOBYTE(Ecx);

            /* Modify the CRTC registers */
            VgaWritePort(VGA_CRTC_INDEX, VGA_CRTC_CURSOR_START_REG);
            VgaWritePort(VGA_CRTC_DATA , Bda->CursorStartLine);
            VgaWritePort(VGA_CRTC_INDEX, VGA_CRTC_CURSOR_END_REG);
            VgaWritePort(VGA_CRTC_DATA , Bda->CursorEndLine);

            break;
        }

        /* Set Cursor Position */
        case 0x02:
        {
            BiosSetCursorPosition(HIBYTE(Edx), LOBYTE(Edx), HIBYTE(Ebx));
            break;
        }

        /* Get Cursor Position */
        case 0x03:
        {
            /* Make sure the selected video page exists */
            if (HIBYTE(Ebx) >= BIOS_MAX_PAGES) break;

            /* Return the result */
            EmulatorSetRegister(EMULATOR_REG_AX, 0);
            EmulatorSetRegister(EMULATOR_REG_CX,
                                (Bda->CursorStartLine << 8) | Bda->CursorEndLine);
            EmulatorSetRegister(EMULATOR_REG_DX, Bda->CursorPosition[HIBYTE(Ebx)]);

            break;
        }

        /* Query Light Pen */
        case 0x04:
        {
            /*
             * On modern BIOSes, this function returns 0
             * so that we can ignore the other registers.
             */
            EmulatorSetRegister(EMULATOR_REG_AX, 0);
            break;
        }

        /* Select Active Display Page */
        case 0x05:
        {
            BiosSetVideoPage(LOBYTE(Eax));
            break;
        }

        /* Scroll Window Up/Down */
        case 0x06:
        case 0x07:
        {
            SMALL_RECT Rectangle =
            {
                LOBYTE(Ecx),
                HIBYTE(Ecx),
                LOBYTE(Edx),
                HIBYTE(Edx)
            };

            /* Call the internal function */
            BiosScrollWindow((HIBYTE(Eax) == 0x06) ? SCROLL_DIRECTION_UP
                                                   : SCROLL_DIRECTION_DOWN,
                             LOBYTE(Eax),
                             Rectangle,
                             Bda->VideoPage,
                             HIBYTE(Ebx));

            break;
        }

        /* Read/Write Character From Cursor Position */
        case 0x08:
        case 0x09:
        case 0x0A:
        {
            WORD CharacterData = MAKEWORD(LOBYTE(Eax), LOBYTE(Ebx));
            BYTE Page = HIBYTE(Ebx);
            DWORD Offset;

            /* Check if the page exists */
            if (Page >= BIOS_MAX_PAGES) break;

            /* Find the offset of the character */
            Offset = Page * Bda->VideoPageSize
                     + (HIBYTE(Bda->CursorPosition[Page]) * Bda->ScreenColumns
                     + LOBYTE(Bda->CursorPosition[Page])) * 2;

            if (HIBYTE(Eax) == 0x08)
            {
                /* Read from the video memory */
                VgaReadMemory(TO_LINEAR(TEXT_VIDEO_SEG, Offset),
                              (LPVOID)&CharacterData,
                              sizeof(WORD));

                /* Return the character in AX */
                EmulatorSetRegister(EMULATOR_REG_AX, CharacterData);
            }
            else
            {
                /* Write to video memory */
                VgaWriteMemory(TO_LINEAR(TEXT_VIDEO_SEG, Offset),
                               (LPVOID)&CharacterData,
                               (HIBYTE(Ebx) == 0x09) ? sizeof(WORD) : sizeof(BYTE));
            }

            break;
        }

        /* Teletype Output */
        case 0x0E:
        {
            BiosPrintCharacter(LOBYTE(Eax), LOBYTE(Ebx), HIBYTE(Ebx));
            break;
        }

        /* Get Current Video Mode */
        case 0x0F:
        {
            EmulatorSetRegister(EMULATOR_REG_AX,
                                MAKEWORD(Bda->VideoMode, Bda->ScreenColumns));
            EmulatorSetRegister(EMULATOR_REG_BX,
                                MAKEWORD(LOBYTE(Ebx), Bda->VideoPage));

            break;
        }

        /* Scroll Window */
        case 0x12:
        {
            SMALL_RECT Rectangle =
            {
                LOBYTE(Ecx),
                HIBYTE(Ecx),
                LOBYTE(Edx),
                HIBYTE(Edx)
            };

            /* Call the internal function */
            BiosScrollWindow(LOBYTE(Ebx),
                             LOBYTE(Eax),
                             Rectangle,
                             Bda->VideoPage,
                             DEFAULT_ATTRIBUTE);

            break;
        }

        /* Display combination code */
        case 0x1A:
        {
            switch(LOBYTE(Eax))
            {
                case 0x00: /* Get Display combiantion code */
                   EmulatorSetRegister(EMULATOR_REG_AX, MAKEWORD(0x1A, 0x1A));
                   EmulatorSetRegister(EMULATOR_REG_BX, MAKEWORD(0x08, 0x0)); /* VGA w/ color analog display */
                   break;
                case 0x01: /* Set Display combination code */
                   DPRINT1("Set Display combination code - Unsupported\n");
                   break;
                default:
                   break;
            }
            break;
        }

        default:
        {
            DPRINT1("BIOS Function INT 10h, AH = 0x%02X NOT IMPLEMENTED\n",
                    HIBYTE(Eax));
        }
    }
}

VOID BiosKeyboardService(LPWORD Stack)
{
    DWORD Eax = EmulatorGetRegister(EMULATOR_REG_AX);

    switch (HIBYTE(Eax))
    {
        /* Wait for keystroke and read */
        case 0x00:
        /* Wait for extended keystroke and read */
        case 0x10:  // FIXME: Temporarily do the same as INT 16h, 00h
        {
            /* Read the character (and wait if necessary) */
            EmulatorSetRegister(EMULATOR_REG_AX, BiosGetCharacter());
            break;
        }

        /* Get keystroke status */
        case 0x01:
        /* Get extended keystroke status */
        case 0x11:  // FIXME: Temporarily do the same as INT 16h, 01h
        {
            WORD Data = BiosPeekCharacter();

            if (Data != 0xFFFF)
            {
                /* There is a character, clear ZF and return it */
                EmulatorSetRegister(EMULATOR_REG_AX, Data);
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_ZF;
            }
            else
            {
                /* No character, set ZF */
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_ZF;
            }

            break;
        }

        /* Get shift status */
        case 0x02:
        {
            /* Return the lower byte of the keyboard shift status word */
            setAL(LOBYTE(Bda->KeybdShiftFlags));
            break;
        }

        /* Reserved */
        case 0x04:
        {
            DPRINT1("BIOS Function INT 16h, AH = 0x04 is RESERVED\n");
            break;
        }

        /* Push keystroke */
        case 0x05:
        {
            /* Return 0 if success, 1 if failure */
            setAL(BiosKbdBufferPush(getCX()) == FALSE);
            break;
        }

        /* Get extended shift status */
        case 0x12:
        {
            /*
             * Be careful! The returned word is similar to Bda->KeybdShiftFlags
             * but the high byte is organized differently:
             * the bytes 2 and 3 of the high byte are not the same...
             */
            WORD KeybdShiftFlags = (Bda->KeybdShiftFlags & 0xF3FF);

            /* Return the extended keyboard shift status word */
            setAX(KeybdShiftFlags);
            break;
        }

        default:
        {
            DPRINT1("BIOS Function INT 16h, AH = 0x%02X NOT IMPLEMENTED\n",
                    HIBYTE(Eax));
        }
    }
}

VOID BiosTimeService(LPWORD Stack)
{
    DWORD Eax = EmulatorGetRegister(EMULATOR_REG_AX);
    DWORD Ecx = EmulatorGetRegister(EMULATOR_REG_CX);
    DWORD Edx = EmulatorGetRegister(EMULATOR_REG_DX);

    switch (HIBYTE(Eax))
    {
        case 0x00:
        {
            /* Set AL to 1 if midnight had passed, 0 otherwise */
            Eax &= 0xFFFFFF00;
            if (Bda->MidnightPassed) Eax |= 1;

            /* Return the tick count in CX:DX */
            EmulatorSetRegister(EMULATOR_REG_AX, Eax);
            EmulatorSetRegister(EMULATOR_REG_CX, HIWORD(Bda->TickCounter));
            EmulatorSetRegister(EMULATOR_REG_DX, LOWORD(Bda->TickCounter));

            /* Reset the midnight flag */
            Bda->MidnightPassed = FALSE;

            break;
        }

        case 0x01:
        {
            /* Set the tick count to CX:DX */
            Bda->TickCounter = MAKELONG(LOWORD(Edx), LOWORD(Ecx));

            /* Reset the midnight flag */
            Bda->MidnightPassed = FALSE;

            break;
        }

        default:
        {
            DPRINT1("BIOS Function INT 1Ah, AH = 0x%02X NOT IMPLEMENTED\n",
                    HIBYTE(Eax));
        }
    }
}

VOID BiosSystemTimerInterrupt(LPWORD Stack)
{
    /* Increase the system tick count */
    Bda->TickCounter++;
}

VOID BiosEquipmentService(LPWORD Stack)
{
    /* Return the equipment list */
    EmulatorSetRegister(EMULATOR_REG_AX, Bda->EquipmentList);
}

VOID BiosHandleIrq(BYTE IrqNumber, LPWORD Stack)
{
    switch (IrqNumber)
    {
        /* PIT IRQ */
        case 0:
        {
            /* Perform the system timer interrupt */
            EmulatorInterrupt(BIOS_SYS_TIMER_INTERRUPT);

            break;
        }

        /* Keyboard IRQ */
        case 1:
        {
            BYTE ScanCode, VirtualKey;
            WORD Character;
            
            /* Loop while there is a scancode available */
            while (KeyboardReadStatus() & 1)
            {
                /* Get the scan code and virtual key code */
                ScanCode = KeyboardReadData();
                VirtualKey = MapVirtualKey(ScanCode & 0x7F, MAPVK_VSC_TO_VK);

                /* Check if this is a key press or release */
                if (!(ScanCode & (1 << 7)))
                {
                    /* Key press */
                    if (VirtualKey == VK_NUMLOCK
                        || VirtualKey == VK_CAPITAL
                        || VirtualKey == VK_SCROLL
                        || VirtualKey == VK_INSERT)
                    {
                        /* For toggle keys, toggle the lowest bit in the keyboard map */
                        BiosKeyboardMap[VirtualKey] ^= ~(1 << 0);
                    }
    
                    /* Set the highest bit */
                    BiosKeyboardMap[VirtualKey] |= (1 << 7);

                    /* Find out which character this is */
                    Character = 0;
                    if (ToAscii(VirtualKey, ScanCode, BiosKeyboardMap, &Character, 0) == 0)
                    {
                        /* Not ASCII */
                        Character = 0;
                    }

                    /* Push it onto the BIOS keyboard queue */
                    BiosKbdBufferPush((ScanCode << 8) | (Character & 0xFF));

                }
                else
                {
                    /* Key release, unset the highest bit */
                    BiosKeyboardMap[VirtualKey] &= ~(1 << 7);
                }
            }

            /* Clear the keyboard flags */
            Bda->KeybdShiftFlags = 0;

            /* Set the appropriate flags based on the state */
            if (BiosKeyboardMap[VK_RSHIFT] & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_RSHIFT;
            if (BiosKeyboardMap[VK_LSHIFT] & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_LSHIFT;
            if (BiosKeyboardMap[VK_CONTROL] & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_CTRL;
            if (BiosKeyboardMap[VK_MENU] & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_ALT;
            if (BiosKeyboardMap[VK_SCROLL] & (1 << 0)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_SCROLL_ON;
            if (BiosKeyboardMap[VK_NUMLOCK] & (1 << 0)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_NUMLOCK_ON;
            if (BiosKeyboardMap[VK_CAPITAL] & (1 << 0)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_CAPSLOCK_ON;
            if (BiosKeyboardMap[VK_INSERT] & (1 << 0)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_INSERT_ON;
            if (BiosKeyboardMap[VK_RMENU] & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_RALT;
            if (BiosKeyboardMap[VK_LMENU] & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_LALT;
            if (BiosKeyboardMap[VK_SNAPSHOT] & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_SYSRQ;
            if (BiosKeyboardMap[VK_PAUSE] & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_PAUSE;
            if (BiosKeyboardMap[VK_SCROLL] & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_SCROLL;
            if (BiosKeyboardMap[VK_NUMLOCK] & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_NUMLOCK;
            if (BiosKeyboardMap[VK_CAPITAL] & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_CAPSLOCK;
            if (BiosKeyboardMap[VK_INSERT] & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_INSERT;

            break;
        }
    }

    /* Send End-of-Interrupt to the PIC */
    if (IrqNumber > 8) PicWriteCommand(PIC_SLAVE_CMD, PIC_OCW2_EOI);
    PicWriteCommand(PIC_MASTER_CMD, PIC_OCW2_EOI);
}

/* EOF */
