/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/arm/trap.s
 * PURPOSE:         Support for exceptions and interrupts on ARM machines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

    .title "ARM Trap Dispatching and Handling"
    .include "ntoskrnl/include/internal/arm/kxarm.h"
    .include "ntoskrnl/include/internal/arm/ksarm.h"

    .global KiArmVectorTable
    KiArmVectorTable:
        b .                                     // Reset
        ldr pc, _KiUndefinedInstructionJump     // Undefined Instruction
        ldr pc, _KiSoftwareInterruptJump        // Software Interrupt
        ldr pc, _KiPrefetchAbortJump            // Prefetch Abort
        ldr pc, _KiDataAbortJump                // Data Abort
        ldr pc, _KiReservedJump                 // Reserved
        ldr pc, _KiInterruptJump                // Interrupt
        ldr pc, _KiFastInterruptJump            // Fast Interrupt
        
    _KiUndefinedInstructionJump:    .word KiUndefinedInstructionException
    _KiSoftwareInterruptJump:       .word KiSoftwareInterruptException
    _KiPrefetchAbortJump:           .word KiPrefetchAbortException
    _KiDataAbortJump:               .word KiDataAbortException
    _KiReservedJump:                .word KiReservedException
    _KiInterruptJump:               .word KiInterruptException
    _KiFastInterruptJump:           .word KiFastInterruptException

    TEXTAREA
    NESTED_ENTRY KiUndefinedInstructionException
    PROLOG_END KiUndefinedInstructionException
    
    //
    // FIXME: TODO
    //
    b .
    
    ENTRY_END KiUndefinedInstructionException
    
    NESTED_ENTRY KiSoftwareInterruptException
    PROLOG_END KiSoftwareInterruptException
    
    //
    // Save return address
    //
    str lr, [sp, #-4]!
    
    //
    // Make space for trap frame
    //
    sub sp, sp, #(4*17)
    
    //
    // Save user-mode registers
    //
    stmia sp, {r0-r12}
    add r0, sp, #(4*13)
    stmia r0, {r13-r14}^
    mov r0, r0
    
    //
    // Save SPSR
    //
    mrs r0, spsr_all
    str r0, [sp, #-4]!
    
    //
    // Call the C handler
    //
    mov r0, sp
    bl KiSoftwareInterruptHandler
    
    //
    // Get the SPSR and restore it
    //
    ldr r0, [sp], #4
    msr spsr_all, r0
    
    //
    // Restore the registers
    //
    ldmia sp, {r0-r14}^
    mov r0, r0
    
    //
    // Advance in the trap frame
    //
    add sp, sp, #(4*17)
    
    //
    // Restore program execution state
    //
    ldr lr, [sp], #4
    movs pc, lr
    b .
    ENTRY_END KiSoftwareInterruptException

    NESTED_ENTRY KiPrefetchAbortException
    PROLOG_END KiPrefetchAbortException
    
    //
    // FIXME: TODO
    //
    b .
    
    ENTRY_END KiPrefetchAbortException

    NESTED_ENTRY KiDataAbortException
    PROLOG_END KiDataAbortException
    
    //
    // Fixup lr
    //
    sub lr, lr, #8
    
    //
    // Save the bottom 4 registers
    //
    stmdb sp, {r0-r3}
    
    //
    // Save the abort lr, sp, spsr, cpsr
    //
    mov r0, lr
    mov r1, sp
    mrs r2, cpsr
    mrs r3, spsr
    
    //
    // Switch to SVC mode
    //
    bic r2, r2, #CPSR_MODES
    orr r2, r2, #CPSR_SVC_MODE
    msr cpsr_c, r2
    
    //
    // Save the SVC sp before we modify it
    //
    mov r2, sp
    
    //
    // Save the abort lr
    //
    str r0, [sp, #-4]!
    
    //
    // Save the SVC lr and sp
    //
    str lr, [sp, #-4]!
    str r2, [sp, #-4]!
    
    //
    // Restore the saved SPSR
    //
    msr spsr_all, r3
    
    //
    // Restore our 4 registers
    //
    ldmdb r1, {r0-r3}
    
    //
    // Make space for the trap frame
    //
    sub sp, sp, #(4*15) // TrapFrameLength
    
    //
    // Save user-mode registers
    //
    stmia sp, {r0-r12}
    add r0, sp, #(4*13)
    stmia r0, {r13-r14}^
    mov r0, r0
    
    //
    // Save SPSR
    //
    mrs r0, spsr_all
    str r0, [sp, #-4]!

    //
    // Call the C handler
    //
    adr lr, AbortExit
    mov r0, sp
    ldr pc, =KiDataAbortHandler

AbortExit:

    //
    // Get the SPSR and restore it
    //
    ldr r0, [sp], #4
    msr spsr_all, r0
    
    //
    // Restore the registers
    //
    ldmia sp, {r0-r14}^
    mov r0, r0
    
    //
    // Advance in the trap frame
    //
    add sp, sp, #(4*15)
    
    //
    // Restore program execution state
    //
    ldmia sp, {sp, lr, pc}^
    b .
    ENTRY_END KiDataAbortException

    NESTED_ENTRY KiInterruptException
    PROLOG_END KiInterruptException
    
    //
    // FIXME: TODO
    //
    b .
    
    ENTRY_END KiInterruptException

    NESTED_ENTRY KiFastInterruptException
    PROLOG_END KiFastInterruptException
    
    //
    // FIXME: TODO
    //
    b .
    
    ENTRY_END KiFastInterruptException
    

    NESTED_ENTRY KiReservedException
    PROLOG_END KiReservedException
    
    //
    // FIXME: TODO
    //
    b .
    
    ENTRY_END KiReservedException


    NESTED_ENTRY KiSystemCall
    PROLOG_END KiSystemCall
    
    //
    // a1 has the function pointer, a2 has an array of arguments, a3 has the count
    // Save these to better locations
    //
    mov r4, a1
    mov r5, a2
    mov r6, a3
    
    //
    // Load up A1-A4 from the argument array
    // It doesn't matter if we had less than 4 arguments, we'll be loading some
    // of the registers with garbage, but nobody will know/care.
    //
    ldmia r5, {a1-a4}
    add r5, r5, #(4* 4)
    //sub r6, r6, #4
    
    //
    // Now copy the other arguments into our stack
    //
CopyLoop:
    cmp r6, #4
    //strne sp, [r5], #4
    //subne r6, r6, #1
    beq .

    //
    // Now do the system call
    //
    mov pc, r4
    
    //
    // Should not get here
    //
    b .
    ENTRY_END KiSystemCall
