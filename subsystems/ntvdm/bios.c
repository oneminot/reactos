/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            bios.c
 * PURPOSE:         VDM BIOS
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#include "bios.h"
#include "emulator.h"
#include "pic.h"
#include "ps2.h"
#include "timer.h"

/* PRIVATE VARIABLES **********************************************************/

static BYTE BiosKeyboardMap[256];
static WORD BiosKbdBuffer[BIOS_KBD_BUFFER_SIZE];
static UINT BiosKbdBufferStart = 0, BiosKbdBufferEnd = 0;
static BOOLEAN BiosKbdBufferEmpty = TRUE;
static DWORD BiosTickCount = 0;
static BOOLEAN BiosPassedMidnight = FALSE;
static HANDLE BiosConsoleInput = INVALID_HANDLE_VALUE;
static HANDLE BiosConsoleOutput = INVALID_HANDLE_VALUE;
static BYTE CurrentVideoMode = BIOS_DEFAULT_VIDEO_MODE;
static BYTE CurrentVideoPage = 0;
static HANDLE ConsoleBuffers[BIOS_MAX_PAGES] = { NULL };
static LPVOID ConsoleFramebuffers[BIOS_MAX_PAGES] = { NULL };
static HANDLE ConsoleMutexes[BIOS_MAX_PAGES] = { NULL };
static BOOLEAN VideoNeedsUpdate = TRUE;
static SMALL_RECT UpdateRectangle = { 0, 0, 0, 0 };
static CONSOLE_SCREEN_BUFFER_INFO BiosSavedBufferInfo;
static VIDEO_MODE VideoModes[] =
{
    /* Width | Height | Text | Colors | Gray | Pages | Segment */
    { 40,       25,     TRUE,   16,     TRUE,   8,      0xB800}, /* Mode 00h */
    { 40,       25,     TRUE,   16,     FALSE,  8,      0xB800}, /* Mode 01h */
    { 80,       25,     TRUE,   16,     TRUE,   8,      0xB800}, /* Mode 02h */
    { 80,       25,     TRUE,   16,     FALSE,  8,      0xB800}, /* Mode 03h */
    { 320,      200,    FALSE,  4,      FALSE,  4,      0xB800}, /* Mode 04h */
    { 320,      200,    FALSE,  4,      TRUE,   4,      0xB800}, /* Mode 05h */
    { 640,      200,    FALSE,  2,      FALSE,  2,      0xB800}, /* Mode 06h */
    { 80,       25,     TRUE,   3,      FALSE,  1,      0xB000}, /* Mode 07h */
    { 0,        0,      FALSE,  0,      FALSE,  0,      0x0000}, /* Mode 08h - not used */
    { 0,        0,      FALSE,  0,      FALSE,  0,      0x0000}, /* Mode 09h - not used */
    { 0,        0,      FALSE,  0,      FALSE,  0,      0x0000}, /* Mode 0Ah - not used */
    { 0,        0,      FALSE,  0,      FALSE,  0,      0x0000}, /* Mode 0Bh - not used */
    { 0,        0,      FALSE,  0,      FALSE,  0,      0x0000}, /* Mode 0Ch - not used */
    { 320,      200,    FALSE,  16,     FALSE,  8,      0xA000}, /* Mode 0Dh */
    { 640,      200,    FALSE,  16,     FALSE,  4,      0xA000}, /* Mode 0Eh */
    { 640,      350,    FALSE,  3,      FALSE,  2,      0xA000}, /* Mode 0Fh */
    { 640,      350,    FALSE,  4,      FALSE,  2,      0xA000}, /* Mode 10h */
    { 640,      480,    FALSE,  2,      FALSE,  1,      0xA000}, /* Mode 11h */
    { 640,      480,    FALSE,  16,     FALSE,  1,      0xA000}, /* Mode 12h */
    { 640,      480,    FALSE,  256,    FALSE,  1,      0xA000}  /* Mode 13h */
};

/* PRIVATE FUNCTIONS **********************************************************/

static INT BiosColorNumberToBits(DWORD Colors)
{
    INT i;

    /* Find the index of the highest-order bit */
    for (i = 31; i >= 0; i--) if (Colors & (1 << i)) break;

    /* Special case for zero */
    if (i == 0) i = 32;

    return i;
}

static COORD BiosVideoAddressToCoord(ULONG Address)
{
    COORD Result = {0, 0};
    INT BitsPerPixel;
    DWORD Offset = Address - (VideoModes[CurrentVideoMode].Segment << 4);

    if (VideoModes[CurrentVideoMode].Text)
    {
        Result.X = (Offset / sizeof(WORD)) % VideoModes[CurrentVideoMode].Width;
        Result.Y = (Offset / sizeof(WORD)) / VideoModes[CurrentVideoMode].Width;
    }
    else
    {
        BitsPerPixel = BiosColorNumberToBits(VideoModes[CurrentVideoMode].Colors);

        Result.X = ((Offset * 8) / BitsPerPixel)
                   % VideoModes[CurrentVideoMode].Width;
        Result.Y = ((Offset * 8) / BitsPerPixel)
                   / VideoModes[CurrentVideoMode].Width;
    }

    return Result;
}

static BOOLEAN BiosKbdBufferPush(WORD Data)
{
    /* If it's full, fail */
    if (!BiosKbdBufferEmpty && (BiosKbdBufferStart == BiosKbdBufferEnd))
    {
        return FALSE;
    }

    /* Otherwise, add the value to the queue */
    BiosKbdBuffer[BiosKbdBufferEnd] = Data;
    BiosKbdBufferEnd++;
    BiosKbdBufferEnd %= BIOS_KBD_BUFFER_SIZE;
    BiosKbdBufferEmpty = FALSE;

    /* Return success */
    return TRUE;
}

static BOOLEAN BiosKbdBufferTop(LPWORD Data)
{
    /* If it's empty, fail */
    if (BiosKbdBufferEmpty) return FALSE;

    /* Otherwise, get the value and return success */
    *Data = BiosKbdBuffer[BiosKbdBufferStart];
    return TRUE;
}

static BOOLEAN BiosKbdBufferPop()
{
    /* If it's empty, fail */
    if (BiosKbdBufferEmpty) return FALSE;

    /* Otherwise, remove the value and return success */
    BiosKbdBufferStart++;
    BiosKbdBufferStart %= BIOS_KBD_BUFFER_SIZE;
    if (BiosKbdBufferStart == BiosKbdBufferEnd) BiosKbdBufferEmpty = TRUE;

    return TRUE;
}

/* PUBLIC FUNCTIONS ***********************************************************/

BYTE BiosGetVideoMode()
{
    return CurrentVideoMode;
}

BOOLEAN BiosSetVideoMode(BYTE ModeNumber)
{
    INT i;
    COORD Coord;
    CONSOLE_GRAPHICS_BUFFER_INFO GraphicsBufferInfo;
    LPBITMAPINFO BitmapInfo;
    LPWORD PaletteIndex;

    /* Make sure this is a valid video mode */
    if (ModeNumber > BIOS_MAX_VIDEO_MODE) return FALSE;
    if (VideoModes[ModeNumber].Pages == 0) return FALSE;

    /* Free the current buffers */
    for (i = 0; i < VideoModes[CurrentVideoMode].Pages; i++)
    {
        if (ConsoleBuffers[i] != NULL) CloseHandle(ConsoleBuffers[i]);
        if (!VideoModes[CurrentVideoMode].Text) CloseHandle(ConsoleMutexes[i]);
    }

    if (VideoModes[ModeNumber].Text)
    {
        /* Page 0 is CONOUT$ */
        ConsoleBuffers[0] = CreateFile(TEXT("CONOUT$"),
                                       GENERIC_READ | GENERIC_WRITE,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                                       NULL,
                                       OPEN_EXISTING,
                                       0,
                                       NULL);

        /* Set the current page to page 0 */
        CurrentVideoPage = 0;

        /* Create console buffers for other pages */
        for (i = 1; i < VideoModes[ModeNumber].Pages; i++)
        {
            ConsoleBuffers[i] = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE,
                                                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                          NULL,
                                                          CONSOLE_TEXTMODE_BUFFER,
                                                          NULL);
        }

        /* Set the size for the buffers */
        for (i = 0; i < VideoModes[ModeNumber].Pages; i++)
        {
            Coord.X = VideoModes[ModeNumber].Width;
            Coord.Y = VideoModes[ModeNumber].Height;

            SetConsoleScreenBufferSize(ConsoleBuffers[i], Coord);
        }
    }
    else
    {
        /* Allocate a bitmap info structure */
        BitmapInfo = (LPBITMAPINFO)HeapAlloc(GetProcessHeap(),
                                             HEAP_ZERO_MEMORY,
                                             sizeof(BITMAPINFOHEADER)
                                             + VideoModes[ModeNumber].Colors
                                             * sizeof(WORD));
        if (BitmapInfo == NULL) return FALSE;

        /* Fill the bitmap info header */
        ZeroMemory(&BitmapInfo->bmiHeader, sizeof(BITMAPINFOHEADER));
        BitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        BitmapInfo->bmiHeader.biWidth = VideoModes[ModeNumber].Width;
        BitmapInfo->bmiHeader.biHeight = VideoModes[ModeNumber].Height;
        BitmapInfo->bmiHeader.biPlanes = 1;
        BitmapInfo->bmiHeader.biCompression = BI_RGB;
        BitmapInfo->bmiHeader.biBitCount = BiosColorNumberToBits(VideoModes[ModeNumber].Colors);

        /* Calculate the image size */
        BitmapInfo->bmiHeader.biSizeImage = BitmapInfo->bmiHeader.biWidth
                                            * BitmapInfo->bmiHeader.biHeight
                                            * (BitmapInfo->bmiHeader.biBitCount >> 3);

        /* Fill the palette data */
        PaletteIndex = (LPWORD)((ULONG_PTR)BitmapInfo + sizeof(BITMAPINFOHEADER));
        for (i = 0; i < VideoModes[ModeNumber].Colors; i++)
        {
            PaletteIndex[i] = i;
        }

        /* Create a console buffer for each page */
        for (i = 0; i < VideoModes[ModeNumber].Pages; i++)
        {
            /* Fill the console graphics buffer info */
            GraphicsBufferInfo.dwBitMapInfoLength = sizeof(BITMAPINFOHEADER)
                                                    + VideoModes[ModeNumber].Colors
                                                    * sizeof(WORD);
            GraphicsBufferInfo.lpBitMapInfo = BitmapInfo;
            GraphicsBufferInfo.dwUsage = DIB_PAL_COLORS;

            /* Create the buffer */
            ConsoleBuffers[i] = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE,
                                                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                          NULL,
                                                          CONSOLE_GRAPHICS_BUFFER,
                                                          &GraphicsBufferInfo);

            /* Save the framebuffer address and mutex */
            ConsoleFramebuffers[i] = GraphicsBufferInfo.lpBitMap;
            ConsoleMutexes[i] = GraphicsBufferInfo.hMutex;
        }

        /* Free the bitmap information */
        HeapFree(GetProcessHeap(), 0, BitmapInfo);
    }

    /* Set the active page console buffer */
    SetConsoleActiveScreenBuffer(ConsoleBuffers[CurrentVideoPage]);

    /* Update the mode number */
    CurrentVideoMode = ModeNumber;

    return TRUE;
}

inline DWORD BiosGetVideoMemoryStart()
{
    return (VideoModes[CurrentVideoMode].Segment << 4);
}

inline VOID BiosVerticalRefresh()
{
    /* Ignore if we're in text mode */
    if (VideoModes[CurrentVideoMode].Text) return;

    /* Ignore if there's nothing to update */
    if (!VideoNeedsUpdate) return;

    /* Redraw the screen */
    InvalidateConsoleDIBits(ConsoleBuffers[CurrentVideoPage],
                            &UpdateRectangle);

    /* Clear the update flag */
    VideoNeedsUpdate = FALSE;
}

BOOLEAN BiosInitialize()
{
    INT i;
    WORD Offset = 0;
    LPWORD IntVecTable = (LPWORD)((ULONG_PTR)BaseAddress);
    LPBYTE BiosCode = (LPBYTE)((ULONG_PTR)BaseAddress + TO_LINEAR(BIOS_SEGMENT, 0));

    /* Generate ISR stubs and fill the IVT */
    for (i = 0; i < 256; i++)
    {
        IntVecTable[i * 2] = Offset;
        IntVecTable[i * 2 + 1] = BIOS_SEGMENT;

        if (i != SPECIAL_INT_NUM)
        {
            BiosCode[Offset++] = 0xFA; // cli

            BiosCode[Offset++] = 0x6A; // push i
            BiosCode[Offset++] = (BYTE)i;

            BiosCode[Offset++] = 0xCD; // int SPECIAL_INT_NUM
            BiosCode[Offset++] = SPECIAL_INT_NUM;

            BiosCode[Offset++] = 0x83; // add sp, 2
            BiosCode[Offset++] = 0xC4;
            BiosCode[Offset++] = 0x02;
        }

        BiosCode[Offset++] = 0xCF; // iret
    }

    /* Get the input and output handles to the real console */
    BiosConsoleInput = CreateFile(TEXT("CONIN$"),
                                  GENERIC_READ | GENERIC_WRITE,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL,
                                  OPEN_EXISTING,
                                  0,
                                  NULL);

    BiosConsoleOutput = CreateFile(TEXT("CONOUT$"),
                                   GENERIC_READ | GENERIC_WRITE,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   NULL,
                                   OPEN_EXISTING,
                                   0,
                                   NULL);

    /* Make sure it was successful */
    if ((BiosConsoleInput == INVALID_HANDLE_VALUE)
        || (BiosConsoleOutput == INVALID_HANDLE_VALUE))
    {
        return FALSE;
    }

    /* Save the console screen buffer information */
    if (!GetConsoleScreenBufferInfo(BiosConsoleOutput, &BiosSavedBufferInfo))
    {
        return FALSE;
    }
    
    /* Set the default video mode */
    BiosSetVideoMode(BIOS_DEFAULT_VIDEO_MODE);

    /* Set the console input mode */
    SetConsoleMode(BiosConsoleInput, ENABLE_MOUSE_INPUT | ENABLE_PROCESSED_INPUT);

    /* Initialize the PIC */
    PicWriteCommand(PIC_MASTER_CMD, PIC_ICW1 | PIC_ICW1_ICW4);
    PicWriteCommand(PIC_SLAVE_CMD, PIC_ICW1 | PIC_ICW1_ICW4);

    /* Set the interrupt offsets */
    PicWriteData(PIC_MASTER_DATA, BIOS_PIC_MASTER_INT);
    PicWriteData(PIC_SLAVE_DATA, BIOS_PIC_SLAVE_INT);

    /* Tell the master PIC there is a slave at IRQ 2 */
    PicWriteData(PIC_MASTER_DATA, 1 << 2);
    PicWriteData(PIC_SLAVE_DATA, 2);

    /* Make sure the PIC is in 8086 mode */
    PicWriteData(PIC_MASTER_DATA, PIC_ICW4_8086);
    PicWriteData(PIC_SLAVE_DATA, PIC_ICW4_8086);

    /* Clear the masks for both PICs */
    PicWriteData(PIC_MASTER_DATA, 0x00);
    PicWriteData(PIC_SLAVE_DATA, 0x00);

    PitWriteCommand(0x34);
    PitWriteData(0, 0x00);
    PitWriteData(0, 0x00);

    return TRUE;
}

VOID BiosCleanup()
{
    INT i;

    /* Restore the old screen buffer */
    SetConsoleActiveScreenBuffer(BiosConsoleOutput);

    /* Restore the screen buffer size */
    SetConsoleScreenBufferSize(BiosConsoleOutput, BiosSavedBufferInfo.dwSize);

    /* Free the buffers */
    for (i = 0; i < VideoModes[CurrentVideoMode].Pages; i++)
    {
        if (ConsoleBuffers[i] != NULL) CloseHandle(ConsoleBuffers[i]);
        if (!VideoModes[CurrentVideoMode].Text) CloseHandle(ConsoleMutexes[i]);
    }

    /* Close the console handles */
    if (BiosConsoleInput != INVALID_HANDLE_VALUE) CloseHandle(BiosConsoleInput);
    if (BiosConsoleOutput != INVALID_HANDLE_VALUE) CloseHandle(BiosConsoleOutput);
}

VOID BiosUpdateConsole(ULONG StartAddress, ULONG EndAddress)
{
    ULONG i;
    COORD Coordinates;
    COORD Origin = { 0, 0 };
    COORD UnitSize = { 1, 1 };
    CHAR_INFO Character;
    SMALL_RECT Rect;

    /* Start from the character address */
    StartAddress &= ~1;

    if (VideoModes[CurrentVideoMode].Text)
    {
        /* Loop through all the addresses */
        for (i = StartAddress; i < EndAddress; i += 2)
        {
            /* Get the coordinates */
            Coordinates = BiosVideoAddressToCoord(i);

            /* Fill the rectangle structure */
            Rect.Left = Coordinates.X;
            Rect.Top = Coordinates.Y;
            Rect.Right = Rect.Left;
            Rect.Bottom = Rect.Top;

            /* Fill the character data */
            Character.Char.AsciiChar = *((PCHAR)((ULONG_PTR)BaseAddress + i));
            Character.Attributes = *((PBYTE)((ULONG_PTR)BaseAddress + i + 1));

            /* Write the character */
            WriteConsoleOutputA(BiosConsoleOutput,
                                &Character,
                                UnitSize,
                                Origin,
                                &Rect);
        }
    }
    else
    {
        /* Wait for the mutex object */
        WaitForSingleObject(ConsoleMutexes[CurrentVideoPage], INFINITE);

        /* Copy the data to the framebuffer */
        RtlCopyMemory((LPVOID)((ULONG_PTR)ConsoleFramebuffers[CurrentVideoPage]
                      + StartAddress - BiosGetVideoMemoryStart()),
                      (LPVOID)((ULONG_PTR)BaseAddress + StartAddress),
                      EndAddress - StartAddress);

        /* Release the mutex */
        ReleaseMutex(ConsoleMutexes[CurrentVideoPage]);

        /* Check if this is the first time the rectangle is updated */
        if (!VideoNeedsUpdate)
        {
            UpdateRectangle.Left = UpdateRectangle.Top = (SHORT)0x7FFF;
            UpdateRectangle.Right = UpdateRectangle.Bottom = (SHORT)0x8000;
        }

        /* Expand the update rectangle */
        for (i = StartAddress; i < EndAddress; i++)
        {
            /* Get the coordinates */
            Coordinates = BiosVideoAddressToCoord(i);

            /* Expand the rectangle to include the point */
            UpdateRectangle.Left = min(UpdateRectangle.Left, Coordinates.X);
            UpdateRectangle.Right = max(UpdateRectangle.Right, Coordinates.X);
            UpdateRectangle.Top = min(UpdateRectangle.Top, Coordinates.Y);
            UpdateRectangle.Bottom = max(UpdateRectangle.Bottom, Coordinates.Y);
        }

        /* Set the update flag */
        VideoNeedsUpdate = TRUE;
    }
}

VOID BiosUpdateVideoMemory(ULONG StartAddress, ULONG EndAddress)
{
    ULONG i;
    COORD Coordinates;
    WORD Attribute;
    DWORD CharsWritten;

    if (VideoModes[CurrentVideoMode].Text)
    {
        /* Loop through all the addresses */
        for (i = StartAddress; i < EndAddress; i++)
        {
            /* Get the coordinates */
            Coordinates = BiosVideoAddressToCoord(i);

            /* Check if this is a character byte or an attribute byte */
            if ((i - (VideoModes[CurrentVideoMode].Segment << 4)) % 2 == 0)
            {
                /* This is a regular character */
                ReadConsoleOutputCharacterA(BiosConsoleOutput,
                                            (LPSTR)((ULONG_PTR)BaseAddress + i),
                                            sizeof(CHAR),
                                            Coordinates,
                                            &CharsWritten);
            }
            else
            {
                /*  This is an attribute */
                ReadConsoleOutputAttribute(BiosConsoleOutput,
                                           &Attribute,
                                           sizeof(CHAR),
                                           Coordinates,
                                           &CharsWritten);

                *(PCHAR)((ULONG_PTR)BaseAddress + i) = LOBYTE(Attribute);
            }
        }
    }
    else
    {
        /* Wait for the mutex object */
        WaitForSingleObject(ConsoleMutexes[CurrentVideoPage], INFINITE);

        /* Copy the data to the emulator memory */
        RtlCopyMemory((LPVOID)((ULONG_PTR)BaseAddress + StartAddress),
                      (LPVOID)((ULONG_PTR)ConsoleFramebuffers[CurrentVideoPage]
                      + StartAddress - BiosGetVideoMemoryStart()),
                      EndAddress - StartAddress);

        /* Release the mutex */
        ReleaseMutex(ConsoleMutexes[CurrentVideoPage]);
    }
}

WORD BiosPeekCharacter()
{
    WORD CharacterData;
    
    /* Check if there is a key available */
    if (BiosKbdBufferEmpty) return 0xFFFF;

    /* Get the key from the queue, but don't remove it */
    BiosKbdBufferTop(&CharacterData);

    return CharacterData;
}

WORD BiosGetCharacter()
{
    WORD CharacterData;
    INPUT_RECORD InputRecord;
    DWORD Count;

    /* Check if there is a key available */
    if (!BiosKbdBufferEmpty)
    {
        /* Get the key from the queue, and remove it */
        BiosKbdBufferTop(&CharacterData);
        BiosKbdBufferPop();
    }
    else
    {
        while (TRUE)
        {
            /* Wait for a console event */
            WaitForSingleObject(BiosConsoleInput, INFINITE);
    
            /* Read the event, and make sure it's a keypress */
            if (!ReadConsoleInput(BiosConsoleInput, &InputRecord, 1, &Count)) continue;
            if (InputRecord.EventType != KEY_EVENT) continue;
            if (!InputRecord.Event.KeyEvent.bKeyDown) continue;

            /* Save the scan code and end the loop */
            CharacterData = (InputRecord.Event.KeyEvent.wVirtualScanCode << 8)
                            | InputRecord.Event.KeyEvent.uChar.AsciiChar;

            break;
        }
    }

    return CharacterData;
}

VOID BiosVideoService()
{
    INT CursorHeight;
    BOOLEAN Invisible = FALSE;
    COORD Position;
    CONSOLE_CURSOR_INFO CursorInfo;
    CONSOLE_SCREEN_BUFFER_INFO ScreenBufferInfo;
    CHAR_INFO Character;
    SMALL_RECT Rect;
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
            break;
        }

        /* Set Text-Mode Cursor Shape */
        case 0x01:
        {
            /* Retrieve and validate the input */
            Invisible = ((HIBYTE(Ecx) >> 5) & 0x03) ? TRUE : FALSE;
            CursorHeight = (HIBYTE(Ecx) & 0x1F) - (LOBYTE(Ecx) & 0x1F);
            if (CursorHeight < 1) CursorHeight = 1;
            if (CursorHeight > 100) CursorHeight = 100;

            /* Set the cursor */
            CursorInfo.dwSize = (CursorHeight * 100) / CONSOLE_FONT_HEIGHT;
            CursorInfo.bVisible = !Invisible;
            SetConsoleCursorInfo(BiosConsoleOutput, &CursorInfo);

            break;
        }

        /* Set Cursor Position */
        case 0x02:
        {
            Position.X = LOBYTE(Edx);
            Position.Y = HIBYTE(Edx);

            SetConsoleCursorPosition(BiosConsoleOutput, Position);
            break;
        }

        /* Get Cursor Position */
        case 0x03:
        {
            INT StartLine;

            /* Retrieve the data */
            GetConsoleCursorInfo(BiosConsoleOutput, &CursorInfo);
            GetConsoleScreenBufferInfo(BiosConsoleOutput, &ScreenBufferInfo);

            /* Find the first line */
            StartLine = 32 - ((CursorInfo.dwSize * 32) / 100);

            /* Return the result */
            EmulatorSetRegister(EMULATOR_REG_AX, 0);
            EmulatorSetRegister(EMULATOR_REG_CX, (StartLine << 8) | 0x1F);
            EmulatorSetRegister(EMULATOR_REG_DX,
                                LOWORD(ScreenBufferInfo.dwCursorPosition.Y) << 8
                                || LOWORD(ScreenBufferInfo.dwCursorPosition.X));
            break;
        }

        /* Scroll Up/Down Window */
        case 0x06:
        case 0x07:
        {
            Rect.Top = HIBYTE(Ecx);
            Rect.Left = LOBYTE(Ecx);
            Rect.Bottom = HIBYTE(Edx);
            Rect.Right = LOBYTE(Edx);
            Character.Char.UnicodeChar = L' ';
            Character.Attributes = HIBYTE(Ebx);
            Position.X = Rect.Left;
            if (HIBYTE(Eax) == 0x06) Position.Y = Rect.Top - LOBYTE(Eax);
            else Position.Y = Rect.Top + LOBYTE(Eax);

            ScrollConsoleScreenBuffer(BiosConsoleOutput,
                                      &Rect,
                                      &Rect,
                                      Position,
                                      &Character);
            break;
        }

        /* Read Character And Attribute At Cursor Position */
        case 0x08:
        {
            COORD BufferSize = { 1, 1 }, Origin = { 0, 0 };

            /* Get the cursor position */
            GetConsoleScreenBufferInfo(BiosConsoleOutput, &ScreenBufferInfo);

            /* Read at cursor position */
            Rect.Left = ScreenBufferInfo.dwCursorPosition.X;
            Rect.Top = ScreenBufferInfo.dwCursorPosition.Y;
            
            /* Read the console output */
            ReadConsoleOutput(BiosConsoleOutput, &Character, BufferSize, Origin, &Rect);

            /* Return the result */
            EmulatorSetRegister(EMULATOR_REG_AX,
                                (LOBYTE(Character.Attributes) << 8)
                                | Character.Char.AsciiChar);

            break;
        }

        /* Write Character And Attribute At Cursor Position */
        case 0x09:
        {
            DWORD CharsWritten;

            /* Get the cursor position */
            GetConsoleScreenBufferInfo(BiosConsoleOutput, &ScreenBufferInfo);

            /* Write the attribute to the output */
            FillConsoleOutputAttribute(BiosConsoleOutput,
                                       LOBYTE(Ebx),
                                       LOWORD(Ecx),
                                       ScreenBufferInfo.dwCursorPosition,
                                       &CharsWritten);

            /* Write the character to the output */
            FillConsoleOutputCharacterA(BiosConsoleOutput,
                                        LOBYTE(Eax),
                                        LOWORD(Ecx),
                                        ScreenBufferInfo.dwCursorPosition,
                                        &CharsWritten);

            break;
        }

        /* Write Character Only At Cursor Position */
        case 0x0A:
        {
            DWORD CharsWritten;

            /* Get the cursor position */
            GetConsoleScreenBufferInfo(BiosConsoleOutput, &ScreenBufferInfo);

            /* Write the character to the output */
            FillConsoleOutputCharacterA(BiosConsoleOutput,
                                        LOBYTE(Eax),
                                        LOWORD(Ecx),
                                        ScreenBufferInfo.dwCursorPosition,
                                        &CharsWritten);

            break;
        }

        /* Get Current Video Mode */
        case 0x0F:
        {
            EmulatorSetRegister(EMULATOR_REG_AX,
                                (VideoModes[CurrentVideoMode].Width << 8)
                                | CurrentVideoMode);
            EmulatorSetRegister(EMULATOR_REG_BX,
                                (CurrentVideoPage << 8) | LOBYTE(Ebx));

            break;
        }

        default:
        {
            DPRINT1("BIOS Function INT 10h, AH = 0x%02X NOT IMPLEMENTED\n",
                    HIBYTE(Eax));
        }
    }
}

VOID BiosKeyboardService()
{
    DWORD Eax = EmulatorGetRegister(EMULATOR_REG_AX);

    switch (HIBYTE(Eax))
    {
        case 0x00:
        {
            /* Read the character (and wait if necessary) */
            EmulatorSetRegister(EMULATOR_REG_AX, BiosGetCharacter());

            break;
        }

        case 0x01:
        {
            WORD Data = BiosPeekCharacter();

            if (Data != 0xFFFF)
            {
                /* There is a character, clear ZF and return it */
                EmulatorSetRegister(EMULATOR_REG_AX, Data);
                EmulatorClearFlag(EMULATOR_FLAG_ZF);
            }
            else
            {
                /* No character, set ZF */
                EmulatorSetFlag(EMULATOR_FLAG_ZF);
            }

            break;
        }
        
        default:
        {
            DPRINT1("BIOS Function INT 16h, AH = 0x%02X NOT IMPLEMENTED\n",
                    HIBYTE(Eax));
        }
    }
}

VOID BiosTimeService()
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
            if (BiosPassedMidnight) Eax |= 1;

            /* Return the tick count in CX:DX */
            EmulatorSetRegister(EMULATOR_REG_AX, Eax);
            EmulatorSetRegister(EMULATOR_REG_CX, HIWORD(BiosTickCount));
            EmulatorSetRegister(EMULATOR_REG_DX, LOWORD(BiosTickCount));

            /* Reset the midnight flag */
            BiosPassedMidnight = FALSE;

            break;
        }

        case 0x01:
        {
            /* Set the tick count to CX:DX */
            BiosTickCount = MAKELONG(LOWORD(Edx), LOWORD(Ecx));

            /* Reset the midnight flag */
            BiosPassedMidnight = FALSE;

            break;
        }

        default:
        {
            DPRINT1("BIOS Function INT 1Ah, AH = 0x%02X NOT IMPLEMENTED\n",
                    HIBYTE(Eax));
        }
    }
}

VOID BiosSystemTimerInterrupt()
{
    /* Increase the system tick count */
    BiosTickCount++;
}

VOID BiosEquipmentService()
{
    /* Return the equipment list */
    EmulatorSetRegister(EMULATOR_REG_AX, BIOS_EQUIPMENT_LIST);
}

VOID BiosHandleIrq(BYTE IrqNumber)
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
            
            /* Check if there is a scancode available */
            if (!(KeyboardReadStatus() & 1)) break;

            /* Get the scan code and virtual key code */
            ScanCode = KeyboardReadData();
            VirtualKey = MapVirtualKey(ScanCode, MAPVK_VSC_TO_VK);

            /* Check if this is a key press or release */
            if (!(ScanCode & (1 << 7)))
            {
                /* Key press */
                if (VirtualKey == VK_NUMLOCK
                    || VirtualKey == VK_CAPITAL
                    || VirtualKey == VK_SCROLL)
                {
                    /* For toggle keys, toggle the lowest bit in the keyboard map */
                    BiosKeyboardMap[VirtualKey] ^= ~(1 << 0);
                }

                /* Set the highest bit */
                BiosKeyboardMap[VirtualKey] |= (1 << 7);

                /* Find out which character this is */
                ToAscii(ScanCode, VirtualKey, BiosKeyboardMap, &Character, 0);

                /* Push it onto the BIOS keyboard queue */
                BiosKbdBufferPush((ScanCode << 8) | (Character & 0xFF));
            }
            else
            {
                /* Key release, unset the highest bit */
                BiosKeyboardMap[VirtualKey] &= ~(1 << 7);
            }

            break;
        }
    }

    /* Send End-of-Interrupt to the PIC */
    if (IrqNumber > 8) PicWriteCommand(PIC_SLAVE_CMD, PIC_OCW2_EOI);
    PicWriteCommand(PIC_MASTER_CMD, PIC_OCW2_EOI);
}

/* EOF */
