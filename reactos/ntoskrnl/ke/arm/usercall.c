/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/ke/arm/usercall.c
 * PURPOSE:         Implements system calls and user-mode callbacks for ARM
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

//
// System call wrapper generator
//
#define BUILD_SYSCALLS \
SYSCALL(00, ()) \
SYSCALL(01, (_1)) \
SYSCALL(02, (_1, _2)) \
SYSCALL(03, (_1, _2, _3)) \
SYSCALL(04, (_1, _2, _3, _4 )) \
SYSCALL(05, (_1, _2, _3, _4, _5)) \
SYSCALL(06, (_1, _2, _3, _4, _5, _6)) \
SYSCALL(07, (_1, _2, _3, _4, _5, _6, _7)) \
SYSCALL(08, (_1, _2, _3, _4, _5, _6, _7, _8)) \
SYSCALL(09, (_1, _2, _3, _4, _5, _6, _7, _8, _9)) \
SYSCALL(0A, (_1, _2, _3, _4, _5, _6, _7, _8, _9, a)) \
SYSCALL(0B, (_1, _2, _3, _4, _5, _6, _7, _8, _9, a, b)) \
SYSCALL(0C, (_1, _2, _3, _4, _5, _6, _7, _8, _9, a, b, c)) \
SYSCALL(0D, (_1, _2, _3, _4, _5, _6, _7, _8, _9, a, b, c, d)) \
SYSCALL(0E, (_1, _2, _3, _4, _5, _6, _7, _8, _9, a, b, c, d, e)) \
SYSCALL(0F, (_1, _2, _3, _4, _5, _6, _7, _8, _9, a, b, c, d, e, f)) \
SYSCALL(10, (_1, _2, _3, _4, _5, _6, _7, _8, _9, a, b, c, d, e, f, _10)) \
SYSCALL(11, (_1, _2, _3, _4, _5, _6, _7, _8, _9, a, b, c, d, e, f, _10, _11))

//
// Generate function pointer definitions
//
#define PROTO
#include "ke_i.h"
BUILD_SYSCALLS

//
// Generate function code
//
#define FUNC
#include "ke_i.h"
BUILD_SYSCALLS

/* SYSTEM CALL STUBS **********************************************************/

typedef NTSTATUS (*PKI_SYSCALL_PARAM_HANDLER)(IN PVOID p, IN PVOID *g);
PKI_SYSCALL_PARAM_HANDLER KiSyscallHandlers[0x12] =
{
    KiSyscall00Param,
    KiSyscall01Param,
    KiSyscall02Param,
    KiSyscall03Param,
    KiSyscall04Param,
    KiSyscall05Param,
    KiSyscall06Param,
    KiSyscall07Param,
    KiSyscall08Param,
    KiSyscall09Param,
    KiSyscall0AParam,
    KiSyscall0BParam,
    KiSyscall0CParam,
    KiSyscall0DParam,
    KiSyscall0EParam,
    KiSyscall0FParam,
    KiSyscall10Param,
    KiSyscall11Param,
};

/* FUNCIONS *******************************************************************/

VOID
KiSystemService(IN PKTHREAD Thread,
                IN PKTRAP_FRAME TrapFrame,
                IN ULONG Instruction)
{
    ULONG Id, Number, ArgumentCount, i;
    PKPCR Pcr;
    ULONG_PTR ServiceTable, Offset;
    PKSERVICE_TABLE_DESCRIPTOR DescriptorTable;
    PVOID SystemCall;
    PVOID* Argument;
    PVOID Arguments[0x11]; // Maximum 17 arguments
    ASSERT(TrapFrame->DbgArgMark == 0xBADB0D00);
    
    //
    // Increase count of system calls
    //
    Pcr = (PKPCR)KeGetPcr();
    Pcr->Prcb->KeSystemCalls++;
    
    //
    // Get the system call ID
    //
    Id = Instruction & 0xFFFFF;
    //DPRINT1("[SWI] (%x) %p (%d) \n", Id, Thread, Thread->PreviousMode);
    
    //
    // Get the descriptor table
    //
    ServiceTable = (ULONG_PTR)Thread->ServiceTable;
    Offset = ((Id >> SERVICE_TABLE_SHIFT) & SERVICE_TABLE_MASK);
    ServiceTable += Offset;
    DescriptorTable = (PVOID)ServiceTable;
    
    //
    // Get the service call number and validate it
    //
    Number = Id & SERVICE_NUMBER_MASK;
    if (Number > DescriptorTable->Limit)
    {
        //
        // Check if this is a GUI call
        //
        UNIMPLEMENTED;
        ASSERT(FALSE);
    }
    
    //
    // Save the function responsible for handling this system call
    //
    SystemCall = (PVOID)DescriptorTable->Base[Number];
    
    //
    // Check if this is a GUI call
    //
    if (Offset & SERVICE_TABLE_TEST)
    {
        //
        // TODO
        //
        UNIMPLEMENTED;
        ASSERT(FALSE);
    }
    
    //
    // Check how many arguments this system call takes
    //
    ArgumentCount = DescriptorTable->Number[Number] / 4;
    ASSERT(ArgumentCount <= 20);
    
    //
    // Copy the register-arguments first
    // First four arguments are in a1, a2, a3, a4
    //
    Argument = (PVOID*)&TrapFrame->R0;
    for (i = 0; (i < ArgumentCount) && (i < 4); i++)
    {
        //
        // Copy them into the kernel stack
        //
        Arguments[i] = *Argument;
        Argument++;
    }
    
    //
    // If more than four, we'll have some on the user stack
    //
    if (ArgumentCount > 4)
    {
        //
        // Check where the stack is
        //
        if (Thread->PreviousMode == UserMode)
        {
            //
            // FIXME-USER: Validate the user stack
            //
            ASSERT(FALSE);
            Argument = (PVOID*)TrapFrame->UserSp;
        }
        else
        {
            //
            // We were called from the kernel
            //
            Argument = (PVOID*)(TrapFrame + 1);
        }

        //
        // Copy the rest
        //
        for (i = 4; i < ArgumentCount; i++)
        {
            //
            // Copy into kernel stack
            //
            Arguments[i] = *Argument;
            Argument++;
        }
    }
    
    //
    // Do the system call and save result in EAX
    //
    TrapFrame->R0 = KiSyscallHandlers[ArgumentCount]((PVOID)SystemCall,
                                                     (PVOID)Arguments);
}
