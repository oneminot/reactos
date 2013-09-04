/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         386/486 CPU Emulation Library
 * FILE:            common.c
 * PURPOSE:         Common functions used internally by Soft386.
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

// #define WIN32_NO_STATUS
// #define _INC_WINDOWS
#include <windef.h>

#include <soft386.h>
#include "common.h"

// #define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS **********************************************************/

static
inline
INT
Soft386GetCurrentPrivLevel(PSOFT386_STATE State)
{
    return GET_SEGMENT_RPL(State->SegmentRegs[SOFT386_REG_CS].Selector);
}

/* PUBLIC FUNCTIONS ***********************************************************/

inline
BOOLEAN
Soft386ReadMemory(PSOFT386_STATE State,
                  INT SegmentReg,
                  ULONG Offset,
                  BOOLEAN InstFetch,
                  PVOID Buffer,
                  ULONG Size)
{
    ULONG LinearAddress;
    PSOFT386_SEG_REG CachedDescriptor;

    ASSERT(SegmentReg < SOFT386_NUM_SEG_REGS);

    /* Get the cached descriptor */
    CachedDescriptor = &State->SegmentRegs[SegmentReg];

    if ((Offset + Size) >= CachedDescriptor->Limit)
    {
        /* Read beyond limit */
        Soft386Exception(State, SOFT386_EXCEPTION_GP);

        return FALSE;
    }

    /* Check for protected mode */
    if (State->ControlRegisters[0] & SOFT386_CR0_PE)
    {
        /* Privilege checks */

        if (!CachedDescriptor->Present)
        {
            Soft386Exception(State, SOFT386_EXCEPTION_NP);
            return FALSE;
        }

        if (GET_SEGMENT_RPL(CachedDescriptor->Selector) > CachedDescriptor->Dpl)
        {
            Soft386Exception(State, SOFT386_EXCEPTION_GP);
            return FALSE;
        }

        if (InstFetch)
        {
            if (!CachedDescriptor->Executable)
            {
                /* Data segment not executable */

                Soft386Exception(State, SOFT386_EXCEPTION_GP);
                return FALSE;
            }
        }
        else
        {
            if (CachedDescriptor->Executable && (!CachedDescriptor->ReadWrite))
            {
                /* Code segment not readable */

                Soft386Exception(State, SOFT386_EXCEPTION_GP);
                return FALSE;
            }
        }
    }

    /* Find the linear address */
    LinearAddress = CachedDescriptor->Base + Offset;

    // TODO: Paging support!

    /* Did the host provide a memory hook? */
    if (State->MemReadCallback)
    {
        /* Yes, call the host */
        State->MemReadCallback(State, LinearAddress, Buffer, Size);
    }
    else
    {
        /* Read the memory directly */
        RtlMoveMemory(Buffer, (PVOID)LinearAddress, Size);
    }

    return TRUE;
}

inline
BOOLEAN
Soft386WriteMemory(PSOFT386_STATE State,
                   INT SegmentReg,
                   ULONG Offset,
                   PVOID Buffer,
                   ULONG Size)
{
    ULONG LinearAddress;
    PSOFT386_SEG_REG CachedDescriptor;

    ASSERT(SegmentReg < SOFT386_NUM_SEG_REGS);

    /* Get the cached descriptor */
    CachedDescriptor = &State->SegmentRegs[SegmentReg];

    if ((Offset + Size) >= CachedDescriptor->Limit)
    {
        /* Write beyond limit */
        Soft386Exception(State, SOFT386_EXCEPTION_GP);

        return FALSE;
    }

    /* Check for protected mode */
    if (State->ControlRegisters[0] & SOFT386_CR0_PE)
    {
        /* Privilege checks */

        if (!CachedDescriptor->Present)
        {
            Soft386Exception(State, SOFT386_EXCEPTION_NP);
            return FALSE;
        }

        if (GET_SEGMENT_RPL(CachedDescriptor->Selector) > CachedDescriptor->Dpl)
        {
            Soft386Exception(State, SOFT386_EXCEPTION_GP);
            return FALSE;
        }

        if (CachedDescriptor->Executable)
        {
            /* Code segment not writable */

            Soft386Exception(State, SOFT386_EXCEPTION_GP);
            return FALSE;
        }
        else if (!CachedDescriptor->ReadWrite)
        {
            /* Data segment not writeable */

            Soft386Exception(State, SOFT386_EXCEPTION_GP);
            return FALSE;
        }
    }

    /* Find the linear address */
    LinearAddress = CachedDescriptor->Base + Offset;

    // TODO: Paging support!

    /* Did the host provide a memory hook? */
    if (State->MemWriteCallback)
    {
        /* Yes, call the host */
        State->MemWriteCallback(State, LinearAddress, Buffer, Size);
    }
    else
    {
        /* Write the memory directly */
        RtlMoveMemory((PVOID)LinearAddress, Buffer, Size);
    }

    return TRUE;
}

inline
BOOLEAN
Soft386StackPush(PSOFT386_STATE State, ULONG Value)
{
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_SS].Size;

    /* The OPSIZE prefix toggles the size */
    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE) Size = !Size;

    if (Size)
    {
        /* 32-bit size */

        /* Check if ESP is between 1 and 3 */
        if (State->GeneralRegs[SOFT386_REG_ESP].Long >= 1
            && State->GeneralRegs[SOFT386_REG_ESP].Long <= 3)
        {
            Soft386Exception(State, SOFT386_EXCEPTION_SS);
            return FALSE;
        }

        /* Subtract ESP by 4 */
        State->GeneralRegs[SOFT386_REG_ESP].Long -= 4;

        /* Store the value in SS:ESP */
        return Soft386WriteMemory(State,
                                  SOFT386_REG_SS,
                                  State->GeneralRegs[SOFT386_REG_ESP].Long,
                                  &Value,
                                  sizeof(ULONG));
    }
    else
    {
        /* 16-bit size */
        USHORT ShortValue = LOWORD(Value);

        /* Check if SP is 1 */
        if (State->GeneralRegs[SOFT386_REG_ESP].Long == 1)
        {
            Soft386Exception(State, SOFT386_EXCEPTION_SS);
            return FALSE;
        }

        /* Subtract SP by 2 */
        State->GeneralRegs[SOFT386_REG_ESP].LowWord -= 2;

        /* Store the value in SS:SP */
        return Soft386WriteMemory(State,
                                  SOFT386_REG_SS,
                                  State->GeneralRegs[SOFT386_REG_ESP].LowWord,
                                  &ShortValue,
                                  sizeof(USHORT));
    }
}

inline
BOOLEAN
Soft386StackPop(PSOFT386_STATE State, PULONG Value)
{
    ULONG LongValue;
    USHORT ShortValue;
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_SS].Size;

    /* The OPSIZE prefix toggles the size */
    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE) Size = !Size;

    if (Size)
    {
        /* 32-bit size */

        /* Check if ESP is 0xFFFFFFFF */
        if (State->GeneralRegs[SOFT386_REG_ESP].Long == 0xFFFFFFFF)
        {
            Soft386Exception(State, SOFT386_EXCEPTION_SS);
            return FALSE;
        }

        /* Read the value from SS:ESP */
        if (!Soft386ReadMemory(State,
                               SOFT386_REG_SS,
                               State->GeneralRegs[SOFT386_REG_ESP].Long,
                               FALSE,
                               &LongValue,
                               sizeof(LongValue)))
        {
            /* An exception occurred */
            return FALSE;
        }

        /* Increment ESP by 4 */
        State->GeneralRegs[SOFT386_REG_ESP].Long += 4;

        /* Store the value in the result */
        *Value = LongValue;
    }
    else
    {
        /* 16-bit size */

        /* Check if SP is 0xFFFF */
        if (State->GeneralRegs[SOFT386_REG_ESP].LowWord == 0xFFFF)
        {
            Soft386Exception(State, SOFT386_EXCEPTION_SS);
            return FALSE;
        }

        /* Read the value from SS:SP */
        if (!Soft386ReadMemory(State,
                               SOFT386_REG_SS,
                               State->GeneralRegs[SOFT386_REG_ESP].LowWord,
                               FALSE,
                               &ShortValue,
                               sizeof(ShortValue)))
        {
            /* An exception occurred */
            return FALSE;
        }

        /* Increment SP by 2 */
        State->GeneralRegs[SOFT386_REG_ESP].Long += 2;

        /* Store the value in the result */
        *Value = ShortValue;
    }

    return TRUE;
}

inline
BOOLEAN
Soft386LoadSegment(PSOFT386_STATE State, INT Segment, USHORT Selector)
{
    PSOFT386_SEG_REG CachedDescriptor;
    SOFT386_GDT_ENTRY GdtEntry;

    ASSERT(Segment < SOFT386_NUM_SEG_REGS);

    /* Get the cached descriptor */
    CachedDescriptor = &State->SegmentRegs[Segment];

    /* Check for protected mode */
    if (State->ControlRegisters[SOFT386_REG_CR0] & SOFT386_CR0_PE)
    {
        /* Make sure the GDT contains the entry */
        if (GET_SEGMENT_INDEX(Selector) >= (State->Gdtr.Size + 1))
        {
            Soft386Exception(State, SOFT386_EXCEPTION_GP);
            return FALSE;
        }

        /* Read the GDT */
        // FIXME: This code is only correct when paging is disabled!!!
        if (State->MemReadCallback)
        {
            State->MemReadCallback(State,
                                   State->Gdtr.Address
                                   + GET_SEGMENT_INDEX(Selector),
                                   &GdtEntry,
                                   sizeof(GdtEntry));
        }
        else
        {
            RtlMoveMemory(&GdtEntry,
                          (PVOID)(State->Gdtr.Address
                          + GET_SEGMENT_INDEX(Selector)),
                          sizeof(GdtEntry));
        }

        /* Check if we are loading SS */
        if (Segment == SOFT386_REG_SS)
        {
            if (GET_SEGMENT_INDEX(Selector) == 0)
            {
                Soft386Exception(State, SOFT386_EXCEPTION_GP);
                return FALSE;
            }

            if (GdtEntry.Executable || !GdtEntry.ReadWrite)
            {
                Soft386Exception(State, SOFT386_EXCEPTION_GP);
                return FALSE;
            }

            if ((GET_SEGMENT_RPL(Selector) != Soft386GetCurrentPrivLevel(State))
                || (GET_SEGMENT_RPL(Selector) != GdtEntry.Dpl))
            {
                Soft386Exception(State, SOFT386_EXCEPTION_GP);
                return FALSE;
            }

            if (!GdtEntry.Present)
            {
                Soft386Exception(State, SOFT386_EXCEPTION_SS);
                return FALSE;
            }
        }
        else
        {
            if ((GET_SEGMENT_RPL(Selector) > GdtEntry.Dpl)
                && (Soft386GetCurrentPrivLevel(State) > GdtEntry.Dpl))
            {
                Soft386Exception(State, SOFT386_EXCEPTION_GP);
                return FALSE;
            }

            if (!GdtEntry.Present)
            {
                Soft386Exception(State, SOFT386_EXCEPTION_NP);
                return FALSE;
            }
        }

        /* Update the cache entry */
        CachedDescriptor->Selector = Selector;
        CachedDescriptor->Base = GdtEntry.Base | (GdtEntry.BaseHigh << 24);
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

inline
BOOLEAN
Soft386FetchByte(PSOFT386_STATE State, PUCHAR Data)
{
    PSOFT386_SEG_REG CachedDescriptor;

    /* Get the cached descriptor of CS */
    CachedDescriptor = &State->SegmentRegs[SOFT386_REG_CS];

    /* Read from memory */
    if (!Soft386ReadMemory(State,
                           SOFT386_REG_CS,
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

inline
BOOLEAN
Soft386FetchWord(PSOFT386_STATE State, PUSHORT Data)
{
    PSOFT386_SEG_REG CachedDescriptor;

    /* Get the cached descriptor of CS */
    CachedDescriptor = &State->SegmentRegs[SOFT386_REG_CS];

    /* Read from memory */
    // FIXME: Fix byte order on big-endian machines
    if (!Soft386ReadMemory(State,
                           SOFT386_REG_CS,
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

inline
BOOLEAN
Soft386FetchDword(PSOFT386_STATE State, PULONG Data)
{
    PSOFT386_SEG_REG CachedDescriptor;

    /* Get the cached descriptor of CS */
    CachedDescriptor = &State->SegmentRegs[SOFT386_REG_CS];

    /* Read from memory */
    // FIXME: Fix byte order on big-endian machines
    if (!Soft386ReadMemory(State,
                           SOFT386_REG_CS,
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

inline
BOOLEAN
Soft386InterruptInternal(PSOFT386_STATE State,
                         USHORT SegmentSelector,
                         ULONG Offset,
                         BOOLEAN InterruptGate)
{
    /* Check for protected mode */
    if (State->ControlRegisters[SOFT386_REG_CR0] & SOFT386_CR0_PE)
    {
        SOFT386_TSS Tss;
        USHORT OldSs = State->SegmentRegs[SOFT386_REG_SS].Selector;
        ULONG OldEsp = State->GeneralRegs[SOFT386_REG_ESP].Long;

        /* Check if the interrupt handler is more privileged */
        if (Soft386GetCurrentPrivLevel(State) > GET_SEGMENT_RPL(SegmentSelector))
        {
            /* Read the TSS */
            // FIXME: This code is only correct when paging is disabled!!!
            if (State->MemReadCallback)
            {
                State->MemReadCallback(State,
                                       State->Tss.Address,
                                       &Tss,
                                       sizeof(Tss));
            }
            else
            {
                RtlMoveMemory(&Tss, (PVOID)State->Tss.Address, sizeof(Tss));
            }

            /* Check the new (higher) privilege level */
            switch (GET_SEGMENT_RPL(SegmentSelector))
            {
                case 0:
                {
                    if (!Soft386LoadSegment(State, SOFT386_REG_SS, Tss.Ss0))
                    {
                        /* Exception occurred */
                        return FALSE;
                    }
                    State->GeneralRegs[SOFT386_REG_ESP].Long = Tss.Esp0;

                    break;
                }

                case 1:
                {
                    if (!Soft386LoadSegment(State, SOFT386_REG_SS, Tss.Ss1))
                    {
                        /* Exception occurred */
                        return FALSE;
                    }
                    State->GeneralRegs[SOFT386_REG_ESP].Long = Tss.Esp1;

                    break;
                }

                case 2:
                {
                    if (!Soft386LoadSegment(State, SOFT386_REG_SS, Tss.Ss2))
                    {
                        /* Exception occurred */
                        return FALSE;
                    }
                    State->GeneralRegs[SOFT386_REG_ESP].Long = Tss.Esp2;

                    break;
                }

                default:
                {
                    /* Should never reach here! */
                    ASSERT(FALSE);
                }
            }

            /* Push SS selector */
            if (!Soft386StackPush(State, OldSs)) return FALSE;

            /* Push stack pointer */
            if (!Soft386StackPush(State, OldEsp)) return FALSE;
        }
    }

    /* Push EFLAGS */
    if (!Soft386StackPush(State, State->Flags.Long)) return FALSE;

    /* Push CS selector */
    if (!Soft386StackPush(State, State->SegmentRegs[SOFT386_REG_CS].Selector)) return FALSE;

    /* Push the instruction pointer */
    if (!Soft386StackPush(State, State->InstPtr.Long)) return FALSE;

    if (InterruptGate)
    {
        /* Disable interrupts after a jump to an interrupt gate handler */
        State->Flags.If = FALSE;
    }

    /* Load new CS */
    if (!Soft386LoadSegment(State, SOFT386_REG_CS, SegmentSelector))
    {
        /* An exception occurred during the jump */
        return FALSE;
    }

    if (State->SegmentRegs[SOFT386_REG_CS].Size)
    {
        /* 32-bit code segment, use EIP */
        State->InstPtr.Long = Offset;
    }
    else
    {
        /* 16-bit code segment, use IP */
        State->InstPtr.LowWord = LOWORD(Offset);
    }

    return TRUE;
}

inline
BOOLEAN
Soft386GetIntVector(PSOFT386_STATE State,
                    UCHAR Number,
                    PSOFT386_IDT_ENTRY IdtEntry)
{
    ULONG FarPointer;

    /* Check for protected mode */
    if (State->ControlRegisters[SOFT386_REG_CR0] & SOFT386_CR0_PE)
    {
        /* Read from the IDT */
        // FIXME: This code is only correct when paging is disabled!!!
        if (State->MemReadCallback)
        {
            State->MemReadCallback(State,
                                   State->Idtr.Address
                                   + Number * sizeof(*IdtEntry),
                                   IdtEntry,
                                   sizeof(*IdtEntry));
        }
        else
        {
            RtlMoveMemory(IdtEntry,
                          (PVOID)(State->Idtr.Address
                          + Number * sizeof(*IdtEntry)),
                          sizeof(*IdtEntry));
        }
    }
    else
    {
        /* Read from the real-mode IVT */
        
        /* Paging is always disabled in real mode */
        if (State->MemReadCallback)
        {
            State->MemReadCallback(State,
                                   State->Idtr.Address
                                   + Number * sizeof(FarPointer),
                                   &FarPointer,
                                   sizeof(FarPointer));
        }
        else
        {
            RtlMoveMemory(IdtEntry,
                          (PVOID)(State->Idtr.Address
                          + Number * sizeof(FarPointer)),
                          sizeof(FarPointer));
        }

        /* Fill a fake IDT entry */
        IdtEntry->Offset = LOWORD(FarPointer);
        IdtEntry->Selector = HIWORD(FarPointer);
        IdtEntry->Zero = 0;
        IdtEntry->Type = SOFT386_IDT_INT_GATE;
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

VOID
FASTCALL
Soft386Exception(PSOFT386_STATE State, INT ExceptionCode)
{
    SOFT386_IDT_ENTRY IdtEntry;

    /* Increment the exception count */
    State->ExceptionCount++;

    /* Check if the exception occurred more than once */
    if (State->ExceptionCount > 1)
    {
        /* Then this is a double fault */
        ExceptionCode = SOFT386_EXCEPTION_DF;
    }

    /* Check if this is a triple fault */
    if (State->ExceptionCount == 3)
    {
        /* Reset the CPU */
        Soft386Reset(State);
        return;
    }

    if (!Soft386GetIntVector(State, ExceptionCode, &IdtEntry))
    {
        /*
         * If this function failed, that means Soft386Exception
         * was called again, so just return in this case.
         */
        return;
    }

    /* Perform the interrupt */
    if (!Soft386InterruptInternal(State,
                                  IdtEntry.Selector,
                                  MAKELONG(IdtEntry.Offset, IdtEntry.OffsetHigh),
                                  IdtEntry.Type))
    {
        /*
         * If this function failed, that means Soft386Exception
         * was called again, so just return in this case.
         */
        return;
    }
}

inline
BOOLEAN
Soft386CalculateParity(UCHAR Number)
{
    Number ^= Number >> 1;
    Number ^= Number >> 2;
    Number ^= Number >> 4;
    return !(Number & 1);
}

inline
BOOLEAN
Soft386ParseModRegRm(PSOFT386_STATE State,
                     BOOLEAN AddressSize,
                     PSOFT386_MOD_REG_RM ModRegRm)
{
    UCHAR ModRmByte, Mode, RegMem;

    /* Fetch the MOD REG R/M byte */
    if (!Soft386FetchByte(State, &ModRmByte))
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
        if (RegMem == SOFT386_REG_ESP)
        {
            UCHAR SibByte;
            ULONG Scale, Index, Base;

            /* Fetch the SIB byte */
            if (!Soft386FetchByte(State, &SibByte))
            {
                /* Exception occurred */
                return FALSE;
            }

            /* Unpack the scale, index and base */
            Scale = 1 << (SibByte >> 6);
            Index = (SibByte >> 3) & 0x07;
            if (Index != SOFT386_REG_ESP) Index = State->GeneralRegs[Index].Long;
            else Index = 0;
            Base = State->GeneralRegs[SibByte & 0x07].Long;

            /* Calculate the address */
            ModRegRm->MemoryAddress = Base + Index * Scale;
        }
        else if (RegMem == SOFT386_REG_EBP)
        {
            if (Mode) ModRegRm->MemoryAddress = State->GeneralRegs[SOFT386_REG_EBP].Long;
            else ModRegRm->MemoryAddress = 0;
        }
        else
        {
            /* Get the base from the register */
            ModRegRm->MemoryAddress = State->GeneralRegs[RegMem].Long;
        }

        /* Check if there is no segment override */
        if (!(State->PrefixFlags & SOFT386_PREFIX_SEG))
        {
            /* Check if the default segment should be SS */
            if ((RegMem == SOFT386_REG_EBP) && Mode)
            {
                /* Add a SS: prefix */
                State->PrefixFlags |= SOFT386_PREFIX_SEG;
                State->SegmentOverride = SOFT386_REG_SS;
            }
        }

        if (Mode == 1)
        {
            CHAR Offset;
            
            /* Fetch the byte */
            if (!Soft386FetchByte(State, (PUCHAR)&Offset))
            {
                /* Exception occurred */
                return FALSE;
            }

            /* Add the signed offset to the address */
            ModRegRm->MemoryAddress += (LONG)Offset;
        }
        else if ((Mode == 2) || ((Mode == 0) && (RegMem == SOFT386_REG_EBP)))
        {
            LONG Offset;
            
            /* Fetch the dword */
            if (!Soft386FetchDword(State, (PULONG)&Offset))
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
            case 2:
            {
                /* (SS:)[BX + SI] */
                ModRegRm->MemoryAddress = State->GeneralRegs[SOFT386_REG_EBX].LowWord
                                           + State->GeneralRegs[SOFT386_REG_ESI].LowWord;

                break;
            }

            case 1:
            case 3:
            {
                /* (SS:)[BX + DI] */
                ModRegRm->MemoryAddress = State->GeneralRegs[SOFT386_REG_EBX].LowWord
                                           + State->GeneralRegs[SOFT386_REG_EDI].LowWord;

                break;
            }

            case 4:
            {
                /* [SI] */
                ModRegRm->MemoryAddress = State->GeneralRegs[SOFT386_REG_ESI].LowWord;

                break;
            }

            case 5:
            {
                /* [DI] */
                ModRegRm->MemoryAddress = State->GeneralRegs[SOFT386_REG_EDI].LowWord;

                break;
            }

            case 6:
            {
                if (Mode)
                {
                    /* [BP] */
                    ModRegRm->MemoryAddress = State->GeneralRegs[SOFT386_REG_EBP].LowWord;
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
                ModRegRm->MemoryAddress = State->GeneralRegs[SOFT386_REG_EBX].LowWord;

                break;
            }
        }

        /* Check if there is no segment override */
        if (!(State->PrefixFlags & SOFT386_PREFIX_SEG))
        {
            /* Check if the default segment should be SS */
            if ((RegMem == 2) || (RegMem == 3) || ((RegMem == 6) && Mode))
            {
                /* Add a SS: prefix */
                State->PrefixFlags |= SOFT386_PREFIX_SEG;
                State->SegmentOverride = SOFT386_REG_SS;
            }
        }

        if (Mode == 1)
        {
            CHAR Offset;
            
            /* Fetch the byte */
            if (!Soft386FetchByte(State, (PUCHAR)&Offset))
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
            if (!Soft386FetchWord(State, (PUSHORT)&Offset))
            {
                /* Exception occurred */
                return FALSE;
            }

            /* Add the signed offset to the address */
            ModRegRm->MemoryAddress += (LONG)Offset;
        }
    }

    return TRUE;
}

inline
BOOLEAN
Soft386ReadModrmByteOperands(PSOFT386_STATE State,
                             PSOFT386_MOD_REG_RM ModRegRm,
                             PUCHAR RegValue,
                             PUCHAR RmValue)
{
    INT Segment = SOFT386_REG_DS;

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
        if (State->PrefixFlags & SOFT386_PREFIX_SEG)
        {
            /* Use the override segment instead */
            Segment = State->SegmentOverride;
        }

        /* Read memory */
        if (!Soft386ReadMemory(State,
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

inline
BOOLEAN
Soft386ReadModrmWordOperands(PSOFT386_STATE State,
                              PSOFT386_MOD_REG_RM ModRegRm,
                              PUSHORT RegValue,
                              PUSHORT RmValue)
{
    INT Segment = SOFT386_REG_DS;

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
        if (State->PrefixFlags & SOFT386_PREFIX_SEG)
        {
            /* Use the override segment instead */
            Segment = State->SegmentOverride;
        }

        /* Read memory */
        if (!Soft386ReadMemory(State,
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

inline
BOOLEAN
Soft386ReadModrmDwordOperands(PSOFT386_STATE State,
                              PSOFT386_MOD_REG_RM ModRegRm,
                              PULONG RegValue,
                              PULONG RmValue)
{
    INT Segment = SOFT386_REG_DS;

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
        if (State->PrefixFlags & SOFT386_PREFIX_SEG)
        {
            /* Use the override segment instead */
            Segment = State->SegmentOverride;
        }

        /* Read memory */
        if (!Soft386ReadMemory(State,
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

inline
BOOLEAN
Soft386WriteModrmByteOperands(PSOFT386_STATE State,
                             PSOFT386_MOD_REG_RM ModRegRm,
                             BOOLEAN WriteRegister,
                             UCHAR Value)
{
    INT Segment = SOFT386_REG_DS;

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
            if (State->PrefixFlags & SOFT386_PREFIX_SEG)
            {
                /* Use the override segment instead */
                Segment = State->SegmentOverride;
            }

            /* Write memory */
            if (!Soft386WriteMemory(State,
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

inline
BOOLEAN
Soft386WriteModrmWordOperands(PSOFT386_STATE State,
                              PSOFT386_MOD_REG_RM ModRegRm,
                              BOOLEAN WriteRegister,
                              USHORT Value)
{
    INT Segment = SOFT386_REG_DS;

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
            if (State->PrefixFlags & SOFT386_PREFIX_SEG)
            {
                /* Use the override segment instead */
                Segment = State->SegmentOverride;
            }

            /* Write memory */
            if (!Soft386WriteMemory(State,
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

inline
BOOLEAN
Soft386WriteModrnDwordOperands(PSOFT386_STATE State,
                               PSOFT386_MOD_REG_RM ModRegRm,
                               BOOLEAN WriteRegister,
                               ULONG Value)
{
    INT Segment = SOFT386_REG_DS;

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
            if (State->PrefixFlags & SOFT386_PREFIX_SEG)
            {
                /* Use the override segment instead */
                Segment = State->SegmentOverride;
            }

            /* Write memory */
            if (!Soft386WriteMemory(State,
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
