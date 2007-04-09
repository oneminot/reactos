/* $Id$
 *
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/fs/notify.c
 * PURPOSE:         No purpose listed.
 *
 * PROGRAMMERS:     Gunnar Dalsnes
 */

/* INCLUDES *****************************************************************/
#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, FsRtlpInitNotifyImplementation)
#endif

/* GLOBALS *******************************************************************/

PAGED_LOOKASIDE_LIST    NotifyEntryLookaside;

typedef struct _NOTIFY_ENTRY
{
    LIST_ENTRY ListEntry;
    PSTRING FullDirectoryName;
    BOOLEAN WatchTree;
    BOOLEAN PendingChanges;
    ULONG CompletionFilter;
    LIST_ENTRY IrpQueue;
    PVOID Fcb;
    PCHECK_FOR_TRAVERSE_ACCESS TraverseCallback;
    PSECURITY_SUBJECT_CONTEXT SubjectContext;
    PVOID FsContext;
    BOOLEAN Unicode;
    BOOLEAN BufferExhausted;
    PVOID Buffer; /* Buffer == NULL equals IgnoreBuffer == TRUE */
    ULONG BufferSize;
    ULONG NextEntryOffset;
    PFILE_NOTIFY_INFORMATION PrevEntry;
} NOTIFY_ENTRY, *PNOTIFY_ENTRY;

/* FUNCTIONS *****************************************************************/

/*++
 * @name FsRtlpInitNotifyImplementation
 * @unimplemented
 *
 * FILLME
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
VOID
NTAPI INIT_FUNCTION
FsRtlpInitNotifyImplementation(VOID)
{
    ExInitializePagedLookasideList(&NotifyEntryLookaside,
                                   NULL,
                                   NULL,
                                   0,
                                   sizeof(NOTIFY_ENTRY),
                                   FSRTL_NOTIFY_TAG,
                                   0);
}

/*++
 * @name FsRtlpIsUnicodePath
 *
 * FILLME
 *
 * @param Path
 *        FILLME
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
static
__inline
BOOLEAN
FsRtlpIsUnicodePath(PSTRING Path)
{
    ASSERT(Path->Length);

    if (Path->Length == 1)
        return FALSE;

    if (*(WCHAR*)Path->Buffer == '\\')
        return TRUE;

    return FALSE;
}

/*++
 * @name FsRtlpNotifyCancelRoutine
 *
 * FILLME
 *
 * @param DeviceObject
 *        FILLME
 *
 * @param Irp
 *        FILLME
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
static
VOID
NTAPI
FsRtlpNotifyCancelRoutine(IN PDEVICE_OBJECT DeviceObject,
                          IN PIRP Irp)
{
   PFAST_MUTEX Lock;

   //don't need this since we have our own sync. protecting irp cancellation
   IoReleaseCancelSpinLock(Irp->CancelIrql);

   Lock = (PFAST_MUTEX)Irp->Tail.Overlay.DriverContext[3];

   ExAcquireFastMutex(Lock );

   RemoveEntryList(&Irp->Tail.Overlay.ListEntry);

   ExReleaseFastMutex(Lock);

   Irp->IoStatus.Status = STATUS_CANCELLED;
   Irp->IoStatus.Information = 0;

   IoCompleteRequest(Irp, IO_NO_INCREMENT);

}

/*++
 * @name FsRtlpFindNotifyEntry
 *
 * FILLME
 *
 * @param NotifyList
 *        FILLME
 *
 * @param FsContext
 *        FILLME
 *
 * @return FILLME
 *
 * @remarks None
 *
 *--*/
static
PNOTIFY_ENTRY
FASTCALL
FsRtlpFindNotifyEntry(PLIST_ENTRY NotifyList,
                      PVOID FsContext)
{
    PNOTIFY_ENTRY NotifyEntry;

    LIST_FOR_EACH(NotifyEntry, NotifyList, NOTIFY_ENTRY, ListEntry)
    {
        if (NotifyEntry->FsContext == FsContext)
        {
            return NotifyEntry;
        }
    }

    return NULL;
}

/*++
 * @name FsRtlNotifyChangeDirectory
 * @implemented
 *
 * FILLME
 *
 * @param NotifySync
 *        FILLME
 *
 * @param FsContext
 *        FILLME
 *
 * @param FullDirectoryName
 *        FILLME
 *
 * @param NotifyList
 *        FILLME
 *
 * @param WatchTree
 *        FILLME
 *
 * @param CompletionFilter
 *        FILLME
 *
 * @param NotifyIrp
 *        FILLME
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
VOID
NTAPI
FsRtlNotifyChangeDirectory(IN PNOTIFY_SYNC NotifySync,
                           IN PVOID        FsContext,
                           IN PSTRING      FullDirectoryName,
                           IN PLIST_ENTRY  NotifyList,
                           IN BOOLEAN      WatchTree,
                           IN ULONG        CompletionFilter,
                           IN PIRP         NotifyIrp)
{
    FsRtlNotifyFullChangeDirectory(NotifySync,
                                   NotifyList,
                                   FsContext,
                                   FullDirectoryName,
                                   WatchTree,
                                   TRUE, /* IgnoreBuffer */
                                   CompletionFilter,
                                   NotifyIrp,
                                   NULL,
                                   NULL);
}

/*++
 * @name FsRtlNotifyCleanup
 * @unimplemented
 *
 * Called by FSD when all handles to FileObject (identified by FsContext) are closed
 *
 * @param NotifySync
 *        FILLME
 *
 * @param NotifyList
 *        FILLME
 *
 * @param FsContext
 *        FILLME
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
VOID
NTAPI
FsRtlNotifyCleanup(IN PNOTIFY_SYNC NotifySync,
                   IN PLIST_ENTRY NotifyList,
                   IN PVOID FsContext)
{
    PNOTIFY_ENTRY NotifyEntry;
    LIST_ENTRY CompletedListHead;
    PLIST_ENTRY TmpEntry;
    PIRP Irp;

    InitializeListHead(&CompletedListHead);

    ExAcquireFastMutex((PFAST_MUTEX)NotifySync);

    NotifyEntry = FsRtlpFindNotifyEntry(NotifyList, FsContext);

    if (NotifyEntry)
    {
        /* free buffered changes */
        if (NotifyEntry->Buffer)
        {
            ExFreePool(NotifyEntry->Buffer);
        }

        /* cancel(?) pending irps */
        while (!IsListEmpty(&NotifyEntry->IrpQueue))
        {
            TmpEntry = RemoveHeadList(&NotifyEntry->IrpQueue);
            Irp = CONTAINING_RECORD(TmpEntry , IRP, Tail.Overlay.ListEntry);

            /* irp cancelation bolilerplate */
            if (!IoSetCancelRoutine(Irp, NULL))
            {
                //The cancel routine will be called. When we release the lock it will complete the irp.
                InitializeListHead(&Irp->Tail.Overlay.ListEntry);
                continue;
            }

            Irp->IoStatus.Status = STATUS_NOTIFY_CLEANUP; /* FIXME: correct status? */
            Irp->IoStatus.Information = 0;

            /* avoid holding lock while completing irp */
            InsertTailList(&CompletedListHead, &Irp->Tail.Overlay.ListEntry);
        }

        /* Unlink and free the NotifyStruct */
        RemoveEntryList(&NotifyEntry->ListEntry);
        ExFreeToPagedLookasideList(&NotifyEntryLookaside, NotifyEntry);
    }

    ExReleaseFastMutex((PFAST_MUTEX)NotifySync);

    /* complete defered irps */
    while (!IsListEmpty(&CompletedListHead))
    {
        TmpEntry = RemoveHeadList(&CompletedListHead);
        Irp = CONTAINING_RECORD(TmpEntry , IRP, Tail.Overlay.ListEntry);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
}

/*++
 * @name FsRtlNotifyFilterChangeDirectory
 * @unimplemented
 *
 * FILLME
 *
 * @param NotifySync
 *        FILLME
 *
 * @param NotifyList
 *        FILLME
 *
 * @param FsContext
 *        FILLME
 *
 * @param FullDirectoryName
 *        FILLME
 *
 * @param WatchTree
 *        FILLME
 *
 * @param IgnoreBuffer
 *        FILLME
 *
 * @param CompletionFilter
 *        FILLME
 *
 * @param NotifyIrp
 *        FILLME
 *
 * @param TraverseCallback
 *        FILLME
 *
 * @param SubjectContext
 *        FILLME
 *
 * @param FilterCallback
 *        FILLME
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
VOID
NTAPI
FsRtlNotifyFilterChangeDirectory(IN PNOTIFY_SYNC NotifySync,
                                 IN PLIST_ENTRY NotifyList,
                                 IN PVOID FsContext,
                                 IN PSTRING FullDirectoryName,
                                 IN BOOLEAN WatchTree,
                                 IN BOOLEAN IgnoreBuffer,
                                 IN ULONG CompletionFilter,
                                 IN PIRP NotifyIrp,
                                 IN PCHECK_FOR_TRAVERSE_ACCESS TraverseCallback OPTIONAL,
                                 IN PSECURITY_SUBJECT_CONTEXT SubjectContext OPTIONAL,
                                 IN PFILTER_REPORT_CHANGE FilterCallback OPTIONAL)
{
    UNIMPLEMENTED;
}

/*++
 * @name FsRtlNotifyFilterReportChange
 * @unimplemented
 *
 * FILLME
 *
 * @param NotifySync
 *        FILLME
 *
 * @param NotifyList
 *        FILLME
 *
 * @param FullTargetName
 *        FILLME
 *
 * @param TargetNameOffset
 *        FILLME
 *
 * @param StreamName
 *        FILLME
 *
 * @param NormalizedParentName
 *        FILLME
 *
 * @param FilterMatch
 *        FILLME
 *
 * @param Action
 *        FILLME
 *
 * @param TargetContext
 *        FILLME
 *
 * @param FilterContext
 *        FILLME
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
VOID
NTAPI
FsRtlNotifyFilterReportChange(IN PNOTIFY_SYNC NotifySync,
                              IN PLIST_ENTRY NotifyList,
                              IN PSTRING FullTargetName,
                              IN USHORT TargetNameOffset,
                              IN PSTRING StreamName OPTIONAL,
                              IN PSTRING NormalizedParentName OPTIONAL,
                              IN ULONG FilterMatch,
                              IN ULONG Action,
                              IN PVOID TargetContext,
                              IN PVOID FilterContext)
{
    UNIMPLEMENTED;
}

/*++
 * @name FsRtlpWatchedDirectoryWasDeleted
 *
 * FILLME
 *
 * @param NotifySync
 *        FILLME
 *
 * @param NotifyList
 *        FILLME
 *
 * @param Fcb
 *        FILLME
 *
 * @return None
 *
 * @remarks Private function
 *
 *--*/
static
VOID
FASTCALL
FsRtlpWatchedDirectoryWasDeleted(IN PNOTIFY_SYNC    NotifySync,
                                 IN PLIST_ENTRY     NotifyList,
                                 IN PVOID           Fcb)
{
    LIST_ENTRY CompletedListHead;
    PLIST_ENTRY TmpEntry;
    PNOTIFY_ENTRY NotifyEntry, tmp;
    PIRP Irp;

    InitializeListHead(&CompletedListHead);

    ExAcquireFastMutex((PFAST_MUTEX)NotifySync);

    LIST_FOR_EACH_SAFE(NotifyEntry, tmp, NotifyList, NOTIFY_ENTRY, ListEntry )
    {
        if (NotifyEntry->Fcb == Fcb)
        {
            RemoveEntryList(&NotifyEntry->ListEntry);

            /* free buffered changes */
            if (NotifyEntry->Buffer)
            {
                ExFreePool(NotifyEntry->Buffer);
            }

            /* cleanup pending irps */
            while (!IsListEmpty(&NotifyEntry->IrpQueue))
            {
                TmpEntry = RemoveHeadList(&NotifyEntry->IrpQueue);
                Irp = CONTAINING_RECORD(TmpEntry , IRP, Tail.Overlay.ListEntry);

                /* irp cancelation bolilerplate */
                if (!IoSetCancelRoutine(Irp, NULL))
                {
                    //The cancel routine will be called. When we release the lock it will complete the irp.
                    InitializeListHead(&Irp->Tail.Overlay.ListEntry);
                    continue;
                }

                Irp->IoStatus.Status = STATUS_DELETE_PENDING;
                Irp->IoStatus.Information = 0;

                /* avoid holding lock while completing irp */
                InsertTailList(&CompletedListHead, &Irp->Tail.Overlay.ListEntry);
            }
        }
    }

    ExReleaseFastMutex((PFAST_MUTEX)NotifySync);

    /* complete defered irps */
    while (!IsListEmpty(&CompletedListHead))
    {
        TmpEntry = RemoveHeadList(&CompletedListHead);
        Irp = CONTAINING_RECORD(TmpEntry , IRP, Tail.Overlay.ListEntry);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
}

/*++
 * @name FsRtlNotifyFullChangeDirectory
 * @unimplemented
 *
 * FILLME
 *
 * @param NotifySync
 *        FILLME
 *
 * @param NotifyList
 *        FILLME
 *
 * @param FsContext
 *        FILLME
 *
 * @param FullDirectoryName
 *        FILLME
 *
 * @param WatchTree
 *        FILLME
 *
 * @param IgnoreBuffer
 *        FILLME
 *
 * @param CompletionFilter
 *        FILLME
 *
 * @param Irp
 *        FILLME
 *
 * @param TraverseCallback
 *        FILLME
 *
 * @param SubjectContext
 *        FILLME
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
VOID
NTAPI
FsRtlNotifyFullChangeDirectory(IN PNOTIFY_SYNC  NotifySync,
                               IN PLIST_ENTRY   NotifyList,
                               IN PVOID         FsContext,
                               IN PSTRING       FullDirectoryName,
                               IN BOOLEAN       WatchTree,
                               IN BOOLEAN       IgnoreBuffer,
                               IN ULONG         CompletionFilter,
                               IN PIRP          Irp,
                               IN PCHECK_FOR_TRAVERSE_ACCESS TraverseCallback OPTIONAL,
                               IN PSECURITY_SUBJECT_CONTEXT SubjectContext OPTIONAL)
{
    PIO_STACK_LOCATION IrpStack;
    PNOTIFY_ENTRY NotifyEntry;
    ULONG IrpBuffLen;

    if (!Irp)
    {
        /* all other params are ignored if NotifyIrp == NULL */
        FsRtlpWatchedDirectoryWasDeleted(NotifySync, NotifyList, FsContext);
        return;
    }

    DPRINT("FullDirectoryName: %wZ\n", FullDirectoryName);

    ExAcquireFastMutex((PFAST_MUTEX)NotifySync);

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    if (IrpStack->FileObject->Flags & FO_CLEANUP_COMPLETE)
    {
        ExReleaseFastMutex((PFAST_MUTEX)NotifySync);

        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_NOTIFY_CLEANUP;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return;
    }

    IrpBuffLen = IrpStack->Parameters.NotifyDirectory.Length;

    NotifyEntry = FsRtlpFindNotifyEntry(NotifyList, FsContext);

    if (!NotifyEntry)
    {
        /* No NotifyStruct for this FileObject existed */

        /* The first request for this FileObject set the standards.
        * For subsequent requests, these params will be ignored.
        * Ref: Windows NT File System Internals page 516
        */

        NotifyEntry = ExAllocateFromPagedLookasideList(&NotifyEntryLookaside);

        RtlZeroMemory(NotifyEntry, sizeof(NOTIFY_ENTRY));

        NotifyEntry->FsContext = FsContext;
        NotifyEntry->FullDirectoryName = FullDirectoryName;
        NotifyEntry->WatchTree = WatchTree;
        NotifyEntry->CompletionFilter = CompletionFilter;
        NotifyEntry->TraverseCallback = TraverseCallback;
        NotifyEntry->SubjectContext = SubjectContext;
        NotifyEntry->Fcb = IrpStack->FileObject->FsContext;
        NotifyEntry->Unicode = FsRtlpIsUnicodePath(FullDirectoryName);

        /* Init. buffer */
        if (IrpBuffLen && !IgnoreBuffer)
        {
            _SEH_TRY
            {
                NotifyEntry->Buffer = ExAllocatePoolWithQuotaTag(
                    PagedPool,
                    IrpBuffLen,
                    FSRTL_NOTIFY_TAG
                    );

                NotifyEntry->BufferSize = IrpBuffLen;
            }
            _SEH_HANDLE
            {
                /* ExAllocatePoolWithQuotaTag raised exception but we dont care.
                The impl. doesnt require a buffer, so well continue as usual.
                */
            }
            _SEH_END;
        }

        InitializeListHead(&NotifyEntry->IrpQueue);

        InsertTailList(NotifyList, &NotifyEntry->ListEntry);
    }


    if (!NotifyEntry->PendingChanges)
    {
        /* No changes are pending. Queue the irp */

        /* Irp cancelation boilerplate */

        /* save NotifySych for use in the cancel routine */
        Irp->Tail.Overlay.DriverContext[3] = NotifySync;

        (void)IoSetCancelRoutine(Irp, FsRtlpNotifyCancelRoutine);
        if (Irp->Cancel && IoSetCancelRoutine(Irp, NULL))
        {
            //irp was canceled
            ExReleaseFastMutex((PFAST_MUTEX)NotifySync);

            Irp->IoStatus.Status = STATUS_CANCELLED;
            Irp->IoStatus.Information = 0;

            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return;
        }

        IoMarkIrpPending(Irp);

        //FIXME: any point in setting irp status/information before queueing?
        Irp->IoStatus.Status = STATUS_PENDING;

        InsertTailList(&NotifyEntry->IrpQueue, &Irp->Tail.Overlay.ListEntry);

        ExReleaseFastMutex((PFAST_MUTEX)NotifySync);
        return;
    }


    /* Pending changes exist */

    if (NotifyEntry->Buffer == NULL ||
        NotifyEntry->BufferExhausted ||
        IrpBuffLen < NotifyEntry->NextEntryOffset)
    {
        /*
        Can't return detailed changes to user cause:
        -No buffer exist, OR
        -Buffer were overflowed, OR
        -Current irp buff was not large enough
        */

        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_NOTIFY_ENUM_DIR;

    }
    else
    {
        PVOID Adr = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, LowPagePriority);

        if (Adr)
        {
            memcpy(Adr, NotifyEntry->Buffer, NotifyEntry->NextEntryOffset);
            Irp->IoStatus.Information = NotifyEntry->NextEntryOffset;
        }
        else
        {
            Irp->IoStatus.Information = 0;
        }

        Irp->IoStatus.Status = STATUS_SUCCESS;
    }

    /* reset buffer */
    NotifyEntry->PrevEntry = NULL;
    NotifyEntry->NextEntryOffset = 0;
    NotifyEntry->BufferExhausted = FALSE;

    NotifyEntry->PendingChanges = FALSE;

    ExReleaseFastMutex((PFAST_MUTEX)NotifySync);

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    /* caller must return STATUS_PENDING */
}

/*++
 * @name FsRtlpGetNextIrp
 *
 * FILLME
 *
 * @param NotifyEntry
 *        FILLME
 *
 * @return FILLME
 *
 * @remarks Private function
 *
 *--*/
static
PIRP
FASTCALL
FsRtlpGetNextIrp(PNOTIFY_ENTRY NotifyEntry)
{
    PIRP Irp;
    PLIST_ENTRY TmpEntry;

    /* Loop to get a non-canceled irp */
    while (!IsListEmpty(&NotifyEntry->IrpQueue))
    {
        /* If we have queued irp(s) we can't possibly have pending changes too */
        ASSERT(!NotifyEntry->PendingChanges);

        TmpEntry = RemoveHeadList(&NotifyEntry->IrpQueue);
        Irp = CONTAINING_RECORD(TmpEntry , IRP, Tail.Overlay.ListEntry);

        /* irp cancelation bolilerplate */
        if (!IoSetCancelRoutine(Irp, NULL))
        {
            //The cancel routine will be called. When we release the lock it will complete the irp.
            InitializeListHead(&Irp->Tail.Overlay.ListEntry);
            continue;
        }

        /* Finally we got a non-canceled irp */
        return Irp;
    }

    return NULL;
}

/*++
 * @name FsRtlpCopyName
 *
 * FILLME
 *
 * @param CurrentEntry
 *        FILLME
 *
 * @param Unicode
 *        FILLME
 *
 * @param RelativeName
 *        FILLME
 *
 * @param StreamName
 *        FILLME
 *
 * @return None
 *
 * @remarks Private function
 *
 *--*/
static
__inline
VOID
FsRtlpCopyName(PFILE_NOTIFY_INFORMATION CurrentEntry,
               BOOLEAN Unicode,
               PSTRING RelativeName,
               PSTRING StreamName)
{
    /* Buffer size is allready probed, so just copy the data */

    if (Unicode)
    {
        memcpy(CurrentEntry->FileName, RelativeName->Buffer, RelativeName->Length);
        if (StreamName)
        {
            CurrentEntry->FileName[RelativeName->Length/sizeof(WCHAR)] = ':';
            memcpy(&CurrentEntry->FileName[(RelativeName->Length/sizeof(WCHAR))+1],
                StreamName->Buffer,
                StreamName->Length);
        }
    }
    else
    {
        //FIXME: convert to unicode etc.
        DPRINT1("FIXME: ansi strings in notify impl. not supported yet\n");
    }
}

/*++
 * @name FsRtlNotifyFullReportChange
 * @unimplemented
 *
 * FILLME
 *
 * @param NotifySync
 *        FILLME
 *
 * @param NotifyList
 *        FILLME
 *
 * @param FullTargetName
 *        FILLME
 *
 * @param TargetNameOffset
 *        FILLME
 *
 * @param StreamName
 *        FILLME
 *
 * @param NormalizedParentName
 *        FILLME
 *
 * @param FilterMatch
 *        FILLME
 *
 * @param Action
 *        FILLME
 *
 * @param TargetContext
 *        FILLME
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
VOID
NTAPI
FsRtlNotifyFullReportChange(IN PNOTIFY_SYNC NotifySync,
                            IN PLIST_ENTRY  NotifyList,
                            IN PSTRING      FullTargetName, /* can include short names! */
                            IN USHORT       TargetNameOffset, /* in bytes */
                            IN PSTRING      StreamName           OPTIONAL,
                            IN PSTRING      NormalizedParentName OPTIONAL, /* same as FullTargetName, but with long names */
                            IN ULONG        FilterMatch,
                            IN ULONG        Action,
                            IN PVOID        TargetContext)
{
    USHORT FullDirLen;
    STRING RelativeName;
    PNOTIFY_ENTRY NotifyEntry, tmp;
    PLIST_ENTRY EnumEntry;
    PIRP Irp;
    LIST_ENTRY CompletedListHead;
    USHORT NameLenU;
    ULONG RecordLen;
    PFILE_NOTIFY_INFORMATION CurrentEntry;

    InitializeListHead(&CompletedListHead);

    DPRINT("FullTargetName: %wZ\n", FullTargetName);

    /*
    I think FullTargetName can include/be a short file name! What the heck do i do with this?
    Dont think this apply to FsRtlNotifyFullChangeDirectory's FullDirectoryName.
    */

    ExAcquireFastMutex((PFAST_MUTEX)NotifySync);

    LIST_FOR_EACH_SAFE(NotifyEntry, tmp, NotifyList, NOTIFY_ENTRY, ListEntry )
    {
        ASSERT(NotifyEntry->Unicode == FsRtlpIsUnicodePath(FullTargetName));

        /* rule out some easy cases */
        /* FIXME: short vs. long names??? lower case/upper case/mixed case? */
        if (!(FilterMatch & NotifyEntry->CompletionFilter)) continue;

        FullDirLen = TargetNameOffset - (NotifyEntry->Unicode ? sizeof(WCHAR) : sizeof(char));
        if (FullDirLen == 0)
        {
            /* special case for root dir */
            FullDirLen = (NotifyEntry->Unicode ? sizeof(WCHAR) : sizeof(char));
        }

        if (FullDirLen < NotifyEntry->FullDirectoryName->Length) continue;

        if (!NotifyEntry->WatchTree && FullDirLen != NotifyEntry->FullDirectoryName->Length) continue;

        DPRINT("NotifyEntry->FullDirectoryName: %wZ\n", NotifyEntry->FullDirectoryName);

        /* FIXME: short vs. long names??? lower case/upper case/mixed case? */
        if (memcmp(NotifyEntry->FullDirectoryName->Buffer,
            FullTargetName->Buffer,
            NotifyEntry->FullDirectoryName->Length) != 0) continue;


        if (NotifyEntry->WatchTree &&
            NotifyEntry->TraverseCallback &&
            FullDirLen != NotifyEntry->FullDirectoryName->Length)
        {
            /* change happend in a subdir. ask caller if we are allowed in here */
            NTSTATUS Status = NotifyEntry->TraverseCallback(NotifyEntry->FsContext,
                TargetContext,
                NotifyEntry->SubjectContext);

            if (!NT_SUCCESS(Status)) continue;

            /*
            FIXME: notify-dir impl. should release and free the SubjectContext
            */
        }

        DPRINT("Found match\n");

        /* Found a valid change */

        RelativeName.Buffer = FullTargetName->Buffer + TargetNameOffset;
        RelativeName.MaximumLength =
            RelativeName.Length =
            FullTargetName->Length - TargetNameOffset;

        DPRINT("RelativeName: %wZ\n",&RelativeName);

        /* calculate unicode bytes of relative-name + stream-name */
        if (NotifyEntry->Unicode)
        {
            NameLenU = RelativeName.Length + (StreamName ? (StreamName->Length + sizeof(WCHAR)) : 0);
        }
        else
        {
            NameLenU = RelativeName.Length * sizeof(WCHAR) +
                (StreamName ? ((StreamName->Length * sizeof(WCHAR)) + sizeof(WCHAR)) : 0);
        }

        RecordLen = FIELD_OFFSET(FILE_NOTIFY_INFORMATION, FileName) + NameLenU;

        if ((Irp = FsRtlpGetNextIrp(NotifyEntry)))
        {
            PIO_STACK_LOCATION IrpStack;
            ULONG IrpBuffLen;

            IrpStack = IoGetCurrentIrpStackLocation(Irp);
            IrpBuffLen = IrpStack->Parameters.NotifyDirectory.Length;

            DPRINT("Got pending irp\n");

            ASSERT(!NotifyEntry->PendingChanges);

            if (NotifyEntry->Buffer == NULL || /* aka. IgnoreBuffer */
                RecordLen > IrpBuffLen)
            {
                /* ignore buffer / buffer not large enough */
                Irp->IoStatus.Status = STATUS_NOTIFY_ENUM_DIR;
                Irp->IoStatus.Information = 0;
            }
            else
            {
                CurrentEntry = (PFILE_NOTIFY_INFORMATION)
                    MmGetSystemAddressForMdlSafe(Irp->MdlAddress, LowPagePriority);

                if (CurrentEntry)
                {
                    CurrentEntry->Action = Action;
                    CurrentEntry->FileNameLength = NameLenU;
                    CurrentEntry->NextEntryOffset = 0;

                    FsRtlpCopyName(
                        CurrentEntry,
                        NotifyEntry->Unicode,
                        &RelativeName,
                        StreamName
                        );

                    Irp->IoStatus.Information = RecordLen;
                }
                else
                {
                    Irp->IoStatus.Information = 0;
                }


                Irp->IoStatus.Status = STATUS_SUCCESS;
            }

            /* avoid holding lock while completing irp */
            InsertTailList(&CompletedListHead, &Irp->Tail.Overlay.ListEntry);
        }
        else
        {
            DPRINT("No irp\n");

            NotifyEntry->PendingChanges = TRUE;

            if (NotifyEntry->Buffer == NULL || NotifyEntry->BufferExhausted) continue;

            if (RecordLen > NotifyEntry->BufferSize - NotifyEntry->NextEntryOffset)
            {
                /* overflow. drop these changes and stop buffering any other changes too */
                NotifyEntry->BufferExhausted = TRUE;
                continue;
            }

            /* The buffer has enough room for the changes.
            * Copy data to buffer.
            */

            CurrentEntry = (PFILE_NOTIFY_INFORMATION)NotifyEntry->Buffer;

            CurrentEntry->Action = Action;
            CurrentEntry->FileNameLength = NameLenU;
            CurrentEntry->NextEntryOffset = 0;

            FsRtlpCopyName(CurrentEntry,
                NotifyEntry->Unicode,
                &RelativeName,
                StreamName
                );

            if (NotifyEntry->PrevEntry)
            {
                NotifyEntry->PrevEntry->NextEntryOffset = (char*)CurrentEntry - (char*)NotifyEntry->PrevEntry;
            }
            NotifyEntry->PrevEntry = CurrentEntry;
            NotifyEntry->NextEntryOffset += RecordLen;

            //         {
            //            UNICODE_STRING TmpStr;
            //            TmpStr.Buffer = BufferedChange->RelativeName;
            //            TmpStr.MaximumLength = TmpStr.Length = BufferedChange->NameLen;
            //            DPRINT("BufferedChange->RelativeName: %wZ\n", &TmpStr);
            //         }

        }
    }

    ExReleaseFastMutex((PFAST_MUTEX)NotifySync);

    /* complete defered irps */
    while (!IsListEmpty(&CompletedListHead))
    {
        EnumEntry = RemoveHeadList(&CompletedListHead);
        Irp = CONTAINING_RECORD(EnumEntry, IRP, Tail.Overlay.ListEntry);

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
}

/*++
 * @name FsRtlNotifyInitializeSync
 * @unimplemented
 *
 * FILLME
 *
 * @param NotifySync
 *        FILLME
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
VOID
NTAPI
FsRtlNotifyInitializeSync(IN PNOTIFY_SYNC *NotifySync)
{
    *NotifySync = ExAllocatePoolWithTag(NonPagedPool, sizeof(FAST_MUTEX), FSRTL_NOTIFY_TAG );
    ExInitializeFastMutex((PFAST_MUTEX)*NotifySync);
}

/*++
 * @name FsRtlNotifyReportChange
 * @unimplemented
 *
 * FILLME
 *
 * @param NotifySync
 *        FILLME
 *
 * @param NotifyList
 *        FILLME
 *
 * @param FullTargetName
 *        FILLME
 *
 * @param FileNamePartLength
 *        FILLME
 *
 * @param FilterMatch
 *        FILLME
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
VOID
NTAPI
FsRtlNotifyReportChange(IN PNOTIFY_SYNC NotifySync,
                        IN PLIST_ENTRY  NotifyList,
                        IN PSTRING      FullTargetName,
                        IN PUSHORT      FileNamePartLength,
                        IN ULONG        FilterMatch)
{
    FsRtlNotifyFullReportChange(
        NotifySync,
        NotifyList,
        FullTargetName,
        (FullTargetName->Length - *FileNamePartLength),
        NULL,
        NULL,
        FilterMatch,
        0,
        NULL
        );
}

/*++
 * @name FsRtlCurrentBatchOplock
 * @implemented
 *
 * Uninitialize a NOTIFY_SYNC object
 *
 * @param NotifySync
 *        Address of a pointer to a PNOTIFY_SYNC object previously
 *        initialized by FsRtlNotifyInitializeSync()
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
VOID
NTAPI
FsRtlNotifyUninitializeSync(IN PNOTIFY_SYNC *NotifySync)
{
    ExFreePool(*NotifySync);
}

/*++
 * @name FsRtlNotifyVolumeEvent
 * @unimplemented
 *
 * FILLME
 *
 * @param FileObject
 *        FILLME
 *
 * @param EventCode
 *        FILLME
 *
 * @return None
 *
 * @remarks Only present in NT 5+.
 *
 *--*/
NTSTATUS
NTAPI
FsRtlNotifyVolumeEvent(IN PFILE_OBJECT  FileObject,
                       IN ULONG         EventCode)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*++
 * @name FsRtlRegisterFileSystemFilterCallbacks
 * @unimplemented
 *
 * FILLME
 *
 * @param FilterDriverObject
 *        FILLME
 *
 * @param Callbacks
 *        FILLME
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
NTSTATUS
NTAPI
FsRtlRegisterFileSystemFilterCallbacks(IN PDRIVER_OBJECT FilterDriverObject,
                                       IN PFS_FILTER_CALLBACKS Callbacks)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}
/* EOF */
