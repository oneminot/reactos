/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/ke/exception.c
 * PURPOSE:         Platform independent exception handling
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ****************************************************************/

VOID
NTAPI
KiContinuePreviousModeUser(IN PCONTEXT Context,
                           IN PKEXCEPTION_FRAME ExceptionFrame,
                           IN PKTRAP_FRAME TrapFrame)
{
    CONTEXT LocalContext;

    /* We'll have to make a copy and probe it */
    ProbeForRead(Context, sizeof(CONTEXT), sizeof(ULONG));
    RtlMoveMemory(&LocalContext, Context, sizeof(CONTEXT));
    Context = &LocalContext;

    /* Convert the context into Exception/Trap Frames */
    KeContextToTrapFrame(&LocalContext, ExceptionFrame, TrapFrame, UserMode);
}

NTSTATUS
NTAPI
KiContinue(IN PCONTEXT Context,
           IN PKEXCEPTION_FRAME ExceptionFrame,
           IN PKTRAP_FRAME TrapFrame)
{
    NTSTATUS Status = STATUS_SUCCESS;
    KIRQL OldIrql = APC_LEVEL;
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();

    /* Raise to APC_LEVEL, only if needed */
    if (KeGetCurrentIrql() < APC_LEVEL)
    {
        /* Raise to APC_LEVEL */
        KeRaiseIrql(APC_LEVEL, &OldIrql);
    }

    /* Set up SEH to validate the context */
    _SEH_TRY
    {
        /* Check the previous mode */
        if (PreviousMode != KernelMode)
        {
            /* Validate from user-mode */
            KiContinuePreviousModeUser(Context,
                                       ExceptionFrame,
                                       TrapFrame);
        }
        else
        {
            /* Convert the context into Exception/Trap Frames */
            KeContextToTrapFrame(Context, ExceptionFrame, TrapFrame, KernelMode);
        }
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    /* Lower the IRQL if needed */
    if (OldIrql < APC_LEVEL) KeLowerIrql(OldIrql);

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
KiRaiseException(PEXCEPTION_RECORD ExceptionRecord,
                 PCONTEXT Context,
                 PKEXCEPTION_FRAME ExceptionFrame,
                 PKTRAP_FRAME TrapFrame,
                 BOOLEAN SearchFrames)
{
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    CONTEXT LocalContext;
    EXCEPTION_RECORD LocalExceptionRecord;
    ULONG ParameterCount, Size;
    NTSTATUS Status = STATUS_SUCCESS;
    DPRINT1("KiRaiseException\n");

    /* Set up SEH */
    _SEH_TRY
    {
        /* Check the previous mode */
        if (PreviousMode != KernelMode)
        {
#if 0
            /* Probe the context */
            ProbeForRead(Context, sizeof(CONTEXT), sizeof(ULONG));

            /* Probe the Exception Record */
            ProbeForRead(ExceptionRecord,
                         FIELD_OFFSET(EXCEPTION_RECORD, NumberParameters) +
                         sizeof(ULONG),
                         sizeof(ULONG));
#endif
            /* Validate the maximum parameters */
            if ((ParameterCount = ExceptionRecord->NumberParameters) >
                EXCEPTION_MAXIMUM_PARAMETERS)
            {
                /* Too large */
                Status = STATUS_INVALID_PARAMETER;
                _SEH_LEAVE;
            }

            /* Probe the entire parameters now*/
            Size = (sizeof(EXCEPTION_RECORD) - 
                    ((EXCEPTION_MAXIMUM_PARAMETERS - ParameterCount) * sizeof(ULONG)));
            ProbeForRead(ExceptionRecord, Size, sizeof(ULONG));

            /* Now make copies in the stack */
            RtlMoveMemory(&LocalContext, Context, sizeof(CONTEXT));
            RtlMoveMemory(&LocalExceptionRecord, ExceptionRecord, Size);
            Context = &LocalContext;
            ExceptionRecord = &LocalExceptionRecord;

            /* Update the parameter count */
            ExceptionRecord->NumberParameters = ParameterCount;
        }
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    if (NT_SUCCESS(Status))
    {
        /* Convert the context record */
        KeContextToTrapFrame(Context, ExceptionFrame, TrapFrame, PreviousMode);

        /* Dispatch the exception */
        KiDispatchException(ExceptionRecord,
                            ExceptionFrame,
                            TrapFrame,
                            PreviousMode,
                            SearchFrames);
    }

    /* Return the status */
    return Status;
}

/* EOF */
