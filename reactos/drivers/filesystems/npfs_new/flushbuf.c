/*
 * PROJECT:     ReactOS Named Pipe FileSystem
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * FILE:        drivers/filesystems/npfs/flushbuf.c
 * PURPOSE:     Buffers Flushing Support
 * PROGRAMMERS: ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "npfs.h"

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
NpCommonFlushBuffers(IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp)
{
    NODE_TYPE_CODE NodeTypeCode;
    PNP_CCB Ccb;
    ULONG NamedPipeEnd;
    NTSTATUS Status;
    PNP_DATA_QUEUE FlushQueue;
    PAGED_CODE();

    NodeTypeCode = NpDecodeFileObject(IoGetCurrentIrpStackLocation(Irp)->FileObject,
                                      NULL,
                                      &Ccb,
                                      &NamedPipeEnd);
    if (NodeTypeCode != NPFS_NTC_CCB) return STATUS_PIPE_DISCONNECTED;

    ExAcquireResourceExclusiveLite(&Ccb->NonPagedCcb->Lock, TRUE);

    if (NamedPipeEnd == FILE_PIPE_SERVER_END)
    {
        FlushQueue = &Ccb->DataQueue[FILE_PIPE_INBOUND];
    }
    else
    {
        FlushQueue = &Ccb->DataQueue[FILE_PIPE_OUTBOUND];
    }

    if (FlushQueue->QueueState == WriteEntries)
    {
        Status = NpAddDataQueueEntry(NamedPipeEnd,
                                     Ccb,
                                     FlushQueue,
                                     WriteEntries,
                                     2,
                                     0,
                                     Irp,
                                     NULL,
                                     0);
    }
    else
    {
        Status = STATUS_SUCCESS;
    }

    ExReleaseResourceLite(&Ccb->NonPagedCcb->Lock);
    return Status;
}

NTSTATUS
NTAPI
NpFsdFlushBuffers(IN PDEVICE_OBJECT DeviceObject,
                  IN PIRP Irp)
{
    NTSTATUS Status;
    PAGED_CODE();

    FsRtlEnterFileSystem();
    NpAcquireSharedVcb();

    Status = NpCommonFlushBuffers(DeviceObject, Irp);

    NpReleaseVcb();
    FsRtlExitFileSystem();

    return Status;
}

/* EOF */
