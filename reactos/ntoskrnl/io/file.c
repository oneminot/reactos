/* $Id: file.c,v 1.13 2001/03/06 23:34:39 cnettel Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/file.c
 * PURPOSE:         Graceful system shutdown if a bug is detected
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/io.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL internalQueryFileInfo (
	IN	PFILE_OBJECT		FileObject,
	IN	FILE_INFORMATION_CLASS	FileInformationClass,
	PIO_STATUS_BLOCK	IoStatusBlock,
	IN	ULONG			Length,
	OUT	PVOID			FileInformation,
	OUT	PULONG			ReturnedLength	// FIXME: ReturnedLength not implemented
	)
{
   //This code is taken from old NtQueryInformationFile, now shared with IoQueryFileInformation
   PIRP Irp;
   PDEVICE_OBJECT DeviceObject;
   NTSTATUS Status;
   KEVENT Event;
   PIO_STACK_LOCATION StackPtr;


   KeInitializeEvent(&Event,NotificationEvent,FALSE);
   DeviceObject = FileObject->DeviceObject;
   
   Irp = IoAllocateIrp(DeviceObject->StackSize, TRUE);
   if (Irp==NULL)
     {
	ObDereferenceObject(FileObject);
	return STATUS_UNSUCCESSFUL;
     }

   Irp->UserIosb = IoStatusBlock;
   Irp->UserEvent = &Event;
   Irp->UserBuffer=FileInformation;
   
   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->MajorFunction = IRP_MJ_QUERY_INFORMATION;
   StackPtr->MinorFunction = 0;
   StackPtr->Flags = 0;
   StackPtr->Control = 0;
   StackPtr->DeviceObject = DeviceObject;
   StackPtr->FileObject = FileObject;
   
   StackPtr->Parameters.QueryFile.FileInformationClass = 
     FileInformationClass;
   StackPtr->Parameters.QueryFile.Length = Length;
   
   Status = IoCallDriver(FileObject->DeviceObject,Irp);
   if (Status==STATUS_PENDING && (FileObject->Flags & FO_SYNCHRONOUS_IO))
     {
        KeWaitForSingleObject(&Event,Executive,KernelMode,FALSE,NULL);
        Status = Irp->IoStatus.Status;
     }

   return Status;
}

NTSTATUS
STDCALL
NtQueryInformationFile (
	HANDLE			FileHandle,
	PIO_STATUS_BLOCK	IoStatusBlock,
	PVOID			FileInformation,
	ULONG			Length,
	FILE_INFORMATION_CLASS	FileInformationClass
	)
{
   PFILE_OBJECT FileObject;
   NTSTATUS Status;
   
   DPRINT("NtQueryInformationFile(Handle %x StatBlk %x FileInfo %x Length %d Class %d)\n",
          FileHandle,
          IoStatusBlock,
          FileInformation,
          Length,
          FileInformationClass);
   
   Status = ObReferenceObjectByHandle(FileHandle,
                                      FILE_READ_ATTRIBUTES,
                                      IoFileObjectType,
                                      UserMode,
				      (PVOID *)&FileObject,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   DPRINT("FileObject %x\n", FileObject);
   Status = internalQueryFileInfo(FileObject, FileInformationClass, IoStatusBlock, Length, FileInformation, NULL);

//   ObDereferenceObject(FileObject); FIXME: Why will a Dereference here make the OS impossible to boot. Incorrect deferring somewhere else?
   return(Status);
}



NTSTATUS 
STDCALL
IoQueryFileInformation (
	IN	PFILE_OBJECT		FileObject,
	IN	FILE_INFORMATION_CLASS	FileInformationClass,
	IN	ULONG			Length,
	OUT	PVOID			FileInformation,
	OUT	PULONG			ReturnedLength	// FIXME: ReturnedLength not implemented
	)
{
   NTSTATUS Status;

   Status = ObReferenceObjectByPointer(FileObject,
                                      FILE_READ_ATTRIBUTES,
                                      IoFileObjectType,
                                      KernelMode); // FIXME: Is KernelMode the correct choice here?
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }

//   ObDereferenceObject(FileObject); For some reason, ObDereference is NOT a good thing here... wish I knew why
   return internalQueryFileInfo(FileObject,FileInformationClass,NULL,Length,FileInformation,ReturnedLength);
}


NTSTATUS
STDCALL
NtSetInformationFile (
	HANDLE			FileHandle,
	PIO_STATUS_BLOCK	IoStatusBlock,
	PVOID			FileInformation,
	ULONG			Length,
	FILE_INFORMATION_CLASS	FileInformationClass
	)
{
	NTSTATUS		Status;
	PFILE_OBJECT		FileObject;
	PIRP			Irp;
	PIO_STACK_LOCATION	StackPtr;
	KEVENT			Event;
   
	DPRINT(
		"NtSetInformationFile(Handle %x StatBlk %x FileInfo %x Length %d Class %d)\n",
		FileHandle,
		IoStatusBlock,
		FileInformation,
		Length,
		FileInformationClass
		);
   
	/*  Get the file object from the file handle  */
	Status = ObReferenceObjectByHandle(
			FileHandle,
                        FILE_WRITE_ATTRIBUTES,
                        IoFileObjectType,
                        UserMode,
                        (PVOID *) & FileObject,
                        NULL
			);
	if (!NT_SUCCESS(Status))
	{
		return Status;
	}
	
	DPRINT("FileObject %x\n", FileObject);
   
	/*
	 * Initialize an event object to wait
	 * on for the request.
	 */
	KeInitializeEvent(
		& Event,
		NotificationEvent,
		FALSE
		);
	/*
	 * Build the IRP to be sent to the driver
	 * for the request.
	 */
	Irp = IoBuildSynchronousFsdRequest(
		IRP_MJ_SET_INFORMATION,
		FileObject->DeviceObject,
		FileInformation,
		Length,
		0,
		& Event,
		IoStatusBlock
		);

	StackPtr = IoGetNextIrpStackLocation(Irp);
	StackPtr->FileObject = FileObject;
	StackPtr->Parameters.SetFile.Length = Length;
	StackPtr->Parameters.SetFile.FileInformationClass =
		FileInformationClass;
   
	/*
	 * Pass the IRP to the FSD (and wait for
	 * it if required)
	 */
	DPRINT("FileObject->DeviceObject %x\n", FileObject->DeviceObject);
	Status = IoCallDriver(
			FileObject->DeviceObject,
			Irp
			);
	if (	(Status == STATUS_PENDING)
		&& (FileObject->Flags & FO_SYNCHRONOUS_IO)
		)
	{
		KeWaitForSingleObject(
			& Event,
			Executive,
			KernelMode,
			FALSE,
			NULL
			);
		Status = Irp->IoStatus.Status;
	}

	return Status;
}


NTSTATUS
STDCALL
NtQueryAttributesFile (
	IN	POBJECT_ATTRIBUTES	ObjectAttributes,
	IN	PVOID	Buffer
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtQueryFullAttributesFile (
	IN	HANDLE	FileHandle,
	IN	PVOID	Attributes
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtQueryEaFile (
	IN	HANDLE			FileHandle,
	OUT	PIO_STATUS_BLOCK	IoStatusBlock,
	OUT	PVOID			Buffer,
	IN	ULONG			Length,
	IN	BOOLEAN			ReturnSingleEntry,
	IN	PVOID			EaList			OPTIONAL,
	IN	ULONG			EaListLength,
	IN	PULONG			EaIndex			OPTIONAL,
	IN	BOOLEAN			RestartScan
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtSetEaFile (
	IN	HANDLE			FileHandle,
	IN	PIO_STATUS_BLOCK	IoStatusBlock,	
		PVOID			EaBuffer, 
		ULONG			EaBufferSize
		)
{
	UNIMPLEMENTED;
}


/* EOF */
