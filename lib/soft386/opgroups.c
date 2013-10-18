/*
 * Soft386 386/486 CPU Emulation Library
 * opgroups.c
 *
 * Copyright (C) 2013 Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/* INCLUDES *******************************************************************/

// #define WIN32_NO_STATUS
// #define _INC_WINDOWS
#include <windef.h>

// #define NDEBUG
#include <debug.h>

#include <soft386.h>
#include "opcodes.h"
#include "common.h"

/* PRIVATE FUNCTIONS **********************************************************/

inline
static
ULONG
Soft386ArithmeticOperation(PSOFT386_STATE State,
                           INT Operation,
                           ULONG FirstValue,
                           ULONG SecondValue,
                           UCHAR Bits)
{
    ULONG Result;
    ULONG SignFlag = 1 << (Bits - 1);
    ULONG MaxValue = (1 << Bits) - 1;

    /* Make sure the values don't exceed the maximum for their size */
    FirstValue &= MaxValue;
    SecondValue &= MaxValue;

    /* Check which operation is this */
    switch (Operation)
    {
        /* ADD */
        case 0:
        {
            Result = (FirstValue + SecondValue) & MaxValue;

            /* Update CF, OF and AF */
            State->Flags.Cf = (Result < FirstValue) && (Result < SecondValue);
            State->Flags.Of = ((FirstValue & SignFlag) == (SecondValue & SignFlag))
                              && ((FirstValue & SignFlag) != (Result & SignFlag));
            State->Flags.Af = (((FirstValue & 0x0F) + (SecondValue & 0x0F)) & 0x10) ? TRUE : FALSE;

            break;
        }

        /* OR */
        case 1:
        {
            Result = FirstValue | SecondValue;
            break;
        }

        /* ADC */
        case 2:
        {
            INT Carry = State->Flags.Cf ? 1 : 0;

            Result = (FirstValue + SecondValue + Carry) & MaxValue;

            /* Update CF, OF and AF */
            State->Flags.Cf = ((SecondValue == MaxValue) && (Carry == 1))
                              || ((Result < FirstValue) && (Result < (SecondValue + Carry)));
            State->Flags.Of = ((FirstValue & SignFlag) == (SecondValue & SignFlag))
                              && ((FirstValue & SignFlag) != (Result & SignFlag));
            State->Flags.Af = (((FirstValue & 0x0F) + ((SecondValue + Carry) & 0x0F)) & 0x10)
                              ? TRUE : FALSE;

            break;
        }

        /* SBB */
        case 3:
        {
            INT Carry = State->Flags.Cf ? 1 : 0;

            Result = (FirstValue - SecondValue - Carry) & MaxValue;

            /* Update CF, OF and AF */
            State->Flags.Cf = FirstValue < (SecondValue + Carry);
            State->Flags.Of = ((FirstValue & SignFlag) != (SecondValue & SignFlag))
                              && ((FirstValue & SignFlag) != (Result & SignFlag));
            State->Flags.Af = (FirstValue & 0x0F) < ((SecondValue + Carry) & 0x0F);

            break;
        }

        /* AND */
        case 4:
        {
            Result = FirstValue & SecondValue;
            break;
        }

        /* SUB or CMP */
        case 5:
        case 7:
        {
            Result = (FirstValue - SecondValue) & MaxValue;

            /* Update CF, OF and AF */
            State->Flags.Cf = FirstValue < SecondValue;
            State->Flags.Of = ((FirstValue & SignFlag) != (SecondValue & SignFlag))
                              && ((FirstValue & SignFlag) != (Result & SignFlag));
            State->Flags.Af = (FirstValue & 0x0F) < (SecondValue & 0x0F);

            break;
        }

        /* XOR */
        case 6:
        {
            Result = FirstValue ^ SecondValue;
            break;
        }

        default:
        {
            /* Shouldn't happen */
            ASSERT(FALSE);
        }
    }

    /* Update ZF, SF and PF */
    State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
    State->Flags.Sf = (Result & SignFlag) ? TRUE : FALSE;
    State->Flags.Pf = Soft386CalculateParity(LOBYTE(Result));

    /* Return the result */
    return Result;
}

static
inline
ULONG
Soft386RotateOperation(PSOFT386_STATE State,
                       INT Operation,
                       ULONG Value,
                       UCHAR Bits,
                       UCHAR Count)
{
    ULONG HighestBit = 1 << (Bits - 1);
    ULONG Result;

    if ((Operation != 2) && (Operation != 3))
    {
        /* Mask the count */
        Count &= Bits - 1;
    }
    else
    {
        /* For RCL and RCR, the CF is included in the value */
        Count %= Bits + 1;
    }

    /* Check which operation is this */
    switch (Operation)
    {
        /* ROL */
        case 0:
        {
            Result = (Value << Count) | (Value >> (Bits - Count));

            /* Update CF and OF */
            State->Flags.Cf = Result & 1;
            if (Count == 1) State->Flags.Of = ((Result & HighestBit) ? TRUE : FALSE)
                                              ^ State->Flags.Cf;

            break;
        }

        /* ROR */
        case 1:
        {
            Result = (Value >> Count) | (Value << (Bits - Count));

            /* Update CF and OF */
            State->Flags.Cf = (Result & HighestBit) ? TRUE : FALSE;
            if (Count == 1) State->Flags.Of = State->Flags.Cf
                                              ^ ((Result & (HighestBit >> 1))
                                              ? TRUE : FALSE);

            break;
        }

        /* RCL */
        case 2:
        {
            Result = (Value << Count)
                     | (State->Flags.Cf << (Count - 1))
                     | (Value >> (Bits - Count + 1));

            /* Update CF and OF */
            State->Flags.Cf = (Value & (1 << (Bits - Count))) ? TRUE : FALSE;
            if (Count == 1) State->Flags.Of = ((Result & HighestBit) ? TRUE : FALSE)
                                              ^ State->Flags.Cf;

            break;
        }

        /* RCR */
        case 3:
        {
            Result = (Value >> Count)
                     | (State->Flags.Cf << (Bits - Count))
                     | (Value << (Bits - Count + 1));

            /* Update CF and OF */
            State->Flags.Cf = (Value & (1 << (Bits - Count))) ? TRUE : FALSE;
            if (Count == 1) State->Flags.Of = State->Flags.Cf
                                              ^ ((Result & (HighestBit >> 1))
                                              ? TRUE : FALSE);

            break;
        }

        /* SHL/SAL */
        case 4:
        case 6:
        {
            Result = Value << Count;

            /* Update CF and OF */
            State->Flags.Cf = (Value & (1 << (Bits - Count))) ? TRUE : FALSE;
            if (Count == 1) State->Flags.Of = ((Result & HighestBit) ? TRUE : FALSE)
                                              ^ (State->Flags.Cf ? TRUE : FALSE);

            break;
        }

        /* SHR */
        case 5:
        {
            Result = Value >> Count;

            /* Update CF and OF */
            State->Flags.Cf = (Value & (1 << (Count - 1))) ? TRUE : FALSE;
            if (Count == 1) State->Flags.Of = (Value & HighestBit) ? TRUE : FALSE;

            break;
        }

        /* SAR */
        case 7:
        {
            Result = Value >> Count;

            /* Fill the top Count bits with the sign bit */
            if (Value & HighestBit) Result |= ((1 << Count) - 1) << (Bits - Count);

            /* Update CF and OF */
            State->Flags.Cf = (Value & (1 << (Count - 1))) ? TRUE : FALSE;
            if (Count == 1) State->Flags.Of = FALSE;

            break;
        }
    }

    /* Update ZF, SF and PF */
    State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
    State->Flags.Sf = (Result & HighestBit) ? TRUE : FALSE;
    State->Flags.Pf = Soft386CalculateParity(Result);

    /* Return the result */
    return Result;
}

/* PUBLIC FUNCTIONS ***********************************************************/

SOFT386_OPCODE_HANDLER(Soft386OpcodeGroup8082)
{
    UCHAR Immediate, Dummy, Value;
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the size */
        AddressSize = !AddressSize;
    }

    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Fetch the immediate operand */
    if (!Soft386FetchByte(State, &Immediate))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Read the operands */
    if (!Soft386ReadModrmByteOperands(State, &ModRegRm, &Dummy, &Value))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Calculate the result */
    Value = Soft386ArithmeticOperation(State, ModRegRm.Register, Value, Immediate, 8);

    /* Unless this is CMP, write back the result */
    if (ModRegRm.Register != 7)
    {
        return Soft386WriteModrmByteOperands(State, &ModRegRm, FALSE, Value);
    }
    
    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeGroup81)
{
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN OperandSize, AddressSize;
    
    OperandSize = AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        OperandSize = !OperandSize;
    }

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the size */
        AddressSize = !AddressSize;
    }

    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    if (OperandSize)
    {
        ULONG Immediate, Value, Dummy;

        /* Fetch the immediate operand */
        if (!Soft386FetchDword(State, &Immediate))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Read the operands */
        if (!Soft386ReadModrmDwordOperands(State, &ModRegRm, &Dummy, &Value))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Calculate the result */
        Value = Soft386ArithmeticOperation(State, ModRegRm.Register, Value, Immediate, 32);

        /* Unless this is CMP, write back the result */
        if (ModRegRm.Register != 7)
        {
            return Soft386WriteModrmDwordOperands(State, &ModRegRm, FALSE, Value);
        }
    }
    else
    {
        USHORT Immediate, Value, Dummy;

        /* Fetch the immediate operand */
        if (!Soft386FetchWord(State, &Immediate))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Read the operands */
        if (!Soft386ReadModrmWordOperands(State, &ModRegRm, &Dummy, &Value))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Calculate the result */
        Value = Soft386ArithmeticOperation(State, ModRegRm.Register, Value, Immediate, 16);

        /* Unless this is CMP, write back the result */
        if (ModRegRm.Register != 7)
        {
            return Soft386WriteModrmWordOperands(State, &ModRegRm, FALSE, Value);
        }
    }

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeGroup83)
{
    CHAR ImmByte;
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN OperandSize, AddressSize;
    
    OperandSize = AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        OperandSize = !OperandSize;
    }

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the size */
        AddressSize = !AddressSize;
    }

    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Fetch the immediate operand */
    if (!Soft386FetchByte(State, (PUCHAR)&ImmByte))
    {
        /* Exception occurred */
        return FALSE;
    }

    if (OperandSize)
    {
        ULONG Immediate = (ULONG)((LONG)ImmByte); // Sign extend
        ULONG Value, Dummy;

        /* Read the operands */
        if (!Soft386ReadModrmDwordOperands(State, &ModRegRm, &Dummy, &Value))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Calculate the result */
        Value = Soft386ArithmeticOperation(State, ModRegRm.Register, Value, Immediate, 32);

        /* Unless this is CMP, write back the result */
        if (ModRegRm.Register != 7)
        {
            return Soft386WriteModrmDwordOperands(State, &ModRegRm, FALSE, Value);
        }
    }
    else
    {
        USHORT Immediate = (USHORT)((SHORT)ImmByte); // Sign extend
        USHORT Value, Dummy;

        /* Read the operands */
        if (!Soft386ReadModrmWordOperands(State, &ModRegRm, &Dummy, &Value))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Calculate the result */
        Value = Soft386ArithmeticOperation(State, ModRegRm.Register, Value, Immediate, 16);

        /* Unless this is CMP, write back the result */
        if (ModRegRm.Register != 7)
        {
            return Soft386WriteModrmWordOperands(State, &ModRegRm, FALSE, Value);
        }
    }

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeGroup8F)
{
    ULONG Value;
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN OperandSize, AddressSize;
    
    OperandSize = AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        OperandSize = !OperandSize;
    }

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the size */
        AddressSize = !AddressSize;
    }

    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    if (ModRegRm.Register != 0)
    {
        /* Invalid */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Pop a value from the stack */
    if (!Soft386StackPop(State, &Value))
    {
        /* Exception occurred */
        return FALSE;
    }

    if (OperandSize)
    {
        return Soft386WriteModrmDwordOperands(State,
                                              &ModRegRm,
                                              FALSE,
                                              Value);
    }
    else
    {
        return Soft386WriteModrmWordOperands(State,
                                             &ModRegRm,
                                             FALSE,
                                             LOWORD(Value));
    }
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeGroupC0)
{
    UCHAR Dummy, Value, Count;
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the size */
        AddressSize = !AddressSize;
    }

    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Fetch the count */
    if (!Soft386FetchByte(State, &Count))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Read the operands */
    if (!Soft386ReadModrmByteOperands(State, &ModRegRm, &Dummy, &Value))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Calculate the result */
    Value = LOBYTE(Soft386RotateOperation(State,
                                          ModRegRm.Register,
                                          Value,
                                          8,
                                          Count));

    /* Write back the result */
    return Soft386WriteModrmByteOperands(State,
                                         &ModRegRm,
                                         FALSE,
                                         Value);
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeGroupC1)
{
    UCHAR Count;
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN OperandSize, AddressSize;
    
    OperandSize = AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        OperandSize = !OperandSize;
    }

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the size */
        AddressSize = !AddressSize;
    }

    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Fetch the count */
    if (!Soft386FetchByte(State, &Count))
    {
        /* Exception occurred */
        return FALSE;
    }

    if (OperandSize)
    {
        ULONG Dummy, Value;

        /* Read the operands */
        if (!Soft386ReadModrmDwordOperands(State, &ModRegRm, &Dummy, &Value))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Calculate the result */
        Value = Soft386RotateOperation(State,
                                       ModRegRm.Register,
                                       Value,
                                       32,
                                       Count);

        /* Write back the result */
        return Soft386WriteModrmDwordOperands(State, &ModRegRm, FALSE, Value);
    }
    else
    {
        USHORT Dummy, Value;

        /* Read the operands */
        if (!Soft386ReadModrmWordOperands(State, &ModRegRm, &Dummy, &Value))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Calculate the result */
        Value = LOWORD(Soft386RotateOperation(State,
                                              ModRegRm.Register,
                                              Value,
                                              16,
                                              Count));

        /* Write back the result */
        return Soft386WriteModrmWordOperands(State, &ModRegRm, FALSE, Value);
    }
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeGroupC6)
{
    UCHAR Immediate;
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the size */
        AddressSize = !AddressSize;
    }

    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    if (ModRegRm.Register != 0)
    {
        /* Invalid */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Get the immediate operand */
    if (!Soft386FetchByte(State, &Immediate))
    {
        /* Exception occurred */
        return FALSE;
    }

    return Soft386WriteModrmByteOperands(State,
                                         &ModRegRm,
                                         FALSE,
                                         Immediate);
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeGroupC7)
{
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN OperandSize, AddressSize;

    OperandSize = AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        OperandSize = !OperandSize;
    }

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the size */
        AddressSize = !AddressSize;
    }

    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    if (ModRegRm.Register != 0)
    {
        /* Invalid */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (OperandSize)
    {
        ULONG Immediate;

        /* Get the immediate operand */
        if (!Soft386FetchDword(State, &Immediate))
        {
            /* Exception occurred */
            return FALSE;
        }

        return Soft386WriteModrmDwordOperands(State,
                                              &ModRegRm,
                                              FALSE,
                                              Immediate);
    }
    else
    {
        USHORT Immediate;

        /* Get the immediate operand */
        if (!Soft386FetchWord(State, &Immediate))
        {
            /* Exception occurred */
            return FALSE;
        }

        return Soft386WriteModrmWordOperands(State,
                                             &ModRegRm,
                                             FALSE,
                                             Immediate);
    }
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeGroupD0)
{
    UCHAR Dummy, Value;
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the size */
        AddressSize = !AddressSize;
    }

    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Read the operands */
    if (!Soft386ReadModrmByteOperands(State, &ModRegRm, &Dummy, &Value))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Calculate the result */
    Value = LOBYTE(Soft386RotateOperation(State, ModRegRm.Register, Value, 8, 1));

    /* Write back the result */
    return Soft386WriteModrmByteOperands(State,
                                         &ModRegRm,
                                         FALSE,
                                         Value);

}

SOFT386_OPCODE_HANDLER(Soft386OpcodeGroupD1)
{
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN OperandSize, AddressSize;
    
    OperandSize = AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        OperandSize = !OperandSize;
    }


    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the size */
        AddressSize = !AddressSize;
    }

    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    if (OperandSize)
    {
        ULONG Dummy, Value;

        /* Read the operands */
        if (!Soft386ReadModrmDwordOperands(State, &ModRegRm, &Dummy, &Value))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Calculate the result */
        Value = Soft386RotateOperation(State, ModRegRm.Register, Value, 32, 1);

        /* Write back the result */
        return Soft386WriteModrmDwordOperands(State, &ModRegRm, FALSE, Value);
    }
    else
    {
        USHORT Dummy, Value;

        /* Read the operands */
        if (!Soft386ReadModrmWordOperands(State, &ModRegRm, &Dummy, &Value))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Calculate the result */
        Value = LOWORD(Soft386RotateOperation(State, ModRegRm.Register, Value, 16, 1));

        /* Write back the result */
        return Soft386WriteModrmWordOperands(State, &ModRegRm, FALSE, Value);
    }
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeGroupD2)
{
    UCHAR Dummy, Value;
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the size */
        AddressSize = !AddressSize;
    }

    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Read the operands */
    if (!Soft386ReadModrmByteOperands(State, &ModRegRm, &Dummy, &Value))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Calculate the result */
    Value = LOBYTE(Soft386RotateOperation(State,
                                          ModRegRm.Register,
                                          Value,
                                          8,
                                          State->GeneralRegs[SOFT386_REG_ECX].LowByte));

    /* Write back the result */
    return Soft386WriteModrmByteOperands(State,
                                         &ModRegRm,
                                         FALSE,
                                         Value);
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeGroupD3)
{
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN OperandSize, AddressSize;
    
    OperandSize = AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        OperandSize = !OperandSize;
    }

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the size */
        AddressSize = !AddressSize;
    }

    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    if (OperandSize)
    {
        ULONG Dummy, Value;

        /* Read the operands */
        if (!Soft386ReadModrmDwordOperands(State, &ModRegRm, &Dummy, &Value))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Calculate the result */
        Value = Soft386RotateOperation(State,
                                       ModRegRm.Register,
                                       Value,
                                       32,
                                       State->GeneralRegs[SOFT386_REG_ECX].LowByte);

        /* Write back the result */
        return Soft386WriteModrmDwordOperands(State, &ModRegRm, FALSE, Value);
    }
    else
    {
        USHORT Dummy, Value;

        /* Read the operands */
        if (!Soft386ReadModrmWordOperands(State, &ModRegRm, &Dummy, &Value))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Calculate the result */
        Value = LOWORD(Soft386RotateOperation(State,
                                              ModRegRm.Register,
                                              Value,
                                              16,
                                              State->GeneralRegs[SOFT386_REG_ECX].LowByte));

        /* Write back the result */
        return Soft386WriteModrmWordOperands(State, &ModRegRm, FALSE, Value);
    }
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeGroupF6)
{
    UCHAR Dummy, Value = 0;
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the size */
        AddressSize = !AddressSize;
    }

    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Read the operands */
    if (!Soft386ReadModrmByteOperands(State, &ModRegRm, &Dummy, &Value))
    {
        /* Exception occurred */
        return FALSE;
    }

    switch (ModRegRm.Register)
    {
        /* TEST */
        case 0:
        case 1:
        {
            UCHAR Immediate, Result;

            /* Fetch the immediate byte */
            if (!Soft386FetchByte(State, &Immediate))
            {
                /* Exception occurred */
                return FALSE;
            }

            /* Calculate the result */
            Result = Value & Immediate;

            /* Update the flags */
            State->Flags.Cf = FALSE;
            State->Flags.Of = FALSE;
            State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
            State->Flags.Sf = (Result & SIGN_FLAG_BYTE) ? TRUE : FALSE;
            State->Flags.Pf = Soft386CalculateParity(Result);

            break;
        }

        /* NOT */
        case 2:
        {
            /* Write back the result */
            return Soft386WriteModrmByteOperands(State, &ModRegRm, FALSE, ~Value);
        }

        /* NEG */
        case 3:
        {
            /* Calculate the result */
            UCHAR Result = -Value;

            /* Update the flags */
            State->Flags.Cf = (Value != 0) ? TRUE : FALSE;
            State->Flags.Of = (Value & SIGN_FLAG_BYTE) && (Result & SIGN_FLAG_BYTE);
            State->Flags.Af = ((Value & 0x0F) != 0) ? TRUE : FALSE;
            State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
            State->Flags.Sf = (Result & SIGN_FLAG_BYTE) ? TRUE : FALSE;
            State->Flags.Pf = Soft386CalculateParity(Result);

            /* Write back the result */
            return Soft386WriteModrmByteOperands(State, &ModRegRm, FALSE, Result);
        }

        /* MUL */
        case 4:
        {
            USHORT Result = (USHORT)Value * (USHORT)State->GeneralRegs[SOFT386_REG_EAX].LowByte;

            /* Update the flags */
            State->Flags.Cf = State->Flags.Of = HIBYTE(Result) ? TRUE : FALSE;

            /* Write back the result */
            State->GeneralRegs[SOFT386_REG_EAX].LowWord = Result;

            break;
        }

        /* IMUL */
        case 5:
        {
            SHORT Result = (SHORT)((CHAR)Value) * (SHORT)((CHAR)State->GeneralRegs[SOFT386_REG_EAX].LowByte);

            /* Update the flags */
            State->Flags.Cf = State->Flags.Of =
            ((Result < -128) || (Result > 127)) ? TRUE : FALSE;

            /* Write back the result */
            State->GeneralRegs[SOFT386_REG_EAX].LowWord = (USHORT)Result;

            break;
        }

        /* DIV */
        case 6:
        {
            UCHAR Quotient = State->GeneralRegs[SOFT386_REG_EAX].LowWord / Value;
            UCHAR Remainder = State->GeneralRegs[SOFT386_REG_EAX].LowWord % Value;

            /* Write back the results */
            State->GeneralRegs[SOFT386_REG_EAX].LowByte = Quotient;
            State->GeneralRegs[SOFT386_REG_EAX].HighByte = Remainder;

            break;
        }

        /* IDIV */
        case 7:
        {
            CHAR Quotient = (SHORT)State->GeneralRegs[SOFT386_REG_EAX].LowWord / (CHAR)Value;
            CHAR Remainder = (SHORT)State->GeneralRegs[SOFT386_REG_EAX].LowWord % (CHAR)Value;

            /* Write back the results */
            State->GeneralRegs[SOFT386_REG_EAX].LowByte = (UCHAR)Quotient;
            State->GeneralRegs[SOFT386_REG_EAX].HighByte = (UCHAR)Remainder;

            break;
        }
    }

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeGroupF7)
{
    ULONG Dummy, Value = 0, SignFlag;
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN OperandSize, AddressSize;
    
    OperandSize = AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        OperandSize = !OperandSize;
    }

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the size */
        AddressSize = !AddressSize;
    }

    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Set the sign flag */
    if (OperandSize) SignFlag = SIGN_FLAG_LONG;
    else SignFlag = SIGN_FLAG_WORD;

    /* Read the operand */
    if (OperandSize)
    {
        /* 32-bit */
        if (!Soft386ReadModrmDwordOperands(State, &ModRegRm, &Dummy, &Value))
        {
            /* Exception occurred */
            return FALSE;
        }
    }
    else
    {
        /* 16-bit */
        if (!Soft386ReadModrmWordOperands(State, &ModRegRm, (PUSHORT)&Dummy, (PUSHORT)&Value))
        {
            /* Exception occurred */
            return FALSE;
        }
    }

    switch (ModRegRm.Register)
    {
        /* TEST */
        case 0:
        case 1:
        {
            ULONG Immediate = 0, Result = 0;

            if (OperandSize)
            {
                /* Fetch the immediate dword */
                if (!Soft386FetchDword(State, &Immediate))
                {
                    /* Exception occurred */
                    return FALSE;
                }
            }
            else
            {
                /* Fetch the immediate word */
                if (!Soft386FetchWord(State, (PUSHORT)&Immediate))
                {
                    /* Exception occurred */
                    return FALSE;
                }
            }

            /* Calculate the result */
            Result = Value & Immediate;

            /* Update the flags */
            State->Flags.Cf = FALSE;
            State->Flags.Of = FALSE;
            State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
            State->Flags.Sf = (Result & SignFlag) ? TRUE : FALSE;
            State->Flags.Pf = Soft386CalculateParity(Result);

            break;
        }

        /* NOT */
        case 2:
        {
            /* Write back the result */
            if (OperandSize)
            {
                /* 32-bit */
                return Soft386WriteModrmDwordOperands(State, &ModRegRm, FALSE, ~Value);
            }
            else
            {
                /* 16-bit */
                return Soft386WriteModrmWordOperands(State, &ModRegRm, FALSE, LOWORD(~Value));
            }
        }

        /* NEG */
        case 3:
        {
            /* Calculate the result */
            ULONG Result = -Value;
            if (!OperandSize) Result &= 0xFFFF;

            /* Update the flags */
            State->Flags.Cf = (Value != 0) ? TRUE : FALSE;
            State->Flags.Of = (Value & SignFlag) && (Result & SignFlag);
            State->Flags.Af = ((Value & 0x0F) != 0) ? TRUE : FALSE;
            State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
            State->Flags.Sf = (Result & SignFlag) ? TRUE : FALSE;
            State->Flags.Pf = Soft386CalculateParity(Result);

            /* Write back the result */
            if (OperandSize)
            {
                /* 32-bit */
                return Soft386WriteModrmDwordOperands(State, &ModRegRm, FALSE, Result);
            }
            else
            {
                /* 16-bit */
                return Soft386WriteModrmWordOperands(State, &ModRegRm, FALSE, LOWORD(Result));
            }
        }

        /* MUL */
        case 4:
        {
            if (OperandSize)
            {
                ULONGLONG Result = (ULONGLONG)Value * (ULONGLONG)State->GeneralRegs[SOFT386_REG_EAX].Long;

                /* Update the flags */
                State->Flags.Cf = State->Flags.Of =
                (Result & 0xFFFFFFFF00000000ULL) ? TRUE : FALSE;

                /* Write back the result */
                State->GeneralRegs[SOFT386_REG_EAX].Long = Result & 0xFFFFFFFFULL;
                State->GeneralRegs[SOFT386_REG_EDX].Long = Result >> 32;
            }
            else
            {
                ULONG Result = (ULONG)Value * (ULONG)State->GeneralRegs[SOFT386_REG_EAX].LowWord;

                /* Update the flags */
                State->Flags.Cf = State->Flags.Of = HIWORD(Result) ? TRUE : FALSE;

                /* Write back the result */
                State->GeneralRegs[SOFT386_REG_EAX].LowWord = LOWORD(Result);
                State->GeneralRegs[SOFT386_REG_EDX].LowWord = HIWORD(Result);
            }

            break;
        }

        /* IMUL */
        case 5:
        {
            if (OperandSize)
            {
                LONGLONG Result = (LONGLONG)((LONG)Value) * (LONGLONG)((LONG)State->GeneralRegs[SOFT386_REG_EAX].Long);

                /* Update the flags */
                State->Flags.Cf = State->Flags.Of =
                ((Result < -2147483648LL) || (Result > 2147483647LL)) ? TRUE : FALSE;

                /* Write back the result */
                State->GeneralRegs[SOFT386_REG_EAX].Long = Result & 0xFFFFFFFFULL;
                State->GeneralRegs[SOFT386_REG_EDX].Long = Result >> 32;
            }
            else
            {
                LONG Result = (LONG)((SHORT)Value) * (LONG)((SHORT)State->GeneralRegs[SOFT386_REG_EAX].LowWord);

                /* Update the flags */
                State->Flags.Cf = State->Flags.Of =
                ((Result < -32768) || (Result > 32767)) ? TRUE : FALSE;

                /* Write back the result */
                State->GeneralRegs[SOFT386_REG_EAX].LowWord = LOWORD(Result);
                State->GeneralRegs[SOFT386_REG_EDX].LowWord = HIWORD(Result);
            }

            break;
        }

        /* DIV */
        case 6:
        {
            if (OperandSize)
            {
                ULONGLONG Dividend = (ULONGLONG)State->GeneralRegs[SOFT386_REG_EAX].Long
                                     | ((ULONGLONG)State->GeneralRegs[SOFT386_REG_EDX].Long << 32);
                ULONG Quotient = Dividend / Value;
                ULONG Remainder = Dividend % Value;

                /* Write back the results */
                State->GeneralRegs[SOFT386_REG_EAX].Long = Quotient;
                State->GeneralRegs[SOFT386_REG_EDX].Long = Remainder;
            }
            else
            {
                ULONG Dividend = (ULONG)State->GeneralRegs[SOFT386_REG_EAX].LowWord
                                 | ((ULONG)State->GeneralRegs[SOFT386_REG_EDX].LowWord << 16);
                USHORT Quotient = Dividend / Value;
                USHORT Remainder = Dividend % Value;

                /* Write back the results */
                State->GeneralRegs[SOFT386_REG_EAX].LowWord = Quotient;
                State->GeneralRegs[SOFT386_REG_EDX].LowWord = Remainder;
            }

            break;
        }

        /* IDIV */
        case 7:
        {
            if (OperandSize)
            {
                LONGLONG Dividend = (LONGLONG)State->GeneralRegs[SOFT386_REG_EAX].Long
                                     | ((LONGLONG)State->GeneralRegs[SOFT386_REG_EDX].Long << 32);
                LONG Quotient = Dividend / (LONG)Value;
                LONG Remainder = Dividend % (LONG)Value;

                /* Write back the results */
                State->GeneralRegs[SOFT386_REG_EAX].Long = (ULONG)Quotient;
                State->GeneralRegs[SOFT386_REG_EDX].Long = (ULONG)Remainder;
            }
            else
            {
                LONG Dividend = (LONG)State->GeneralRegs[SOFT386_REG_EAX].LowWord
                                 | ((LONG)State->GeneralRegs[SOFT386_REG_EDX].LowWord << 16);
                SHORT Quotient = Dividend / (SHORT)LOWORD(Value);
                SHORT Remainder = Dividend % (SHORT)LOWORD(Value);

                /* Write back the results */
                State->GeneralRegs[SOFT386_REG_EAX].LowWord = (USHORT)Quotient;
                State->GeneralRegs[SOFT386_REG_EDX].LowWord = (USHORT)Remainder;
            }

            break;
        }
    }

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeGroupFE)
{
    UCHAR Dummy, Value;
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the size */
        AddressSize = !AddressSize;
    }

    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    if (ModRegRm.Register > 1)
    {
        /* Invalid */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Read the operands */
    if (!Soft386ReadModrmByteOperands(State, &ModRegRm, &Dummy, &Value))
    {
        /* Exception occurred */
        return FALSE;
    }

    if (ModRegRm.Register == 0)
    {
        /* Increment and update OF and AF */
        Value++;
        State->Flags.Of = (Value == SIGN_FLAG_BYTE) ? TRUE : FALSE;
        State->Flags.Af = ((Value & 0x0F) == 0);
    }
    else
    {
        /* Decrement and update OF and AF */
        State->Flags.Of = (Value == SIGN_FLAG_BYTE) ? TRUE : FALSE;
        Value--;
        State->Flags.Af = ((Value & 0x0F) == 0x0F);
    }

    /* Update flags */
    State->Flags.Sf = (Value & SIGN_FLAG_BYTE) ? TRUE : FALSE;
    State->Flags.Zf = (Value == 0) ? TRUE : FALSE;
    State->Flags.Pf = Soft386CalculateParity(Value);

    /* Write back the result */
    return Soft386WriteModrmByteOperands(State,
                                         &ModRegRm,
                                         FALSE,
                                         Value);
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeGroupFF)
{
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN OperandSize, AddressSize;
    
    OperandSize = AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        OperandSize = !OperandSize;
    }

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the size */
        AddressSize = !AddressSize;
    }

    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    if (ModRegRm.Register == 7)
    {
        /* Invalid */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Read the operands */
    if (OperandSize)
    {
        ULONG Dummy, Value;

        if (!Soft386ReadModrmDwordOperands(State, &ModRegRm, &Dummy, &Value))
        {
            /* Exception occurred */
            return FALSE;
        }

        if (ModRegRm.Register == 0)
        {
            /* Increment and update OF and AF */
            Value++;
            State->Flags.Of = (Value == SIGN_FLAG_LONG) ? TRUE : FALSE;
            State->Flags.Af = ((Value & 0x0F) == 0);
        }
        else if (ModRegRm.Register == 1)
        {
            /* Decrement and update OF and AF */
            State->Flags.Of = (Value == SIGN_FLAG_LONG) ? TRUE : FALSE;
            Value--;
            State->Flags.Af = ((Value & 0x0F) == 0x0F);
        }
        else if (ModRegRm.Register == 2)
        {
            /* Push the current value of EIP */
            if (!Soft386StackPush(State, State->InstPtr.Long))
            {
                /* Exception occurred */
                return FALSE;
            }

            /* Set the EIP to the address */
            State->InstPtr.Long = Value;
        }
        else if (ModRegRm.Register == 3)
        {
            USHORT Selector;
            INT Segment = SOFT386_REG_DS;

            /* Check for the segment override */
            if (State->PrefixFlags & SOFT386_PREFIX_SEG)
            {
                /* Use the override segment instead */
                Segment = State->SegmentOverride;
            }

            /* Read the selector */
            if (!Soft386ReadMemory(State,
                                   Segment,
                                   ModRegRm.MemoryAddress + sizeof(ULONG),
                                   FALSE,
                                   &Selector,
                                   sizeof(USHORT)))
            {
                /* Exception occurred */
                return FALSE;
            }

            /* Push the current value of CS */
            if (!Soft386StackPush(State, State->SegmentRegs[SOFT386_REG_CS].Selector))
            {
                /* Exception occurred */
                return FALSE;
            }

            /* Push the current value of EIP */
            if (!Soft386StackPush(State, State->InstPtr.Long))
            {
                /* Exception occurred */
                return FALSE;
            }

            /* Load the new code segment */
            if (!Soft386LoadSegment(State, SOFT386_REG_CS, Selector))
            {
                /* Exception occurred */
                return FALSE;
            }

            /* Set the EIP to the address */
            State->InstPtr.Long = Value;
        }
        else if (ModRegRm.Register == 4)
        {
            /* Set the EIP to the address */
            State->InstPtr.Long = Value;
        }
        else if (ModRegRm.Register == 5)
        {
            USHORT Selector;
            INT Segment = SOFT386_REG_DS;

            /* Check for the segment override */
            if (State->PrefixFlags & SOFT386_PREFIX_SEG)
            {
                /* Use the override segment instead */
                Segment = State->SegmentOverride;
            }

            /* Read the selector */
            if (!Soft386ReadMemory(State,
                                   Segment,
                                   ModRegRm.MemoryAddress + sizeof(ULONG),
                                   FALSE,
                                   &Selector,
                                   sizeof(USHORT)))
            {
                /* Exception occurred */
                return FALSE;
            }

            /* Load the new code segment */
            if (!Soft386LoadSegment(State, SOFT386_REG_CS, Selector))
            {
                /* Exception occurred */
                return FALSE;
            }

            /* Set the EIP to the address */
            State->InstPtr.Long = Value;
        }
        else if (ModRegRm.Register == 6)
        {
            /* Push the value on to the stack */
            return Soft386StackPush(State, Value);
        }

        if (ModRegRm.Register <= 1)
        {
            /* Update flags */
            State->Flags.Sf = (Value & SIGN_FLAG_LONG) ? TRUE : FALSE;
            State->Flags.Zf = (Value == 0) ? TRUE : FALSE;
            State->Flags.Pf = Soft386CalculateParity(Value);

            /* Write back the result */
            return Soft386WriteModrmDwordOperands(State,
                                                  &ModRegRm,
                                                  FALSE,
                                                  Value);
        }
    }
    else
    {
        USHORT Dummy, Value;

        if (!Soft386ReadModrmWordOperands(State, &ModRegRm, &Dummy, &Value))
        {
            /* Exception occurred */
            return FALSE;
        }

        if (ModRegRm.Register == 0)
        {
            /* Increment and update OF */
            Value++;
            State->Flags.Of = (Value == SIGN_FLAG_WORD) ? TRUE : FALSE;
            State->Flags.Af = ((Value & 0x0F) == 0);
        }
        else if (ModRegRm.Register == 1)
        {
            /* Decrement and update OF */
            State->Flags.Of = (Value == SIGN_FLAG_WORD) ? TRUE : FALSE;
            Value--;
            State->Flags.Af = ((Value & 0x0F) == 0x0F);
        }
        else if (ModRegRm.Register == 2)
        {
            /* Push the current value of IP */
            if (!Soft386StackPush(State, State->InstPtr.LowWord))
            {
                /* Exception occurred */
                return FALSE;
            }

            /* Set the IP to the address */
            State->InstPtr.LowWord = Value;
        }
        else if (ModRegRm.Register == 3)
        {
            USHORT Selector;
            INT Segment = SOFT386_REG_DS;

            /* Check for the segment override */
            if (State->PrefixFlags & SOFT386_PREFIX_SEG)
            {
                /* Use the override segment instead */
                Segment = State->SegmentOverride;
            }

            /* Read the selector */
            if (!Soft386ReadMemory(State,
                                   Segment,
                                   ModRegRm.MemoryAddress + sizeof(USHORT),
                                   FALSE,
                                   &Selector,
                                   sizeof(USHORT)))
            {
                /* Exception occurred */
                return FALSE;
            }

            /* Push the current value of CS */
            if (!Soft386StackPush(State, State->SegmentRegs[SOFT386_REG_CS].Selector))
            {
                /* Exception occurred */
                return FALSE;
            }

            /* Push the current value of IP */
            if (!Soft386StackPush(State, State->InstPtr.LowWord))
            {
                /* Exception occurred */
                return FALSE;
            }

            /* Load the new code segment */
            if (!Soft386LoadSegment(State, SOFT386_REG_CS, Selector))
            {
                /* Exception occurred */
                return FALSE;
            }

            /* Set the IP to the address */
            State->InstPtr.LowWord = Value;

        }
        else if (ModRegRm.Register == 4)
        {
            /* Set the IP to the address */
            State->InstPtr.LowWord = Value;
        }
        else if (ModRegRm.Register == 5)
        {
            USHORT Selector;
            INT Segment = SOFT386_REG_DS;

            /* Check for the segment override */
            if (State->PrefixFlags & SOFT386_PREFIX_SEG)
            {
                /* Use the override segment instead */
                Segment = State->SegmentOverride;
            }

            /* Read the selector */
            if (!Soft386ReadMemory(State,
                                   Segment,
                                   ModRegRm.MemoryAddress + sizeof(USHORT),
                                   FALSE,
                                   &Selector,
                                   sizeof(USHORT)))
            {
                /* Exception occurred */
                return FALSE;
            }

            /* Load the new code segment */
            if (!Soft386LoadSegment(State, SOFT386_REG_CS, Selector))
            {
                /* Exception occurred */
                return FALSE;
            }

            /* Set the IP to the address */
            State->InstPtr.LowWord = Value;
        }
        else if (ModRegRm.Register == 6)
        {
            /* Push the value on to the stack */
            return Soft386StackPush(State, Value);
        }
        else
        {
            /* Invalid */
            Soft386Exception(State, SOFT386_EXCEPTION_UD);
            return FALSE;
        }

        if (ModRegRm.Register <= 1)
        {
            /* Update flags */
            State->Flags.Sf = (Value & SIGN_FLAG_WORD) ? TRUE : FALSE;
            State->Flags.Zf = (Value == 0) ? TRUE : FALSE;
            State->Flags.Pf = Soft386CalculateParity(Value);

            /* Write back the result */
            return Soft386WriteModrmWordOperands(State,
                                                 &ModRegRm,
                                                 FALSE,
                                                 Value);
        }
    }

    return TRUE;
}

/* EOF */

