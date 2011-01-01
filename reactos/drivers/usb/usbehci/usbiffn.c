/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbehci/usbiffn.c
 * PURPOSE:     Direct Call Interface Functions.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 */

#include "usbehci.h"
#include <hubbusif.h>
#include <usbbusif.h>
#include "transfer.h"

PVOID InternalCreateUsbDevice(UCHAR DeviceNumber, ULONG Port, PUSB_DEVICE Parent, BOOLEAN Hub)
{
    PUSB_DEVICE UsbDevicePointer = NULL;

    UsbDevicePointer = ExAllocatePoolWithTag(NonPagedPool, sizeof(USB_DEVICE), USB_POOL_TAG);

    if (!UsbDevicePointer)
    {
        DPRINT1("Out of memory\n");
        return NULL;
    }

    RtlZeroMemory(UsbDevicePointer, sizeof(USB_DEVICE));

    if ((Hub) && (!Parent))
    {
        DPRINT1("This is the root hub\n");
    }

    UsbDevicePointer->Address = 0;//DeviceNumber;
    UsbDevicePointer->Port = Port - 1;
    UsbDevicePointer->ParentDevice = Parent;

    UsbDevicePointer->IsHub = Hub;

    return UsbDevicePointer;
}

VOID
USB_BUSIFFN
InterfaceReference(PVOID BusContext)
{
    DPRINT1("Ehci: InterfaceReference called\n");
}

VOID
USB_BUSIFFN
InterfaceDereference(PVOID BusContext)
{
    DPRINT1("Ehci: InterfaceDereference called\n");
}

/* Bus Interface Hub V5 Functions */


NTSTATUS
USB_BUSIFFN
CreateUsbDevice(PVOID BusContext,
                PUSB_DEVICE_HANDLE *NewDevice,
                PUSB_DEVICE_HANDLE HubDeviceHandle,
                USHORT PortStatus, USHORT PortNumber)
{
    PPDO_DEVICE_EXTENSION PdoDeviceExtension;
    PUSB_DEVICE UsbDevice;
    LONG i = 0;

    PdoDeviceExtension = (PPDO_DEVICE_EXTENSION)((PDEVICE_OBJECT)BusContext)->DeviceExtension;
    DPRINT1("Ehci: CreateUsbDevice: HubDeviceHandle %x, PortStatus %x, PortNumber %x\n", HubDeviceHandle, PortStatus, PortNumber);

    if (PdoDeviceExtension->UsbDevices[0] != HubDeviceHandle)
    {
        DPRINT1("Not a valid HubDeviceHandle\n");
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    UsbDevice = InternalCreateUsbDevice(PdoDeviceExtension->ChildDeviceCount, PortNumber, HubDeviceHandle, FALSE);

    if (!UsbDevice)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Add it to the list */
    while (TRUE)
    {
        if (PdoDeviceExtension->UsbDevices[i] == NULL)
        {
            PdoDeviceExtension->UsbDevices[i] = (PUSB_DEVICE)UsbDevice;
            break;
        }
        i++;
    }

    PdoDeviceExtension->Ports[PortNumber - 1].PortStatus = PortStatus;

    /* Return it */
    *NewDevice = UsbDevice;
    return STATUS_SUCCESS;
}

NTSTATUS
USB_BUSIFFN
InitializeUsbDevice(PVOID BusContext, PUSB_DEVICE_HANDLE DeviceHandle)
{
    PPDO_DEVICE_EXTENSION PdoDeviceExtension;
    PFDO_DEVICE_EXTENSION FdoDeviceExtension;
    USB_DEFAULT_PIPE_SETUP_PACKET CtrlSetup;
    PUSB_CONFIGURATION_DESCRIPTOR ConfigDesc;
    PUSB_INTERFACE_DESCRIPTOR InterfaceDesc;
    PUSB_ENDPOINT_DESCRIPTOR EndpointDesc;
    PUSB_DEVICE UsbDevice;
    PVOID Buffer;
    PUCHAR Ptr;
    LONG i, j, k;

    PdoDeviceExtension = (PPDO_DEVICE_EXTENSION)((PDEVICE_OBJECT)BusContext)->DeviceExtension;
    FdoDeviceExtension = (PFDO_DEVICE_EXTENSION)PdoDeviceExtension->ControllerFdo->DeviceExtension;

    UsbDevice = DeviceHandleToUsbDevice(BusContext, DeviceHandle);

    if (!UsbDevice)
    {
        DPRINT1("Invalid DeviceHandle or device not connected\n");
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    CtrlSetup.bRequest = USB_REQUEST_GET_DESCRIPTOR;
    CtrlSetup.wValue.LowByte = 0;
    CtrlSetup.wValue.HiByte = USB_DEVICE_DESCRIPTOR_TYPE;
    CtrlSetup.wIndex.W = 0;
    CtrlSetup.wLength = sizeof(USB_DEVICE_DESCRIPTOR);    
    CtrlSetup.bmRequestType.B = 0x80;
    
    SubmitControlTransfer(&FdoDeviceExtension->hcd,
                          &CtrlSetup,
                          &UsbDevice->DeviceDescriptor,
                          sizeof(USB_DEVICE_DESCRIPTOR),
                          NULL);
                          
    //DumpDeviceDescriptor(&UsbDevice->DeviceDescriptor);
    
    if (UsbDevice->DeviceDescriptor.bLength != 0x12)
    {
        DPRINT1("Failed to get Device Descriptor from device connected on port %d\n", UsbDevice->Port);
        return STATUS_DEVICE_DATA_ERROR;
    }
    
    if (UsbDevice->DeviceDescriptor.bNumConfigurations == 0)
    {
        DPRINT1("Device on port %d has no configurations!\n", UsbDevice->Port);
        return STATUS_DEVICE_DATA_ERROR;
    }
    UsbDevice->Configs = ExAllocatePoolWithTag(NonPagedPool,
                                               sizeof(PVOID) * UsbDevice->DeviceDescriptor.bNumConfigurations,
                                               USB_POOL_TAG);

    if (!UsbDevice->Configs)
    {
        DPRINT1("Out of memory\n");
        return STATUS_NO_MEMORY;
    }

    Buffer = ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, USB_POOL_TAG);

    if (!Buffer)
    {
        DPRINT1("Out of memory\n");
        return STATUS_NO_MEMORY;
    }

    Ptr = Buffer;
    
    for (i = 0; i < UsbDevice->DeviceDescriptor.bNumConfigurations; i++)
    {
        /* Get the Device Configuration Descriptor */
        CtrlSetup.bmRequestType._BM.Recipient = BMREQUEST_TO_DEVICE;
        CtrlSetup.bmRequestType._BM.Type = BMREQUEST_STANDARD;
        CtrlSetup.bmRequestType._BM.Reserved = 0;
        CtrlSetup.bmRequestType._BM.Dir = BMREQUEST_DEVICE_TO_HOST;
        CtrlSetup.bRequest = USB_REQUEST_GET_DESCRIPTOR;
        CtrlSetup.wValue.LowByte = 0;
        CtrlSetup.wValue.HiByte = USB_CONFIGURATION_DESCRIPTOR_TYPE;
        CtrlSetup.wIndex.W = 0;
        CtrlSetup.wLength = PAGE_SIZE;
        
        SubmitControlTransfer(&FdoDeviceExtension->hcd,
                              &CtrlSetup,
                              Buffer,
                              PAGE_SIZE,
                              NULL);

        ConfigDesc = (PUSB_CONFIGURATION_DESCRIPTOR)Ptr;

        //DumpFullConfigurationDescriptor(ConfigDesc);
        ASSERT(ConfigDesc->wTotalLength <= PAGE_SIZE);

        UsbDevice->Configs[i] = ExAllocatePoolWithTag(NonPagedPool,
                                                      sizeof(USB_CONFIGURATION) + sizeof(PVOID) * ConfigDesc->bNumInterfaces,
                                                      USB_POOL_TAG);
        UsbDevice->Configs[i]->Device = UsbDevice;

        RtlCopyMemory(&UsbDevice->Configs[0]->ConfigurationDescriptor,
                      ConfigDesc, sizeof(USB_CONFIGURATION_DESCRIPTOR));
        Ptr += ConfigDesc->bLength;

        for (j = 0; j < ConfigDesc->bNumInterfaces; j++)
        {
            InterfaceDesc = (PUSB_INTERFACE_DESCRIPTOR) Ptr;
            UsbDevice->Configs[i]->Interfaces[j] = ExAllocatePoolWithTag(NonPagedPool,
                                                                         sizeof(USB_INTERFACE) + sizeof(PVOID) * InterfaceDesc->bNumEndpoints,
                                                                         USB_POOL_TAG);
            RtlCopyMemory(&UsbDevice->Configs[i]->Interfaces[j]->InterfaceDescriptor,
                          InterfaceDesc,
                          sizeof(USB_INTERFACE_DESCRIPTOR));

            Ptr += InterfaceDesc->bLength;

            for (k = 0; k < InterfaceDesc->bNumEndpoints; k++)
            {
                EndpointDesc = (PUSB_ENDPOINT_DESCRIPTOR)Ptr;
                UsbDevice->Configs[i]->Interfaces[j]->EndPoints[k] = ExAllocatePoolWithTag(NonPagedPool, sizeof(USB_ENDPOINT), USB_POOL_TAG);
                RtlCopyMemory(&UsbDevice->Configs[i]->Interfaces[j]->EndPoints[k]->EndPointDescriptor,
                              EndpointDesc, sizeof(USB_ENDPOINT_DESCRIPTOR));
            }

        }
    }

    UsbDevice->ActiveConfig = UsbDevice->Configs[0];
    UsbDevice->ActiveInterface = UsbDevice->Configs[0]->Interfaces[0];

    return STATUS_SUCCESS;

    /* Set the device address */
    CtrlSetup.bmRequestType._BM.Recipient = BMREQUEST_TO_DEVICE;
    CtrlSetup.bmRequestType._BM.Type = BMREQUEST_STANDARD;
    CtrlSetup.bmRequestType._BM.Reserved = 0;
    CtrlSetup.bmRequestType._BM.Dir = BMREQUEST_HOST_TO_DEVICE;
    CtrlSetup.bRequest = USB_REQUEST_SET_ADDRESS;
    CtrlSetup.wValue.W = UsbDevice->Address;
    CtrlSetup.wIndex.W = 0;
    CtrlSetup.wLength = 0;

    DPRINT1("Setting Address to %x\n", UsbDevice->Address);
    
    SubmitControlTransfer(&FdoDeviceExtension->hcd,
                          &CtrlSetup,
                          NULL,
                          0,
                          NULL);

    return STATUS_SUCCESS;
}

NTSTATUS
USB_BUSIFFN
GetUsbDescriptors(PVOID BusContext,
                  PUSB_DEVICE_HANDLE DeviceHandle,
                  PUCHAR DeviceDescriptorBuffer,
                  PULONG DeviceDescriptorBufferLength,
                  PUCHAR ConfigDescriptorBuffer,
                  PULONG ConfigDescriptorBufferLength)
{
    PUSB_DEVICE UsbDevice;
    DPRINT1("Ehci: GetUsbDescriptor %x, %x, %x, %x\n", DeviceDescriptorBuffer, *DeviceDescriptorBufferLength,
                                                       ConfigDescriptorBuffer, *ConfigDescriptorBufferLength);

    UsbDevice = DeviceHandleToUsbDevice(BusContext, DeviceHandle);

    if (!UsbDevice)
    {
        DPRINT1("Invalid DeviceHandle or device not connected\n");
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    if ((DeviceDescriptorBuffer) && (DeviceDescriptorBufferLength))
    {
        RtlCopyMemory(DeviceDescriptorBuffer, &UsbDevice->DeviceDescriptor, sizeof(USB_DEVICE_DESCRIPTOR));
        *DeviceDescriptorBufferLength = sizeof(USB_DEVICE_DESCRIPTOR);
    }

    if ((ConfigDescriptorBuffer) && (ConfigDescriptorBufferLength))
    {
        RtlCopyMemory(ConfigDescriptorBuffer, &UsbDevice->ActiveConfig->ConfigurationDescriptor, sizeof(USB_CONFIGURATION_DESCRIPTOR));
        *ConfigDescriptorBufferLength = sizeof(USB_CONFIGURATION_DESCRIPTOR);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
USB_BUSIFFN
RemoveUsbDevice(PVOID BusContext, PUSB_DEVICE_HANDLE DeviceHandle, ULONG Flags)
{
    PPDO_DEVICE_EXTENSION PdoDeviceExtension;
    PUSB_DEVICE UsbDevice;
    LONG i, j, k;

    DPRINT1("RemoveUsbDevice called, DeviceHandle %x, Flags %x\n", DeviceHandle, Flags);

    PdoDeviceExtension = (PPDO_DEVICE_EXTENSION)((PDEVICE_OBJECT)BusContext)->DeviceExtension;

    UsbDevice = DeviceHandleToUsbDevice(BusContext, DeviceHandle);

    if (!UsbDevice)
    {
        DPRINT1("Invalid DeviceHandle or device not connected\n");
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    switch (Flags)
    {
       case 0:
            DPRINT1("Number of Configurations %d\n", UsbDevice->DeviceDescriptor.bNumConfigurations);
            for (i = 0; i < UsbDevice->DeviceDescriptor.bNumConfigurations; i++)
            {
                for (j = 0; j < UsbDevice->Configs[i]->ConfigurationDescriptor.bNumInterfaces; j++)
                {
                    for (k = 0; k < UsbDevice->Configs[i]->Interfaces[j]->InterfaceDescriptor.bNumEndpoints; k++)
                    {
                        ExFreePool(UsbDevice->Configs[i]->Interfaces[j]->EndPoints[k]);
                    }
                    ExFreePool(UsbDevice->Configs[i]->Interfaces[j]);
                }
                ExFreePool(UsbDevice->Configs[i]);
            }

            for (i = 0; i < 127; i++)
            {
                if (PdoDeviceExtension->UsbDevices[i] == UsbDevice)
                    PdoDeviceExtension->UsbDevices[i] = NULL;
            }

            ExFreePool(UsbDevice);

            /* DeConfig Device */
            break;
       case USBD_KEEP_DEVICE_DATA:
            DPRINT1("USBD_KEEP_DEVICE_DATA Not implemented!\n");
            break;

        case USBD_MARK_DEVICE_BUSY:
            DPRINT1("USBD_MARK_DEVICE_BUSY Not implemented!\n");
            break;
        default:
            DPRINT1("Unknown Remove Flags %x\n", Flags);
    }
    return STATUS_SUCCESS;
}

NTSTATUS
USB_BUSIFFN
RestoreUsbDevice(PVOID BusContext, PUSB_DEVICE_HANDLE OldDeviceHandle, PUSB_DEVICE_HANDLE NewDeviceHandle)
{
    DPRINT1("Ehci: RestoreUsbDevice not implemented! %x, %x, %x\n", BusContext, OldDeviceHandle, NewDeviceHandle);
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
USB_BUSIFFN
GetPortHackFlags(PVOID BusContext, PULONG Flags)
{
    DPRINT1("Ehci: GetPortHackFlags not implemented. %x, %x\n", BusContext, Flags);
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
USB_BUSIFFN
QueryDeviceInformation(PVOID BusContext,
                       PUSB_DEVICE_HANDLE DeviceHandle,
                       PVOID DeviceInformationBuffer,
                       ULONG DeviceInformationBufferLength,
                       PULONG LengthReturned)
{
    PUSB_DEVICE_INFORMATION_0 DeviceInfo = DeviceInformationBuffer;
    PUSB_DEVICE UsbDevice;
    ULONG SizeNeeded;
    LONG i;

    DPRINT1("Ehci: QueryDeviceInformation (%x, %x, %x, %d, %x\n", BusContext, DeviceHandle, DeviceInformationBuffer,
                                                                  DeviceInformationBufferLength, LengthReturned);

    UsbDevice = DeviceHandleToUsbDevice(BusContext, DeviceHandle);

    if (!UsbDevice)
    {
        DPRINT1("Invalid DeviceHandle or device not connected\n");
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    SizeNeeded = FIELD_OFFSET(USB_DEVICE_INFORMATION_0, PipeList[UsbDevice->ActiveInterface->InterfaceDescriptor.bNumEndpoints]);
    *LengthReturned = SizeNeeded;

    DeviceInfo->ActualLength = SizeNeeded;

    if (DeviceInformationBufferLength < SizeNeeded)
    {
        DPRINT1("Buffer to small\n");
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (DeviceInfo->InformationLevel != 0)
    {
        DPRINT1("Invalid Param\n");
        return STATUS_INVALID_PARAMETER;
    }

    DeviceInfo->PortNumber = UsbDevice->Port;
    DeviceInfo->HubAddress = 1;
    DeviceInfo->DeviceAddress = UsbDevice->Address;
    DeviceInfo->DeviceSpeed = UsbDevice->DeviceSpeed;
    DeviceInfo->DeviceType = UsbDevice->DeviceType;
    DeviceInfo->CurrentConfigurationValue = UsbDevice->ActiveConfig->ConfigurationDescriptor.bConfigurationValue;
    DeviceInfo->NumberOfOpenPipes = UsbDevice->ActiveInterface->InterfaceDescriptor.bNumEndpoints;

    RtlCopyMemory(&DeviceInfo->DeviceDescriptor, &UsbDevice->DeviceDescriptor, sizeof(USB_DEVICE_DESCRIPTOR));

    for (i = 0; i < UsbDevice->ActiveInterface->InterfaceDescriptor.bNumEndpoints; i++)
    {
        RtlCopyMemory(&DeviceInfo->PipeList[i].EndpointDescriptor,
                      &UsbDevice->ActiveInterface->EndPoints[i]->EndPointDescriptor,
                      sizeof(USB_ENDPOINT_DESCRIPTOR));
    }
    return STATUS_SUCCESS;
}

NTSTATUS
USB_BUSIFFN
GetControllerInformation(PVOID BusContext,
                         PVOID ControllerInformationBuffer,
                         ULONG ControllerInformationBufferLength,
                         PULONG LengthReturned)
{
    PUSB_CONTROLLER_INFORMATION_0 ControllerInfo;

    DPRINT1("Ehci: GetControllerInformation called\n");

    ControllerInfo = ControllerInformationBuffer;

    if (ControllerInformationBufferLength < sizeof(USB_CONTROLLER_INFORMATION_0))
    {
        DPRINT1("Buffer to small\n");
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (ControllerInfo->InformationLevel != 0)
    {
        DPRINT1("InformationLevel other than 0 not supported\n");
        return STATUS_NOT_SUPPORTED;
    }

    ControllerInfo->ActualLength = sizeof(USB_CONTROLLER_INFORMATION_0);
    ControllerInfo->SelectiveSuspendEnabled = FALSE;
    ControllerInfo->IsHighSpeedController = TRUE;

    *LengthReturned = ControllerInfo->ActualLength;

    return STATUS_SUCCESS;
}

NTSTATUS
USB_BUSIFFN
ControllerSelectiveSuspend(PVOID BusContext, BOOLEAN Enable)
{
    DPRINT1("Ehci: ControllerSelectiveSuspend not implemented\n");
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
USB_BUSIFFN
GetExtendedHubInformation(PVOID BusContext,
                          PDEVICE_OBJECT HubPhysicalDeviceObject,
                          PVOID HubInformationBuffer,
                          ULONG HubInformationBufferLength,
                          PULONG LengthReturned)
{

    PUSB_EXTHUB_INFORMATION_0 UsbExtHubInfo = HubInformationBuffer;
    PPDO_DEVICE_EXTENSION PdoDeviceExtension = (PPDO_DEVICE_EXTENSION)((PDEVICE_OBJECT)BusContext)->DeviceExtension;
    PFDO_DEVICE_EXTENSION FdoDeviceExntension = (PFDO_DEVICE_EXTENSION)PdoDeviceExtension->ControllerFdo->DeviceExtension;
    LONG i;
    DPRINT1("Ehci: GetExtendedHubInformation BusContext %x, PDO %x, InformationBuffer %x\n", 
            BusContext, HubPhysicalDeviceObject, HubInformationBuffer);

    /* Set the default return value */
    *LengthReturned = 0;

    DPRINT("InformationLevel %x\n", UsbExtHubInfo->InformationLevel);

    /* Caller is suppose to have set InformationLevel to 0. However usbehci from MS seems to ignore this */
    if (UsbExtHubInfo->InformationLevel != 0)
    {
        DPRINT1("InformationLevel should really be set to 0. Ignoring\n");
    }

    UsbExtHubInfo->NumberOfPorts = 8;

    for (i=0; i < UsbExtHubInfo->NumberOfPorts; i++)
    {
        UsbExtHubInfo->Port[i].PhysicalPortNumber = i + 1;
        UsbExtHubInfo->Port[i].PortLabelNumber = FdoDeviceExntension->hcd.ECHICaps.HCSParams.PortCount;
        UsbExtHubInfo->Port[i].VidOverride = 0;
        UsbExtHubInfo->Port[i].PidOverride = 0;
        UsbExtHubInfo->Port[i].PortAttributes = USB_PORTATTR_SHARED_USB2;
    }

    *LengthReturned = FIELD_OFFSET(USB_EXTHUB_INFORMATION_0, Port[8]);

    return STATUS_SUCCESS;
}

NTSTATUS
USB_BUSIFFN
GetRootHubSymbolicName(PVOID BusContext,
                       PVOID HubSymNameBuffer,
                       ULONG HubSymNameBufferLength,
                       PULONG HubSymNameActualLength)
{
    DPRINT1("Ehci: GetRootHubSymbolicName called\n");

    if (HubSymNameBufferLength < 16)
        return STATUS_UNSUCCESSFUL;
    RtlCopyMemory(HubSymNameBuffer, L"ROOT_HUB", HubSymNameBufferLength);
    *HubSymNameActualLength = 16;

    return STATUS_SUCCESS;
}

PVOID
USB_BUSIFFN
GetDeviceBusContext(PVOID HubBusContext, PVOID DeviceHandle)
{
    PUSB_DEVICE UsbDevice;

    DPRINT1("Ehci: GetDeviceBusContext called\n");
    UsbDevice = DeviceHandleToUsbDevice(HubBusContext, DeviceHandle);

    if (!UsbDevice)
    {
        DPRINT1("Invalid DeviceHandle or device not connected\n");
        return NULL;
    }

    return NULL;
}

NTSTATUS
USB_BUSIFFN
Initialize20Hub(PVOID BusContext, PUSB_DEVICE_HANDLE HubDeviceHandle, ULONG TtCount)
{
    DPRINT1("Ehci: Initialize20Hub called, HubDeviceHandle: %x, TtCount %x\n", HubDeviceHandle, TtCount);
    /* FIXME: */
    /* Create the Irp Queue for SCE */
    /* Should queue be created for each device or each enpoint??? */
    return STATUS_SUCCESS;
}

NTSTATUS
USB_BUSIFFN
RootHubInitNotification(PVOID BusContext, PVOID CallbackContext, PRH_INIT_CALLBACK CallbackRoutine)
{
    PPDO_DEVICE_EXTENSION PdoDeviceExtension;
    DPRINT1("Ehci: RootHubInitNotification %x, %x, %x\n", BusContext, CallbackContext, CallbackRoutine);

    PdoDeviceExtension = (PPDO_DEVICE_EXTENSION)((PDEVICE_OBJECT)BusContext)->DeviceExtension;
    PdoDeviceExtension->CallbackContext = CallbackContext;
    PdoDeviceExtension->CallbackRoutine = CallbackRoutine;
    if (PdoDeviceExtension->CallbackRoutine)
    {
        DPRINT1("Called Callbackrountine\n");
        PdoDeviceExtension->CallbackRoutine(PdoDeviceExtension->CallbackContext);
        DPRINT1("Done Callbackrountine\n");
    }
    else
    {
        DPRINT1("PdoDeviceExtension->CallbackRoutine is NULL!\n");
    }

    return STATUS_SUCCESS;
}

VOID
USB_BUSIFFN
FlushTransfers(PVOID BusContext, PVOID DeviceHandle)
{
    PUSB_DEVICE UsbDevice;
    UsbDevice = DeviceHandleToUsbDevice(BusContext, DeviceHandle);

    if (!UsbDevice)
    {
        DPRINT1("Invalid DeviceHandle or device not connected\n");
    }

    DPRINT1("FlushTransfers\n");
}

VOID
USB_BUSIFFN
SetDeviceHandleData(PVOID BusContext, PVOID DeviceHandle, PDEVICE_OBJECT UsbDevicePdo)
{
    DPRINT1("Ehci: SetDeviceHandleData not implemented %x, %x, %x\n", BusContext, DeviceHandle, UsbDevicePdo);
}


/* USB_BUS_INTERFACE_USBDI_V2 Functions */

VOID
USB_BUSIFFN
GetUSBDIVersion(PVOID BusContext, PUSBD_VERSION_INFORMATION VersionInformation, PULONG HcdCapabilites)
{
    DPRINT1("Ehci: GetUSBDIVersion called\n");
    return;
}

NTSTATUS
USB_BUSIFFN
QueryBusTime(PVOID BusContext, PULONG CurrentFrame)
{
    DPRINT1("Ehci: QueryBusTime called\n");
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
USB_BUSIFFN
SubmitIsoOutUrb(PVOID BusContext, PURB Urb)
{
    DPRINT1("Ehci: SubmitIsoOutUrb called\n");
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
USB_BUSIFFN
QueryBusInformation(PVOID BusContext,
                    ULONG Level,
                    PVOID BusInformationBuffer,
                    PULONG BusInformationBufferLength,
                    PULONG BusInformationActualLength)
{
    DPRINT1("Ehci: QueryBusInformation called\n");
    return STATUS_NOT_SUPPORTED;
}

BOOLEAN
USB_BUSIFFN
IsDeviceHighSpeed(PVOID BusContext)
{
    DPRINT1("Ehci: IsDeviceHighSpeed called\n");
    return TRUE;
}

NTSTATUS
USB_BUSIFFN
EnumLogEntry(PVOID BusContext, ULONG DriverTag, ULONG EnumTag, ULONG P1, ULONG P2)
{
    DPRINT1("Ehci: EnumLogEntry called\n");
    return STATUS_SUCCESS;
}
