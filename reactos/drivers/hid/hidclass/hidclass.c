/*
 * PROJECT:     ReactOS Universal Serial Bus Human Interface Device Driver
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/hid/hidclass/hidclass.c
 * PURPOSE:     HID Class Driver
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "precomp.h"

static LPWSTR ClientIdentificationAddress = L"HIDCLASS";
static ULONG HidClassDeviceNumber = 0;

ULONG
NTAPI
DllInitialize(ULONG Unknown)
{
    return 0;
}

ULONG
NTAPI
DllUnload()
{
    return 0;
}

NTSTATUS
NTAPI
HidClassAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    WCHAR CharDeviceName[64];
    NTSTATUS Status;
    UNICODE_STRING DeviceName;
    PDEVICE_OBJECT NewDeviceObject;
    PHIDCLASS_FDO_EXTENSION FDODeviceExtension;
    ULONG DeviceExtensionSize;
    PHIDCLASS_DRIVER_EXTENSION DriverExtension;


    /* increment device number */
    InterlockedIncrement((PLONG)&HidClassDeviceNumber);

    /* construct device name */
    swprintf(CharDeviceName, L"\\Device\\_HID%08x", HidClassDeviceNumber);

    /* initialize device name */
    RtlInitUnicodeString(&DeviceName, CharDeviceName);

    /* get driver object extension */
    DriverExtension = (PHIDCLASS_DRIVER_EXTENSION) IoGetDriverObjectExtension(DriverObject, ClientIdentificationAddress);
    if (!DriverExtension)
    {
        /* device removed */
        ASSERT(FALSE);
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    /* calculate device extension size */
    DeviceExtensionSize = sizeof(HIDCLASS_FDO_EXTENSION) + DriverExtension->DeviceExtensionSize;

    /* now create the device */
    Status = IoCreateDevice(DriverObject, DeviceExtensionSize, &DeviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, &NewDeviceObject);
    if (!NT_SUCCESS(Status))
    {
        /* failed to create device object */
        ASSERT(FALSE);
        return Status;
    }

    /* get device extension */
    FDODeviceExtension = (PHIDCLASS_FDO_EXTENSION)NewDeviceObject->DeviceExtension;

    /* zero device extension */
    RtlZeroMemory(FDODeviceExtension, sizeof(HIDCLASS_FDO_EXTENSION));

    /* initialize device extension */
    FDODeviceExtension->Common.IsFDO = TRUE;
    FDODeviceExtension->Common.DriverExtension = DriverExtension;
    FDODeviceExtension->Common.HidDeviceExtension.PhysicalDeviceObject = PhysicalDeviceObject;
    FDODeviceExtension->Common.HidDeviceExtension.MiniDeviceExtension = (PVOID)((ULONG_PTR)FDODeviceExtension + sizeof(HIDCLASS_FDO_EXTENSION));
    FDODeviceExtension->Common.HidDeviceExtension.NextDeviceObject = IoAttachDeviceToDeviceStack(NewDeviceObject, PhysicalDeviceObject);
    if (FDODeviceExtension->Common.HidDeviceExtension.NextDeviceObject == NULL)
    {
        /* no PDO */
        IoDeleteDevice(NewDeviceObject);
        DPRINT1("[HIDCLASS] failed to attach to device stack\n");
        return STATUS_DEVICE_REMOVED;
    }

    /* sanity check */
    ASSERT(FDODeviceExtension->Common.HidDeviceExtension.NextDeviceObject);

    /* increment stack size */
    NewDeviceObject->StackSize++;

    /* init device object */
    NewDeviceObject->Flags |= DO_BUFFERED_IO | DO_POWER_PAGABLE;
    NewDeviceObject->Flags  &= ~DO_DEVICE_INITIALIZING;

    /* now call driver provided add device routine */
    ASSERT(DriverExtension->AddDevice != 0);
    Status = DriverExtension->AddDevice(DriverObject, NewDeviceObject);
    if (!NT_SUCCESS(Status))
    {
        /* failed */
        DPRINT1("HIDCLASS: AddDevice failed with %x\n", Status);
        IoDetachDevice(FDODeviceExtension->Common.HidDeviceExtension.NextDeviceObject);
        IoDeleteDevice(NewDeviceObject);
        return Status;
    }

    /* succeeded */
    return Status;
}

VOID
NTAPI
HidClassDriverUnload(
    IN PDRIVER_OBJECT DriverObject)
{
    UNIMPLEMENTED
}

NTSTATUS
NTAPI
HidClass_Create(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PHIDCLASS_COMMON_DEVICE_EXTENSION CommonDeviceExtension;
    PHIDCLASS_PDO_DEVICE_EXTENSION PDODeviceExtension;
    PHIDCLASS_FILEOP_CONTEXT Context;

    //
    // get device extension
    //
    CommonDeviceExtension = (PHIDCLASS_COMMON_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    if (CommonDeviceExtension->IsFDO)
    {
#ifndef __REACTOS__

         //
         // only supported for PDO
         //
         Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
         IoCompleteRequest(Irp, IO_NO_INCREMENT);
         return STATUS_UNSUCCESSFUL;
#else
         //
         // ReactOS PnP manager [...]
         //
         DPRINT1("[HIDCLASS] PnP HACK\n");
         Irp->IoStatus.Status = STATUS_SUCCESS;
         IoCompleteRequest(Irp, IO_NO_INCREMENT);
         return STATUS_SUCCESS;
#endif
    }

    //
    // must be a PDO
    //
    ASSERT(CommonDeviceExtension->IsFDO == FALSE);

    //
    // get device extension
    //
    PDODeviceExtension = (PHIDCLASS_PDO_DEVICE_EXTENSION)CommonDeviceExtension;

    //
    // get stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("ShareAccess %x\n", IoStack->Parameters.Create.ShareAccess);
    DPRINT("Options %x\n", IoStack->Parameters.Create.Options);
    DPRINT("DesiredAccess %x\n", IoStack->Parameters.Create.SecurityContext->DesiredAccess);

    //
    // allocate context
    //
    Context = (PHIDCLASS_FILEOP_CONTEXT)ExAllocatePool(NonPagedPool, sizeof(HIDCLASS_FILEOP_CONTEXT));
    if (!Context)
    {
        //
        // no memory
        //
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // init context
    //
    RtlZeroMemory(Context, sizeof(HIDCLASS_FILEOP_CONTEXT));
    Context->DeviceExtension = PDODeviceExtension;
    KeInitializeSpinLock(&Context->Lock);
    InitializeListHead(&Context->ReadPendingIrpListHead);
    InitializeListHead(&Context->IrpCompletedListHead);

    //
    // store context
    //
    ASSERT(IoStack->FileObject);
    IoStack->FileObject->FsContext = (PVOID)Context;

    //
    // done
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
HidClass_Close(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PHIDCLASS_COMMON_DEVICE_EXTENSION CommonDeviceExtension;
    PHIDCLASS_IRP_CONTEXT IrpContext;

    //
    // get device extension
    //
    CommonDeviceExtension = (PHIDCLASS_COMMON_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // is it a FDO request
    //
    if (CommonDeviceExtension->IsFDO)
    {
        //
        // how did the request get there
        //
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER_1;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INVALID_PARAMETER_1;
    }

    //
    // get stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // sanity checks
    //
    ASSERT(IoStack->FileObject);
    ASSERT(IoStack->FileObject->FsContext);

    //
    // get irp context
    //
    IrpContext = (PHIDCLASS_IRP_CONTEXT)IoStack->FileObject->FsContext;

    //
    // cancel pending irps
    //
    UNIMPLEMENTED

    //
    // remove context
    //
    IoStack->FileObject->FsContext = NULL;

    //
    // free context
    //
    ExFreePool(IrpContext);

    //
    // complete request
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

PVOID
HidClass_GetSystemAddress(
    IN PMDL ReportMDL)
{
    //
    // sanity check
    //
    ASSERT(ReportMDL);

    if (ReportMDL->MdlFlags & (MDL_SOURCE_IS_NONPAGED_POOL | MDL_MAPPED_TO_SYSTEM_VA))
    {
       //
       // buffer is non paged pool
       //
       return ReportMDL->MappedSystemVa;
    }
    else
    {
       //
       // map mdl
       //
       return MmMapLockedPages(ReportMDL, KernelMode);
    }
}

NTSTATUS
NTAPI
HidClass_ReadCompleteIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Ctx)
{
    PHIDCLASS_IRP_CONTEXT IrpContext;
    KIRQL OldLevel;
    PUCHAR Address;
    ULONG Offset;
    PHIDP_COLLECTION_DESC CollectionDescription;
    PHIDP_REPORT_IDS ReportDescription;

    //
    // get irp context
    //
    IrpContext = (PHIDCLASS_IRP_CONTEXT)Ctx;

    DPRINT("HidClass_ReadCompleteIrp Irql %lu\n", KeGetCurrentIrql());
    DPRINT("HidClass_ReadCompleteIrp Status %lx\n", Irp->IoStatus.Status);
    DPRINT("HidClass_ReadCompleteIrp Length %lu\n", Irp->IoStatus.Information);
    DPRINT("HidClass_ReadCompleteIrp Irp %p\n", Irp);
    DPRINT("HidClass_ReadCompleteIrp InputReportBuffer %p\n", IrpContext->InputReportBuffer);
    DPRINT("HidClass_ReadCompleteIrp InputReportBufferLength %li\n", IrpContext->InputReportBufferLength);
    DPRINT("HidClass_ReadCompleteIrp OriginalIrp %p\n", IrpContext->OriginalIrp);

    //
    // copy result
    //
    if (Irp->IoStatus.Information)
    {
        //
        // get address
        //
        Address = (PUCHAR)HidClass_GetSystemAddress(IrpContext->OriginalIrp->MdlAddress);
        if (Address)
        {
            //
            // reports may have a report id prepended
            //
            Offset = 0;

            //
            // get collection description
            //
            CollectionDescription = HidClassPDO_GetCollectionDescription(&IrpContext->FileOp->DeviceExtension->Common.DeviceDescription, IrpContext->FileOp->DeviceExtension->CollectionNumber);
            ASSERT(CollectionDescription);

            //
            // get report description
            //
            ReportDescription = HidClassPDO_GetReportDescription(&IrpContext->FileOp->DeviceExtension->Common.DeviceDescription, IrpContext->FileOp->DeviceExtension->CollectionNumber);
            ASSERT(ReportDescription);

            if (CollectionDescription && ReportDescription)
            {
                //
                // calculate offset
                //
                ASSERT(CollectionDescription->InputLength >= ReportDescription->InputLength);
                Offset = CollectionDescription->InputLength - ReportDescription->InputLength;
            }

            //
            // copy result
            //
            RtlCopyMemory(&Address[Offset], IrpContext->InputReportBuffer, IrpContext->InputReportBufferLength);
        }
    }

    //
    // copy result status
    //
    IrpContext->OriginalIrp->IoStatus.Status = Irp->IoStatus.Status;
    Irp->IoStatus.Information = Irp->IoStatus.Information;

    //
    // free input report buffer
    //
    ExFreePool(IrpContext->InputReportBuffer);

    //
    // remove us from pending list
    //
    KeAcquireSpinLock(&IrpContext->FileOp->Lock, &OldLevel);

    //
    // remove from pending list
    //
    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);

    //
    // insert into completed list
    //
    InsertTailList(&IrpContext->FileOp->IrpCompletedListHead, &Irp->Tail.Overlay.ListEntry);

    //
    // release lock
    //
    KeReleaseSpinLock(&IrpContext->FileOp->Lock, OldLevel);

    //
    // complete original request
    //
    IoCompleteRequest(IrpContext->OriginalIrp, IO_NO_INCREMENT);

    //
    // free irp context
    //
    ExFreePool(IrpContext);

    //
    // done
    //
    return STATUS_MORE_PROCESSING_REQUIRED;
}

PIRP
HidClass_GetIrp(
    IN PHIDCLASS_FILEOP_CONTEXT Context)
{
   KIRQL OldLevel;
   PIRP Irp = NULL;
   PLIST_ENTRY ListEntry;

    //
    // acquire lock
    //
    KeAcquireSpinLock(&Context->Lock, &OldLevel);

    //
    // is list empty?
    //
    if (!IsListEmpty(&Context->IrpCompletedListHead))
    {
        //
        // grab first entry
        //
        ListEntry = RemoveHeadList(&Context->IrpCompletedListHead);

        //
        // get irp
        //
        Irp = (PIRP)CONTAINING_RECORD(ListEntry, IRP, Tail.Overlay.ListEntry);
    }

    //
    // release lock
    //
    KeReleaseSpinLock(&Context->Lock, OldLevel);

    //
    // done
    //
    return Irp;
}

NTSTATUS
HidClass_BuildIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP RequestIrp,
    IN PHIDCLASS_FILEOP_CONTEXT Context,
    IN ULONG DeviceIoControlCode,
    IN ULONG BufferLength,
    OUT PIRP *OutIrp,
    OUT PHIDCLASS_IRP_CONTEXT *OutIrpContext)
{
    PIRP Irp;
    PIO_STACK_LOCATION IoStack;
    PHIDCLASS_IRP_CONTEXT IrpContext;
    PHIDCLASS_PDO_DEVICE_EXTENSION PDODeviceExtension;
    PHIDP_COLLECTION_DESC CollectionDescription;
    PHIDP_REPORT_IDS ReportDescription;

    //
    // get an irp from fresh list
    //
    Irp = HidClass_GetIrp(Context);
    if (!Irp)
    {
        //
        // build new irp
        //
        Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
        if (!Irp)
        {
            //
            // no memory
            //
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else
    {
        //
        // re-use irp
        //
        IoReuseIrp(Irp, STATUS_SUCCESS);
    }

    //
    // allocate completion context
    //
    IrpContext = (PHIDCLASS_IRP_CONTEXT)ExAllocatePool(NonPagedPool, sizeof(HIDCLASS_IRP_CONTEXT));
    if (!IrpContext)
    {
        //
        // no memory
        //
        IoFreeIrp(Irp);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // get device extension
    //
    PDODeviceExtension = (PHIDCLASS_PDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT(PDODeviceExtension->Common.IsFDO == FALSE);

    //
    // init irp context
    //
    RtlZeroMemory(IrpContext, sizeof(HIDCLASS_IRP_CONTEXT));
    IrpContext->OriginalIrp = RequestIrp;
    IrpContext->FileOp = Context;

    //
    // get collection description
    //
    CollectionDescription = HidClassPDO_GetCollectionDescription(&IrpContext->FileOp->DeviceExtension->Common.DeviceDescription, IrpContext->FileOp->DeviceExtension->CollectionNumber);
    ASSERT(CollectionDescription);

    //
    // get report description
    //
    ReportDescription = HidClassPDO_GetReportDescription(&IrpContext->FileOp->DeviceExtension->Common.DeviceDescription, IrpContext->FileOp->DeviceExtension->CollectionNumber);
    ASSERT(ReportDescription);

    //
    // sanity check
    //
    ASSERT(CollectionDescription->InputLength >= ReportDescription->InputLength);

    //
    // store report length
    //
    IrpContext->InputReportBufferLength = ReportDescription->InputLength;

    //
    // allocate buffer
    //
    IrpContext->InputReportBuffer = ExAllocatePool(NonPagedPool, IrpContext->InputReportBufferLength);
    if (!IrpContext->InputReportBuffer)
    {
        //
        // no memory
        //
        IoFreeIrp(Irp);
        ExFreePool(IrpContext);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // get stack location
    //
    IoStack = IoGetNextIrpStackLocation(Irp);

    //
    // init stack location
    //
    IoStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    IoStack->Parameters.DeviceIoControl.IoControlCode = DeviceIoControlCode;
    IoStack->Parameters.DeviceIoControl.OutputBufferLength = IrpContext->InputReportBufferLength;
    IoStack->Parameters.DeviceIoControl.InputBufferLength = 0;
    IoStack->Parameters.DeviceIoControl.Type3InputBuffer = NULL;
    Irp->UserBuffer = IrpContext->InputReportBuffer;
    IoStack->DeviceObject = DeviceObject;

    //
    // store result
    //
    *OutIrp = Irp;
    *OutIrpContext = IrpContext;

    //
    // done
    //
    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
HidClass_Read(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PHIDCLASS_FILEOP_CONTEXT Context;
    KIRQL OldLevel;
    NTSTATUS Status;
    PIRP NewIrp;
    PHIDCLASS_IRP_CONTEXT NewIrpContext;
    PHIDCLASS_COMMON_DEVICE_EXTENSION CommonDeviceExtension;

    //
    // get current stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // get device extension
    //
    CommonDeviceExtension = (PHIDCLASS_COMMON_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT(CommonDeviceExtension->IsFDO == FALSE);

    //
    // sanity check
    //
    ASSERT(IoStack->FileObject);
    ASSERT(IoStack->FileObject->FsContext);

    //
    // get context
    //
    Context = (PHIDCLASS_FILEOP_CONTEXT)IoStack->FileObject->FsContext;
    ASSERT(Context);

    //
    // FIXME support polled devices
    //
    ASSERT(Context->DeviceExtension->Common.DriverExtension->DevicesArePolled == FALSE);

    //
    // build irp request
    //
    Status = HidClass_BuildIrp(DeviceObject, Irp, Context, IOCTL_HID_READ_REPORT, IoStack->Parameters.Read.Length, &NewIrp, &NewIrpContext);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed
        //
        DPRINT1("HidClass_BuildIrp failed with %x\n", Status);
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }

    //
    // acquire lock
    //
    KeAcquireSpinLock(&Context->Lock, &OldLevel);

    //
    // insert irp into pending list
    //
    InsertTailList(&Context->ReadPendingIrpListHead, &NewIrp->Tail.Overlay.ListEntry);

    //
    // set completion routine
    //
    IoSetCompletionRoutine(NewIrp, HidClass_ReadCompleteIrp, NewIrpContext, TRUE, TRUE, TRUE);

    //
    // make next location current
    //
    IoSetNextIrpStackLocation(NewIrp);

    //
    // release spin lock
    //
    KeReleaseSpinLock(&Context->Lock, OldLevel);

    //
    // mark irp pending
    //
    IoMarkIrpPending(Irp);

    //
    // lets dispatch the request
    //
    ASSERT(Context->DeviceExtension);
    Status = Context->DeviceExtension->Common.DriverExtension->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL](Context->DeviceExtension->FDODeviceObject, NewIrp);

    //
    // complete
    //
    return STATUS_PENDING;
}

NTSTATUS
NTAPI
HidClass_Write(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    UNIMPLEMENTED
    ASSERT(FALSE);
    Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
HidClass_DeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PHIDCLASS_COMMON_DEVICE_EXTENSION CommonDeviceExtension;
    PHID_COLLECTION_INFORMATION CollectionInformation;
    PHIDP_COLLECTION_DESC CollectionDescription;
    PHIDCLASS_PDO_DEVICE_EXTENSION PDODeviceExtension;

    //
    // get device extension
    //
    CommonDeviceExtension = (PHIDCLASS_COMMON_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // only PDO are supported
    //
    if (CommonDeviceExtension->IsFDO)
    {
        //
        // invalid request
        //
        DPRINT1("[HIDCLASS] DeviceControl Irp for FDO arrived\n");
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INVALID_PARAMETER_1;
    }

    ASSERT(CommonDeviceExtension->IsFDO == FALSE);

    //
    // get pdo device extension
    //
    PDODeviceExtension = (PHIDCLASS_PDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // get stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    switch(IoStack->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_HID_GET_COLLECTION_INFORMATION:
        {
            //
            // check if output buffer is big enough
            //
            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(HID_COLLECTION_INFORMATION))
            {
                //
                // invalid buffer size
                //
                Irp->IoStatus.Status = STATUS_INVALID_BUFFER_SIZE;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_INVALID_BUFFER_SIZE;
            }

            //
            // get output buffer
            //
            CollectionInformation = (PHID_COLLECTION_INFORMATION)Irp->AssociatedIrp.SystemBuffer;
            ASSERT(CollectionInformation);

            //
            // get collection description
            //
            CollectionDescription = HidClassPDO_GetCollectionDescription(&CommonDeviceExtension->DeviceDescription, PDODeviceExtension->CollectionNumber);
            ASSERT(CollectionDescription);

            //
            // init result buffer
            //
            CollectionInformation->DescriptorSize = CollectionDescription->PreparsedDataLength;
            CollectionInformation->Polled = CommonDeviceExtension->DriverExtension->DevicesArePolled;
            CollectionInformation->VendorID = CommonDeviceExtension->Attributes.VendorID;
            CollectionInformation->ProductID = CommonDeviceExtension->Attributes.ProductID;
            CollectionInformation->VersionNumber = CommonDeviceExtension->Attributes.VersionNumber;

            //
            // complete request
            //
            Irp->IoStatus.Information = sizeof(HID_COLLECTION_INFORMATION);
            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_SUCCESS;
        }
        case IOCTL_HID_GET_COLLECTION_DESCRIPTOR:
        {
            //
            // get collection description
            //
            CollectionDescription = HidClassPDO_GetCollectionDescription(&CommonDeviceExtension->DeviceDescription, PDODeviceExtension->CollectionNumber);
            ASSERT(CollectionDescription);

            //
            // check if output buffer is big enough
            //
            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < CollectionDescription->PreparsedDataLength)
            {
                //
                // invalid buffer size
                //
                Irp->IoStatus.Status = STATUS_INVALID_BUFFER_SIZE;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_INVALID_BUFFER_SIZE;
            }

            //
            // copy result
            //
            ASSERT(Irp->UserBuffer);
            RtlCopyMemory(Irp->UserBuffer, CollectionDescription->PreparsedData, CollectionDescription->PreparsedDataLength);

            //
            // complete request
            //
            Irp->IoStatus.Information = CollectionDescription->PreparsedDataLength;
            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_SUCCESS;
        }
        default:
        {
            DPRINT1("[HIDCLASS] DeviceControl IoControlCode 0x%x not implemented\n", IoStack->Parameters.DeviceIoControl.IoControlCode);
            Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_NOT_IMPLEMENTED;
        }
    }
}

NTSTATUS
NTAPI
HidClass_InternalDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    UNIMPLEMENTED
    ASSERT(FALSE);
    Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
NTAPI
HidClass_Power(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
HidClass_PnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PHIDCLASS_COMMON_DEVICE_EXTENSION CommonDeviceExtension;

    //
    // get common device extension
    //
    CommonDeviceExtension = (PHIDCLASS_COMMON_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // check type of device object
    //
    if (CommonDeviceExtension->IsFDO)
    {
        //
        // handle request
        //
        return HidClassFDO_PnP(DeviceObject, Irp);
    }
    else
    {
        //
        // handle request
        //
        return HidClassPDO_PnP(DeviceObject, Irp);
    }
}

NTSTATUS
NTAPI
HidClass_DispatchDefault(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PHIDCLASS_COMMON_DEVICE_EXTENSION CommonDeviceExtension;

    //
    // get common device extension
    //
    CommonDeviceExtension = (PHIDCLASS_COMMON_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // FIXME: support PDO
    //
    ASSERT(CommonDeviceExtension->IsFDO == TRUE);

    //
    // skip current irp stack location
    //
    IoSkipCurrentIrpStackLocation(Irp);

    //
    // dispatch to lower device object
    //
    return IoCallDriver(CommonDeviceExtension->HidDeviceExtension.NextDeviceObject, Irp);
}


NTSTATUS
NTAPI
HidClassDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;

    //
    // get current stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);
    DPRINT("[HIDCLASS] Dispatch Major %x Minor %x\n", IoStack->MajorFunction, IoStack->MinorFunction);

    //
    // dispatch request based on major function
    //
    switch(IoStack->MajorFunction)
    {
        case IRP_MJ_CREATE:
            return HidClass_Create(DeviceObject, Irp);
        case IRP_MJ_CLOSE:
            return HidClass_Close(DeviceObject, Irp);
        case IRP_MJ_READ:
            return HidClass_Read(DeviceObject, Irp);
        case IRP_MJ_WRITE:
            return HidClass_Write(DeviceObject, Irp);
        case IRP_MJ_DEVICE_CONTROL:
            return HidClass_DeviceControl(DeviceObject, Irp);
        case IRP_MJ_INTERNAL_DEVICE_CONTROL:
           return HidClass_InternalDeviceControl(DeviceObject, Irp);
        case IRP_MJ_POWER:
            return HidClass_Power(DeviceObject, Irp);
        case IRP_MJ_PNP:
            return HidClass_PnP(DeviceObject, Irp);
        default:
            return HidClass_DispatchDefault(DeviceObject, Irp);
    }
}

NTSTATUS
NTAPI
HidRegisterMinidriver(
    IN PHID_MINIDRIVER_REGISTRATION MinidriverRegistration)
{
    NTSTATUS Status;
    PHIDCLASS_DRIVER_EXTENSION DriverExtension;

    /* check if the version matches */
    if (MinidriverRegistration->Revision > HID_REVISION)
    {
        /* revision mismatch */
        ASSERT(FALSE);
        return STATUS_REVISION_MISMATCH;
    }

    /* now allocate the driver object extension */
    Status = IoAllocateDriverObjectExtension(MinidriverRegistration->DriverObject, (PVOID)ClientIdentificationAddress, sizeof(HIDCLASS_DRIVER_EXTENSION), (PVOID*)&DriverExtension);
    if (!NT_SUCCESS(Status))
    {
        /* failed to allocate driver extension */
        ASSERT(FALSE);
        return Status;
    }

    /* zero driver extension */
    RtlZeroMemory(DriverExtension, sizeof(HIDCLASS_DRIVER_EXTENSION));

    /* init driver extension */
    DriverExtension->DriverObject = MinidriverRegistration->DriverObject;
    DriverExtension->DeviceExtensionSize = MinidriverRegistration->DeviceExtensionSize;
    DriverExtension->DevicesArePolled = MinidriverRegistration->DevicesArePolled;
    DriverExtension->AddDevice = MinidriverRegistration->DriverObject->DriverExtension->AddDevice;
    DriverExtension->DriverUnload = MinidriverRegistration->DriverObject->DriverUnload;

    /* copy driver dispatch routines */
    RtlCopyMemory(DriverExtension->MajorFunction, MinidriverRegistration->DriverObject->MajorFunction, sizeof(PDRIVER_DISPATCH) * (IRP_MJ_MAXIMUM_FUNCTION+1));

    /* initialize lock */
    KeInitializeSpinLock(&DriverExtension->Lock);

    /* now replace dispatch routines */
    DriverExtension->DriverObject->DriverExtension->AddDevice = HidClassAddDevice;
    DriverExtension->DriverObject->DriverUnload = HidClassDriverUnload;
    DriverExtension->DriverObject->MajorFunction[IRP_MJ_CREATE] = HidClassDispatch;
    DriverExtension->DriverObject->MajorFunction[IRP_MJ_CLOSE] = HidClassDispatch;
    DriverExtension->DriverObject->MajorFunction[IRP_MJ_READ] = HidClassDispatch;
    DriverExtension->DriverObject->MajorFunction[IRP_MJ_WRITE] = HidClassDispatch;
    DriverExtension->DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = HidClassDispatch;
    DriverExtension->DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = HidClassDispatch;
    DriverExtension->DriverObject->MajorFunction[IRP_MJ_POWER] = HidClassDispatch;
    DriverExtension->DriverObject->MajorFunction[IRP_MJ_PNP] = HidClassDispatch;

    /* done */
    return STATUS_SUCCESS;
}
