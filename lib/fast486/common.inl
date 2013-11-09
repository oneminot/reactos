/*
 * Fast486 386/486 CPU Emulation Library
 * common.inl
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

/* PUBLIC FUNCTIONS ***********************************************************/

FORCEINLINE
VOID
Fast486Exception(PFAST486_STATE State,
                 FAST486_EXCEPTIONS ExceptionCode)
{
    /* Call the internal function */
    Fast486ExceptionWithErrorCode(State, ExceptionCode, 0);
}

FORCEINLINE
BOOLEAN
Fast486StackPush(PFAST486_STATE State,
                 ULONG Value)
{
    BOOLEAN Size = State->SegmentRegs[FAST486_REG_SS].Size;

    /* The OPSIZE prefix toggles the size */
    if (State->PrefixFlags & FAST486_PREFIX_OPSIZE) Size = !Size;

    if (Size)
    {
        /* 32-bit size */

        /* Check if ESP is between 1 and 3 */
        if (State->GeneralRegs[FAST486_REG_ESP].Long >= 1
            && State->GeneralRegs[FAST486_REG_ESP].Long <= 3)
        {
            Fast486Exception(State, FAST486_EXCEPTION_SS);
            return FALSE;
        }

        /* Subtract ESP by 4 */
        State->GeneralRegs[FAST486_REG_ESP].Long -= 4;

        /* Store the value in SS:ESP */
        return Fast486WriteMemory(State,
                                  FAST486_REG_SS,
                                  State->GeneralRegs[FAST486_REG_ESP].Long,
                                  &Value,
                                  sizeof(ULONG));
    }
    else
    {
        /* 16-bit size */
        USHORT ShortValue = LOWORD(Value);

        /* Check if SP is 1 */
        if (State->GeneralRegs[FAST486_REG_ESP].Long == 1)
        {
            Fast486Exception(State, FAST486_EXCEPTION_SS);
            return FALSE;
        }

        /* Subtract SP by 2 */
        State->GeneralRegs[FAST486_REG_ESP].LowWord -= 2;

        /* Store the value in SS:SP */
        return Fast486WriteMemory(State,
                                  FAST486_REG_SS,
                                  State->GeneralRegs[FAST486_REG_ESP].LowWord,
                                  &ShortValue,
                                  sizeof(USHORT));
    }
}

FORCEINLINE
BOOLEAN
Fast486StackPop(PFAST486_STATE State,
                PULONG Value)
{
    ULONG LongValue;
    USHORT ShortValue;
    BOOLEAN Size = State->SegmentRegs[FAST486_REG_SS].Size;

    /* The OPSIZE prefix toggles the size */
    if (State->PrefixFlags & FAST486_PREFIX_OPSIZE) Size = !Size;

    if (Size)
    {
        /* 32-bit size */

        /* Check if ESP is 0xFFFFFFFF */
        if (State->GeneralRegs[FAST486_REG_ESP].Long == 0xFFFFFFFF)
        {
            Fast486Exception(State, FAST486_EXCEPTION_SS);
            return FALSE;
        }

        /* Read the value from SS:ESP */
        if (!Fast486ReadMemory(State,
                               FAST486_REG_SS,
                               State->GeneralRegs[FAST486_REG_ESP].Long,
                               FALSE,
                               &LongValue,
                               sizeof(LongValue)))
        {
            /* An exception occurred */
            return FALSE;
        }

        /* Increment ESP by 4 */
        State->GeneralRegs[FAST486_REG_ESP].Long += 4;

        /* Store the value in the result */
        *Value = LongValue;
    }
    else
    {
        /* 16-bit size */

        /* Check if SP is 0xFFFF */
        if (State->GeneralRegs[FAST486_REG_ESP].LowWord == 0xFFFF)
        {
            Fast486Exception(State, FAST486_EXCEPTION_SS);
            return FALSE;
        }

        /* Read the value from SS:SP */
        if (!Fast486ReadMemory(State,
                               FAST486_REG_SS,
                               State->GeneralRegs[FAST486_REG_ESP].LowWord,
                               FALSE,
                               &ShortValue,
                               sizeof(ShortValue)))
        {
            /* An exception occurred */
            return FALSE;
        }

        /* Increment SP by 2 */
        State->GeneralRegs[FAST486_REG_ESP].Long += 2;

        /* Store the value in the result */
        *Value = ShortValue;
    }

    return TRUE;
}

FORCEINLINE
BOOLEAN
Fast486LoadSegment(PFAST486_STATE State,
                   INT Segment,
                   USHORT Selector)
{
    PFAST486_SEG_REG CachedDescriptor;
    FAST486_GDT_ENTRY GdtEntry;

    ASSERT(Segment < FAST486_NUM_SEG_REGS);

    /* Get the cached descriptor */
    CachedDescriptor = &State->SegmentRegs[Segment];

    /* Check for protected mode */
    if ((State->ControlRegisters[FAST486_REG_CR0] & FAST486_CR0_PE) && !State->Flags.Vm)
    {
        /* Make sure the GDT contains the entry */
        if (GET_SEGMENT_INDEX(Selector) >= (State->Gdtr.Size + 1))
        {
            Fast486Exception(State, FAST486_EXCEPTION_GP);
            return FALSE;
        }

        /* Read the GDT */
        // FIXME: This code is only correct when paging is disabled!!!
        State->MemReadCallback(State,
                               State->Gdtr.Address
                               + GET_SEGMENT_INDEX(Selector),
                               &GdtEntry,
                               sizeof(GdtEntry));

        if (Segment == FAST486_REG_SS)
        {
            /* Loading the stack segment */

            if (GET_SEGMENT_INDEX(Selector) == 0)
            {
                Fast486Exception(State, FAST486_EXCEPTION_GP);
                return FALSE;
            }

            if (!GdtEntry.SystemType)
            {
                /* This is a special descriptor */
                Fast486Exception(State, FAST486_EXCEPTION_GP);
                return FALSE;
            }

            if (GdtEntry.Executable || !GdtEntry.ReadWrite)
            {
                Fast486Exception(State, FAST486_EXCEPTION_GP);
                return FALSE;
            }

            if ((GET_SEGMENT_RPL(Selector) != Fast486GetCurrentPrivLevel(State))
                || (GET_SEGMENT_RPL(Selector) != GdtEntry.Dpl))
            {
                Fast486Exception(State, FAST486_EXCEPTION_GP);
                return FALSE;
            }

            if (!GdtEntry.Present)
            {
                Fast486Exception(State, FAST486_EXCEPTION_SS);
                return FALSE;
            }
        }
        else if (Segment == FAST486_REG_CS)
        {
            /* Loading the code segment */
            // TODO: NOT IMPLEMENTED
        }
        else
        {
            /* Loading a data segment */

            if (!GdtEntry.SystemType)
            {
                /* This is a special descriptor */
                Fast486Exception(State, FAST486_EXCEPTION_GP);
                return FALSE;
            }

            if ((GET_SEGMENT_RPL(Selector) > GdtEntry.Dpl)
                && (Fast486GetCurrentPrivLevel(State) > GdtEntry.Dpl))
            {
                Fast486Exception(State, FAST486_EXCEPTION_GP);
                return FALSE;
            }

            if (!GdtEntry.Present)
            {
                Fast486Exception(State, FAST486_EXCEPTION_NP);
                return FALSE;
            }
        }

        /* Update the cache entry */
        CachedDescriptor->Selector = Selector;
        CachedDescriptor->Base = GdtEntry.Base | (GdtEntry.BaseMid << 16) | (GdtEntry.BaseHigh << 24);
        CachedDescriptor->Limit = GdtEntry.Limit | (GdtEntry.LimitHigh << 16);
        CachedDescriptor->Accessed = GdtEntry.Accessed;
        CachedDescriptor->ReadWrite = GdtEntry.ReadWrite;
        CachedDescriptor->DirConf = GdtEntry.DirConf;
        CachedDescriptor->Executable = GdtEntry.Executable;
        CachedDescriptor->SystemType = GdtEntry.SystemType;
        CachedDescriptor->Dpl = GdtEntry.Dpl;
        CachedDescriptor->Present = GdtEntry.Present;
        CachedDescriptor->Size = GdtEntry.Size;

        /* Check for page granularity */
        if (GdtEntry.Granularity) CachedDescriptor->Limit <<= 12;
    }
    else
    {
        /* Update the selector and base */
        CachedDescriptor->Selector = Selector;
        CachedDescriptor->Base = Selector << 4;
    }

    return TRUE;
}

FORCEINLINE
BOOLEAN
Fast486FetchByte(PFAST486_STATE State,
                 PUCHAR Data)
{
    PFAST486_SEG_REG CachedDescriptor;

    /* Get the cached descriptor of CS */
    CachedDescriptor = &State->SegmentRegs[FAST486_REG_CS];

    /* Read from memory */
    if (!Fast486ReadMemory(State,
                           FAST486_REG_CS,
                           (CachedDescriptor->Size) ? State->InstPtr.Long
                                                    : State->InstPtr.LowWord,
                           TRUE,
                           Data,
                           sizeof(UCHAR)))
    {
        /* Exception occurred during instruction fetch */
        return FALSE;
    }

    /* Advance the instruction pointer */
    if (CachedDescriptor->Size) State->InstPtr.Long++;
    else State->InstPtr.LowWord++;

    return TRUE;
}

FORCEINLINE
BOOLEAN
Fast486FetchWord(PFAST486_STATE State,
                 PUSHORT Data)
{
    PFAST486_SEG_REG CachedDescriptor;

    /* Get the cached descriptor of CS */
    CachedDescriptor = &State->SegmentRegs[FAST486_REG_CS];

    /* Read from memory */
    // FIXME: Fix byte order on big-endian machines
    if (!Fast486ReadMemory(State,
                           FAST486_REG_CS,
                           (CachedDescriptor->Size) ? State->InstPtr.Long
                                                    : State->InstPtr.LowWord,
                           TRUE,
                           Data,
                           sizeof(USHORT)))
    {
        /* Exception occurred during instruction fetch */
        return FALSE;
    }

    /* Advance the instruction pointer */
    if (CachedDescriptor->Size) State->InstPtr.Long += sizeof(USHORT);
    else State->InstPtr.LowWord += sizeof(USHORT);

    return TRUE;
}

FORCEINLINE
BOOLEAN
Fast486FetchDword(PFAST486_STATE State,
                  PULONG Data)
{
    PFAST486_SEG_REG CachedDescriptor;

    /* Get the cached descriptor of CS */
    CachedDescriptor = &State->SegmentRegs[FAST486_REG_CS];

    /* Read from memory */
    // FIXME: Fix byte order on big-endian machines
    if (!Fast486ReadMemory(State,
                           FAST486_REG_CS,
                           (CachedDescriptor->Size) ? State->InstPtr.Long
                                                    : State->InstPtr.LowWord,
                           TRUE,
                           Data,
                           sizeof(ULONG)))
    {
        /* Exception occurred during instruction fetch */
        return FALSE;
    }

    /* Advance the instruction pointer */
    if (CachedDescriptor->Size) State->InstPtr.Long += sizeof(ULONG);
    else State->InstPtr.LowWord += sizeof(ULONG);

    return TRUE;
}

FORCEINLINE
BOOLEAN
Fast486GetIntVector(PFAST486_STATE State,
                    UCHAR Number,
                    PFAST486_IDT_ENTRY IdtEntry)
{
    ULONG FarPointer;

    /* Check for protected mode */
    if (State->ControlRegisters[FAST486_REG_CR0] & FAST486_CR0_PE)
    {
        /* Read from the IDT */
        // FIXME: This code is only correct when paging is disabled!!!
        State->MemReadCallback(State,
                               State->Idtr.Address
                               + Number * sizeof(*IdtEntry),
                               IdtEntry,
                               sizeof(*IdtEntry));
    }
    else
    {
        /* Read from the real-mode IVT */

        /* Paging is always disabled in real mode */
        State->MemReadCallback(State,
                               State->Idtr.Address
                               + Number * sizeof(FarPointer),
                               &FarPointer,
                               sizeof(FarPointer));

        /* Fill a fake IDT entry */
        IdtEntry->Offset = LOWORD(FarPointer);
        IdtEntry->Selector = HIWORD(FarPointer);
        IdtEntry->Zero = 0;
        IdtEntry->Type = FAST486_IDT_INT_GATE;
        IdtEntry->Storage = FALSE;
        IdtEntry->Dpl = 0;
        IdtEntry->Present = TRUE;
        IdtEntry->OffsetHigh = 0;
    }

    /*
     * Once paging support is implemented this function
     * will not always return true
     */
    return TRUE;
}

FORCEINLINE
BOOLEAN
Fast486CalculateParity(UCHAR Number)
{
    // See http://graphics.stanford.edu/~seander/bithacks.html#ParityLookupTable too...
    return (0x9669 >> ((Number & 0x0F) ^ (Number >> 4))) & 1;
}

FORCEINLINE
BOOLEAN
Fast486ParseModRegRm(PFAST486_STATE State,
                     BOOLEAN AddressSize,
                     PFAST486_MOD_REG_RM ModRegRm)
{
    UCHAR ModRmByte, Mode, RegMem;

    /* Fetch the MOD REG R/M byte */
    if (!Fast486FetchByte(State, &ModRmByte))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Unpack the mode and R/M */
    Mode = ModRmByte >> 6;
    RegMem = ModRmByte & 0x07;

    /* Set the register operand */
    ModRegRm->Register = (ModRmByte >> 3) & 0x07;

    /* Check the mode */
    if ((ModRmByte >> 6) == 3)
    {
        /* The second operand is also a register */
        ModRegRm->Memory = FALSE;
        ModRegRm->SecondRegister = RegMem;

        /* Done parsing */
        return TRUE;
    }

    /* The second operand is memory */
    ModRegRm->Memory = TRUE;

    if (AddressSize)
    {
        if (RegMem == FAST486_REG_ESP)
        {
            UCHAR SibByte;
            ULONG Scale, Index, Base;

            /* Fetch the SIB byte */
            if (!Fast486FetchByte(State, &SibByte))
            {
                /* Exception occurred */
                return FALSE;
            }

            /* Unpack the scale, index and base */
            Scale = 1 << (SibByte >> 6);
            Index = (SibByte >> 3) & 0x07;
            if (Index != FAST486_REG_ESP) Index = State->GeneralRegs[Index].Long;
            else Index = 0;
            Base = State->GeneralRegs[SibByte & 0x07].Long;

            /* Calculate the address */
            ModRegRm->MemoryAddress = Base + Index * Scale;
        }
        else if (RegMem == FAST486_REG_EBP)
        {
            if (Mode) ModRegRm->MemoryAddress = State->GeneralRegs[FAST486_REG_EBP].Long;
            else ModRegRm->MemoryAddress = 0;
        }
        else
        {
            /* Get the base from the register */
            ModRegRm->MemoryAddress = State->GeneralRegs[RegMem].Long;
        }

        /* Check if there is no segment override */
        if (!(State->PrefixFlags & FAST486_PREFIX_SEG))
        {
            /* Check if the default segment should be SS */
            if ((RegMem == FAST486_REG_EBP) && Mode)
            {
                /* Add a SS: prefix */
                State->PrefixFlags |= FAST486_PREFIX_SEG;
                State->SegmentOverride = FAST486_REG_SS;
            }
        }

        if (Mode == 1)
        {
            CHAR Offset;

            /* Fetch the byte */
            if (!Fast486FetchByte(State, (PUCHAR)&Offset))
            {
                /* Exception occurred */
                return FALSE;
            }

            /* Add the signed offset to the address */
            ModRegRm->MemoryAddress += (LONG)Offset;
        }
        else if ((Mode == 2) || ((Mode == 0) && (RegMem == FAST486_REG_EBP)))
        {
            LONG Offset;

            /* Fetch the dword */
            if (!Fast486FetchDword(State, (PULONG)&Offset))
            {
                /* Exception occurred */
                return FALSE;
            }

            /* Add the signed offset to the address */
            ModRegRm->MemoryAddress += Offset;
        }
    }
    else
    {
        /* Check the operand */
        switch (RegMem)
        {
            case 0:
            {
                /* [BX + SI] */
                ModRegRm->MemoryAddress = State->GeneralRegs[FAST486_REG_EBX].LowWord
                                           + State->GeneralRegs[FAST486_REG_ESI].LowWord;

                break;
            }

            case 1:
            {
                /* [BX + DI] */
                ModRegRm->MemoryAddress = State->GeneralRegs[FAST486_REG_EBX].LowWord
                                           + State->GeneralRegs[FAST486_REG_EDI].LowWord;

                break;
            }

            case 2:
            {
                /* SS:[BP + SI] */
                ModRegRm->MemoryAddress = State->GeneralRegs[FAST486_REG_EBP].LowWord
                                           + State->GeneralRegs[FAST486_REG_ESI].LowWord;

                break;
            }

            case 3:
            {
                /* SS:[BP + DI] */
                ModRegRm->MemoryAddress = State->GeneralRegs[FAST486_REG_EBP].LowWord
                                           + State->GeneralRegs[FAST486_REG_EDI].LowWord;

                break;
            }

            case 4:
            {
                /* [SI] */
                ModRegRm->MemoryAddress = State->GeneralRegs[FAST486_REG_ESI].LowWord;

                break;
            }

            case 5:
            {
                /* [DI] */
                ModRegRm->MemoryAddress = State->GeneralRegs[FAST486_REG_EDI].LowWord;

                break;
            }

            case 6:
            {
                if (Mode)
                {
                    /* [BP] */
                    ModRegRm->MemoryAddress = State->GeneralRegs[FAST486_REG_EBP].LowWord;
                }
                else
                {
                    /* [constant] (added later) */
                    ModRegRm->MemoryAddress = 0;
                }

                break;
            }

            case 7:
            {
                /* [BX] */
                ModRegRm->MemoryAddress = State->GeneralRegs[FAST486_REG_EBX].LowWord;

                break;
            }
        }

        /* Check if there is no segment override */
        if (!(State->PrefixFlags & FAST486_PREFIX_SEG))
        {
            /* Check if the default segment should be SS */
            if ((RegMem == 2) || (RegMem == 3) || ((RegMem == 6) && Mode))
            {
                /* Add a SS: prefix */
                State->PrefixFlags |= FAST486_PREFIX_SEG;
                State->SegmentOverride = FAST486_REG_SS;
            }
        }

        if (Mode == 1)
        {
            CHAR Offset;

            /* Fetch the byte */
            if (!Fast486FetchByte(State, (PUCHAR)&Offset))
            {
                /* Exception occurred */
                return FALSE;
            }

            /* Add the signed offset to the address */
            ModRegRm->MemoryAddress += (LONG)Offset;
        }
        else if ((Mode == 2) || ((Mode == 0) && (RegMem == 6)))
        {
            SHORT Offset;

            /* Fetch the word */
            if (!Fast486FetchWord(State, (PUSHORT)&Offset))
            {
                /* Exception occurred */
                return FALSE;
            }

            /* Add the signed offset to the address */
            ModRegRm->MemoryAddress += (LONG)Offset;
        }

        /* Clear the top 16 bits */
        ModRegRm->MemoryAddress &= 0x0000FFFF;
    }

    return TRUE;
}

FORCEINLINE
BOOLEAN
Fast486ReadModrmByteOperands(PFAST486_STATE State,
                             PFAST486_MOD_REG_RM ModRegRm,
                             PUCHAR RegValue,
                             PUCHAR RmValue)
{
    FAST486_SEG_REGS Segment = FAST486_REG_DS;

    /* Get the register value */
    if (ModRegRm->Register & 0x04)
    {
        /* AH, CH, DH, BH */
        *RegValue = State->GeneralRegs[ModRegRm->Register & 0x03].HighByte;
    }
    else
    {
        /* AL, CL, DL, BL */
        *RegValue = State->GeneralRegs[ModRegRm->Register & 0x03].LowByte;
    }

    if (!ModRegRm->Memory)
    {
        /* Get the second register value */
        if (ModRegRm->SecondRegister & 0x04)
        {
            /* AH, CH, DH, BH */
            *RmValue = State->GeneralRegs[ModRegRm->SecondRegister & 0x03].HighByte;
        }
        else
        {
            /* AL, CL, DL, BL */
            *RmValue = State->GeneralRegs[ModRegRm->SecondRegister & 0x03].LowByte;
        }
    }
    else
    {
        /* Check for the segment override */
        if (State->PrefixFlags & FAST486_PREFIX_SEG)
        {
            /* Use the override segment instead */
            Segment = State->SegmentOverride;
        }

        /* Read memory */
        if (!Fast486ReadMemory(State,
                               Segment,
                               ModRegRm->MemoryAddress,
                               FALSE,
                               RmValue,
                               sizeof(UCHAR)))
        {
            /* Exception occurred */
            return FALSE;
        }
    }

    return TRUE;
}

FORCEINLINE
BOOLEAN
Fast486ReadModrmWordOperands(PFAST486_STATE State,
                             PFAST486_MOD_REG_RM ModRegRm,
                             PUSHORT RegValue,
                             PUSHORT RmValue)
{
    FAST486_SEG_REGS Segment = FAST486_REG_DS;

    /* Get the register value */
    *RegValue = State->GeneralRegs[ModRegRm->Register].LowWord;

    if (!ModRegRm->Memory)
    {
        /* Get the second register value */
        *RmValue = State->GeneralRegs[ModRegRm->SecondRegister].LowWord;
    }
    else
    {
        /* Check for the segment override */
        if (State->PrefixFlags & FAST486_PREFIX_SEG)
        {
            /* Use the override segment instead */
            Segment = State->SegmentOverride;
        }

        /* Read memory */
        if (!Fast486ReadMemory(State,
                               Segment,
                               ModRegRm->MemoryAddress,
                               FALSE,
                               RmValue,
                               sizeof(USHORT)))
        {
            /* Exception occurred */
            return FALSE;
        }
    }

    return TRUE;
}

FORCEINLINE
BOOLEAN
Fast486ReadModrmDwordOperands(PFAST486_STATE State,
                              PFAST486_MOD_REG_RM ModRegRm,
                              PULONG RegValue,
                              PULONG RmValue)
{
    FAST486_SEG_REGS Segment = FAST486_REG_DS;

    /* Get the register value */
    *RegValue = State->GeneralRegs[ModRegRm->Register].Long;

    if (!ModRegRm->Memory)
    {
        /* Get the second register value */
        *RmValue = State->GeneralRegs[ModRegRm->SecondRegister].Long;
    }
    else
    {
        /* Check for the segment override */
        if (State->PrefixFlags & FAST486_PREFIX_SEG)
        {
            /* Use the override segment instead */
            Segment = State->SegmentOverride;
        }

        /* Read memory */
        if (!Fast486ReadMemory(State,
                               Segment,
                               ModRegRm->MemoryAddress,
                               FALSE,
                               RmValue,
                               sizeof(ULONG)))
        {
            /* Exception occurred */
            return FALSE;
        }
    }

    return TRUE;
}

FORCEINLINE
BOOLEAN
Fast486WriteModrmByteOperands(PFAST486_STATE State,
                              PFAST486_MOD_REG_RM ModRegRm,
                              BOOLEAN WriteRegister,
                              UCHAR Value)
{
    FAST486_SEG_REGS Segment = FAST486_REG_DS;

    if (WriteRegister)
    {
        /* Store the value in the register */
        if (ModRegRm->Register & 0x04)
        {
            /* AH, CH, DH, BH */
            State->GeneralRegs[ModRegRm->Register & 0x03].HighByte = Value;
        }
        else
        {
            /* AL, CL, DL, BL */
            State->GeneralRegs[ModRegRm->Register & 0x03].LowByte = Value;
        }
    }
    else
    {
        if (!ModRegRm->Memory)
        {
            /* Store the value in the second register */
            if (ModRegRm->SecondRegister & 0x04)
            {
                /* AH, CH, DH, BH */
                State->GeneralRegs[ModRegRm->SecondRegister & 0x03].HighByte = Value;
            }
            else
            {
                /* AL, CL, DL, BL */
                State->GeneralRegs[ModRegRm->SecondRegister & 0x03].LowByte = Value;
            }
        }
        else
        {
            /* Check for the segment override */
            if (State->PrefixFlags & FAST486_PREFIX_SEG)
            {
                /* Use the override segment instead */
                Segment = State->SegmentOverride;
            }

            /* Write memory */
            if (!Fast486WriteMemory(State,
                                    Segment,
                                    ModRegRm->MemoryAddress,
                                    &Value,
                                    sizeof(UCHAR)))
            {
                /* Exception occurred */
                return FALSE;
            }
        }
    }

    return TRUE;
}

FORCEINLINE
BOOLEAN
Fast486WriteModrmWordOperands(PFAST486_STATE State,
                              PFAST486_MOD_REG_RM ModRegRm,
                              BOOLEAN WriteRegister,
                              USHORT Value)
{
    FAST486_SEG_REGS Segment = FAST486_REG_DS;

    if (WriteRegister)
    {
        /* Store the value in the register */
        State->GeneralRegs[ModRegRm->Register].LowWord = Value;
    }
    else
    {
        if (!ModRegRm->Memory)
        {
            /* Store the value in the second register */
            State->GeneralRegs[ModRegRm->SecondRegister].LowWord = Value;
        }
        else
        {
            /* Check for the segment override */
            if (State->PrefixFlags & FAST486_PREFIX_SEG)
            {
                /* Use the override segment instead */
                Segment = State->SegmentOverride;
            }

            /* Write memory */
            if (!Fast486WriteMemory(State,
                                    Segment,
                                    ModRegRm->MemoryAddress,
                                    &Value,
                                    sizeof(USHORT)))
            {
                /* Exception occurred */
                return FALSE;
            }
        }
    }

    return TRUE;
}

FORCEINLINE
BOOLEAN
Fast486WriteModrmDwordOperands(PFAST486_STATE State,
                               PFAST486_MOD_REG_RM ModRegRm,
                               BOOLEAN WriteRegister,
                               ULONG Value)
{
    FAST486_SEG_REGS Segment = FAST486_REG_DS;

    if (WriteRegister)
    {
        /* Store the value in the register */
        State->GeneralRegs[ModRegRm->Register].Long = Value;
    }
    else
    {
        if (!ModRegRm->Memory)
        {
            /* Store the value in the second register */
            State->GeneralRegs[ModRegRm->SecondRegister].Long = Value;
        }
        else
        {
            /* Check for the segment override */
            if (State->PrefixFlags & FAST486_PREFIX_SEG)
            {
                /* Use the override segment instead */
                Segment = State->SegmentOverride;
            }

            /* Write memory */
            if (!Fast486WriteMemory(State,
                                    Segment,
                                    ModRegRm->MemoryAddress,
                                    &Value,
                                    sizeof(ULONG)))
            {
                /* Exception occurred */
                return FALSE;
            }
        }
    }

    return TRUE;
}

/* EOF */
