/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/mp.c
 * PURPOSE:         Multiprocessor stubs
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

ULONG KeGetCurrentProcessorNumber(VOID)
/*
 * FUNCTION: Returns the system assigned number of the current processor
 */
{
   return(0);
}
