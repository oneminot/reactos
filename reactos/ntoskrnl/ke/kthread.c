/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/kthread.c
 * PURPOSE:         Microkernel thread support
 * 
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID
KiServiceCheck (VOID)
{
  PETHREAD Thread;

  Thread = PsGetCurrentThread();

  if (Thread->Tcb.ServiceTable != KeServiceDescriptorTableShadow)
    {
      PsInitWin32Thread (Thread);

      Thread->Tcb.ServiceTable = KeServiceDescriptorTableShadow;
    }
}

/*
 * @unimplemented
 */
VOID
STDCALL
KeCapturePersistentThreadState(
	IN PVOID	CurrentThread,
	IN ULONG	Setting1,
	IN ULONG	Setting2,
	IN ULONG	Setting3,
	IN ULONG	Setting4,
	IN ULONG	Setting5,
	IN PVOID	ThreadState
)
{
	UNIMPLEMENTED;
}

VOID
KeFreeStackPage(PVOID Context, MEMORY_AREA* MemoryArea, PVOID Address, 
		PFN_TYPE Page, SWAPENTRY SwapEntry, BOOLEAN Dirty)
{
  ASSERT(SwapEntry == 0);
  if (Page != 0)
    {
      MmReleasePageMemoryConsumer(MC_NPPOOL, Page);
    }
}

/*
 * @implemented
 */
KPRIORITY
STDCALL
KeQueryPriorityThread (
    IN PKTHREAD Thread
    )
{
	return Thread->Priority;
}

NTSTATUS 
KeReleaseThread(PKTHREAD Thread)
/*
 * FUNCTION: Releases the resource allocated for a thread by
 * KeInitializeThread
 * NOTE: The thread had better not be running when this is called
 */
{
  extern unsigned int init_stack;

  /* FIXME - lock the process */
  RemoveEntryList(&Thread->ThreadListEntry);
  
  if (Thread->StackLimit != (ULONG_PTR)&init_stack)
    {       
      MmLockAddressSpace(MmGetKernelAddressSpace());
      MmFreeMemoryAreaByPtr(MmGetKernelAddressSpace(),
                            (PVOID)Thread->StackLimit,
                            KeFreeStackPage,
                            NULL);
      MmUnlockAddressSpace(MmGetKernelAddressSpace());
    }
  Thread->StackLimit = 0;
  Thread->InitialStack = NULL;
  Thread->StackBase = NULL;
  Thread->KernelStack = NULL;
  return(STATUS_SUCCESS);
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
KeSetKernelStackSwapEnable(
	IN BOOLEAN Enable
	)
{
	PKTHREAD Thread;
	BOOLEAN PreviousState;
	
	Thread = KeGetCurrentThread();
	
	/* Save Old State */
	PreviousState = Thread->EnableStackSwap;
	
	/* Set New State */
	Thread->EnableStackSwap = Enable;

	/* Return Old State */
	return PreviousState;
}

VOID
KeInitializeThread(PKPROCESS Process, PKTHREAD Thread, BOOLEAN First)
/*
 * FUNCTION: Initialize the microkernel state of the thread
 */
{
  PVOID KernelStack;
  NTSTATUS Status;
  extern unsigned int init_stack_top;
  extern unsigned int init_stack;
  PMEMORY_AREA StackArea;
  ULONG i;
  PHYSICAL_ADDRESS BoundaryAddressMultiple;
  
  BoundaryAddressMultiple.QuadPart = 0;
  
  KeInitializeDispatcherHeader(&Thread->DispatcherHeader,
			       InternalThreadType,
			       sizeof(ETHREAD),
			       FALSE);
  InitializeListHead(&Thread->MutantListHead);
  if (!First)
    {
      PFN_TYPE Page[MM_STACK_SIZE / PAGE_SIZE];
      KernelStack = NULL;
      
      MmLockAddressSpace(MmGetKernelAddressSpace());
      Status = MmCreateMemoryArea(NULL,
				  MmGetKernelAddressSpace(),
				  MEMORY_AREA_KERNEL_STACK,
				  &KernelStack,
				  MM_STACK_SIZE,
				  0,
				  &StackArea,
				  FALSE,
				  FALSE,
				  BoundaryAddressMultiple);
      MmUnlockAddressSpace(MmGetKernelAddressSpace());
      
      if (!NT_SUCCESS(Status))
	{
	  DPRINT1("Failed to create thread stack\n");
	  KEBUGCHECK(0);
	}
      for (i = 0; i < (MM_STACK_SIZE / PAGE_SIZE); i++)
	{
	  Status = MmRequestPageMemoryConsumer(MC_NPPOOL, TRUE, &Page[i]);
	  if (!NT_SUCCESS(Status))
	    {
	      KEBUGCHECK(0);
	    }
	}
      Status = MmCreateVirtualMapping(NULL,
				      KernelStack,
				      PAGE_READWRITE,
				      Page,
				      MM_STACK_SIZE / PAGE_SIZE);
      if (!NT_SUCCESS(Status))
        {
          KEBUGCHECK(0);
	}
      Thread->InitialStack = (PCHAR)KernelStack + MM_STACK_SIZE;
      Thread->StackBase    = (PCHAR)KernelStack + MM_STACK_SIZE;
      Thread->StackLimit   = (ULONG_PTR)KernelStack;
      Thread->KernelStack  = (PCHAR)KernelStack + MM_STACK_SIZE;
    }
  else
    {
      Thread->InitialStack = (PCHAR)&init_stack_top;
      Thread->StackBase = (PCHAR)&init_stack_top;
      Thread->StackLimit = (ULONG_PTR)&init_stack;
      Thread->KernelStack = (PCHAR)&init_stack_top;
    }

  /* 
   * Establish the pde's for the new stack and the thread structure within the 
   * address space of the new process. They are accessed while taskswitching or
   * while handling page faults. At this point it isn't possible to call the 
   * page fault handler for the missing pde's. 
   */
  
  MmUpdatePageDir((PEPROCESS)Process, (PVOID)Thread->StackLimit, MM_STACK_SIZE);
  MmUpdatePageDir((PEPROCESS)Process, (PVOID)Thread, sizeof(ETHREAD));

  /* 
   * The Native API function will initialize the TEB field later 
   */
  Thread->Teb = NULL;
  Thread->TlsArray = NULL;
  Thread->DebugActive = 0;
  Thread->State = THREAD_STATE_INITIALIZED;
  Thread->Alerted[0] = 0;
  Thread->Alerted[1] = 0;
  Thread->Iopl = 0;
  /*
   * FIXME: Think how this might work
   */
  Thread->NpxState = 0;
  
  Thread->Saturation = 0;
  Thread->Priority = Process->BasePriority; 
  InitializeListHead(&Thread->ApcState.ApcListHead[0]);
  InitializeListHead(&Thread->ApcState.ApcListHead[1]);
  Thread->ApcState.Process = Process;
  Thread->ApcState.KernelApcInProgress = 0;
  Thread->ApcState.KernelApcPending = 0;
  Thread->ApcState.UserApcPending = 0;
  Thread->ContextSwitches = 0;
  Thread->WaitStatus = STATUS_SUCCESS;
  Thread->WaitIrql = PASSIVE_LEVEL;
  Thread->WaitMode = 0;
  Thread->WaitNext = FALSE;
  Thread->WaitBlockList = NULL;
  Thread->WaitListEntry.Flink = NULL;
  Thread->WaitListEntry.Blink = NULL;
  Thread->WaitTime = 0;
  Thread->BasePriority = Process->BasePriority;
  Thread->DecrementCount = 0;
  Thread->PriorityDecrement = 0;
  Thread->Quantum = Process->ThreadQuantum;
  memset(Thread->WaitBlock, 0, sizeof(KWAIT_BLOCK)*4);
  Thread->LegoData = 0; 
  Thread->UserAffinity = Process->Affinity;
  Thread->SystemAffinityActive = 0;
  Thread->PowerState = 0;
  Thread->NpxIrql = 0;
  Thread->ServiceTable = KeServiceDescriptorTable;
  Thread->Queue = NULL;
  KeInitializeSpinLock(&Thread->ApcQueueLock);
  memset(&Thread->Timer, 0, sizeof(KTIMER));
  KeInitializeTimer(&Thread->Timer);
  Thread->QueueListEntry.Flink = NULL;
  Thread->QueueListEntry.Blink = NULL;
  Thread->Affinity = Process->Affinity;
  Thread->Preempted = 0;
  Thread->ProcessReadyQueue = 0;
  Thread->KernelStackResident = 1;
  Thread->NextProcessor = 0;
  Thread->CallbackStack = NULL;
  Thread->Win32Thread = NULL;
  Thread->TrapFrame = NULL;
  Thread->ApcStatePointer[OriginalApcEnvironment] = &Thread->ApcState;
  Thread->ApcStatePointer[AttachedApcEnvironment] = &Thread->SavedApcState;
  Thread->EnableStackSwap = 0;
  Thread->LargeStack = 0;
  Thread->ResourceIndex = 0;
  Thread->PreviousMode = KernelMode;
  Thread->KernelTime = 0;
  Thread->UserTime = 0;
  memset(&Thread->SavedApcState, 0, sizeof(KAPC_STATE));
   
  Thread->ApcStateIndex = OriginalApcEnvironment;
  Thread->ApcQueueable = TRUE;
  Thread->AutoAlignment = Process->AutoAlignment;
  
  KeInitializeApc(&Thread->SuspendApc,
		  Thread,
		  OriginalApcEnvironment,
		  PiSuspendThreadKernelRoutine,
		  PiSuspendThreadRundownRoutine,
		  PiSuspendThreadNormalRoutine,
		  KernelMode,
		  NULL);
  KeInitializeSemaphore(&Thread->SuspendSemaphore, 0, 128);
  
  InsertTailList(&Process->ThreadListHead,
                 &Thread->ThreadListEntry);
  Thread->FreezeCount = 0;
  Thread->SuspendCount = 0;
   
   /*
    * Do x86 specific part
    */
}

/*
 * @implemented
 */
VOID
STDCALL
KeRevertToUserAffinityThread(VOID)
{
#ifdef CONFIG_SMP
	PKTHREAD CurrentThread;
	KIRQL oldIrql;

        oldIrql = KeAcquireDispatcherDatabaseLock();

	CurrentThread = KeGetCurrentThread();

	ASSERT(CurrentThread->SystemAffinityActive != FALSE);

	/* Return to User Affinity */
	CurrentThread->Affinity = CurrentThread->UserAffinity;
	
	/* Disable System Affinity */
	CurrentThread->SystemAffinityActive = FALSE;

	if (CurrentThread->Affinity & (1 << KeGetCurrentProcessorNumber()))
	{
	   KeReleaseDispatcherDatabaseLock(oldIrql);
	}
	else
	{
	   CurrentThread->WaitIrql = oldIrql;
           PsDispatchThreadNoLock(THREAD_STATE_READY);
           KeLowerIrql(oldIrql);
	}
#endif
}

/*
 * @implemented
 */
CCHAR
STDCALL
KeSetIdealProcessorThread (
	IN PKTHREAD Thread,
	IN CCHAR Processor)
{
	CCHAR PreviousIdealProcessor;

	/* Save Old Ideal Processor */
	PreviousIdealProcessor = Thread->IdealProcessor;
	
	/* Set New Ideal Processor */
	Thread->IdealProcessor = Processor;
	
	/* Return Old Ideal Processor */
	return PreviousIdealProcessor;
}

/*
 * @implemented
 */
VOID
STDCALL
KeSetSystemAffinityThread(IN KAFFINITY Affinity)
{
#ifdef CONFIG_SMP
	PKTHREAD CurrentThread;
	KIRQL oldIrql;

        oldIrql = KeAcquireDispatcherDatabaseLock();

	CurrentThread = KeGetCurrentThread();

	ASSERT(Affinity & ((1 << KeNumberProcessors) - 1));
	
        /* Set the System Affinity Specified */
	CurrentThread->Affinity = Affinity;
	
	/* Enable System Affinity */
	CurrentThread->SystemAffinityActive = TRUE;

	if (Affinity & (1 << KeGetCurrentProcessorNumber()))
	{
	   KeReleaseDispatcherDatabaseLock(oldIrql);
	}
	else
	{
	   CurrentThread->WaitIrql = oldIrql;
           PsDispatchThreadNoLock(THREAD_STATE_READY);
           KeLowerIrql(oldIrql);
	}
#endif
}

/*
 * @implemented
 */
VOID
STDCALL
KeTerminateThread(IN KPRIORITY Increment)
{
	/* The Increment Argument seems to be ignored by NT and always 0 when called */
	
	/* Call our own internal routine */
	PsTerminateCurrentThread(0);
}
