/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             block.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://ext2.yeah.net
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include "ext2fs.h"

/* GLOBALS ***************************************************************/

extern PEXT2_GLOBAL Ext2Global;

/* DEFINITIONS *************************************************************/

NTSTATUS
Ext2ReadWriteBlockSyncCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context    );

NTSTATUS
Ext2ReadWriteBlockAsyncCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context    );


NTSTATUS
Ext2MediaEjectControlCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt     );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, Ext2LockUserBuffer)
#pragma alloc_text(PAGE, Ext2GetUserBuffer)
#pragma alloc_text(PAGE, Ext2ReadSync)
#pragma alloc_text(PAGE, Ext2ReadDisk)
#pragma alloc_text(PAGE, Ext2DiskIoControl)
#pragma alloc_text(PAGE, Ext2ReadWriteBlocks)
#pragma alloc_text(PAGE, Ext2MediaEjectControl)
#pragma alloc_text(PAGE, Ext2DiskShutDown)
#endif


/* FUNCTIONS ***************************************************************/

PMDL
Ext2CreateMdl (
        IN PVOID Buffer,
        IN BOOLEAN bPaged,
        IN ULONG Length,
        IN LOCK_OPERATION Operation
        )
{
        NTSTATUS Status;
        PMDL Mdl = NULL;

        ASSERT (Buffer != NULL);
        Mdl = IoAllocateMdl (Buffer, Length, FALSE, FALSE, NULL);
        if (Mdl == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
        } else {
                _SEH2_TRY {
                        if (bPaged) {
                                MmProbeAndLockPages(Mdl, KernelMode, Operation);
                        } else {
                                MmBuildMdlForNonPagedPool (Mdl);
                        }
                        Status = STATUS_SUCCESS;
                } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
                        IoFreeMdl (Mdl);
                        Mdl = NULL;
                        DbgBreak();
                        Status = STATUS_INVALID_USER_BUFFER;
                } _SEH2_END;
        }
        return Mdl;
}


VOID
Ext2DestroyMdl (IN PMDL Mdl)
{
        ASSERT (Mdl != NULL);
        while (Mdl) {
                PMDL Next;
                Next = Mdl->Next;
                if (IsFlagOn(Mdl->MdlFlags, MDL_PAGES_LOCKED)) {
                        MmUnlockPages (Mdl);
                }
                IoFreeMdl (Mdl);
                Mdl = Next;
        }
}

NTSTATUS
Ext2LockUserBuffer (IN PIRP     Irp,
            IN ULONG            Length,
            IN LOCK_OPERATION   Operation)
{
    NTSTATUS Status;
    ASSERT(Irp != NULL);

    if (Irp->MdlAddress != NULL) {
        return STATUS_SUCCESS;
    }

    IoAllocateMdl(Irp->UserBuffer, Length, FALSE, FALSE, Irp);
    if (Irp->MdlAddress == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    _SEH2_TRY {

        MmProbeAndLockPages(Irp->MdlAddress, Irp->RequestorMode, Operation);
        Status = STATUS_SUCCESS;

    } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {

        DbgBreak();
        IoFreeMdl(Irp->MdlAddress);
        Irp->MdlAddress = NULL;
        Status = STATUS_INVALID_USER_BUFFER;
    } _SEH2_END;

    return Status;
}

PVOID
Ext2GetUserBuffer (IN PIRP Irp )
{
    ASSERT(Irp != NULL);
    
    if (Irp->MdlAddress) {

#if (_WIN32_WINNT >= 0x0500)
        return MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
#else
        return MmGetSystemAddressForMdl(Irp->MdlAddress);
#endif
    } else {

        return Irp->UserBuffer;
    }
}

NTSTATUS
Ext2ReadWriteBlockSyncCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context    )
{
    PEXT2_RW_CONTEXT pContext = (PEXT2_RW_CONTEXT)Context;

    if (!NT_SUCCESS(Irp->IoStatus.Status)) {
        pContext->MasterIrp->IoStatus = Irp->IoStatus;
    }

    IoFreeMdl(Irp->MdlAddress);
    IoFreeIrp(Irp );

    if (InterlockedDecrement(&pContext->Blocks) == 0) {

        pContext->MasterIrp->IoStatus.Information = 0;
        if (NT_SUCCESS(pContext->MasterIrp->IoStatus.Status)) {

            pContext->MasterIrp->IoStatus.Information =
                pContext->Length;
        }

        KeSetEvent(&pContext->Event, 0, FALSE);
    }

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
Ext2ReadWriteBlockAsyncCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PEXT2_RW_CONTEXT pContext = (PEXT2_RW_CONTEXT)Context;

    ASSERT(FALSE == pContext->Wait);

    if (!NT_SUCCESS(Irp->IoStatus.Status)) {
        pContext->MasterIrp->IoStatus = Irp->IoStatus;
    }

    if (InterlockedDecrement(&pContext->Blocks) == 0) {

        if (NT_SUCCESS(pContext->MasterIrp->IoStatus.Status)) {

            /* set written bytes to status information */
            pContext->MasterIrp->IoStatus.Information =
                pContext->Length;

            /* modify FileObject flags, skip this for volume direct access */
            if (pContext->FileObject != NULL &&
                !IsFlagOn(pContext->MasterIrp->Flags, IRP_PAGING_IO)) {
                SetFlag( pContext->FileObject->Flags,
                         IsFlagOn(pContext->Flags, EXT2_RW_CONTEXT_WRITE) ?
                         FO_FILE_MODIFIED : FO_FILE_FAST_IO_READ);
            }

        } else {
            pContext->MasterIrp->IoStatus.Information = 0;
        }

        /* release the PagingIo resource */
        if (pContext->Resource) {
            ExReleaseResourceForThread(pContext->Resource, pContext->ThreadId);
        }

        /* mark current Irp StackLocation as pending */
        IoMarkIrpPending(pContext->MasterIrp);

        ExFreePoolWithTag(pContext, EXT2_RWC_MAGIC);
        DEC_MEM_COUNT(PS_RW_CONTEXT, pContext, sizeof(EXT2_RW_CONTEXT));
    }

    return STATUS_SUCCESS;
}

NTSTATUS
Ext2ReadWriteBlocks(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN PEXT2_EXTENT         Chain,
    IN ULONG                Length,
    IN BOOLEAN              bVerify )
{
    PMDL                Mdl;
    PIRP                Irp;
    PIRP                MasterIrp = IrpContext->Irp;
    PIO_STACK_LOCATION  IrpSp;
    NTSTATUS            Status = STATUS_SUCCESS;
    PEXT2_RW_CONTEXT    pContext = NULL;
    BOOLEAN             bBugCheck = FALSE;
    PEXT2_EXTENT        Extent;

    ASSERT(MasterIrp);

    _SEH2_TRY {

        pContext = ExAllocatePoolWithTag(NonPagedPool, sizeof(EXT2_RW_CONTEXT), EXT2_RWC_MAGIC);

        if (!pContext) {
            DEBUG(DL_ERR, ( "Ex2ReadWriteBlocks: failed to allocate pContext.\n"));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            _SEH2_LEAVE;
        }

        INC_MEM_COUNT(PS_RW_CONTEXT, pContext, sizeof(EXT2_RW_CONTEXT));
        RtlZeroMemory(pContext, sizeof(EXT2_RW_CONTEXT));
        pContext->Wait = IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);
        pContext->MasterIrp = MasterIrp;
        pContext->Length = Length;

        if (IrpContext->MajorFunction == IRP_MJ_WRITE) {
            SetFlag(pContext->Flags, EXT2_RW_CONTEXT_WRITE);
        }

        if (pContext->Wait) {

            KeInitializeEvent(&(pContext->Event), NotificationEvent, FALSE);

        } else if (IrpContext->Fcb->Identifier.Type == EXT2FCB) {

            if (IsFlagOn(MasterIrp->Flags, IRP_PAGING_IO)) {
                pContext->Resource = &IrpContext->Fcb->PagingIoResource;
            } else {
                pContext->Resource = &IrpContext->Fcb->MainResource;
            }

            pContext->FileObject = IrpContext->FileObject;
            pContext->ThreadId = ExGetCurrentResourceThread();
        }

        for (Extent = Chain; Extent != NULL; Extent = Extent->Next) {

            Irp = IoMakeAssociatedIrp( 
                        MasterIrp,
                        (CCHAR)(Vcb->TargetDeviceObject->StackSize + 1) );

            if (!Irp) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                _SEH2_LEAVE;
            }

            Mdl = IoAllocateMdl( (PCHAR)MasterIrp->UserBuffer +
                    Extent->Offset,
                    Extent->Length,
                    FALSE,
                    FALSE,
                    Irp );

            if (!Mdl)  {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                _SEH2_LEAVE;
            }
            
            IoBuildPartialMdl( MasterIrp->MdlAddress,
                        Mdl,
                        (PCHAR)MasterIrp->UserBuffer +Extent->Offset,
                        Extent->Length );
                
            IoSetNextIrpStackLocation( Irp );
            IrpSp = IoGetCurrentIrpStackLocation( Irp );
            
            IrpSp->MajorFunction = IrpContext->MajorFunction;
            IrpSp->Parameters.Read.Length = Extent->Length;
            IrpSp->Parameters.Read.ByteOffset.QuadPart = Extent->Lba;

            IoSetCompletionRoutine(
                    Irp,
                    IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) ?
                    &Ext2ReadWriteBlockSyncCompletionRoutine :
                    &Ext2ReadWriteBlockAsyncCompletionRoutine,
                    (PVOID) pContext,
                    TRUE,
                    TRUE,
                    TRUE );

            IrpSp = IoGetNextIrpStackLocation( Irp );

            IrpSp->MajorFunction = IrpContext->MajorFunction;
            IrpSp->Parameters.Read.Length =Extent->Length;
            IrpSp->Parameters.Read.ByteOffset.QuadPart = Extent->Lba;

            if (bVerify) {
                SetFlag( IrpSp->Flags, SL_OVERRIDE_VERIFY_VOLUME );
            }

            Extent->Irp = Irp;
            pContext->Blocks += 1;
        }

        MasterIrp->AssociatedIrp.IrpCount = pContext->Blocks;

        if (IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)) {
            MasterIrp->AssociatedIrp.IrpCount += 1;
        }

        bBugCheck = TRUE;

        for (Extent = Chain; Extent != NULL; Extent = Extent->Next) {
            Status = IoCallDriver ( Vcb->TargetDeviceObject,
                                    Extent->Irp);
        }

        if (IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)) {
            KeWaitForSingleObject( &(pContext->Event),
                                   Executive, KernelMode, FALSE, NULL );

            KeClearEvent( &(pContext->Event) );
        }

    } _SEH2_FINALLY {

        if (_SEH2_AbnormalTermination()) {

            if (bBugCheck) {
                Ext2BugCheck(EXT2_BUGCHK_BLOCK, 0, 0, 0);
            }

            for (Extent = Chain; Extent != NULL; Extent = Extent->Next)  {
                if (Extent->Irp != NULL ) {
                    if (Extent->Irp->MdlAddress != NULL) {
                        IoFreeMdl(Extent->Irp->MdlAddress );
                    }
                    IoFreeIrp(Extent->Irp);
                }
            }

            if (pContext) {
                ExFreePoolWithTag(pContext, EXT2_RWC_MAGIC);
                DEC_MEM_COUNT(PS_RW_CONTEXT, pContext, sizeof(EXT2_RW_CONTEXT));
            }

        } else {

            if (IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)) {
                if (MasterIrp) {
                    Status = MasterIrp->IoStatus.Status;
                }
                if (pContext) {
                    ExFreePoolWithTag(pContext, EXT2_RWC_MAGIC);
                    DEC_MEM_COUNT(PS_RW_CONTEXT, pContext, sizeof(EXT2_RW_CONTEXT));
                }
            } else {
                IrpContext->Irp = NULL;
                Status = STATUS_PENDING;
            }
        }
    } _SEH2_END;

    return Status;
}

NTSTATUS
Ext2ReadSync(
    IN PEXT2_VCB        Vcb,
    IN ULONGLONG        Offset,
    IN ULONG            Length,
    OUT PVOID           Buffer,
    BOOLEAN             bVerify
    )
{
    PKEVENT         Event = NULL;

    PIRP            Irp;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS        Status = STATUS_INSUFFICIENT_RESOURCES;


    ASSERT(Vcb != NULL);
    ASSERT(Vcb->TargetDeviceObject != NULL);
    ASSERT(Buffer != NULL);

    _SEH2_TRY {
    
        Event = ExAllocatePoolWithTag(NonPagedPool, sizeof(KEVENT), 'EK2E');

        if (NULL == Event) {
            DEBUG(DL_ERR, ( "Ex2ReadSync: failed to allocate Event.\n"));
            _SEH2_LEAVE;
        }

        INC_MEM_COUNT(PS_DISK_EVENT, Event, sizeof(KEVENT));

        KeInitializeEvent(Event, NotificationEvent, FALSE);

        Irp = IoBuildSynchronousFsdRequest(
            IRP_MJ_READ,
            Vcb->TargetDeviceObject,
            Buffer,
            Length,
            (PLARGE_INTEGER)(&Offset),
            Event,
            &IoStatus
            );

        if (!Irp) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            _SEH2_LEAVE;
        }

        if (bVerify) {
            SetFlag( IoGetNextIrpStackLocation(Irp)->Flags, 
                     SL_OVERRIDE_VERIFY_VOLUME );
        }

        Status = IoCallDriver(Vcb->TargetDeviceObject, Irp);

        if (Status == STATUS_PENDING) {
            KeWaitForSingleObject(
                Event,
                Suspended,
                KernelMode,
                FALSE,
                NULL
                );

            Status = IoStatus.Status;
        }

    } _SEH2_FINALLY {

        if (Event) {
            ExFreePoolWithTag(Event, 'EK2E');
            DEC_MEM_COUNT(PS_DISK_EVENT, Event, sizeof(KEVENT));
        }
    } _SEH2_END;

    return Status;
}


NTSTATUS
Ext2ReadDisk(
         IN PEXT2_VCB   Vcb,
         IN ULONGLONG   Offset,
         IN ULONG       Size,
         IN PVOID       Buffer,
         IN BOOLEAN     bVerify )
{
    NTSTATUS    Status;
    PUCHAR      Buf;
    ULONG       Length;
    ULONGLONG   Lba;

    Lba = Offset & (~((ULONGLONG)SECTOR_SIZE - 1));
    Length = (ULONG)(Size + Offset + SECTOR_SIZE - 1 - Lba) &
             (~((ULONG)SECTOR_SIZE - 1));

    Buf = ExAllocatePoolWithTag(PagedPool, Length, EXT2_DATA_MAGIC);
    if (!Buf) {
        DEBUG(DL_ERR, ( "Ext2ReadDisk: failed to allocate Buffer.\n"));
        Status = STATUS_INSUFFICIENT_RESOURCES;

        goto errorout;
    }
    INC_MEM_COUNT(PS_DISK_BUFFER, Buf, Length);

    Status = Ext2ReadSync(  Vcb, 
                            Lba,
                            Length,
                            Buf,
                            FALSE );

    if (!NT_SUCCESS(Status)) {
        DEBUG(DL_ERR, ( "Ext2ReadDisk: disk device error.\n"));

        goto errorout;
    }

    RtlCopyMemory(Buffer, &Buf[Offset - Lba], Size);

errorout:

    if (Buf) {
        ExFreePoolWithTag(Buf, EXT2_DATA_MAGIC);
        DEC_MEM_COUNT(PS_DISK_BUFFER, Buf, Length);
    }

    return Status;
}


NTSTATUS 
Ext2DiskIoControl (
           IN PDEVICE_OBJECT   DeviceObject,
           IN ULONG            IoctlCode,
           IN PVOID            InputBuffer,
           IN ULONG            InputBufferSize,
           IN OUT PVOID        OutputBuffer,
           IN OUT PULONG       OutputBufferSize)
{
    ULONG           OutBufferSize = 0;
    KEVENT          Event;
    PIRP            Irp;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS        Status;
    
    ASSERT(DeviceObject != NULL);
    
    if (OutputBufferSize)
    {
        OutBufferSize = *OutputBufferSize;
    }
    
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    
    Irp = IoBuildDeviceIoControlRequest(
        IoctlCode,
        DeviceObject,
        InputBuffer,
        InputBufferSize,
        OutputBuffer,
        OutBufferSize,
        FALSE,
        &Event,
        &IoStatus
        );
    
    if (Irp == NULL) {
        DEBUG(DL_ERR, ( "Ext2DiskIoControl: failed to build Irp!\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    Status = IoCallDriver(DeviceObject, Irp);
    
    if (Status == STATUS_PENDING)  {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatus.Status;
    }
    
    if (OutputBufferSize) {
        *OutputBufferSize = (ULONG)(IoStatus.Information);
    }
    
    return Status;
}


NTSTATUS
Ext2MediaEjectControlCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    )
{
    PKEVENT Event = (PKEVENT)Contxt;

    KeSetEvent( Event, 0, FALSE );

    UNREFERENCED_PARAMETER( DeviceObject );

    return STATUS_SUCCESS;
}

VOID
Ext2MediaEjectControl (
    IN PEXT2_IRP_CONTEXT IrpContext,
    IN PEXT2_VCB Vcb,
    IN BOOLEAN bPrevent
    )
{
    PIRP                    Irp;
    KEVENT                  Event;
    NTSTATUS                Status;
    PREVENT_MEDIA_REMOVAL   Prevent;
    IO_STATUS_BLOCK         IoStatus;


    ExAcquireResourceExclusiveLite(
            &Vcb->MainResource,
            TRUE            );

    if (bPrevent != IsFlagOn(Vcb->Flags, VCB_REMOVAL_PREVENTED)) {
        if (bPrevent) {
            SetFlag(Vcb->Flags, VCB_REMOVAL_PREVENTED);
        } else {
            ClearFlag(Vcb->Flags, VCB_REMOVAL_PREVENTED);
        }
    }

    ExReleaseResourceLite(&Vcb->MainResource);

    Prevent.PreventMediaRemoval = bPrevent;

    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    Irp = IoBuildDeviceIoControlRequest( IOCTL_DISK_MEDIA_REMOVAL,
                                         Vcb->TargetDeviceObject,
                                         &Prevent,
                                         sizeof(PREVENT_MEDIA_REMOVAL),
                                         NULL,
                                         0,
                                         FALSE,
                                         NULL,
                                         &IoStatus );

    if (Irp != NULL) {
        IoSetCompletionRoutine( Irp,
                                Ext2MediaEjectControlCompletion,
                                &Event,
                                TRUE,
                                TRUE,
                                TRUE );

        Status = IoCallDriver(Vcb->TargetDeviceObject, Irp);

        if (Status == STATUS_PENDING) {
            Status = KeWaitForSingleObject( &Event,
                                            Executive,
                                            KernelMode,
                                            FALSE,
                                            NULL );
        }
    }
}


NTSTATUS
Ext2DiskShutDown(PEXT2_VCB Vcb)
{
    PIRP                Irp;
    KEVENT              Event;

    NTSTATUS            Status;
    IO_STATUS_BLOCK     IoStatus;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_SHUTDOWN,
                                       Vcb->TargetDeviceObject,
                                       NULL,
                                       0,
                                       NULL,
                                       &Event,
                                       &IoStatus);

    if (Irp) {
        Status = IoCallDriver(Vcb->TargetDeviceObject, Irp);

        if (Status == STATUS_PENDING) {
            KeWaitForSingleObject(&Event,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);

            Status = IoStatus.Status;
        }
    } else  {
        Status = IoStatus.Status;
    }

    return Status;
}

