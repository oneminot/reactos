/* $Id: parttab.c,v 1.1 2000/03/26 19:38:18 ea Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/parttab.c (was ntoskrnl/io/fdisk.c)
 * PURPOSE:         Handling fixed disks (partition table functions)
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 * 2000-03-25 (ea)
 * 	Moved here from ntoskrnl/io/fdisk.c
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS
STDCALL
IoReadPartitionTable (
	PDEVICE_OBJECT				DeviceObject,
	ULONG					SectorSize,
	BOOLEAN					ReturnRecognizedPartitions,
	struct _DRIVE_LAYOUT_INFORMATION	** PBuffer
	)
{
	UNIMPLEMENTED;
}

NTSTATUS
STDCALL
IoSetPartitionInformation (
	PDEVICE_OBJECT	DeviceObject,
	ULONG		SectorSize,
	ULONG		PartitionNumber,
	ULONG		PartitionType)
{
	UNIMPLEMENTED;
}

NTSTATUS
STDCALL
IoWritePartitionTable (
	PDEVICE_OBJECT				DeviceObject,
	ULONG					SectorSize,
	ULONG					SectorsPerTrack,
	ULONG					NumberOfHeads,
	struct _DRIVE_LAYOUT_INFORMATION	* PBuffer
	)
{
	UNIMPLEMENTED;
}


/* EOF */
