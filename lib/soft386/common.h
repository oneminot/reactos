/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         386/486 CPU Emulation Library
 * FILE:            common.h
 * PURPOSE:         Common functions used internally by Soft386 (header file).
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#ifndef _COMMON_H_
#define _COMMON_H_

/* DEFINES ********************************************************************/

#ifndef FASTCALL
#define FASTCALL __fastcall
#endif

#define SIGN_FLAG_BYTE 0x80
#define SIGN_FLAG_WORD 0x8000
#define SIGN_FLAG_LONG 0x80000000
#define GET_SEGMENT_RPL(s) ((s) & 3)
#define GET_SEGMENT_INDEX(s) ((s) & 0xFFF8)

typedef struct _SOFT386_MOD_REG_RM
{
    INT Register;
    BOOLEAN Memory;
    union
    {
        INT SecondRegister;
        ULONG MemoryAddress;
    };
} SOFT386_MOD_REG_RM, *PSOFT386_MOD_REG_RM;

/* FUNCTIONS ******************************************************************/

inline
BOOLEAN
Soft386ReadMemory
(
    PSOFT386_STATE State,
    INT SegmentReg,
    ULONG Offset,
    BOOLEAN InstFetch,
    PVOID Buffer,
    ULONG Size
);

inline
BOOLEAN
Soft386WriteMemory
(
    PSOFT386_STATE State,
    INT SegmentReg,
    ULONG Offset,
    PVOID Buffer,
    ULONG Size
);

inline
BOOLEAN
Soft386StackPush
(
    PSOFT386_STATE State,
    ULONG Value
);

inline
BOOLEAN
Soft386StackPop
(
    PSOFT386_STATE State,
    PULONG Value
);

inline
BOOLEAN
Soft386LoadSegment
(
    PSOFT386_STATE State,
    INT Segment,
    USHORT Selector
);

inline
BOOLEAN
Soft386FetchByte
(
    PSOFT386_STATE State,
    PUCHAR Data
);

inline
BOOLEAN
Soft386FetchWord
(
    PSOFT386_STATE State,
    PUSHORT Data
);

inline
BOOLEAN
Soft386FetchDword
(
    PSOFT386_STATE State,
    PULONG Data
);

inline
BOOLEAN
Soft386InterruptInternal
(
    PSOFT386_STATE State,
    USHORT SegmentSelector,
    ULONG Offset,
    BOOLEAN InterruptGate
);

inline
BOOLEAN
Soft386GetIntVector
(
    PSOFT386_STATE State,
    UCHAR Number,
    PSOFT386_IDT_ENTRY IdtEntry
);

VOID
__fastcall
Soft386Exception
(
    PSOFT386_STATE State,
    INT ExceptionCode
);

inline
BOOLEAN
Soft386CalculateParity
(
    UCHAR Number
);

inline
BOOLEAN
Soft386ParseModRegRm
(
    PSOFT386_STATE State,
    BOOLEAN AddressSize,
    PSOFT386_MOD_REG_RM ModRegRm
);

#endif // _COMMON_H_

/* EOF */
