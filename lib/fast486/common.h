/*
 * Fast486 386/486 CPU Emulation Library
 * common.h
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

#ifndef _COMMON_H_
#define _COMMON_H_

/* DEFINES ********************************************************************/

#ifndef FASTCALL
#define FASTCALL __fastcall
#endif

#define SIGN_FLAG_BYTE  0x80
#define SIGN_FLAG_WORD  0x8000
#define SIGN_FLAG_LONG  0x80000000
#define REAL_MODE_FLAGS_MASK 0x17FD5
#define PROT_MODE_FLAGS_MASK 0x10DD5

/* Block size for string operations */
#define STRING_BLOCK_SIZE 4096

#define GET_SEGMENT_RPL(s)          ((s) & 3)
#define GET_SEGMENT_INDEX(s)        ((s) & 0xFFF8)
#define EXCEPTION_HAS_ERROR_CODE(x) (((x) == 8) || ((x) >= 10 && (x) <= 14))

#define NO_LOCK_PREFIX() if (State->PrefixFlags & FAST486_PREFIX_LOCK)\
{\
    Fast486Exception(State, FAST486_EXCEPTION_UD);\
    return FALSE;\
}
#define TOGGLE_OPSIZE(x) if (State->PrefixFlags & FAST486_PREFIX_OPSIZE)\
{\
    x = !x;\
}
#define TOGGLE_ADSIZE(x) if (State->PrefixFlags & FAST486_PREFIX_ADSIZE)\
{\
    x = !x;\
}
#define SWAP(x, y) { (x) ^= (y); (y) ^= (x); (x) ^= (y); }

#define PAGE_ALIGN(x)   ((x) & 0xFFFFF000)
#define PAGE_OFFSET(x)  ((x) & 0x00000FFF)

#ifndef PAGE_SIZE
#define PAGE_SIZE   4096
#endif

typedef struct _FAST486_MOD_REG_RM
{
    FAST486_GEN_REGS Register;
    BOOLEAN Memory;
    union
    {
        FAST486_GEN_REGS SecondRegister;
        ULONG MemoryAddress;
    };
} FAST486_MOD_REG_RM, *PFAST486_MOD_REG_RM;

#pragma pack(push, 1)

typedef union _FAST486_PAGE_DIR
{
    struct
    {
        ULONG Present       : 1;
        ULONG Writeable     : 1;
        ULONG Usermode      : 1;
        ULONG WriteThrough  : 1;
        ULONG NoCache       : 1;
        ULONG Accessed      : 1;
        ULONG AlwaysZero    : 1;
        ULONG Size          : 1;
        ULONG Unused        : 4;
        ULONG TableAddress  : 20;
    };
    ULONG Value;
} FAST486_PAGE_DIR, *PFAST486_PAGE_DIR;

typedef union _FAST486_PAGE_TABLE
{
    struct
    {
        ULONG Present       : 1;
        ULONG Writeable     : 1;
        ULONG Usermode      : 1;
        ULONG WriteThrough  : 1;
        ULONG NoCache       : 1;
        ULONG Accessed      : 1;
        ULONG Dirty         : 1;
        ULONG AlwaysZero    : 1;
        ULONG Global        : 1;
        ULONG Unused        : 3;
        ULONG Address       : 20;
    };
    ULONG Value;
} FAST486_PAGE_TABLE, *PFAST486_PAGE_TABLE;

#pragma pack(pop)

/* FUNCTIONS ******************************************************************/

BOOLEAN
Fast486ReadMemory
(
    PFAST486_STATE State,
    FAST486_SEG_REGS SegmentReg,
    ULONG Offset,
    BOOLEAN InstFetch,
    PVOID Buffer,
    ULONG Size
);

BOOLEAN
Fast486WriteMemory
(
    PFAST486_STATE State,
    FAST486_SEG_REGS SegmentReg,
    ULONG Offset,
    PVOID Buffer,
    ULONG Size
);

BOOLEAN
Fast486InterruptInternal
(
    PFAST486_STATE State,
    USHORT SegmentSelector,
    ULONG Offset,
    BOOLEAN InterruptGate
);

VOID
FASTCALL
Fast486ExceptionWithErrorCode
(
    PFAST486_STATE State,
    FAST486_EXCEPTIONS ExceptionCode,
    ULONG ErrorCode
);

/* INLINED FUNCTIONS **********************************************************/

/* static */ FORCEINLINE
INT
Fast486GetCurrentPrivLevel(PFAST486_STATE State)
{
    return GET_SEGMENT_RPL(State->SegmentRegs[FAST486_REG_CS].Selector);
}

#include "common.inl"

#endif // _COMMON_H_

/* EOF */
