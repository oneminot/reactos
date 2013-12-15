/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            int32.c
 * PURPOSE:         32-bit Interrupt Handlers
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "emulator.h"
#include "int32.h"

#include "bop.h"
#include "bios.h"
#include "registers.h"

/* PRIVATE VARIABLES **********************************************************/

LPCWSTR ExceptionName[] =
{
    L"Division By Zero",
    L"Debug",
    L"Unexpected Error",
    L"Breakpoint",
    L"Integer Overflow",
    L"Bound Range Exceeded",
    L"Invalid Opcode",
    L"FPU Not Available"
};

/*
 * This is the list of registered 32-bit Interrupt handlers.
 */
EMULATOR_INT32_PROC Int32Proc[EMULATOR_MAX_INT32_NUM] = { NULL };

/* BOP Identifiers */
#define BOP_CONTROL             0xFF    // Control BOP Handler
    #define BOP_CONTROL_DEFFUNC 0x00    // Default Control BOP Function

/* 32-bit Interrupt dispatcher function code for the Control BOP Handler */
#define BOP_CONTROL_INT32       0xFF

/* PUBLIC FUNCTIONS ***********************************************************/

VOID WINAPI Exception(BYTE ExceptionNumber, LPWORD Stack)
{
    WORD CodeSegment, InstructionPointer;
    PBYTE Opcode;

    ASSERT(ExceptionNumber < 8);

    /* Get the CS:IP */
    InstructionPointer = Stack[STACK_IP];
    CodeSegment = Stack[STACK_CS];
    Opcode = (PBYTE)SEG_OFF_TO_PTR(CodeSegment, InstructionPointer);

    /* Display a message to the user */
    DisplayMessage(L"Exception: %s occured at %04X:%04X\n"
                   L"Opcode: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
                   ExceptionName[ExceptionNumber],
                   CodeSegment,
                   InstructionPointer,
                   Opcode[0],
                   Opcode[1],
                   Opcode[2],
                   Opcode[3],
                   Opcode[4],
                   Opcode[5],
                   Opcode[6],
                   Opcode[7],
                   Opcode[8],
                   Opcode[9]);

    /* Stop the VDM */
    VdmRunning = FALSE;
    return;
}

#if 0
VOID WINAPI IrqDispatch(BYTE IrqNumber, LPWORD Stack)
{
    /* Check if this was an PIC IRQ */
    if (IntNum >= BIOS_PIC_MASTER_INT && IntNum < BIOS_PIC_MASTER_INT + 8)
    {
        /* It was an IRQ from the master PIC */
        BiosHandleIrq(IntNum - BIOS_PIC_MASTER_INT, Stack);
    }
    else if (IntNum >= BIOS_PIC_SLAVE_INT && IntNum < BIOS_PIC_SLAVE_INT + 8)
    {
        /* It was an IRQ from the slave PIC */
        BiosHandleIrq(IntNum - BIOS_PIC_SLAVE_INT + 8, Stack);
    }

    return;
}
#endif

VOID WINAPI Int32Dispatch(LPWORD Stack)
{
    BYTE IntNum;

    /* Get the interrupt number */
    IntNum = LOBYTE(Stack[STACK_INT_NUM]);

    /* Check if this was an exception */
    if (IntNum < 8)
    {
        Exception(IntNum, Stack);
        return;
    }

    /* Check if this was an PIC IRQ */
    if (IntNum >= BIOS_PIC_MASTER_INT && IntNum < BIOS_PIC_MASTER_INT + 8)
    {
        /* It was an IRQ from the master PIC */
        BiosHandleIrq(IntNum - BIOS_PIC_MASTER_INT, Stack);
        return;
    }
    else if (IntNum >= BIOS_PIC_SLAVE_INT && IntNum < BIOS_PIC_SLAVE_INT + 8)
    {
        /* It was an IRQ from the slave PIC */
        BiosHandleIrq(IntNum - BIOS_PIC_SLAVE_INT + 8, Stack);
        return;
    }

    /* Call the 32-bit Interrupt handler */
    if (Int32Proc[IntNum] != NULL)
        Int32Proc[IntNum](Stack);
    else
        DPRINT1("Unhandled 32-bit interrupt: 0x%02X, AX = 0x%04X\n", IntNum, getAX());
}

VOID WINAPI ControlBop(LPWORD Stack)
{
    /* Get the Function Number and skip it */
    BYTE FuncNum = *(PBYTE)SEG_OFF_TO_PTR(getCS(), getIP());
    setIP(getIP() + 1);

    if (FuncNum == BOP_CONTROL_INT32)
        Int32Dispatch(Stack);
    else
        DPRINT1("Unassigned Control BOP Function: 0x%02X\n", FuncNum);
}

VOID WINAPI InitializeInt32(WORD BiosSegment)
{
    LPDWORD IntVecTable = (LPDWORD)BaseAddress;
    LPBYTE  BiosCode    = (LPBYTE)SEG_OFF_TO_PTR(BiosSegment, 0);
    USHORT i;
    WORD BopSeqOffset, Offset = 0;

    /* Generate ISR stubs and fill the IVT */
    for (i = 0x00; i <= 0xFF; i++)
    {
        Offset = INT_HANDLER_OFFSET + (i << 4);
        IntVecTable[i] = MAKELONG(Offset, BiosSegment);

        BiosCode[Offset++] = 0xFA; // cli

        BiosCode[Offset++] = 0x6A; // push i
        BiosCode[Offset++] = (UCHAR)i;

        BiosCode[Offset++] = 0x6A; // push 0
        BiosCode[Offset++] = 0x00;

        BopSeqOffset = COMMON_STUB_OFFSET - (Offset + 3);

        BiosCode[Offset++] = 0xE9; // jmp near BOP_SEQ
        BiosCode[Offset++] = LOBYTE(BopSeqOffset);
        BiosCode[Offset++] = HIBYTE(BopSeqOffset);
    }

    /* Write the common stub code */
    Offset = COMMON_STUB_OFFSET;

// BOP_SEQ:
    BiosCode[Offset++] = 0xF8; // clc

    BiosCode[Offset++] = LOBYTE(EMULATOR_BOP);  // BOP sequence
    BiosCode[Offset++] = HIBYTE(EMULATOR_BOP);
    BiosCode[Offset++] = BOP_CONTROL;           // Control BOP
    BiosCode[Offset++] = BOP_CONTROL_INT32;     // 32-bit Interrupt dispatcher

    BiosCode[Offset++] = 0x73; // jnc EXIT (offset +4)
    BiosCode[Offset++] = 0x04;

    BiosCode[Offset++] = 0xFB; // sti

    // HACK: The following instruction should be HLT!
    BiosCode[Offset++] = 0x90; // nop

    BiosCode[Offset++] = 0xEB; // jmp BOP_SEQ (offset -11)
    BiosCode[Offset++] = 0xF5;

// EXIT:
    BiosCode[Offset++] = 0x83; // add sp, 4
    BiosCode[Offset++] = 0xC4;
    BiosCode[Offset++] = 0x04;

    BiosCode[Offset++] = 0xCF; // iret

    /* Register the Control BOP */
    RegisterBop(BOP_CONTROL, ControlBop);
}

VOID WINAPI RegisterInt32(BYTE IntNumber, EMULATOR_INT32_PROC IntHandler)
{
    Int32Proc[IntNumber] = IntHandler;
}

/* EOF */
