/* $Id: spinlock.c,v 1.5 2003/11/05 22:37:42 gvg Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/spinlock.c
 * PURPOSE:         Implements spinlocks
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 *                  Eric Kohl (ekohl@rz-online.de)
 * UPDATE HISTORY:
 *                  09/06/2000 Created
 */

/*
 * NOTE: On a uniprocessor machine spinlocks are implemented by raising
 * the irq level
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS ***************************************************************/

VOID STDCALL
KeAcquireSpinLock (
	PKSPIN_LOCK	SpinLock,
	PKIRQL		OldIrql
	)
/*
 * FUNCTION: Acquires a spinlock
 * ARGUMENTS:
 *         SpinLock = Spinlock to acquire
 *         OldIrql (OUT) = Caller supplied storage for the previous irql
 */
{
   KeRaiseIrql(DISPATCH_LEVEL,OldIrql);
   KeAcquireSpinLockAtDpcLevel(SpinLock);
}

KIRQL FASTCALL
KeAcquireSpinLockRaiseToSynch (
	PKSPIN_LOCK	SpinLock
	)
{
  KIRQL OldIrql;
  KIRQL NewIrql;

#ifdef MP
  NewIrql = IPI_LEVEL - 1;
#else
  NewIrql = DISPATCH_LEVEL;
#endif

  OldIrql = KeGetCurrentIrql();
  if (OldIrql < NewIrql)
    {
      KeRaiseIrql(NewIrql, &OldIrql);
    }
  KeAcquireSpinLockAtDpcLevel(SpinLock);

  return OldIrql;
}

VOID STDCALL
KeReleaseSpinLock (
	PKSPIN_LOCK	SpinLock,
	KIRQL		NewIrql
	)
/*
 * FUNCTION: Releases a spinlock
 * ARGUMENTS:
 *        SpinLock = Spinlock to release
 *        NewIrql = Irql level before acquiring the spinlock
 */
{
   KeReleaseSpinLockFromDpcLevel(SpinLock);
   KeLowerIrql(NewIrql);
}

KIRQL FASTCALL
KfAcquireSpinLock (
	PKSPIN_LOCK	SpinLock
	)
{
   KIRQL OldIrql;

   KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
   KeAcquireSpinLockAtDpcLevel(SpinLock);

   return OldIrql;
}

VOID FASTCALL
KfReleaseSpinLock (
	PKSPIN_LOCK	SpinLock,
	KIRQL		NewIrql
	)
/*
 * FUNCTION: Releases a spinlock
 * ARGUMENTS:
 *        SpinLock = Spinlock to release
 *        NewIrql = Irql level before acquiring the spinlock
 */
{
   KeReleaseSpinLockFromDpcLevel(SpinLock);
   KeLowerIrql(NewIrql);
}

/* EOF */
