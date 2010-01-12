#ifndef __NTOSKRNL_INCLUDE_INTERNAL_I386_KE_H
#define __NTOSKRNL_INCLUDE_INTERNAL_I386_KE_H

#ifndef __ASM__

#include "intrin_i.h"
#include "v86m.h"

extern ULONG Ke386CacheAlignment;

//
// Thread Dispatcher Header DebugActive Mask
//
#define DR_MASK(x)                              (1 << (x))
#define DR_REG_MASK                             0x4F

#define IMAGE_FILE_MACHINE_ARCHITECTURE IMAGE_FILE_MACHINE_I386

//
// INT3 is 1 byte long
//
#define KD_BREAKPOINT_TYPE        UCHAR
#define KD_BREAKPOINT_SIZE        sizeof(UCHAR)
#define KD_BREAKPOINT_VALUE       0xCC

//
// Macros for getting and setting special purpose registers in portable code
//
#define KeGetContextPc(Context) \
    ((Context)->Eip)

#define KeSetContextPc(Context, ProgramCounter) \
    ((Context)->Eip = (ProgramCounter))

#define KeGetTrapFramePc(TrapFrame) \
    ((TrapFrame)->Eip)

#define KeGetContextReturnRegister(Context) \
    ((Context)->Eax)

#define KeSetContextReturnRegister(Context, ReturnValue) \
    ((Context)->Eax = (ReturnValue))

//
// Macro to get trap and exception frame from a thread stack
//
#define KeGetTrapFrame(Thread) \
    (PKTRAP_FRAME)((ULONG_PTR)((Thread)->InitialStack) - \
                   sizeof(KTRAP_FRAME) - \
                   sizeof(FX_SAVE_AREA))

#define KeGetExceptionFrame(Thread) \
    NULL

//
// Macro to get context switches from the PRCB
// All architectures but x86 have it in the PRCB's KeContextSwitches
//
#define KeGetContextSwitches(Prcb)  \
    CONTAINING_RECORD(Prcb, KIPCR, PrcbData)->ContextSwitches

//
// Returns the Interrupt State from a Trap Frame.
// ON = TRUE, OFF = FALSE
//
#define KeGetTrapFrameInterruptState(TrapFrame) \
        BooleanFlagOn((TrapFrame)->EFlags, EFLAGS_INTERRUPT_MASK)

//
// Flags for exiting a trap
//
#define KTE_SKIP_PM_BIT  (((KTRAP_EXIT_SKIP_BITS) { { .SkipPreviousMode = TRUE } }).Bits)
#define KTE_SKIP_SEG_BIT (((KTRAP_EXIT_SKIP_BITS) { { .SkipSegments  = TRUE } }).Bits)
#define KTE_SKIP_VOL_BIT (((KTRAP_EXIT_SKIP_BITS) { { .SkipVolatiles = TRUE } }).Bits)
 
typedef union _KTRAP_EXIT_SKIP_BITS
{
    struct
    {
        UCHAR SkipPreviousMode:1;
        UCHAR SkipSegments:1;
        UCHAR SkipVolatiles:1;
        UCHAR Reserved:5;
    };
    UCHAR Bits;
} KTRAP_EXIT_SKIP_BITS, *PKTRAP_EXIT_SKIP_BITS;


//
// Flags used by the VDM/V8086 emulation engine for determining instruction prefixes
//
#define PFX_FLAG_ES                0x00000100
#define PFX_FLAG_CS                0x00000200
#define PFX_FLAG_SS                0x00000400
#define PFX_FLAG_DS                0x00000800
#define PFX_FLAG_FS                0x00001000
#define PFX_FLAG_GS                0x00002000
#define PFX_FLAG_OPER32            0x00004000
#define PFX_FLAG_ADDR32            0x00008000
#define PFX_FLAG_LOCK              0x00010000
#define PFX_FLAG_REPNE             0x00020000
#define PFX_FLAG_REP               0x00040000

//
// VDM Helper Macros
//
// All VDM/V8086 opcode emulators have the same FASTCALL function definition.
// We need to keep 2 parameters while the original ASM implementation uses 4:
// TrapFrame, PrefixFlags, Eip, InstructionSize;
//
// We pass the trap frame, and prefix flags, in our two parameters.
//
// We then realize that since the smallest prefix flag is 0x100, this gives us
// a count of up to 0xFF. So we OR in the instruction size with the prefix flags
//
// We further realize that we always have access to EIP from the trap frame, and
// that if we want the *current instruction* EIP, we simply have to add the
// instruction size *MINUS ONE*, and that gives us the EIP we should be looking
// at now, so we don't need to use the stack to push this parameter.
//
// We actually only care about the *current instruction* EIP in one location,
// so although it may be slightly more expensive to re-calculate the EIP one
// more time, this way we don't redefine ALL opcode handlers to have 3 parameters,
// which would be forcing stack usage in all other scenarios.
//
#define KiVdmSetVdmEFlags(x)        InterlockedOr((PLONG)KiNtVdmState, (x));
#define KiVdmClearVdmEFlags(x)      InterlockedAnd((PLONG)KiNtVdmState, ~(x))
#define KiCallVdmHandler(x)         KiVdmOpcode##x(TrapFrame, Flags)
#define KiCallVdmPrefixHandler(x)   KiVdmOpcodePrefix(TrapFrame, Flags | x)
#define KiVdmUnhandledOpcode(x)                     \
    BOOLEAN                                         \
    FASTCALL                                        \
    KiVdmOpcode##x(IN PKTRAP_FRAME TrapFrame,       \
                   IN ULONG Flags)                  \
    {                                               \
        /* Not yet handled */                       \
        UNIMPLEMENTED;                              \
        while (TRUE);                               \
        return TRUE;                                \
    }

C_ASSERT(NPX_FRAME_LENGTH == sizeof(FX_SAVE_AREA));

//
// Local parameters
//
typedef struct _KV86_FRAME
{
    PVOID ThreadStack;
    PVOID ThreadTeb;
    PVOID PcrTeb;
} KV86_FRAME, *PKV86_FRAME;

//
// Virtual Stack Frame
//
typedef struct _KV8086_STACK_FRAME
{
    KTRAP_FRAME TrapFrame;
    FX_SAVE_AREA NpxArea;
    KV86_FRAME V86Frame;
} KV8086_STACK_FRAME, *PKV8086_STACK_FRAME;
              
//
// Registers an interrupt handler with an IDT vector
//
FORCEINLINE
VOID
KeRegisterInterruptHandler(IN ULONG Vector,
                           IN PVOID Handler)
{                           
    UCHAR Entry;
    ULONG_PTR Address;
    PKIPCR Pcr = (PKIPCR)KeGetPcr();

    //
    // Get the entry from the HAL
    //
    Entry = HalVectorToIDTEntry(Vector);
    Address = PtrToUlong(Handler);

    //
    // Now set the data
    //
    Pcr->IDT[Entry].ExtendedOffset = (USHORT)(Address >> 16);
    Pcr->IDT[Entry].Offset = (USHORT)Address;
}

//
// Returns the registered interrupt handler for a given IDT vector
//
FORCEINLINE
PVOID
KeQueryInterruptHandler(IN ULONG Vector)
{
    PKIPCR Pcr = (PKIPCR)KeGetPcr();
    UCHAR Entry;

    //
    // Get the entry from the HAL
    //
    Entry = HalVectorToIDTEntry(Vector);

    //
    // Read the entry from the IDT
    //
    return (PVOID)(((Pcr->IDT[Entry].ExtendedOffset << 16) & 0xFFFF0000) |
                    (Pcr->IDT[Entry].Offset & 0xFFFF));
}

//
// Invalidates the TLB entry for a specified address
//
FORCEINLINE
VOID
KeInvalidateTlbEntry(IN PVOID Address)
{
    /* Invalidate the TLB entry for this address */
    __invlpg(Address);
}

FORCEINLINE
VOID
KeFlushProcessTb(VOID)
{
    /* Flush the TLB by resetting CR3 */
    __writecr3(__readcr3());
}

FORCEINLINE
PRKTHREAD
KeGetCurrentThread(VOID)
{
    /* Return the current thread */
    return ((PKIPCR)KeGetPcr())->PrcbData.CurrentThread;
}

FORCEINLINE
VOID
KiRundownThread(IN PKTHREAD Thread)
{
#ifndef CONFIG_SMP
    /* Check if this is the NPX Thread */
    if (KeGetCurrentPrcb()->NpxThread == Thread)
    {
        /* Clear it */
        KeGetCurrentPrcb()->NpxThread = NULL;
        Ke386FnInit();
    }
#else
    /* Nothing to do */
#endif
}

VOID
FASTCALL
Ki386InitializeTss(
    IN PKTSS Tss,
    IN PKIDTENTRY Idt,
    IN PKGDTENTRY Gdt
);

VOID
NTAPI
KiSetCR0Bits(VOID);

VOID
NTAPI
KiGetCacheInformation(VOID);

BOOLEAN
NTAPI
KiIsNpxPresent(
    VOID
);

BOOLEAN
NTAPI
KiIsNpxErrataPresent(
    VOID
);

VOID
NTAPI
KiSetProcessorType(VOID);

ULONG
NTAPI
KiGetFeatureBits(VOID);

#ifdef _NTOSKRNL_ /* FIXME: Move flags above to NDK instead of here */
VOID
NTAPI
KiThreadStartup(PKSYSTEM_ROUTINE SystemRoutine,
                PKSTART_ROUTINE StartRoutine,
                PVOID StartContext,
                BOOLEAN UserThread,
                KTRAP_FRAME TrapFrame);
#endif

NTSTATUS
NTAPI
Ke386GetGdtEntryThread(
    IN PKTHREAD Thread,
    IN ULONG Offset,
    IN PKGDTENTRY Descriptor
);

VOID
NTAPI
KiFlushNPXState(
    IN FLOATING_SAVE_AREA *SaveArea
);

VOID
NTAPI
Ki386AdjustEsp0(
    IN PKTRAP_FRAME TrapFrame
);

VOID
NTAPI
Ki386SetupAndExitToV86Mode(
    OUT PTEB VdmTeb
);

VOID
NTAPI
KeI386VdmInitialize(
    VOID
);

ULONG_PTR
NTAPI
Ki386EnableGlobalPage(
    IN volatile ULONG_PTR Context
);

VOID
NTAPI
KiI386PentiumLockErrataFixup(
    VOID
);

VOID
NTAPI
KiInitializePAT(
    VOID
);

VOID
NTAPI
KiInitializeMTRR(
    IN BOOLEAN FinalCpu
);

VOID
NTAPI
KiAmdK6InitializeMTRR(
    VOID
);

VOID
NTAPI
KiRestoreFastSyscallReturnState(
    VOID
);

ULONG_PTR
NTAPI
Ki386EnableDE(
    IN ULONG_PTR Context
);

ULONG_PTR
NTAPI
Ki386EnableFxsr(
    IN ULONG_PTR Context
);

ULONG_PTR
NTAPI
Ki386EnableXMMIExceptions(
    IN ULONG_PTR Context
);

BOOLEAN
NTAPI
VdmDispatchBop(
    IN PKTRAP_FRAME TrapFrame
);
 
BOOLEAN
FASTCALL
KiVdmOpcodePrefix(
    IN PKTRAP_FRAME TrapFrame,
    IN ULONG Flags
);

BOOLEAN
FASTCALL
Ki386HandleOpcodeV86(
    IN PKTRAP_FRAME TrapFrame
);

VOID
FASTCALL
KiEoiHelper(
    IN PKTRAP_FRAME TrapFrame
);

VOID
FASTCALL
Ki386BiosCallReturnAddress(
    IN PKTRAP_FRAME TrapFrame
);

ULONG_PTR
FASTCALL
KiExitV86Mode(
    IN PKTRAP_FRAME TrapFrame
);

//
// Global x86 only Kernel data
//
extern PVOID Ki386IopmSaveArea;
extern ULONG KeI386EFlagsAndMaskV86;
extern ULONG KeI386EFlagsOrMaskV86;
extern BOOLEAN KeI386VirtualIntExtensions;
extern KIDTENTRY KiIdt[MAXIMUM_IDTVECTOR];
extern KDESCRIPTOR KiIdtDescriptor;
extern ULONG Ke386GlobalPagesEnabled;
extern BOOLEAN KiI386PentiumLockErrataPresent;
extern ULONG KeI386NpxPresent;
extern ULONG KeI386XMMIPresent;
extern ULONG KeI386FxsrPresent;
extern ULONG KiMXCsrMask;
extern ULONG KeI386CpuType;
extern ULONG KeI386CpuStep;
extern UCHAR KiDebugRegisterTrapOffsets[9];
extern UCHAR KiDebugRegisterContextOffsets[9];
extern VOID __cdecl KiTrap02(VOID);
extern VOID __cdecl KiTrap08(VOID);
extern VOID __cdecl KiTrap13(VOID);
extern VOID __cdecl KiFastCallEntry(VOID);
extern VOID NTAPI ExpInterlockedPopEntrySListFault(VOID);
extern VOID __cdecl CopyParams(VOID);
extern VOID __cdecl ReadBatch(VOID);
extern VOID __cdecl FrRestore(VOID);

PFX_SAVE_AREA
FORCEINLINE
KiGetThreadNpxArea(IN PKTHREAD Thread)
{
    return (PFX_SAVE_AREA)((ULONG_PTR)Thread->InitialStack - sizeof(FX_SAVE_AREA));
}

//
// Sanitizes a selector
//
FORCEINLINE
ULONG
Ke386SanitizeSeg(IN ULONG Cs,
                IN KPROCESSOR_MODE Mode)
{
    //
    // Check if we're in kernel-mode, and force CPL 0 if so.
    // Otherwise, force CPL 3.
    //
    return ((Mode == KernelMode) ?
            (Cs & (0xFFFF & ~RPL_MASK)) :
            (RPL_MASK | (Cs & 0xFFFF)));
}

//
// Sanitizes EFLAGS
//
FORCEINLINE
ULONG
Ke386SanitizeFlags(IN ULONG Eflags,
                   IN KPROCESSOR_MODE Mode)
{
    //
    // Check if we're in kernel-mode, and sanitize EFLAGS if so.
    // Otherwise, also force interrupt mask on.
    //
    return ((Mode == KernelMode) ?
            (Eflags & (EFLAGS_USER_SANITIZE | EFLAGS_INTERRUPT_MASK)) :
            (EFLAGS_INTERRUPT_MASK | (Eflags & EFLAGS_USER_SANITIZE)));
}

//
// Gets a DR register from a CONTEXT structure
//
FORCEINLINE
PVOID
KiDrFromContext(IN ULONG Dr,
                IN PCONTEXT Context)
{
    return *(PVOID*)((ULONG_PTR)Context + KiDebugRegisterContextOffsets[Dr]);
}

//
// Gets a DR register from a KTRAP_FRAME structure
//
FORCEINLINE
PVOID*
KiDrFromTrapFrame(IN ULONG Dr,
                  IN PKTRAP_FRAME TrapFrame)
{
    return (PVOID*)((ULONG_PTR)TrapFrame + KiDebugRegisterTrapOffsets[Dr]);
}

//
// Sanitizes a Debug Register
//
FORCEINLINE
PVOID
Ke386SanitizeDr(IN PVOID DrAddress,
                IN KPROCESSOR_MODE Mode)
{
    //
    // Check if we're in kernel-mode, and return the address directly if so.
    // Otherwise, make sure it's not inside the kernel-mode address space.
    // If it is, then clear the address.
    //
    return ((Mode == KernelMode) ? DrAddress :
            (DrAddress <= MM_HIGHEST_USER_ADDRESS) ? DrAddress : 0);
}

FORCEINLINE
VOID
KiV86TrapReturn(IN ULONG_PTR Stack)
{
    /* Restore volatiles and stack */
    __asm__ __volatile__
    (
        "movl %0, %%esp\n"
        "popa\n"
        "ret\n"
        :
        : "r"(Stack)
        : "%esp"
    );
}
#endif
#endif /* __NTOSKRNL_INCLUDE_INTERNAL_I386_KE_H */
