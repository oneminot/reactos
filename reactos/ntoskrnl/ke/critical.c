/* $Id: critical.c,v 1.10 2004/08/15 16:39:05 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/critical.c
 * PURPOSE:         Implement critical regions
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
VOID STDCALL KeEnterCriticalRegion (VOID)
{
   DPRINT("KeEnterCriticalRegion()\n");
   KeGetCurrentThread()->KernelApcDisable--;
}

/*
 * @implemented
 */
VOID STDCALL KeLeaveCriticalRegion (VOID)
{
  PKTHREAD Thread = KeGetCurrentThread(); 

  DPRINT("KeLeaveCriticalRegion()\n");

  /* Reference: http://www.ntfsd.org/archive/ntfsd0104/msg0203.html */
  if(++Thread->KernelApcDisable == 0) 
  { 
    if (!IsListEmpty(&Thread->ApcState.ApcListHead[KernelMode])) 
    { 
      Thread->ApcState.KernelApcPending = TRUE; 
      HalRequestSoftwareInterrupt(APC_LEVEL); 
    } 
  } 

}

/* EOF */
