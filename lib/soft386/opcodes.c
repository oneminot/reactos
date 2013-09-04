/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         386/486 CPU Emulation Library
 * FILE:            opcodes.c
 * PURPOSE:         Opcode handlers.
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

// #define WIN32_NO_STATUS
// #define _INC_WINDOWS
#include <windef.h>

#include <soft386.h>
#include "opcodes.h"
#include "common.h"

// #define NDEBUG
#include <debug.h>

/* PUBLIC VARIABLES ***********************************************************/

SOFT386_OPCODE_HANDLER_PROC
Soft386OpcodeHandlers[SOFT386_NUM_OPCODE_HANDLERS] =
{
    Soft386OpcodeAddByteModrm,
    Soft386OpcodeAddModrm,
    Soft386OpcodeAddByteModrm,
    Soft386OpcodeAddModrm,
    Soft386OpcodeAddAl,
    Soft386OpcodeAddEax,
    NULL, // TODO: OPCODE 0x06 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x07 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x08 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x09 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x0A NOT SUPPORTED
    NULL, // TODO: OPCODE 0x0B NOT SUPPORTED
    NULL, // TODO: OPCODE 0x0C NOT SUPPORTED
    NULL, // TODO: OPCODE 0x0D NOT SUPPORTED
    NULL, // TODO: OPCODE 0x0E NOT SUPPORTED
    NULL, // TODO: OPCODE 0x0F NOT SUPPORTED
    NULL, // TODO: OPCODE 0x10 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x11 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x12 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x13 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x14 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x15 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x16 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x17 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x18 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x19 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x1A NOT SUPPORTED
    NULL, // TODO: OPCODE 0x1B NOT SUPPORTED
    NULL, // TODO: OPCODE 0x1C NOT SUPPORTED
    NULL, // TODO: OPCODE 0x1D NOT SUPPORTED
    NULL, // TODO: OPCODE 0x1E NOT SUPPORTED
    NULL, // TODO: OPCODE 0x1F NOT SUPPORTED
    NULL, // TODO: OPCODE 0x20 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x21 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x22 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x23 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x24 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x25 NOT SUPPORTED
    Soft386OpcodePrefix,
    NULL, // TODO: OPCODE 0x27 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x28 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x29 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x2A NOT SUPPORTED
    NULL, // TODO: OPCODE 0x2B NOT SUPPORTED
    NULL, // TODO: OPCODE 0x2C NOT SUPPORTED
    NULL, // TODO: OPCODE 0x2D NOT SUPPORTED
    Soft386OpcodePrefix,
    NULL, // TODO: OPCODE 0x2F NOT SUPPORTED
    NULL, // TODO: OPCODE 0x30 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x31 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x32 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x33 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x34 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x35 NOT SUPPORTED
    Soft386OpcodePrefix,
    NULL, // TODO: OPCODE 0x37 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x38 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x39 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x3A NOT SUPPORTED
    NULL, // TODO: OPCODE 0x3B NOT SUPPORTED
    NULL, // TODO: OPCODE 0x3C NOT SUPPORTED
    NULL, // TODO: OPCODE 0x3D NOT SUPPORTED
    Soft386OpcodePrefix,
    NULL, // TODO: OPCODE 0x3F NOT SUPPORTED
    Soft386OpcodeIncrement,
    Soft386OpcodeIncrement,
    Soft386OpcodeIncrement,
    Soft386OpcodeIncrement,
    Soft386OpcodeIncrement,
    Soft386OpcodeIncrement,
    Soft386OpcodeIncrement,
    Soft386OpcodeIncrement,
    Soft386OpcodeDecrement,
    Soft386OpcodeDecrement,
    Soft386OpcodeDecrement,
    Soft386OpcodeDecrement,
    Soft386OpcodeDecrement,
    Soft386OpcodeDecrement,
    Soft386OpcodeDecrement,
    Soft386OpcodeDecrement,
    Soft386OpcodePushReg,
    Soft386OpcodePushReg,
    Soft386OpcodePushReg,
    Soft386OpcodePushReg,
    Soft386OpcodePushReg,
    Soft386OpcodePushReg,
    Soft386OpcodePushReg,
    Soft386OpcodePushReg,
    Soft386OpcodePopReg,
    Soft386OpcodePopReg,
    Soft386OpcodePopReg,
    Soft386OpcodePopReg,
    Soft386OpcodePopReg,
    Soft386OpcodePopReg,
    Soft386OpcodePopReg,
    Soft386OpcodePopReg,
    NULL, // TODO: OPCODE 0x60 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x61 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x62 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x63 NOT SUPPORTED
    Soft386OpcodePrefix,
    Soft386OpcodePrefix,
    Soft386OpcodePrefix,
    Soft386OpcodePrefix,
    NULL, // TODO: OPCODE 0x68 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x69 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x6A NOT SUPPORTED
    NULL, // TODO: OPCODE 0x6B NOT SUPPORTED
    NULL, // TODO: OPCODE 0x6C NOT SUPPORTED
    NULL, // TODO: OPCODE 0x6D NOT SUPPORTED
    NULL, // TODO: OPCODE 0x6E NOT SUPPORTED
    NULL, // TODO: OPCODE 0x6F NOT SUPPORTED
    Soft386OpcodeShortConditionalJmp,
    Soft386OpcodeShortConditionalJmp,
    Soft386OpcodeShortConditionalJmp,
    Soft386OpcodeShortConditionalJmp,
    Soft386OpcodeShortConditionalJmp,
    Soft386OpcodeShortConditionalJmp,
    Soft386OpcodeShortConditionalJmp,
    Soft386OpcodeShortConditionalJmp,
    Soft386OpcodeShortConditionalJmp,
    Soft386OpcodeShortConditionalJmp,
    Soft386OpcodeShortConditionalJmp,
    Soft386OpcodeShortConditionalJmp,
    Soft386OpcodeShortConditionalJmp,
    Soft386OpcodeShortConditionalJmp,
    Soft386OpcodeShortConditionalJmp,
    Soft386OpcodeShortConditionalJmp,
    NULL, // TODO: OPCODE 0x80 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x81 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x82 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x83 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x84 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x85 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x86 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x87 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x88 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x89 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x8A NOT SUPPORTED
    NULL, // TODO: OPCODE 0x8B NOT SUPPORTED
    NULL, // TODO: OPCODE 0x8C NOT SUPPORTED
    NULL, // TODO: OPCODE 0x8D NOT SUPPORTED
    NULL, // TODO: OPCODE 0x8E NOT SUPPORTED
    NULL, // TODO: OPCODE 0x8F NOT SUPPORTED
    Soft386OpcodeNop,
    Soft386OpcodeExchangeEax,
    Soft386OpcodeExchangeEax,
    Soft386OpcodeExchangeEax,
    Soft386OpcodeExchangeEax,
    Soft386OpcodeExchangeEax,
    Soft386OpcodeExchangeEax,
    Soft386OpcodeExchangeEax,
    NULL, // TODO: OPCODE 0x98 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x99 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x9A NOT SUPPORTED
    NULL, // TODO: OPCODE 0x9B NOT SUPPORTED
    NULL, // TODO: OPCODE 0x9C NOT SUPPORTED
    NULL, // TODO: OPCODE 0x9D NOT SUPPORTED
    NULL, // TODO: OPCODE 0x9E NOT SUPPORTED
    NULL, // TODO: OPCODE 0x9F NOT SUPPORTED
    NULL, // TODO: OPCODE 0xA0 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xA1 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xA2 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xA3 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xA4 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xA5 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xA6 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xA7 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xA8 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xA9 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xAA NOT SUPPORTED
    NULL, // TODO: OPCODE 0xAB NOT SUPPORTED
    NULL, // TODO: OPCODE 0xAC NOT SUPPORTED
    NULL, // TODO: OPCODE 0xAD NOT SUPPORTED
    NULL, // TODO: OPCODE 0xAE NOT SUPPORTED
    NULL, // TODO: OPCODE 0xAF NOT SUPPORTED
    Soft386OpcodeMovByteRegImm,
    Soft386OpcodeMovByteRegImm,
    Soft386OpcodeMovByteRegImm,
    Soft386OpcodeMovByteRegImm,
    Soft386OpcodeMovByteRegImm,
    Soft386OpcodeMovByteRegImm,
    Soft386OpcodeMovByteRegImm,
    Soft386OpcodeMovByteRegImm,
    Soft386OpcodeMovRegImm,
    Soft386OpcodeMovRegImm,
    Soft386OpcodeMovRegImm,
    Soft386OpcodeMovRegImm,
    Soft386OpcodeMovRegImm,
    Soft386OpcodeMovRegImm,
    Soft386OpcodeMovRegImm,
    Soft386OpcodeMovRegImm,
    NULL, // TODO: OPCODE 0xC0 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xC1 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xC2 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xC3 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xC4 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xC5 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xC6 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xC7 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xC8 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xC9 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xCA NOT SUPPORTED
    NULL, // TODO: OPCODE 0xCB NOT SUPPORTED
    NULL, // TODO: OPCODE 0xCC NOT SUPPORTED
    NULL, // TODO: OPCODE 0xCD NOT SUPPORTED
    NULL, // TODO: OPCODE 0xCE NOT SUPPORTED
    NULL, // TODO: OPCODE 0xCF NOT SUPPORTED
    NULL, // TODO: OPCODE 0xD0 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xD1 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xD2 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xD3 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xD4 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xD5 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xD6 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xD7 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xD8 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xD9 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xDA NOT SUPPORTED
    NULL, // TODO: OPCODE 0xDB NOT SUPPORTED
    NULL, // TODO: OPCODE 0xDC NOT SUPPORTED
    NULL, // TODO: OPCODE 0xDD NOT SUPPORTED
    NULL, // TODO: OPCODE 0xDE NOT SUPPORTED
    NULL, // TODO: OPCODE 0xDF NOT SUPPORTED
    NULL, // TODO: OPCODE 0xE0 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xE1 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xE2 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xE3 NOT SUPPORTED
    Soft386OpcodeInByte,
    Soft386OpcodeIn,
    Soft386OpcodeOutByte,
    Soft386OpcodeOut,
    NULL, // TODO: OPCODE 0xE8 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xE9 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xEA NOT SUPPORTED
    Soft386OpcodeShortJump,
    Soft386OpcodeInByte,
    Soft386OpcodeIn,
    Soft386OpcodeOutByte,
    Soft386OpcodeOut,
    Soft386OpcodePrefix,
    NULL, // Invalid
    Soft386OpcodePrefix,
    Soft386OpcodePrefix,
    Soft386OpcodeHalt,
    Soft386OpcodeComplCarry,
    NULL, // TODO: OPCODE 0xF6 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xF7 NOT SUPPORTED
    Soft386OpcodeClearCarry,
    Soft386OpcodeSetCarry,
    Soft386OpcodeClearInt,
    Soft386OpcodeSetInt,
    Soft386OpcodeClearDir,
    Soft386OpcodeSetDir,
    NULL, // TODO: OPCODE 0xFE NOT SUPPORTED
    NULL, // TODO: OPCODE 0xFF NOT SUPPORTED
};

SOFT386_OPCODE_HANDLER(Soft386OpcodePrefix)
{
    BOOLEAN Valid = FALSE;

    switch (Opcode)
    {
        /* ES: */
        case 0x26:
        {
            if (!(State->PrefixFlags & SOFT386_PREFIX_SEG))
            {
                State->PrefixFlags |= SOFT386_PREFIX_SEG;
                State->SegmentOverride = SOFT386_REG_ES;
                Valid = TRUE;
            }

            break;
        }

        /* CS: */
        case 0x2E:
        {
            if (!(State->PrefixFlags & SOFT386_PREFIX_SEG))
            {
                State->PrefixFlags |= SOFT386_PREFIX_SEG;
                State->SegmentOverride = SOFT386_REG_CS;
                Valid = TRUE;
            }

            break;
        }

        /* SS: */
        case 0x36:
        {
            if (!(State->PrefixFlags & SOFT386_PREFIX_SEG))
            {
                State->PrefixFlags |= SOFT386_PREFIX_SEG;
                State->SegmentOverride = SOFT386_REG_SS;
                Valid = TRUE;
            }

            break;
        }

        /* DS: */
        case 0x3E:
        {
            if (!(State->PrefixFlags & SOFT386_PREFIX_SEG))
            {
                State->PrefixFlags |= SOFT386_PREFIX_SEG;
                State->SegmentOverride = SOFT386_REG_DS;
                Valid = TRUE;
            }

            break;
        }

        /* FS: */
        case 0x64:
        {
            if (!(State->PrefixFlags & SOFT386_PREFIX_SEG))
            {
                State->PrefixFlags |= SOFT386_PREFIX_SEG;
                State->SegmentOverride = SOFT386_REG_FS;
                Valid = TRUE;
            }

            break;
        }

        /* GS: */
        case 0x65:
        {
            if (!(State->PrefixFlags & SOFT386_PREFIX_SEG))
            {
                State->PrefixFlags |= SOFT386_PREFIX_SEG;
                State->SegmentOverride = SOFT386_REG_GS;
                Valid = TRUE;
            }

            break;
        }

        /* OPSIZE */
        case 0x66:
        {
            if (!(State->PrefixFlags & SOFT386_PREFIX_OPSIZE))
            {
                State->PrefixFlags |= SOFT386_PREFIX_OPSIZE;
                Valid = TRUE;
            }

            break;
        }

        /* ADSIZE */
        case 0x67:
        {
            if (!(State->PrefixFlags & SOFT386_PREFIX_ADSIZE))
            {
                State->PrefixFlags |= SOFT386_PREFIX_ADSIZE;
                Valid = TRUE;
            }
            break;
        }

        /* LOCK */
        case 0xF0:
        {
            if (!(State->PrefixFlags & SOFT386_PREFIX_LOCK))
            {
                State->PrefixFlags |= SOFT386_PREFIX_LOCK;
                Valid = TRUE;
            }

            break;
        }

        /* REPNZ */
        case 0xF2:
        {
            /* Mutually exclusive with REP */
            if (!(State->PrefixFlags
                & (SOFT386_PREFIX_REPNZ | SOFT386_PREFIX_REP)))
            {
                State->PrefixFlags |= SOFT386_PREFIX_REPNZ;
                Valid = TRUE;
            }

            break;
        }

        /* REP / REPZ */
        case 0xF3:
        {
            /* Mutually exclusive with REPNZ */
            if (!(State->PrefixFlags
                & (SOFT386_PREFIX_REPNZ | SOFT386_PREFIX_REP)))
            {
                State->PrefixFlags |= SOFT386_PREFIX_REP;
                Valid = TRUE;
            }

            break;
        }
    }

    if (!Valid)
    {
        /* Clear all prefixes */
        State->PrefixFlags = 0;

        /* Throw an exception */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeIncrement)
{
    ULONG Value;
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_CS].Size;

    if (State->PrefixFlags == SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        Size = !Size;
    }
    else if (State->PrefixFlags != 0)
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xF8) == 0x40);

    if (Size)
    {
        Value = ++State->GeneralRegs[Opcode & 0x07].Long;

        State->Flags.Of = (Value == SIGN_FLAG_LONG) ? TRUE : FALSE;
        State->Flags.Sf = (Value & SIGN_FLAG_LONG) ? TRUE : FALSE;
    }
    else
    {
        Value = ++State->GeneralRegs[Opcode & 0x07].LowWord;

        State->Flags.Of = (Value == SIGN_FLAG_WORD) ? TRUE : FALSE;
        State->Flags.Sf = (Value & SIGN_FLAG_WORD) ? TRUE : FALSE;
    }

    State->Flags.Zf = (Value == 0) ? TRUE : FALSE;
    State->Flags.Af = ((Value & 0x0F) == 0) ? TRUE : FALSE;
    State->Flags.Pf = Soft386CalculateParity(LOBYTE(Value));

    /* Return success */
    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeDecrement)
{
    ULONG Value;
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_CS].Size;

    if (State->PrefixFlags == SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        Size = !Size;
    }
    else if (State->PrefixFlags != 0)
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xF8) == 0x48);

    if (Size)
    {
        Value = --State->GeneralRegs[Opcode & 0x07].Long;

        State->Flags.Of = (Value == (SIGN_FLAG_LONG - 1)) ? TRUE : FALSE;
        State->Flags.Sf = (Value & SIGN_FLAG_LONG) ? TRUE : FALSE;
    }
    else
    {
        Value = --State->GeneralRegs[Opcode & 0x07].LowWord;

        State->Flags.Of = (Value == (SIGN_FLAG_WORD - 1)) ? TRUE : FALSE;
        State->Flags.Sf = (Value & SIGN_FLAG_WORD) ? TRUE : FALSE;
    }

    State->Flags.Zf = (Value == 0) ? TRUE : FALSE;
    State->Flags.Af = ((Value & 0x0F) == 0x0F) ? TRUE : FALSE;
    State->Flags.Pf = Soft386CalculateParity(LOBYTE(Value));

    /* Return success */
    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodePushReg)
{
    if ((State->PrefixFlags != SOFT386_PREFIX_OPSIZE)
        && (State->PrefixFlags != 0))
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xF8) == 0x50);

    /* Call the internal function */
    return Soft386StackPush(State, State->GeneralRegs[Opcode & 0x07].Long);
}

SOFT386_OPCODE_HANDLER(Soft386OpcodePopReg)
{
    ULONG Value;
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_SS].Size;

    if (State->PrefixFlags == SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        Size = !Size;
    }
    else if (State->PrefixFlags != 0)
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xF8) == 0x58);

    /* Call the internal function */
    if (!Soft386StackPop(State, &Value)) return FALSE;

    /* Store the value */
    if (Size) State->GeneralRegs[Opcode & 0x07].Long = Value;
    else State->GeneralRegs[Opcode & 0x07].LowWord = Value;

    /* Return success */
    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeNop)
{
    if (State->PrefixFlags & ~(SOFT386_PREFIX_OPSIZE | SOFT386_PREFIX_REP))
    {
        /* Allowed prefixes are REP and OPSIZE */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (State->PrefixFlags & SOFT386_PREFIX_REP)
    {
        /* Idle cycle */
        State->IdleCallback(State);
    }

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeExchangeEax)
{
    INT Reg = Opcode & 0x07;
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_CS].Size;

    if (State->PrefixFlags == SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        Size = !Size;
    }
    else if (State->PrefixFlags != 0)
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xF8) == 0x90);

    /* Exchange the values */
    if (Size)
    {
        ULONG Value;

        Value = State->GeneralRegs[Reg].Long;
        State->GeneralRegs[Reg].Long = State->GeneralRegs[SOFT386_REG_EAX].Long;
        State->GeneralRegs[SOFT386_REG_EAX].Long = Value;
    }
    else
    {
        USHORT Value;

        Value = State->GeneralRegs[Reg].LowWord;
        State->GeneralRegs[Reg].LowWord = State->GeneralRegs[SOFT386_REG_EAX].LowWord;
        State->GeneralRegs[SOFT386_REG_EAX].LowWord = Value;
    }

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeShortConditionalJmp)
{
    BOOLEAN Jump = FALSE;
    CHAR Offset = 0;

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xF0) == 0x70);

    /* Fetch the offset */
    if (!Soft386FetchByte(State, (PUCHAR)&Offset))
    {
        /* An exception occurred */
        return FALSE;
    }

    switch ((Opcode & 0x0F) >> 1)
    {
        /* JO / JNO */
        case 0:
        {
            Jump = State->Flags.Of;
            break;
        }

        /* JC / JNC */
        case 1:
        {
            Jump = State->Flags.Cf;
            break;
        }

        /* JZ / JNZ */
        case 2:
        {
            Jump = State->Flags.Zf;
            break;
        }

        /* JBE / JNBE */
        case 3:
        {
            Jump = State->Flags.Cf || State->Flags.Zf;
            break;
        }

        /* JS / JNS */
        case 4:
        {
            Jump = State->Flags.Sf;
            break;
        }

        /* JP / JNP */
        case 5:
        {
            Jump = State->Flags.Pf;
            break;
        }

        /* JL / JNL */
        case 6:
        {
            Jump = State->Flags.Sf != State->Flags.Of;
            break;
        }

        /* JLE / JNLE */
        case 7:
        {
            Jump = (State->Flags.Sf != State->Flags.Of) || State->Flags.Zf;
            break;
        }
    }

    if (Opcode & 1)
    {
        /* Invert the result */
        Jump = !Jump;
    }

    if (Jump)
    {
        /* Move the instruction pointer */        
        State->InstPtr.Long += Offset;
    }

    /* Return success */
    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeClearCarry)
{
    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0xF8);

    /* No prefixes allowed */
    if (State->PrefixFlags)
    {
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Clear CF and return success */
    State->Flags.Cf = FALSE;
    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeSetCarry)
{
    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0xF9);

    /* No prefixes allowed */
    if (State->PrefixFlags)
    {
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Set CF and return success*/
    State->Flags.Cf = TRUE;
    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeComplCarry)
{
    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0xF5);

    /* No prefixes allowed */
    if (State->PrefixFlags)
    {
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Toggle CF and return success */
    State->Flags.Cf = !State->Flags.Cf;
    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeClearInt)
{
    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0xFA);

    /* No prefixes allowed */
    if (State->PrefixFlags)
    {
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Check for protected mode */
    if (State->ControlRegisters[SOFT386_REG_CR0] & SOFT386_CR0_PE)
    {
        /* Check IOPL */
        if (State->Flags.Iopl >= State->SegmentRegs[SOFT386_REG_CS].Dpl)
        {
            /* Clear the interrupt flag */
            State->Flags.If = FALSE;
        }
        else
        {
            /* General Protection Fault */
            Soft386Exception(State, SOFT386_EXCEPTION_GP);
            return FALSE;
        }
    }
    else
    {
        /* Just clear the interrupt flag */
        State->Flags.If = FALSE;
    }

    /* Return success */
    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeSetInt)
{
    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0xFB);

    /* No prefixes allowed */
    if (State->PrefixFlags)
    {
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Check for protected mode */
    if (State->ControlRegisters[SOFT386_REG_CR0] & SOFT386_CR0_PE)
    {
        /* Check IOPL */
        if (State->Flags.Iopl >= State->SegmentRegs[SOFT386_REG_CS].Dpl)
        {
            /* Set the interrupt flag */
            State->Flags.If = TRUE;
        }
        else
        {
            /* General Protection Fault */
            Soft386Exception(State, SOFT386_EXCEPTION_GP);
            return FALSE;
        }
    }
    else
    {
        /* Just set the interrupt flag */
        State->Flags.If = TRUE;
    }

    /* Return success */
    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeClearDir)
{
    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0xFC);

    /* No prefixes allowed */
    if (State->PrefixFlags)
    {
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Clear DF and return success */
    State->Flags.Df = FALSE;
    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeSetDir)
{
    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0xFD);

    /* No prefixes allowed */
    if (State->PrefixFlags)
    {
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Set DF and return success*/
    State->Flags.Df = TRUE;
    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeHalt)
{
    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0xF4);

    /* No prefixes allowed */
    if (State->PrefixFlags)
    {
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Privileged instructions can only be executed under CPL = 0 */
    if (State->SegmentRegs[SOFT386_REG_CS].Dpl != 0)
    {
        Soft386Exception(State, SOFT386_EXCEPTION_GP);
        return FALSE;
    }

    /* Halt */
    while (!State->HardwareInt) State->IdleCallback(State);

    /* Return success */
    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeInByte)
{
    UCHAR Data;
    ULONG Port;

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xF7) == 0xE4);

    if (Opcode == 0xE4)
    {
        /* Fetch the parameter */
        if (!Soft386FetchByte(State, &Data))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Set the port number to the parameter */
        Port = Data;
    }
    else
    {
        /* The port number is in DX */
        Port = State->GeneralRegs[SOFT386_REG_EDX].LowWord;
    }

    /* Read a byte from the I/O port */
    State->IoReadCallback(State, Port, &Data, sizeof(UCHAR));

    /* Store the result in AL */
    State->GeneralRegs[SOFT386_REG_EAX].LowByte = Data;

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeIn)
{
    ULONG Port;
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_CS].Size;

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xF7) == 0xE5);

    if (State->PrefixFlags == SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        Size = !Size;
    }
    else if (State->PrefixFlags != 0)
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (Opcode == 0xE5)
    {
        UCHAR Data;

        /* Fetch the parameter */
        if (!Soft386FetchByte(State, &Data))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Set the port number to the parameter */
        Port = Data;
    }
    else
    {
        /* The port number is in DX */
        Port = State->GeneralRegs[SOFT386_REG_EDX].LowWord;
    }

    if (Size)
    {
        ULONG Data;

        /* Read a dword from the I/O port */
        State->IoReadCallback(State, Port, &Data, sizeof(ULONG));

        /* Store the value in EAX */
        State->GeneralRegs[SOFT386_REG_EAX].Long = Data;
    }
    else
    {
        USHORT Data;

        /* Read a word from the I/O port */
        State->IoReadCallback(State, Port, &Data, sizeof(USHORT));

        /* Store the value in AX */
        State->GeneralRegs[SOFT386_REG_EAX].LowWord = Data;
    }

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeOutByte)
{
    UCHAR Data;
    ULONG Port;

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xF7) == 0xE6);

    if (Opcode == 0xE6)
    {
        /* Fetch the parameter */
        if (!Soft386FetchByte(State, &Data))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Set the port number to the parameter */
        Port = Data;
    }
    else
    {
        /* The port number is in DX */
        Port = State->GeneralRegs[SOFT386_REG_EDX].LowWord;
    }

    /* Read the value from AL */
    Data = State->GeneralRegs[SOFT386_REG_EAX].LowByte;
    
    /* Write the byte to the I/O port */
    State->IoWriteCallback(State, Port, &Data, sizeof(UCHAR));

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeOut)
{
    ULONG Port;
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_CS].Size;

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xF7) == 0xE7);

    if (State->PrefixFlags == SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        Size = !Size;
    }
    else if (State->PrefixFlags != 0)
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (Opcode == 0xE7)
    {
        UCHAR Data;

        /* Fetch the parameter */
        if (!Soft386FetchByte(State, &Data))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Set the port number to the parameter */
        Port = Data;
    }
    else
    {
        /* The port number is in DX */
        Port = State->GeneralRegs[SOFT386_REG_EDX].LowWord;
    }

    if (Size)
    {
        /* Get the value from EAX */
        ULONG Data = State->GeneralRegs[SOFT386_REG_EAX].Long;

        /* Write a dword to the I/O port */
        State->IoReadCallback(State, Port, &Data, sizeof(ULONG));
    }
    else
    {
        /* Get the value from AX */
        USHORT Data = State->GeneralRegs[SOFT386_REG_EAX].LowWord;

        /* Write a word to the I/O port */
        State->IoWriteCallback(State, Port, &Data, sizeof(USHORT));
    }

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeShortJump)
{
    CHAR Offset = 0;

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0xEB);

    /* Fetch the offset */
    if (!Soft386FetchByte(State, (PUCHAR)&Offset))
    {
        /* An exception occurred */
        return FALSE;
    }

    /* Move the instruction pointer */        
    State->InstPtr.Long += Offset;

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeMovRegImm)
{
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_CS].Size;

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xF8) == 0xB8);

    if (State->PrefixFlags == SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        Size = !Size;
    }
    else if (State->PrefixFlags != 0)
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (Size)
    {
        ULONG Value;

        /* Fetch the dword */
        if (!Soft386FetchDword(State, &Value))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Store the value in the register */
        State->GeneralRegs[Opcode & 0x07].Long = Value;
    }
    else
    {
        USHORT Value;

        /* Fetch the word */
        if (!Soft386FetchWord(State, &Value))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Store the value in the register */
        State->GeneralRegs[Opcode & 0x07].LowWord = Value;
    }

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeMovByteRegImm)
{
    UCHAR Value;

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xF8) == 0xB0);

    if (State->PrefixFlags != 0)
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Fetch the byte */
    if (!Soft386FetchByte(State, &Value))
    {
        /* Exception occurred */
        return FALSE;
    }

    if (Opcode & 0x04)
    {
        /* AH, CH, DH or BH */
        State->GeneralRegs[Opcode & 0x03].HighByte = Value;
    }
    else
    {
        /* AL, CL, DL or BL */
        State->GeneralRegs[Opcode & 0x03].LowByte = Value;
    }

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeAddByteModrm)
{
    UCHAR FirstValue, SecondValue, Result;
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xFD) == 0x00);

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the size */
        AddressSize = !AddressSize;
    }
    else if (State->PrefixFlags
             & ~(SOFT386_PREFIX_ADSIZE
             | SOFT386_PREFIX_SEG
             | SOFT386_PREFIX_LOCK))
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Get the operands */
    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    if (!Soft386ReadModrmByteOperands(State,
                                      &ModRegRm,
                                      &FirstValue,
                                      &SecondValue))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Calculate the result */
    Result = FirstValue + SecondValue;

    /* Update the flags */
    State->Flags.Cf = (Result < FirstValue) && (Result < SecondValue);
    State->Flags.Of = ((FirstValue & SIGN_FLAG_BYTE) == (SecondValue & SIGN_FLAG_BYTE))
                      && ((FirstValue & SIGN_FLAG_BYTE) != (Result & SIGN_FLAG_BYTE));
    State->Flags.Af = (((FirstValue & 0x0F) + (SecondValue & 0x0F)) & 0x10) ? TRUE : FALSE;
    State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
    State->Flags.Sf = (Result & SIGN_FLAG_BYTE) ? TRUE : FALSE;
    State->Flags.Pf = Soft386CalculateParity(Result);

    /* Write back the result */
    return Soft386WriteModrmByteOperands(State,
                                         &ModRegRm,
                                         Opcode & SOFT386_OPCODE_WRITE_REG,
                                         Result);
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeAddModrm)
{
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN OperandSize, AddressSize;

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xFD) == 0x01);

    OperandSize = AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the address size */
        AddressSize = !AddressSize;
    }

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the operand size */
        OperandSize = !OperandSize;
    }

    if (State->PrefixFlags
             & ~(SOFT386_PREFIX_ADSIZE
             | SOFT386_PREFIX_OPSIZE
             | SOFT386_PREFIX_SEG
             | SOFT386_PREFIX_LOCK))
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Get the operands */
    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Check the operand size */
    if (OperandSize)
    {
        ULONG FirstValue, SecondValue, Result;

        if (!Soft386ReadModrmDwordOperands(State,
                                          &ModRegRm,
                                          &FirstValue,
                                          &SecondValue))
        {
            /* Exception occurred */
            return FALSE;
        }
    
        /* Calculate the result */
        Result = FirstValue + SecondValue;

        /* Update the flags */
        State->Flags.Cf = (Result < FirstValue) && (Result < SecondValue);
        State->Flags.Of = ((FirstValue & SIGN_FLAG_LONG) == (SecondValue & SIGN_FLAG_LONG))
                          && ((FirstValue & SIGN_FLAG_LONG) != (Result & SIGN_FLAG_LONG));
        State->Flags.Af = (((FirstValue & 0x0F) + (SecondValue & 0x0F)) & 0x10) ? TRUE : FALSE;
        State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
        State->Flags.Sf = (Result & SIGN_FLAG_LONG) ? TRUE : FALSE;
        State->Flags.Pf = Soft386CalculateParity(Result);

        /* Write back the result */
        return Soft386WriteModrmDwordOperands(State,
                                              &ModRegRm,
                                              Opcode & SOFT386_OPCODE_WRITE_REG,
                                              Result);
    }
    else
    {
        USHORT FirstValue, SecondValue, Result;

        if (!Soft386ReadModrmWordOperands(State,
                                          &ModRegRm,
                                          &FirstValue,
                                          &SecondValue))
        {
            /* Exception occurred */
            return FALSE;
        }
    
        /* Calculate the result */
        Result = FirstValue + SecondValue;

        /* Update the flags */
        State->Flags.Cf = (Result < FirstValue) && (Result < SecondValue);
        State->Flags.Of = ((FirstValue & SIGN_FLAG_WORD) == (SecondValue & SIGN_FLAG_WORD))
                          && ((FirstValue & SIGN_FLAG_WORD) != (Result & SIGN_FLAG_WORD));
        State->Flags.Af = (((FirstValue & 0x0F) + (SecondValue & 0x0F)) & 0x10) ? TRUE : FALSE;
        State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
        State->Flags.Sf = (Result & SIGN_FLAG_WORD) ? TRUE : FALSE;
        State->Flags.Pf = Soft386CalculateParity(Result);

        /* Write back the result */
        return Soft386WriteModrmWordOperands(State,
                                              &ModRegRm,
                                              Opcode & SOFT386_OPCODE_WRITE_REG,
                                              Result);
    }
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeAddAl)
{
    UCHAR FirstValue = State->GeneralRegs[SOFT386_REG_EAX].LowByte;
    UCHAR SecondValue, Result;

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0x04);

    if (State->PrefixFlags)
    {
        /* This opcode doesn't take any prefixes */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (!Soft386FetchByte(State, &SecondValue))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Calculate the result */
    Result = FirstValue + SecondValue;

    /* Update the flags */
    State->Flags.Cf = (Result < FirstValue) && (Result < SecondValue);
    State->Flags.Of = ((FirstValue & SIGN_FLAG_BYTE) == (SecondValue & SIGN_FLAG_BYTE))
                      && ((FirstValue & SIGN_FLAG_BYTE) != (Result & SIGN_FLAG_BYTE));
    State->Flags.Af = (((FirstValue & 0x0F) + (SecondValue & 0x0F)) & 0x10) ? TRUE : FALSE;
    State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
    State->Flags.Sf = (Result & SIGN_FLAG_BYTE) ? TRUE : FALSE;
    State->Flags.Pf = Soft386CalculateParity(Result);

    /* Write back the result */
    State->GeneralRegs[SOFT386_REG_EAX].LowByte = Result;

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeAddEax)
{
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_CS].Size;

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0x05);

    if (State->PrefixFlags == SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        Size = !Size;
    }
    else
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (Size)
    {
        ULONG FirstValue = State->GeneralRegs[SOFT386_REG_EAX].Long;
        ULONG SecondValue, Result;

        if (!Soft386FetchDword(State, &SecondValue))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Calculate the result */
        Result = FirstValue + SecondValue;

        /* Update the flags */
        State->Flags.Cf = (Result < FirstValue) && (Result < SecondValue);
        State->Flags.Of = ((FirstValue & SIGN_FLAG_LONG) == (SecondValue & SIGN_FLAG_LONG))
                          && ((FirstValue & SIGN_FLAG_LONG) != (Result & SIGN_FLAG_LONG));
        State->Flags.Af = (((FirstValue & 0x0F) + (SecondValue & 0x0F)) & 0x10) ? TRUE : FALSE;
        State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
        State->Flags.Sf = (Result & SIGN_FLAG_LONG) ? TRUE : FALSE;
        State->Flags.Pf = Soft386CalculateParity(Result);

        /* Write back the result */
        State->GeneralRegs[SOFT386_REG_EAX].Long = Result;
    }
    else
    {
        USHORT FirstValue = State->GeneralRegs[SOFT386_REG_EAX].LowWord;
        USHORT SecondValue, Result;

        if (!Soft386FetchWord(State, &SecondValue))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Calculate the result */
        Result = FirstValue + SecondValue;

        /* Update the flags */
        State->Flags.Cf = (Result < FirstValue) && (Result < SecondValue);
        State->Flags.Of = ((FirstValue & SIGN_FLAG_WORD) == (SecondValue & SIGN_FLAG_WORD))
                          && ((FirstValue & SIGN_FLAG_WORD) != (Result & SIGN_FLAG_WORD));
        State->Flags.Af = (((FirstValue & 0x0F) + (SecondValue & 0x0F)) & 0x10) ? TRUE : FALSE;
        State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
        State->Flags.Sf = (Result & SIGN_FLAG_WORD) ? TRUE : FALSE;
        State->Flags.Pf = Soft386CalculateParity(Result);

        /* Write back the result */
        State->GeneralRegs[SOFT386_REG_EAX].LowWord = Result;
    }

    return TRUE;
}
