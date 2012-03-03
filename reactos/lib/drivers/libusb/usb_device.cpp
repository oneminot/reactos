/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Driver Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/drivers/libusb/usb_device.cpp
 * PURPOSE:     USB Common Driver Library.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */


#define INITGUID
#include "libusb.h"

class CUSBDevice : public IUSBDevice
{
public:
    STDMETHODIMP QueryInterface( REFIID InterfaceId, PVOID* Interface);

    STDMETHODIMP_(ULONG) AddRef()
    {
        InterlockedIncrement(&m_Ref);
        return m_Ref;
    }
    STDMETHODIMP_(ULONG) Release()
    {
        InterlockedDecrement(&m_Ref);

        if (!m_Ref)
        {
            delete this;
            return 0;
        }
        return m_Ref;
    }

    // IUSBDevice interface functions
    virtual NTSTATUS Initialize(IN PHUBCONTROLLER HubController, IN PUSBHARDWAREDEVICE Device, IN PVOID Parent, IN ULONG Port, IN ULONG PortStatus);
    virtual BOOLEAN IsHub();
    virtual NTSTATUS GetParent(PVOID * Parent);
    virtual UCHAR GetDeviceAddress();
    virtual ULONG GetPort();
    virtual USB_DEVICE_SPEED GetSpeed();
    virtual USB_DEVICE_TYPE GetType();
    virtual ULONG GetState();
    virtual void SetDeviceHandleData(PVOID Data);
    virtual NTSTATUS SetDeviceAddress(UCHAR DeviceAddress);
    virtual void GetDeviceDescriptor(PUSB_DEVICE_DESCRIPTOR DeviceDescriptor);
    virtual UCHAR GetConfigurationValue();
    virtual NTSTATUS SubmitIrp(PIRP Irp);
    virtual VOID GetConfigurationDescriptors(IN PUSB_CONFIGURATION_DESCRIPTOR ConfigDescriptorBuffer, IN ULONG BufferLength, OUT PULONG OutBufferLength);
    virtual ULONG GetConfigurationDescriptorsLength();
    virtual NTSTATUS SubmitSetupPacket(IN PUSB_DEFAULT_PIPE_SETUP_PACKET SetupPacket, OUT ULONG BufferLength, OUT PVOID Buffer);
    virtual NTSTATUS SelectConfiguration(IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor, IN PUSBD_INTERFACE_INFORMATION Interface, OUT USBD_CONFIGURATION_HANDLE *ConfigurationHandle);
    virtual NTSTATUS SelectInterface(IN USBD_CONFIGURATION_HANDLE ConfigurationHandle, IN OUT PUSBD_INTERFACE_INFORMATION Interface);
    virtual NTSTATUS AbortPipe(IN PUSB_ENDPOINT_DESCRIPTOR EndpointDescriptor);
    virtual UCHAR GetMaxPacketSize();


    // local function
    virtual NTSTATUS CommitIrp(PIRP Irp);
    virtual NTSTATUS CommitSetupPacket(PUSB_DEFAULT_PIPE_SETUP_PACKET Packet, IN OPTIONAL PUSB_ENDPOINT EndpointDescriptor, IN ULONG BufferLength, IN OUT PMDL Mdl);
    virtual NTSTATUS CreateConfigurationDescriptor(UCHAR ConfigurationIndex);
    virtual NTSTATUS CreateDeviceDescriptor();
    virtual VOID DumpDeviceDescriptor(PUSB_DEVICE_DESCRIPTOR DeviceDescriptor);
    virtual VOID DumpConfigurationDescriptor(PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor);
    virtual NTSTATUS GetConfigurationDescriptor(UCHAR ConfigurationIndex, USHORT BufferSize, PVOID Buffer);

    // constructor / destructor
    CUSBDevice(IUnknown *OuterUnknown){}
    virtual ~CUSBDevice(){}

protected:
    LONG m_Ref;
    PHUBCONTROLLER m_HubController;
    PUSBHARDWAREDEVICE m_Device;
    PVOID m_Parent; 
    ULONG m_Port;
    UCHAR m_DeviceAddress;
    PVOID m_Data;
    UCHAR m_ConfigurationIndex;
    KSPIN_LOCK m_Lock;
    USB_DEVICE_DESCRIPTOR m_DeviceDescriptor;
    ULONG m_PortStatus;
    PUSBQUEUE m_Queue;
    PDMAMEMORYMANAGER m_DmaManager;

    PUSB_CONFIGURATION m_ConfigurationDescriptors;
};

//----------------------------------------------------------------------------------------
NTSTATUS
STDMETHODCALLTYPE
CUSBDevice::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    return STATUS_UNSUCCESSFUL;
}

//----------------------------------------------------------------------------------------
NTSTATUS
CUSBDevice::Initialize(
    IN PHUBCONTROLLER HubController, 
    IN PUSBHARDWAREDEVICE Device, 
    IN PVOID Parent, 
    IN ULONG Port,
    IN ULONG PortStatus)
{
    NTSTATUS Status;

    //
    // initialize members
    //
    m_HubController = HubController;
    m_Device = Device;
    m_Parent = Parent;
    m_Port = Port;
    m_PortStatus = PortStatus;

    //
    // initialize device lock
    //
    KeInitializeSpinLock(&m_Lock);

    //
    // no device address has been set yet
    //
    m_DeviceAddress = 0;

    //
    // get usb request queue
    //
    Status = m_Device->GetUSBQueue(&m_Queue);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to get usb queue
        //
        DPRINT1("CUSBDevice::Initialize GetUsbQueue failed with %x\n", Status);
        return Status;
    }

    //
    // get dma manager
    //
    Status = m_Device->GetDMA(&m_DmaManager);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to get dma manager
        //
        DPRINT1("CUSBDevice::Initialize GetDMA failed with %x\n", Status);
        return Status;
    }

    //
    // sanity check
    //
    PC_ASSERT(m_DmaManager);

    //
    // get device descriptor
    //
    Status = CreateDeviceDescriptor();
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to get device descriptor
        //
        DPRINT1("CUSBDevice::Initialize Failed to get device descriptor with %x\n", Status);
        return Status;
    }

    //
    // done
    //
    return Status;
}

//----------------------------------------------------------------------------------------
BOOLEAN
CUSBDevice::IsHub()
{
    //
    // USB Standard Device Class see http://www.usb.org/developers/defined_class/#BaseClass09h
    // for details
    //
    return (m_DeviceDescriptor.bDeviceClass == 0x09 && m_DeviceDescriptor.bDeviceSubClass == 0x00);
}

//----------------------------------------------------------------------------------------
NTSTATUS
CUSBDevice::GetParent(
    PVOID * Parent)
{
    //
    // returns parent
    //
    *Parent = m_Parent;

    //
    // done
    //
    return STATUS_SUCCESS;
}

//----------------------------------------------------------------------------------------
UCHAR
CUSBDevice::GetDeviceAddress()
{
    //
    // get device address
    //
    return m_DeviceAddress;
}

//----------------------------------------------------------------------------------------
ULONG
CUSBDevice::GetPort()
{
    //
    // get port to which this device is connected to
    //
    return m_Port;
}

//----------------------------------------------------------------------------------------
USB_DEVICE_SPEED
CUSBDevice::GetSpeed()
{
    if (m_PortStatus & USB_PORT_STATUS_LOW_SPEED)
    {
        //
        // low speed device
        //
        return UsbLowSpeed;
    }
    else if (m_PortStatus & USB_PORT_STATUS_HIGH_SPEED)
    {
        //
        // high speed device
        //
        return UsbHighSpeed;
    }

    //
    // default to full speed
    //
    return UsbFullSpeed;
}

//----------------------------------------------------------------------------------------
USB_DEVICE_TYPE
CUSBDevice::GetType()
{
    //
    // device is encoded into bcdUSB
    //
    if (m_DeviceDescriptor.bcdUSB == 0x110)
    {
        //
        // USB 1.1 device
        //
        return Usb11Device;
    }
    else if (m_DeviceDescriptor.bcdUSB == 0x200)
    {
        //
        // USB 2.0 device
        //
        return Usb20Device;
    }

    DPRINT1("CUSBDevice::GetType Unknown bcdUSB Type %x\n", m_DeviceDescriptor.bcdUSB);
    //PC_ASSERT(FALSE);

    return Usb11Device;
}

//----------------------------------------------------------------------------------------
ULONG
CUSBDevice::GetState()
{
    UNIMPLEMENTED
    return FALSE;
}

//----------------------------------------------------------------------------------------
void
CUSBDevice::SetDeviceHandleData(
    PVOID Data)
{
    //
    // set device data, for debugging issues
    //
    m_Data = Data;
}

//----------------------------------------------------------------------------------------
NTSTATUS
CUSBDevice::SetDeviceAddress(
    UCHAR DeviceAddress)
{
    PUSB_DEFAULT_PIPE_SETUP_PACKET CtrlSetup;
    NTSTATUS Status;
    UCHAR OldAddress;
    UCHAR Index;

    DPRINT1("CUSBDevice::SetDeviceAddress Address %d\n", DeviceAddress);

    CtrlSetup = (PUSB_DEFAULT_PIPE_SETUP_PACKET)ExAllocatePoolWithTag(NonPagedPool, sizeof(USB_DEFAULT_PIPE_SETUP_PACKET), TAG_USBLIB);
    if (!CtrlSetup)
        return STATUS_INSUFFICIENT_RESOURCES;

    // zero request
    RtlZeroMemory(CtrlSetup, sizeof(USB_DEFAULT_PIPE_SETUP_PACKET));

    // initialize request
    CtrlSetup->bRequest = USB_REQUEST_SET_ADDRESS;
    CtrlSetup->wValue.W = DeviceAddress;

    // set device address
    Status = CommitSetupPacket(CtrlSetup, 0, 0, 0);

    // free setup packet
    ExFreePoolWithTag(CtrlSetup, TAG_USBLIB);

    // check for success
    if (!NT_SUCCESS(Status))
    {
        // failed to set device address
        DPRINT1("CUSBDevice::SetDeviceAddress> failed to set device address with %x Address %x\n", Status, DeviceAddress);
        return Status;
    }

    // lets have a short nap
    KeStallExecutionProcessor(300);

    // back up old address
    OldAddress = m_DeviceAddress;

    // store new device address
    m_DeviceAddress = DeviceAddress;

    // fetch device descriptor
    Status = CreateDeviceDescriptor();
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CUSBbDevice::SetDeviceAddress> failed to retrieve device descriptor with device address set Error %x\n", Status);
        // return error status
        return Status;
    }

    // check for invalid device descriptor
    if (m_DeviceDescriptor.bLength != sizeof(USB_DEVICE_DESCRIPTOR) ||
        m_DeviceDescriptor.bDescriptorType != USB_DEVICE_DESCRIPTOR_TYPE ||
        m_DeviceDescriptor.bNumConfigurations == 0)
    {
        // failed to retrieve device descriptor
        DPRINT1("CUSBbDevice::SetDeviceAddress> device returned bogus device descriptor\n");
        DumpDeviceDescriptor(&m_DeviceDescriptor);

        // return error status
        return STATUS_UNSUCCESSFUL;
    }

    // dump device descriptor
    DumpDeviceDescriptor(&m_DeviceDescriptor);

    // sanity checks
    PC_ASSERT(m_DeviceDescriptor.bNumConfigurations);

    // allocate configuration descriptor
    m_ConfigurationDescriptors = (PUSB_CONFIGURATION) ExAllocatePoolWithTag(NonPagedPool, sizeof(USB_CONFIGURATION) * m_DeviceDescriptor.bNumConfigurations, TAG_USBLIB);

    // zero configuration descriptor
    RtlZeroMemory(m_ConfigurationDescriptors, sizeof(USB_CONFIGURATION) * m_DeviceDescriptor.bNumConfigurations);

    // retrieve the configuration descriptors
    for(Index = 0; Index < m_DeviceDescriptor.bNumConfigurations; Index++)
    {
        // retrieve configuration descriptors from device
        Status = CreateConfigurationDescriptor(Index);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("CUSBDevice::SetDeviceAddress> failed to retrieve configuration %lu\n", Index);
            ASSERT(FALSE);
            break;
        }
    }

    //
    // done
    //
    return Status;

}

//----------------------------------------------------------------------------------------
void
CUSBDevice::GetDeviceDescriptor(
    PUSB_DEVICE_DESCRIPTOR DeviceDescriptor)
{
    RtlMoveMemory(DeviceDescriptor, &m_DeviceDescriptor, sizeof(USB_DEVICE_DESCRIPTOR));
}

//----------------------------------------------------------------------------------------
UCHAR
CUSBDevice::GetConfigurationValue()
{
    //
    // return configuration index
    //
    return m_ConfigurationIndex;
}

//----------------------------------------------------------------------------------------
NTSTATUS
CUSBDevice::CommitIrp(
    PIRP Irp)
{
    NTSTATUS Status;
    PUSBREQUEST Request;

    if (!m_Queue || !m_DmaManager)
    {
        //
        // no queue, wtf?
        //
        DPRINT1("CUSBDevice::CommitUrb> no queue / dma !!!\n");
        return STATUS_UNSUCCESSFUL;
    }

    //
    // build usb request
    //
    Status = m_Queue->CreateUSBRequest(&Request);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to build request
        //
        DPRINT1("CUSBDevice::CommitSetupPacket> CreateUSBRequest failed with %x\n", Status);
        return Status;
    }

    //
    // initialize request
    //
    Status = Request->InitializeWithIrp(m_DmaManager, PUSBDEVICE(this), Irp);

    //
    // mark irp as pending
    //
    IoMarkIrpPending(Irp);

    //
    // now add the request
    //
    Status = m_Queue->AddUSBRequest(Request);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to add request
        //
        DPRINT1("CUSBDevice::CommitSetupPacket> failed add request to queue with %x\n", Status);
        Request->Release();
        return Status;
    }

    //
    // done
    //
    return STATUS_PENDING;
}

//----------------------------------------------------------------------------------------
NTSTATUS
CUSBDevice::SubmitIrp(
    PIRP Irp)
{
    KIRQL OldLevel;
    NTSTATUS Status;

    //
    // acquire device lock
    //
    KeAcquireSpinLock(&m_Lock, &OldLevel);

    //
    // commit urb
    //
    Status = CommitIrp(Irp);

    //
    // release lock
    //
    KeReleaseSpinLock(&m_Lock, OldLevel);

    return Status;
}
//----------------------------------------------------------------------------------------
NTSTATUS
CUSBDevice::CommitSetupPacket(
    IN PUSB_DEFAULT_PIPE_SETUP_PACKET Packet,
    IN OPTIONAL PUSB_ENDPOINT EndpointDescriptor,
    IN ULONG BufferLength, 
    IN OUT PMDL Mdl)
{
    NTSTATUS Status;
    PUSBREQUEST Request;

    if (!m_Queue)
    {
        //
        // no queue, wtf?
        //
        DPRINT1("CUSBDevice::CommitSetupPacket> no queue!!!\n");
        return STATUS_UNSUCCESSFUL;
    }

    //
    // build usb request
    //
    Status = m_Queue->CreateUSBRequest(&Request);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to build request
        //
        DPRINT1("CUSBDevice::CommitSetupPacket> CreateUSBRequest failed with %x\n", Status);
        return Status;
    }

    //
    // initialize request
    //
    Status = Request->InitializeWithSetupPacket(m_DmaManager, Packet, PUSBDEVICE(this), EndpointDescriptor, BufferLength, Mdl);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to initialize request
        //
        DPRINT1("CUSBDevice::CommitSetupPacket> failed to initialize  usb request with %x\n", Status);
        Request->Release();
        return Status;
    }

    //
    // now add the request
    //
    Status = m_Queue->AddUSBRequest(Request);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to add request
        //
        DPRINT1("CUSBDevice::CommitSetupPacket> failed add request to queue with %x\n", Status);
        Request->Release();
        return Status;
    }

    //
    // get the result code when the operation has been finished
    //
    Request->GetResultStatus(&Status, NULL);

    //
    // release request
    //
    Request->Release();

    //
    // done
    //
    return Status;
}

//----------------------------------------------------------------------------------------
NTSTATUS
CUSBDevice::CreateDeviceDescriptor()
{
    USB_DEFAULT_PIPE_SETUP_PACKET CtrlSetup;
    PMDL Mdl;
    NTSTATUS Status;
    PVOID Buffer;

    //
    // zero descriptor
    //
    RtlZeroMemory(&m_DeviceDescriptor, sizeof(USB_DEVICE_DESCRIPTOR));
    RtlZeroMemory(&CtrlSetup, sizeof(USB_DEFAULT_PIPE_SETUP_PACKET));

    //
    // setup request
    //
    CtrlSetup.bRequest = USB_REQUEST_GET_DESCRIPTOR;
    CtrlSetup.wValue.HiByte = USB_DEVICE_DESCRIPTOR_TYPE;
    CtrlSetup.wLength = sizeof(USB_DEVICE_DESCRIPTOR);
    CtrlSetup.bmRequestType.B = 0x80;

    //
    // allocate buffer
    //
    Buffer = ExAllocatePool(NonPagedPool, PAGE_SIZE);
    if (!Buffer)
    {
        //
        // failed to allocate
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // zero buffer
    //
    RtlZeroMemory(Buffer, PAGE_SIZE);

    //
    // allocate mdl describing the device descriptor
    //
    Mdl = IoAllocateMdl(Buffer, sizeof(USB_DEVICE_DESCRIPTOR), FALSE, FALSE, 0);
    if (!Mdl)
    {
        //
        // failed to allocate mdl
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // build mdl for non paged pool
    //
    MmBuildMdlForNonPagedPool(Mdl);

    //
    // commit setup packet
    //
    Status = CommitSetupPacket(&CtrlSetup, 0, sizeof(USB_DEVICE_DESCRIPTOR), Mdl);

    //
    // now free the mdl
    //
    IoFreeMdl(Mdl);

    if (NT_SUCCESS(Status))
    {
        //
        // copy device descriptor
        //
        RtlCopyMemory(&m_DeviceDescriptor, Buffer, sizeof(USB_DEVICE_DESCRIPTOR));
    }

    //
    // free buffer
    //
    ExFreePool(Buffer);

    //
    // done
    //
    return Status;

}

//----------------------------------------------------------------------------------------
NTSTATUS
CUSBDevice::GetConfigurationDescriptor(
    IN UCHAR ConfigurationIndex, 
    IN USHORT BufferSize,
    IN PVOID Buffer)
{
    USB_DEFAULT_PIPE_SETUP_PACKET CtrlSetup;
    NTSTATUS Status;
    PMDL Mdl;


    //
    // now build MDL describing the buffer
    //
    Mdl = IoAllocateMdl(Buffer, BufferSize, FALSE, FALSE, 0);
    if (!Mdl)
    {
        //
        // failed to allocate mdl
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // build mdl for non paged pool
    //
    MmBuildMdlForNonPagedPool(Mdl);


    //
    // build setup packet
    //
    CtrlSetup.bmRequestType._BM.Recipient = BMREQUEST_TO_DEVICE;
    CtrlSetup.bmRequestType._BM.Type = BMREQUEST_STANDARD;
    CtrlSetup.bmRequestType._BM.Reserved = 0;
    CtrlSetup.bmRequestType._BM.Dir = BMREQUEST_DEVICE_TO_HOST;
    CtrlSetup.bRequest = USB_REQUEST_GET_DESCRIPTOR;
    CtrlSetup.wValue.LowByte = ConfigurationIndex;
    CtrlSetup.wValue.HiByte = USB_CONFIGURATION_DESCRIPTOR_TYPE;
    CtrlSetup.wIndex.W = 0;
    CtrlSetup.wLength = BufferSize;

    //
    // commit packet
    //
    Status = CommitSetupPacket(&CtrlSetup, 0, BufferSize, Mdl);

    //
    // free mdl
    //
    IoFreeMdl(Mdl);

    //
    // done
    //
    return Status;
}

//----------------------------------------------------------------------------------------
NTSTATUS
CUSBDevice::CreateConfigurationDescriptor(
    UCHAR Index)
{
    NTSTATUS Status;
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor;

    //
    // sanity checks
    //
    PC_ASSERT(m_ConfigurationDescriptors);

    //
    // first allocate a buffer which should be enough to store all different interfaces and endpoints
    //
    ConfigurationDescriptor = (PUSB_CONFIGURATION_DESCRIPTOR)ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, TAG_USBLIB);
    if (!ConfigurationDescriptor)
    {
        //
        // failed to allocate buffer
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // get partial configuration descriptor
    //
    Status = GetConfigurationDescriptor(Index, sizeof(USB_CONFIGURATION_DESCRIPTOR), ConfigurationDescriptor);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to get partial configuration descriptor
        //
        DPRINT1("[USBLIB] Failed to get partial configuration descriptor Status %x Index %x\n", Status, Index);
        ExFreePoolWithTag(ConfigurationDescriptor, TAG_USBLIB);
        return Status;
    }

    //
    // now get full descriptor
    //
    Status = GetConfigurationDescriptor(Index, ConfigurationDescriptor->wTotalLength, ConfigurationDescriptor);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to get full configuration descriptor
        //
        DPRINT1("[USBLIB] Failed to get full configuration descriptor Status %x Index %x\n", Status, Index);
        ExFreePoolWithTag(ConfigurationDescriptor, TAG_USBLIB);
        return Status;
    }

    //
    // informal debug print
    //
    DumpConfigurationDescriptor(ConfigurationDescriptor);

    //
    // sanity check
    //
    PC_ASSERT(ConfigurationDescriptor->bLength == sizeof(USB_CONFIGURATION_DESCRIPTOR));
    PC_ASSERT(ConfigurationDescriptor->wTotalLength <= PAGE_SIZE);
    PC_ASSERT(ConfigurationDescriptor->bNumInterfaces);

    //
    // request is complete, initialize configuration descriptor
    //
    m_ConfigurationDescriptors[Index].ConfigurationDescriptor = ConfigurationDescriptor;
    InitializeListHead(&m_ConfigurationDescriptors[Index].InterfaceList);

    //
    // done
    //
    return Status;
}
//----------------------------------------------------------------------------------------
VOID
CUSBDevice::GetConfigurationDescriptors(
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigDescriptorBuffer,
    IN ULONG BufferLength,
    OUT PULONG OutBufferLength)
{
    // sanity check
    ASSERT(BufferLength >= sizeof(USB_CONFIGURATION_DESCRIPTOR));
    ASSERT(ConfigDescriptorBuffer);
    ASSERT(OutBufferLength);

    // reset copied length
    *OutBufferLength = 0;

    // FIXME: support multiple configurations
    PC_ASSERT(m_DeviceDescriptor.bNumConfigurations == 1);

    // copy configuration descriptor
    RtlCopyMemory(ConfigDescriptorBuffer, m_ConfigurationDescriptors[0].ConfigurationDescriptor, min(m_ConfigurationDescriptors[0].ConfigurationDescriptor->wTotalLength, BufferLength));
    *OutBufferLength = m_ConfigurationDescriptors[0].ConfigurationDescriptor->wTotalLength;
}

//----------------------------------------------------------------------------------------
ULONG
CUSBDevice::GetConfigurationDescriptorsLength()
{
    //
    // FIXME: support multiple configurations
    //
    PC_ASSERT(m_DeviceDescriptor.bNumConfigurations == 1);

    return m_ConfigurationDescriptors[0].ConfigurationDescriptor->wTotalLength;
}
//----------------------------------------------------------------------------------------
VOID
CUSBDevice::DumpDeviceDescriptor(PUSB_DEVICE_DESCRIPTOR DeviceDescriptor)
{
    DPRINT1("Dumping Device Descriptor %x\n", DeviceDescriptor);
    DPRINT1("bLength %x\n", DeviceDescriptor->bLength);
    DPRINT1("bDescriptorType %x\n", DeviceDescriptor->bDescriptorType);
    DPRINT1("bcdUSB %x\n", DeviceDescriptor->bcdUSB);
    DPRINT1("bDeviceClass %x\n", DeviceDescriptor->bDeviceClass);
    DPRINT1("bDeviceSubClass %x\n", DeviceDescriptor->bDeviceSubClass);
    DPRINT1("bDeviceProtocol %x\n", DeviceDescriptor->bDeviceProtocol);
    DPRINT1("bMaxPacketSize0 %x\n", DeviceDescriptor->bMaxPacketSize0);
    DPRINT1("idVendor %x\n", DeviceDescriptor->idVendor);
    DPRINT1("idProduct %x\n", DeviceDescriptor->idProduct);
    DPRINT1("bcdDevice %x\n", DeviceDescriptor->bcdDevice);
    DPRINT1("iManufacturer %x\n", DeviceDescriptor->iManufacturer);
    DPRINT1("iProduct %x\n", DeviceDescriptor->iProduct);
    DPRINT1("iSerialNumber %x\n", DeviceDescriptor->iSerialNumber);
    DPRINT1("bNumConfigurations %x\n", DeviceDescriptor->bNumConfigurations);
}

//----------------------------------------------------------------------------------------
VOID
CUSBDevice::DumpConfigurationDescriptor(PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor)
{
    DPRINT1("Dumping ConfigurationDescriptor %x\n", ConfigurationDescriptor);
    DPRINT1("bLength %x\n", ConfigurationDescriptor->bLength);
    DPRINT1("bDescriptorType %x\n", ConfigurationDescriptor->bDescriptorType);
    DPRINT1("wTotalLength %x\n", ConfigurationDescriptor->wTotalLength);
    DPRINT1("bNumInterfaces %x\n", ConfigurationDescriptor->bNumInterfaces);
    DPRINT1("bConfigurationValue %x\n", ConfigurationDescriptor->bConfigurationValue);
    DPRINT1("iConfiguration %x\n", ConfigurationDescriptor->iConfiguration);
    DPRINT1("bmAttributes %x\n", ConfigurationDescriptor->bmAttributes);
    DPRINT1("MaxPower %x\n", ConfigurationDescriptor->MaxPower);
}
//----------------------------------------------------------------------------------------
NTSTATUS
CUSBDevice::SubmitSetupPacket(
    IN PUSB_DEFAULT_PIPE_SETUP_PACKET SetupPacket, 
    IN OUT ULONG BufferLength, 
    OUT PVOID Buffer)
{
    NTSTATUS Status;
    PMDL Mdl = NULL;

    if (BufferLength)
    {
        //
        // allocate mdl
        //
        Mdl = IoAllocateMdl(Buffer, BufferLength, FALSE, FALSE, 0);
        if (!Mdl)
        {
            //
            // no memory
            //
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // HACK HACK HACK: assume the buffer is build from non paged pool
        //
        MmBuildMdlForNonPagedPool(Mdl);
    }

    //
    // commit setup packet
    //
    Status = CommitSetupPacket(SetupPacket, 0, BufferLength, Mdl);

    if (Mdl != NULL)
    {
        //
        // free mdl
        //
        IoFreeMdl(Mdl);
    }

    //
    // done
    //
    return Status;
}

//----------------------------------------------------------------------------------------
NTSTATUS
CUSBDevice::SelectConfiguration(
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    IN PUSBD_INTERFACE_INFORMATION InterfaceInfo,
    OUT USBD_CONFIGURATION_HANDLE *ConfigurationHandle)
{
    ULONG InterfaceIndex, PipeIndex;
    USB_DEFAULT_PIPE_SETUP_PACKET CtrlSetup;
    NTSTATUS Status;
    UCHAR bConfigurationValue = 0;
    ULONG ConfigurationIndex = 0, Index;
    UCHAR Found = FALSE;
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
    PUSB_INTERFACE UsbInterface;
    PUSB_ENDPOINT_DESCRIPTOR EndpointDescriptor;
    PLIST_ENTRY Entry;

    if (ConfigurationDescriptor)
    {
        // find configuration index
        for(Index = 0; Index < m_DeviceDescriptor.bNumConfigurations; Index++)
        {
            if (m_ConfigurationDescriptors[Index].ConfigurationDescriptor->bConfigurationValue  == ConfigurationDescriptor->bConfigurationValue)
            {
                // found configuration index
                ConfigurationIndex = Index;
                Found = TRUE;
            }
        }

        if (!Found)
        {
            DPRINT1("[USBUHCI] invalid configuration value %lu\n", ConfigurationDescriptor->bConfigurationValue);
            return STATUS_INVALID_PARAMETER;
        }

        // sanity check
        ASSERT(ConfigurationDescriptor->bNumInterfaces <= m_ConfigurationDescriptors[ConfigurationIndex].ConfigurationDescriptor->bNumInterfaces);

        // get configuration value
        bConfigurationValue = ConfigurationDescriptor->bConfigurationValue;
    }

    // now build setup packet
    RtlZeroMemory(&CtrlSetup, sizeof(USB_DEFAULT_PIPE_SETUP_PACKET));
    CtrlSetup.bRequest = USB_REQUEST_SET_CONFIGURATION;
    CtrlSetup.wValue.W = bConfigurationValue;

    // select configuration
    Status = CommitSetupPacket(&CtrlSetup, 0, 0, 0);

    if (!ConfigurationDescriptor)
    {
        // unconfigure request
        DPRINT1("CUsbDevice::SelectConfiguration Unconfigure Request Status %x\n", Status);
        m_ConfigurationIndex = 0;
        return Status;
    }

    // informal debug print
    DPRINT1("CUsbDevice::SelectConfiguration New Configuration %x Old Configuration %x Result %x\n", ConfigurationIndex, m_ConfigurationIndex, Status);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed
        //
        return Status;
    }

    // destroy old interface info
    while(!IsListEmpty(&m_ConfigurationDescriptors[m_ConfigurationIndex].InterfaceList))
    {
        // remove entry
        Entry = RemoveHeadList(&m_ConfigurationDescriptors[m_ConfigurationIndex].InterfaceList);

        // get interface info
        UsbInterface = (PUSB_INTERFACE)CONTAINING_RECORD(Entry, USB_INTERFACE, ListEntry);

        // free interface info
        ExFreePool(UsbInterface);
    }

    // sanity check
    ASSERT(IsListEmpty(&m_ConfigurationDescriptors[ConfigurationIndex].InterfaceList));

    // store new configuration device index
    m_ConfigurationIndex = ConfigurationIndex;

    // store configuration handle
    *ConfigurationHandle = &m_ConfigurationDescriptors[ConfigurationIndex];

    // copy interface info and pipe info
    for(InterfaceIndex = 0; InterfaceIndex < ConfigurationDescriptor->bNumInterfaces; InterfaceIndex++)
    {
        // interface info checks
        ASSERT(InterfaceInfo->Length != 0);

#ifdef _MSC_VER
        PC_ASSERT(InterfaceInfo->Length == FIELD_OFFSET(USBD_INTERFACE_INFORMATION, Pipes[InterfaceInfo->NumberOfPipes]));
#endif

        // find interface descriptor
        InterfaceDescriptor = USBD_ParseConfigurationDescriptor(m_ConfigurationDescriptors[ConfigurationIndex].ConfigurationDescriptor, InterfaceInfo->InterfaceNumber, InterfaceInfo->AlternateSetting);

        // sanity checks
        ASSERT(InterfaceDescriptor != NULL);

        // check if the number of pipes have been properly set
        ASSERT(InterfaceInfo->NumberOfPipes == InterfaceDescriptor->bNumEndpoints);

        // copy interface info
        InterfaceInfo->Class = InterfaceDescriptor->bInterfaceClass;
        InterfaceInfo->SubClass = InterfaceDescriptor->bInterfaceSubClass;
        InterfaceInfo->Protocol = InterfaceDescriptor->bInterfaceProtocol;
        InterfaceInfo->Reserved = 0;

        // allocate interface handle
        UsbInterface = (PUSB_INTERFACE)ExAllocatePool(NonPagedPool, sizeof(USB_INTERFACE) + (InterfaceInfo->NumberOfPipes - 1) * sizeof(USB_ENDPOINT));
        if (!UsbInterface)
        {
            // failed to allocate memory
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        // store handle
        InterfaceInfo->InterfaceHandle = (USBD_INTERFACE_HANDLE)UsbInterface;

        // init interface handle
        UsbInterface->InterfaceDescriptor = InterfaceDescriptor;
        InsertTailList(&m_ConfigurationDescriptors[ConfigurationIndex].InterfaceList, &UsbInterface->ListEntry);

        // grab first endpoint descriptor
        EndpointDescriptor = (PUSB_ENDPOINT_DESCRIPTOR) (InterfaceDescriptor + 1);

        // now copy all endpoint information
        for(PipeIndex = 0; PipeIndex < InterfaceInfo->NumberOfPipes; PipeIndex++)
        {
            while(EndpointDescriptor->bDescriptorType != USB_ENDPOINT_DESCRIPTOR_TYPE && EndpointDescriptor->bLength != sizeof(USB_ENDPOINT_DESCRIPTOR))
            {
                // skip intermediate descriptors
                if (EndpointDescriptor->bLength == 0 || EndpointDescriptor->bDescriptorType == USB_INTERFACE_DESCRIPTOR_TYPE)
                {
                    // bogus configuration descriptor
                    DPRINT1("[USBEHCI] Bogus descriptor found in InterfaceNumber %x Alternate %x EndpointIndex %x bLength %x bDescriptorType %x\n", InterfaceDescriptor->bInterfaceNumber, InterfaceDescriptor->bAlternateSetting, PipeIndex,
                            EndpointDescriptor->bLength, EndpointDescriptor->bDescriptorType);

                    // failed
                    return STATUS_UNSUCCESSFUL;
                }

                // move to next descriptor
                EndpointDescriptor = (PUSB_ENDPOINT_DESCRIPTOR)((ULONG_PTR)EndpointDescriptor + EndpointDescriptor->bLength);
            }

            // store in interface info
            RtlCopyMemory(&UsbInterface->EndPoints[PipeIndex].EndPointDescriptor, EndpointDescriptor, sizeof(USB_ENDPOINT_DESCRIPTOR));

            // copy pipe info
            InterfaceInfo->Pipes[PipeIndex].MaximumPacketSize = UsbInterface->EndPoints[PipeIndex].EndPointDescriptor.wMaxPacketSize;
            InterfaceInfo->Pipes[PipeIndex].EndpointAddress = UsbInterface->EndPoints[PipeIndex].EndPointDescriptor.bEndpointAddress;
            InterfaceInfo->Pipes[PipeIndex].Interval = UsbInterface->EndPoints[PipeIndex].EndPointDescriptor.bInterval;
            InterfaceInfo->Pipes[PipeIndex].PipeType = (USBD_PIPE_TYPE)UsbInterface->EndPoints[PipeIndex].EndPointDescriptor.bmAttributes;
            InterfaceInfo->Pipes[PipeIndex].PipeHandle = (PVOID)&UsbInterface->EndPoints[PipeIndex];

            // move to next descriptor
            EndpointDescriptor = (PUSB_ENDPOINT_DESCRIPTOR)((ULONG_PTR)EndpointDescriptor + EndpointDescriptor->bLength);
        }

        // move offset
        InterfaceInfo = (PUSBD_INTERFACE_INFORMATION)((ULONG_PTR)PtrToUlong(InterfaceInfo) + InterfaceInfo->Length);
    }

    //
    // done
    //
    return Status;
}

//----------------------------------------------------------------------------------------
NTSTATUS
CUSBDevice::SelectInterface(
    IN USBD_CONFIGURATION_HANDLE ConfigurationHandle,
    IN OUT PUSBD_INTERFACE_INFORMATION InterfaceInfo)
{
    ULONG PipeIndex;
    USB_DEFAULT_PIPE_SETUP_PACKET CtrlSetup;
    NTSTATUS Status;
    ULONG Index, ConfigurationIndex = 0, Found = FALSE;
    PUSB_INTERFACE UsbInterface;
    PLIST_ENTRY Entry;

    // check if handle is valid
    for(Index = 0; Index < m_DeviceDescriptor.bNumConfigurations; Index++)
    {
        if (&m_ConfigurationDescriptors[Index] == ConfigurationHandle)
        {
            // found configuration index
            ConfigurationIndex = Index;
            Found = TRUE;
        }
    }

    if (!Found)
    {
        // invalid handle passed
        DPRINT1("[USBEHCI] Invalid configuration handle passed %p\n", ConfigurationHandle);
        return STATUS_INVALID_PARAMETER;
    }

    // initialize setup packet
    RtlZeroMemory(&CtrlSetup, sizeof(USB_DEFAULT_PIPE_SETUP_PACKET));
    CtrlSetup.bRequest = USB_REQUEST_SET_INTERFACE;
    CtrlSetup.wValue.W = InterfaceInfo->AlternateSetting;
    CtrlSetup.wIndex.W = InterfaceInfo->InterfaceNumber;
    CtrlSetup.bmRequestType.B = 0x01;

    // issue request
    Status = CommitSetupPacket(&CtrlSetup, 0, 0, 0);

    // informal debug print
    DPRINT1("CUSBDevice::SelectInterface AlternateSetting %x InterfaceNumber %x Status %x\n", InterfaceInfo->AlternateSetting, InterfaceInfo->InterfaceNumber, Status);
#if 0
	if (!NT_SUCCESS(Status))
    {
        // failed to select interface
        return Status;
    }
#endif

    Status = STATUS_SUCCESS;


    // find interface
    Found = FALSE;
    Entry = m_ConfigurationDescriptors[ConfigurationIndex].InterfaceList.Flink;
    while(Entry != &m_ConfigurationDescriptors[ConfigurationIndex].InterfaceList)
    {
        // grab interface descriptor
        UsbInterface = (PUSB_INTERFACE)CONTAINING_RECORD(Entry, USB_INTERFACE, ListEntry);
        if (UsbInterface->InterfaceDescriptor->bAlternateSetting == InterfaceInfo->AlternateSetting &&
            UsbInterface->InterfaceDescriptor->bInterfaceNumber == InterfaceInfo->InterfaceNumber)
        {
            // found interface
            Found = TRUE;
            break;
        }

        // next entry
        Entry = Entry->Flink;
    }

    if (!Found)
    {
        // selected interface but interface not found
        // something is really wrong
        DPRINT1("[USBEHCI] Error: interface not found!!!\n");
        return STATUS_UNSUCCESSFUL;
    }

    // assert on pipe length mismatch
    ASSERT(InterfaceInfo->NumberOfPipes == UsbInterface->InterfaceDescriptor->bNumEndpoints);

    // copy pipe handles
    for(PipeIndex = 0; PipeIndex < min(InterfaceInfo->NumberOfPipes, UsbInterface->InterfaceDescriptor->bNumEndpoints); PipeIndex++)
    {
        // copy pipe handle
        DPRINT1("PipeIndex %lu\n", PipeIndex);
        DPRINT1("EndpointAddress %x\n", InterfaceInfo->Pipes[PipeIndex].EndpointAddress);
        DPRINT1("Interval %d\n", InterfaceInfo->Pipes[PipeIndex].Interval);
        DPRINT1("MaximumPacketSize %d\n", InterfaceInfo->Pipes[PipeIndex].MaximumPacketSize);
        DPRINT1("MaximumTransferSize %d\n", InterfaceInfo->Pipes[PipeIndex].MaximumTransferSize);
        DPRINT1("PipeFlags %d\n", InterfaceInfo->Pipes[PipeIndex].PipeFlags);
        DPRINT1("PipeType %dd\n", InterfaceInfo->Pipes[PipeIndex].PipeType);
        DPRINT1("UsbEndPoint %x\n", InterfaceInfo->Pipes[PipeIndex].EndpointAddress);

        // sanity checks
        ASSERT(InterfaceInfo->Pipes[PipeIndex].EndpointAddress == UsbInterface->EndPoints[PipeIndex].EndPointDescriptor.bEndpointAddress);
        ASSERT(InterfaceInfo->Pipes[PipeIndex].Interval == UsbInterface->EndPoints[PipeIndex].EndPointDescriptor.bInterval);

        // store pipe handle
        InterfaceInfo->Pipes[PipeIndex].PipeHandle = &UsbInterface->EndPoints[PipeIndex];

        // data toggle is reset on select interface requests
        UsbInterface->EndPoints[PipeIndex].DataToggle = FALSE;
    }


    //
    // done
    //
    return Status;
}

NTSTATUS
CUSBDevice::AbortPipe(
    IN PUSB_ENDPOINT_DESCRIPTOR EndpointDescriptor)
{
    //
    // let it handle usb queue
    //
    ASSERT(m_Queue);
    ASSERT(m_DeviceAddress);

    //
    // done
    //
    return m_Queue->AbortDevicePipe(m_DeviceAddress, EndpointDescriptor);
}

UCHAR
CUSBDevice::GetMaxPacketSize()
{
    return m_DeviceDescriptor.bMaxPacketSize0;
}


//----------------------------------------------------------------------------------------
NTSTATUS
NTAPI
CreateUSBDevice(
    PUSBDEVICE *OutDevice)
{
    CUSBDevice * This;

    //
    // allocate controller
    //
    This = new(NonPagedPool, TAG_USBLIB) CUSBDevice(0);
    if (!This)
    {
        //
        // failed to allocate
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // add reference count
    //
    This->AddRef();

    //
    // return result
    //
    *OutDevice = (PUSBDEVICE)This;

    //
    // done
    //
    return STATUS_SUCCESS;
}

