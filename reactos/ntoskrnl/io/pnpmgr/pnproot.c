/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/pnproot.c
 * PURPOSE:         PnP manager root device
 *
 * PROGRAMMERS:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

#define ENUM_NAME_ROOT L"Root"

/* DATA **********************************************************************/

typedef struct _PNPROOT_DEVICE
{
  // Entry on device list
  LIST_ENTRY ListEntry;
  // Physical Device Object of device
  PDEVICE_OBJECT Pdo;
  // Service name
  UNICODE_STRING ServiceName;
  // Device ID
  UNICODE_STRING DeviceID;
  // Instance ID
  UNICODE_STRING InstanceID;
  // Device description
  UNICODE_STRING DeviceDescription;
  // Boot resource list
  PCM_FULL_RESOURCE_DESCRIPTOR BootResourceList;
  SIZE_T BootResourceListSize;
  // Resource requirement list
  PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirementsList;
} PNPROOT_DEVICE, *PPNPROOT_DEVICE;

typedef enum
{
  dsStopped,
  dsStarted,
  dsPaused,
  dsRemoved,
  dsSurpriseRemoved
} PNPROOT_DEVICE_STATE;


#include <pshpack1.h>

typedef struct _PNPROOT_COMMON_DEVICE_EXTENSION
{
  // Pointer to device object, this device extension is associated with
  PDEVICE_OBJECT DeviceObject;
  // Wether this device extension is for an FDO or PDO
  BOOLEAN IsFDO;
  // Wether the device is removed
  BOOLEAN Removed;
  // Current device power state for the device
  DEVICE_POWER_STATE DevicePowerState;
} PNPROOT_COMMON_DEVICE_EXTENSION, *PPNPROOT_COMMON_DEVICE_EXTENSION;


/* Physical Device Object device extension for a child device */
typedef struct _PNPROOT_PDO_DEVICE_EXTENSION
{
  // Common device data
  PNPROOT_COMMON_DEVICE_EXTENSION Common;
  // Device
  PPNPROOT_DEVICE Device;
} PNPROOT_PDO_DEVICE_EXTENSION, *PPNPROOT_PDO_DEVICE_EXTENSION;


/* Functional Device Object device extension for the PCI driver device object */
typedef struct _PNPROOT_FDO_DEVICE_EXTENSION
{
  // Common device data
  PNPROOT_COMMON_DEVICE_EXTENSION Common;
  // Physical Device Object
  PDEVICE_OBJECT Pdo;
  // Lower device object
  PDEVICE_OBJECT Ldo;
  // Current state of the driver
  PNPROOT_DEVICE_STATE State;
  // Namespace device list
  LIST_ENTRY DeviceListHead;
  // Number of (not removed) devices in device list
  ULONG DeviceListCount;
  // Lock for namespace device list
  // FIXME: Use fast mutex instead?
  KSPIN_LOCK DeviceListLock;
} PNPROOT_FDO_DEVICE_EXTENSION, *PPNPROOT_FDO_DEVICE_EXTENSION;

#include <poppack.h>



PDEVICE_OBJECT PnpRootDeviceObject;


/* FUNCTIONS *****************************************************************/

/* Physical Device Object routines */

NTSTATUS
PnpRootCreateDevice(
  PDEVICE_OBJECT *PhysicalDeviceObject)
{
  PPNPROOT_PDO_DEVICE_EXTENSION PdoDeviceExtension;
  PPNPROOT_FDO_DEVICE_EXTENSION DeviceExtension;
  PPNPROOT_DEVICE Device;
  NTSTATUS Status;

  /* This function should be obsoleted soon */

  DPRINT("Called\n");

  DeviceExtension = (PPNPROOT_FDO_DEVICE_EXTENSION)PnpRootDeviceObject->DeviceExtension;

  Device = (PPNPROOT_DEVICE)ExAllocatePoolWithTag(PagedPool, sizeof(PNPROOT_DEVICE), TAG_PNP_ROOT);
  if (!Device)
    return STATUS_INSUFFICIENT_RESOURCES;

  RtlZeroMemory(Device, sizeof(PNPROOT_DEVICE));

  Status = IoCreateDevice(
    PnpRootDeviceObject->DriverObject,
    sizeof(PNPROOT_PDO_DEVICE_EXTENSION),
    NULL,
    FILE_DEVICE_CONTROLLER,
    FILE_AUTOGENERATED_DEVICE_NAME,
    FALSE,
    &Device->Pdo);
  if (!NT_SUCCESS(Status)) {
    DPRINT("IoCreateDevice() failed with status 0x%X\n", Status);
    ExFreePool(Device);
    return Status;
  }

  Device->Pdo->Flags |= DO_BUS_ENUMERATED_DEVICE;

  Device->Pdo->Flags &= ~DO_DEVICE_INITIALIZING;

  //Device->Pdo->Flags |= DO_POWER_PAGABLE;

  PdoDeviceExtension = (PPNPROOT_PDO_DEVICE_EXTENSION)Device->Pdo->DeviceExtension;

  RtlZeroMemory(PdoDeviceExtension, sizeof(PNPROOT_PDO_DEVICE_EXTENSION));

  PdoDeviceExtension->Common.IsFDO = FALSE;

  PdoDeviceExtension->Common.DeviceObject = Device->Pdo;

  PdoDeviceExtension->Common.DevicePowerState = PowerDeviceD0;

  ExInterlockedInsertTailList(
    &DeviceExtension->DeviceListHead,
    &Device->ListEntry,
    &DeviceExtension->DeviceListLock);

  DeviceExtension->DeviceListCount++;

  *PhysicalDeviceObject = Device->Pdo;

  return STATUS_SUCCESS;
}


NTSTATUS
PdoQueryId(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp,
  PIO_STACK_LOCATION IrpSp)
{
  PPNPROOT_PDO_DEVICE_EXTENSION DeviceExtension;
  UNICODE_STRING String;
  NTSTATUS Status;

  DPRINT("Called\n");

  DeviceExtension = (PPNPROOT_PDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

//  Irp->IoStatus.Information = 0;

  Status = STATUS_SUCCESS;

  RtlInitUnicodeString(&String, NULL);

  switch (IrpSp->Parameters.QueryId.IdType) {
    case BusQueryDeviceID:
      if (DeviceExtension->Device)
        Status = RtlDuplicateUnicodeString(TRUE,
                                         &DeviceExtension->Device->DeviceID,
                                         &String);
      else
        Status = RtlCreateUnicodeString(&String, ENUM_NAME_ROOT L"\\LEGACY_UNKNOWN");

      DPRINT("DeviceID: %wZ\n", &String);

      Irp->IoStatus.Information = (ULONG_PTR)String.Buffer;
      break;

    case BusQueryHardwareIDs:
    case BusQueryCompatibleIDs:
      Status = STATUS_NOT_IMPLEMENTED;
      break;

    case BusQueryInstanceID:
      if (DeviceExtension->Device)
        Status = RtlDuplicateUnicodeString(TRUE,
                                         &DeviceExtension->Device->InstanceID,
                                         &String);
      else
        Status = RtlCreateUnicodeString(&String, L"0000");

      DPRINT("InstanceID: %S\n", String.Buffer);

      Irp->IoStatus.Information = (ULONG_PTR)String.Buffer;
      break;

    case BusQueryDeviceSerialNumber:
    default:
      Status = STATUS_NOT_IMPLEMENTED;
  }

  return Status;
}


NTSTATUS
PdoQueryResources(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp,
  PIO_STACK_LOCATION IrpSp)
{
  PPNPROOT_PDO_DEVICE_EXTENSION DeviceExtension;
  PCM_RESOURCE_LIST ResourceList;
  ULONG ResourceListSize = FIELD_OFFSET(CM_RESOURCE_LIST, List);

  DPRINT("Called\n");

  DeviceExtension = (PPNPROOT_PDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
  
  if (!DeviceExtension->Device || DeviceExtension->Device->BootResourceList == NULL)
  {
    /* Create an empty resource list */
    ResourceList = ExAllocatePool(PagedPool, ResourceListSize);
    if (ResourceList == NULL)
      return STATUS_INSUFFICIENT_RESOURCES;

    ResourceList->Count = 0;

    Irp->IoStatus.Information = (ULONG_PTR)ResourceList;
  }
  else
  {
    /* Copy existing resource list */
    ResourceList = ExAllocatePool(PagedPool,
      FIELD_OFFSET(CM_RESOURCE_LIST, List) +  DeviceExtension->Device->BootResourceListSize);
    if (ResourceList == NULL)
      return STATUS_INSUFFICIENT_RESOURCES;

    ResourceList->Count = 1;
    RtlCopyMemory(
      &ResourceList->List,
      DeviceExtension->Device->BootResourceList,
      DeviceExtension->Device->BootResourceListSize);
    Irp->IoStatus.Information = (ULONG_PTR)ResourceList;
  }

  return STATUS_SUCCESS;
}


NTSTATUS
PdoQueryResourceRequirements(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp,
  PIO_STACK_LOCATION IrpSp)
{
  PPNPROOT_PDO_DEVICE_EXTENSION DeviceExtension;
  PIO_RESOURCE_REQUIREMENTS_LIST ResourceList;
  ULONG ResourceListSize = FIELD_OFFSET(IO_RESOURCE_REQUIREMENTS_LIST, List);

  DPRINT("Called\n");

  DeviceExtension = (PPNPROOT_PDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
  
  if (!DeviceExtension->Device || DeviceExtension->Device->ResourceRequirementsList == NULL)
  {
    /* Create an empty resource list */
    ResourceList = ExAllocatePool(PagedPool, ResourceListSize);
    if (ResourceList == NULL)
      return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(ResourceList, ResourceListSize);
    ResourceList->ListSize = ResourceListSize;

    Irp->IoStatus.Information = (ULONG_PTR)ResourceList;
  }
  else
  {
    /* Copy existing resource requirement list */
    ResourceList = ExAllocatePool(PagedPool, DeviceExtension->Device->ResourceRequirementsList->ListSize);
    if (ResourceList == NULL)
      return STATUS_INSUFFICIENT_RESOURCES;

    RtlCopyMemory(
      ResourceList,
      DeviceExtension->Device->ResourceRequirementsList,
      DeviceExtension->Device->ResourceRequirementsList->ListSize);
    Irp->IoStatus.Information = (ULONG_PTR)ResourceList;
  }

  return STATUS_SUCCESS;
}


static NTSTATUS
PnpRootPdoQueryCapabilities(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp,
  PIO_STACK_LOCATION IrpSp)
{
  PPNPROOT_FDO_DEVICE_EXTENSION DeviceExtension;
  PDEVICE_CAPABILITIES DeviceCapabilities;

  DPRINT("Called\n");

  DeviceExtension = (PPNPROOT_FDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
  DeviceCapabilities = IrpSp->Parameters.DeviceCapabilities.Capabilities;

  if (DeviceCapabilities->Version != 1)
    return STATUS_UNSUCCESSFUL;

  DeviceCapabilities->UniqueID = TRUE;
  /* FIXME: Fill other fields */

  return STATUS_SUCCESS;
}


/*
 * FUNCTION: Handle Plug and Play IRPs for the child device
 * ARGUMENTS:
 *     DeviceObject = Pointer to physical device object of the child device
 *     Irp          = Pointer to IRP that should be handled
 * RETURNS:
 *     Status
 */
NTSTATUS
PnpRootPdoPnpControl(
  PDEVICE_OBJECT DeviceObject,
  PIRP Irp)
{
  PIO_STACK_LOCATION IrpSp;
  NTSTATUS Status;

  DPRINT("Called\n");

  Status = Irp->IoStatus.Status;

  IrpSp = IoGetCurrentIrpStackLocation(Irp);

  switch (IrpSp->MinorFunction) {
#if 0
  case IRP_MN_QUERY_BUS_INFORMATION:
    break;

  case IRP_MN_QUERY_DEVICE_RELATIONS:
    /* FIXME: Handle for TargetDeviceRelation */
    break;
#endif

  case IRP_MN_QUERY_ID:
    Status = PdoQueryId(DeviceObject, Irp, IrpSp);
    break;

  case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
    Status = PdoQueryResourceRequirements(DeviceObject, Irp, IrpSp);
    break;

  case IRP_MN_QUERY_RESOURCES:
    Status = PdoQueryResources(DeviceObject, Irp, IrpSp);
    break;

  case IRP_MN_QUERY_CAPABILITIES:
    Status = PnpRootPdoQueryCapabilities(DeviceObject, Irp, IrpSp);
    break;

  case IRP_MN_START_DEVICE:
  case IRP_MN_QUERY_STOP_DEVICE:
  case IRP_MN_CANCEL_STOP_DEVICE:
  case IRP_MN_STOP_DEVICE:
  case IRP_MN_QUERY_REMOVE_DEVICE:
  case IRP_MN_CANCEL_REMOVE_DEVICE:
  case IRP_MN_REMOVE_DEVICE:
  case IRP_MN_SURPRISE_REMOVAL:
    Status = STATUS_SUCCESS;
    break;

  default:
    DPRINT("Unknown IOCTL 0x%X\n", IrpSp->MinorFunction);
    break;
  }

  Irp->IoStatus.Status = Status;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  DPRINT("Leaving. Status 0x%X\n", Status);

  return Status;
}


/*
 * FUNCTION: Handle power management IRPs for the child device
 * ARGUMENTS:
 *     DeviceObject = Pointer to physical device object of the child device
 *     Irp          = Pointer to IRP that should be handled
 * RETURNS:
 *     Status
 */
NTSTATUS
PnpRootPdoPowerControl(
  PDEVICE_OBJECT DeviceObject,
  PIRP Irp)
{
  PIO_STACK_LOCATION IrpSp;
  NTSTATUS Status;

  DPRINT("Called\n");

  Status = Irp->IoStatus.Status;

  IrpSp = IoGetCurrentIrpStackLocation(Irp);

  switch (IrpSp->MinorFunction) {
  default:
    DPRINT("Unknown IOCTL 0x%X\n", IrpSp->MinorFunction);
    Status = STATUS_NOT_IMPLEMENTED;
    break;
  }

  Irp->IoStatus.Status = Status;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  DPRINT("Leaving. Status 0x%X\n", Status);

  return Status;
}


/* Functional Device Object routines */

static NTSTATUS
PnpRootReadRegistryBinary(
  IN PWSTR KeyName,
  IN PWSTR ValueKeyName,
  OUT PVOID* Buffer,
  OUT SIZE_T* BufferSize OPTIONAL)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING KeyNameU;
  UNICODE_STRING ValueKeyNameU;
  KEY_VALUE_PARTIAL_INFORMATION Size;
  PKEY_VALUE_PARTIAL_INFORMATION Data = NULL;
  ULONG DataSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION);
  HANDLE KeyHandle;
  NTSTATUS Status;
  
  DPRINT("Called\n");
  
  RtlInitUnicodeString(&KeyNameU, KeyName);
  RtlInitUnicodeString(&ValueKeyNameU, ValueKeyName);
  
  InitializeObjectAttributes(
    &ObjectAttributes,
    &KeyNameU,
    OBJ_CASE_INSENSITIVE,
    NULL, /* Root dir */
    NULL); /* Security descriptor */
  Status = ZwOpenKey(&KeyHandle, KEY_READ, &ObjectAttributes);
  if (!NT_SUCCESS(Status))
  {
    DPRINT("ZwOpenKey() failed (Status 0x%08lx)\n", Status);
    return Status;
  }
  
  Status = ZwQueryValueKey(
    KeyHandle,
    &ValueKeyNameU,
    KeyValuePartialInformation,
    &Size, DataSize,
    &DataSize);
  if (Status != STATUS_BUFFER_OVERFLOW)
  {
    DPRINT("ZwQueryValueKey() failed (Status 0x%08lx)\n", Status);
    ZwClose(KeyHandle);
    return Status;
  }
  
  while (Status == STATUS_BUFFER_OVERFLOW)
  {
    if (Data)
      ExFreePoolWithTag(Data, TAG_PNP_ROOT);
    Data = (PKEY_VALUE_PARTIAL_INFORMATION)ExAllocatePoolWithTag(PagedPool, DataSize, TAG_PNP_ROOT);
    if (!Data)
    {
      DPRINT("ExAllocatePoolWithTag() failed\n", Status);
      ZwClose(KeyHandle);
      return STATUS_NO_MEMORY;
    }
    
    Status = ZwQueryValueKey(
      KeyHandle,
      &ValueKeyNameU,
      KeyValuePartialInformation,
      Data, DataSize,
      &DataSize);
    if (NT_SUCCESS(Status))
    {
      *Buffer = ExAllocatePoolWithTag(PagedPool, Data->DataLength, TAG_PNP_ROOT);
      if (BufferSize) *BufferSize = Data->DataLength;
      if (!*Buffer)
      {
        DPRINT("ExAllocatePoolWithTag() failed\n", Status);
        ExFreePoolWithTag(Data, TAG_PNP_ROOT);
        ZwClose(KeyHandle);
        return STATUS_NO_MEMORY;
      }
      
      RtlCopyMemory(
        *Buffer,
        Data->Data,
        Data->DataLength);
      break;
    }
  }
  
  if (Data)
    ExFreePoolWithTag(Data, TAG_PNP_ROOT);
  ZwClose(KeyHandle);
  
  return Status;
}

NTSTATUS
PnpRootFdoReadDeviceInfo(
  PPNPROOT_DEVICE Device)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[2];
  PUNICODE_STRING DeviceDesc;
  WCHAR KeyName[MAX_PATH];
  NTSTATUS Status;

  DPRINT("Called\n");

  /* Retrieve configuration from Enum key */

  DeviceDesc = &Device->DeviceDescription;

  wcscpy(KeyName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Enum\\");
  wcscat(KeyName, ENUM_NAME_ROOT);
  wcscat(KeyName, L"\\");
  wcscat(KeyName, Device->ServiceName.Buffer);
  wcscat(KeyName, L"\\");
  wcscat(KeyName, Device->InstanceID.Buffer);

  DPRINT("KeyName %S\n", KeyName);

  /* 1. Read informations in instance key */
  RtlZeroMemory(QueryTable, sizeof(QueryTable));

  RtlInitUnicodeString(DeviceDesc, NULL);

  QueryTable[0].Name = L"DeviceDesc";
  QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
  QueryTable[0].EntryContext = DeviceDesc;

  Status = RtlQueryRegistryValues(
    RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
    KeyName,
    QueryTable,
    NULL,
    NULL);

  DPRINT("RtlQueryRegistryValues() returned status 0x%08lx\n", Status);

  if (!NT_SUCCESS(Status))
  {
    Device->DeviceDescription.Buffer = NULL;
  }

  DPRINT("Got device description: %S\n", DeviceDesc->Buffer);

  /* 2. Read informations in instance key, LogConf subkey */
  RtlZeroMemory(QueryTable, sizeof(QueryTable));
  wcscat(KeyName, L"\\LogConf");
  
  Status = PnpRootReadRegistryBinary(
    KeyName,
    L"BasicConfigVector",
    (PVOID*)&Device->ResourceRequirementsList,
    NULL);

  DPRINT("PnpRootReadRegistryBinary() returned status 0x%08lx\n", Status);

  if (!NT_SUCCESS(Status))
  {
    /* FIXME: */
  }

  Status = PnpRootReadRegistryBinary(
    KeyName,
    L"BootConfig",
    (PVOID*)&Device->BootResourceList,
    &Device->BootResourceListSize);

  DPRINT("PnpRootReadRegistryBinary() returned status 0x%08lx\n", Status);

  if (!NT_SUCCESS(Status))
  {
    /* FIXME: */
  }

  return STATUS_SUCCESS;
}


NTSTATUS
PnpRootFdoEnumerateDevices(
  PDEVICE_OBJECT DeviceObject)
{
  PPNPROOT_FDO_DEVICE_EXTENSION DeviceExtension;
  OBJECT_ATTRIBUTES ObjectAttributes, SubKeyAttributes;
  PKEY_BASIC_INFORMATION KeyInfo, SubKeyInfo;
  UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\System\\CurrentControlSet\\Enum\\" ENUM_NAME_ROOT);
  UNICODE_STRING SubKeyName;
  PPNPROOT_DEVICE Device;
  WCHAR Buffer[MAX_PATH];
  HANDLE KeyHandle, SubKeyHandle;
  ULONG BufferSize;
  ULONG ResultSize;
  NTSTATUS Status;
  ULONG Index1, Index2;

  DPRINT("Called\n");

  DeviceExtension = (PPNPROOT_FDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

  BufferSize = sizeof(KEY_BASIC_INFORMATION) + (MAX_PATH+1) * sizeof(WCHAR);
  KeyInfo = ExAllocatePoolWithTag(PagedPool, BufferSize, TAG_PNP_ROOT);
  if (!KeyInfo)
  {
    return STATUS_INSUFFICIENT_RESOURCES;
  }
  SubKeyInfo = ExAllocatePoolWithTag(PagedPool, BufferSize, TAG_PNP_ROOT);
  if (!SubKeyInfo)
  {
    ExFreePoolWithTag(KeyInfo, TAG_PNP_ROOT);
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  InitializeObjectAttributes(
    &ObjectAttributes,
		&KeyName,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);

  Status = ZwOpenKey(&KeyHandle, KEY_ENUMERATE_SUB_KEYS, &ObjectAttributes);
  if (!NT_SUCCESS(Status))
  {
    DPRINT("ZwOpenKey() failed (Status 0x%08lx)\n", Status);
    ExFreePoolWithTag(KeyInfo, TAG_PNP_ROOT);
    ExFreePoolWithTag(SubKeyInfo, TAG_PNP_ROOT);
    return Status;
  }

  /* FIXME: Disabled due to still using the old method of auto loading drivers e.g.
            there are more entries in the list than found in the registry as some
            drivers are passed on the command line */
//  DeviceExtension->DeviceListCount = 0;

  /* Devices are sub-sub-keys of 'KeyName'. KeyName is already opened as
   * KeyHandle. We'll first do a first enumeration to have first level keys,
   * and an inner one to have the real devices list.
   */
  Index1 = 0;
  
  while (TRUE)
  {
    Status = ZwEnumerateKey(
      KeyHandle,
      Index1,
      KeyBasicInformation,
      KeyInfo,
      BufferSize,
      &ResultSize);
    if (!NT_SUCCESS(Status))
    {
      DPRINT("ZwEnumerateKey() (Status 0x%08lx)\n", Status);
      break;
    }

    /* Terminate the string */
    KeyInfo->Name[KeyInfo->NameLength / sizeof(WCHAR)] = 0;
    
    /* Open the key */
    RtlInitUnicodeString(&SubKeyName, KeyInfo->Name);
    InitializeObjectAttributes(
      &SubKeyAttributes,
      &SubKeyName,
      0, /* Attributes */
      KeyHandle,
      NULL); /* Security descriptor */
    Status = ZwOpenKey(&SubKeyHandle, KEY_ENUMERATE_SUB_KEYS, &SubKeyAttributes);
    if (!NT_SUCCESS(Status))
    {
      DPRINT("ZwOpenKey() failed (Status 0x%08lx)\n", Status);
      break;
    }
    
    /* Enumerate the sub-keys */
    Index2 = 0;
    while (TRUE)
    {
      Status = ZwEnumerateKey(
        SubKeyHandle,
        Index2,
        KeyBasicInformation,
        SubKeyInfo,
        BufferSize,
        &ResultSize);
      if (!NT_SUCCESS(Status))
      {
        DPRINT("ZwEnumerateKey() (Status 0x%08lx)\n", Status);
        break;
      }
      
      /* Terminate the string */
      SubKeyInfo->Name[SubKeyInfo->NameLength / sizeof(WCHAR)] = 0;
      
      Device = (PPNPROOT_DEVICE)ExAllocatePoolWithTag(PagedPool, sizeof(PNPROOT_DEVICE), TAG_PNP_ROOT);
      if (!Device)
      {
        /* FIXME: */
        DPRINT("ExAllocatePoolWithTag() failed\n");
        break;
      }

      RtlZeroMemory(Device, sizeof(PNPROOT_DEVICE));

      if (!RtlCreateUnicodeString(&Device->ServiceName, KeyInfo->Name))
      {
        /* FIXME: */
        DPRINT("RtlCreateUnicodeString() failed\n");
      }

      wcscpy(Buffer, ENUM_NAME_ROOT);
      wcscat(Buffer, L"\\");
      wcscat(Buffer, KeyInfo->Name);

      if (!RtlCreateUnicodeString(&Device->DeviceID, Buffer))
      {
        /* FIXME: */
        DPRINT("RtlCreateUnicodeString() failed\n");
      }

      DPRINT("Got entry: %S\n", Device->DeviceID.Buffer);

      if (!RtlCreateUnicodeString(
        &Device->InstanceID,
        SubKeyInfo->Name))
      {
        /* FIXME: */
        DPRINT("RtlCreateUnicodeString() failed\n");
      }

      Status = PnpRootFdoReadDeviceInfo(Device);
      if (!NT_SUCCESS(Status))
      {
        /* FIXME */
        DPRINT("PnpRootFdoReadDeviceInfo() failed with status 0x%08lx\n", Status);
      }

      ExInterlockedInsertTailList(
        &DeviceExtension->DeviceListHead,
        &Device->ListEntry,
        &DeviceExtension->DeviceListLock);

      DeviceExtension->DeviceListCount++;
      
      Index2++;
    }
    
    ZwClose(SubKeyHandle);
    Index1++;
  }

  ZwClose(KeyHandle);

  ExFreePoolWithTag(KeyInfo, TAG_PNP_ROOT);
  ExFreePoolWithTag(SubKeyInfo, TAG_PNP_ROOT);

  return STATUS_SUCCESS;
}


NTSTATUS
PnpRootQueryBusRelations(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp,
  IN PIO_STACK_LOCATION IrpSp)
{
  PPNPROOT_PDO_DEVICE_EXTENSION PdoDeviceExtension;
  PPNPROOT_FDO_DEVICE_EXTENSION DeviceExtension;
  PDEVICE_RELATIONS Relations;
  PPNPROOT_DEVICE Device;
  NTSTATUS Status;
  ULONG Size;
  ULONG i;

  DPRINT("Called\n");

  Status = PnpRootFdoEnumerateDevices(DeviceObject);
  if (!NT_SUCCESS(Status))
    return Status;

  DeviceExtension = (PPNPROOT_FDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

  if (Irp->IoStatus.Information)
  {
    /* FIXME: Another bus driver has already created a DEVICE_RELATIONS
              structure so we must merge this structure with our own */
  }

  Size = sizeof(DEVICE_RELATIONS) + sizeof(Relations->Objects) *
    (DeviceExtension->DeviceListCount - 1);

  Relations = (PDEVICE_RELATIONS)ExAllocatePool(PagedPool, Size);
  if (!Relations)
    return STATUS_INSUFFICIENT_RESOURCES;

  Relations->Count = DeviceExtension->DeviceListCount;

  i = 0;
  LIST_FOR_EACH(Device,&DeviceExtension->DeviceListHead,PNPROOT_DEVICE, ListEntry) 
  {
    if (!Device->Pdo)
    {
      /* Create a physical device object for the
         device as it does not already have one */
      Status = IoCreateDevice(
        DeviceObject->DriverObject,
        sizeof(PNPROOT_PDO_DEVICE_EXTENSION),
        NULL,
        FILE_DEVICE_CONTROLLER,
        FILE_AUTOGENERATED_DEVICE_NAME,
        FALSE,
        &Device->Pdo);
      if (!NT_SUCCESS(Status))
      {
        DPRINT("IoCreateDevice() failed with status 0x%X\n", Status);
        ExFreePool(Relations);
        return Status;
      }

      DPRINT("Created PDO 0x%p\n", Device->Pdo);

      Device->Pdo->Flags |= DO_BUS_ENUMERATED_DEVICE;

      Device->Pdo->Flags &= ~DO_DEVICE_INITIALIZING;

      //Device->Pdo->Flags |= DO_POWER_PAGABLE;

      PdoDeviceExtension = (PPNPROOT_PDO_DEVICE_EXTENSION)Device->Pdo->DeviceExtension;

      RtlZeroMemory(PdoDeviceExtension, sizeof(PNPROOT_PDO_DEVICE_EXTENSION));

      PdoDeviceExtension->Common.IsFDO = FALSE;

      PdoDeviceExtension->Common.DeviceObject = Device->Pdo;

      PdoDeviceExtension->Common.DevicePowerState = PowerDeviceD0;

      PdoDeviceExtension->Device = Device;

      DPRINT("DeviceID: %wZ  PDO %p\n",
        &PdoDeviceExtension->Device->DeviceID,
        Device->Pdo);

      DPRINT("InstanceID: %wZ  PDO %p\n",
        &PdoDeviceExtension->Device->InstanceID,
        Device->Pdo);

      DPRINT("ResourceRequirementsList: %p  PDO %p\n",
        PdoDeviceExtension->Device->ResourceRequirementsList,
        Device->Pdo);
    }

    /* Reference the physical device object. The PnP manager
       will dereference it again when it is no longer needed */
    ObReferenceObject(Device->Pdo);

    Relations->Objects[i] = Device->Pdo;

    i++;
  }

  if (NT_SUCCESS(Status))
  {
    Irp->IoStatus.Information = (ULONG_PTR)Relations;
  }
  else
  {
    Irp->IoStatus.Information = 0;
  }

  return Status;
}


NTSTATUS
PnpRootQueryDeviceRelations(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp,
  IN PIO_STACK_LOCATION IrpSp)
{
  NTSTATUS Status;

  DPRINT("Called\n");

  switch (IrpSp->Parameters.QueryDeviceRelations.Type) {
  case BusRelations:
    Status = PnpRootQueryBusRelations(DeviceObject, Irp, IrpSp);
    break;

  default:
    Status = STATUS_NOT_IMPLEMENTED;
  }

  return Status;
}


/*
 * FUNCTION: Handle Plug and Play IRPs for the root bus device object
 * ARGUMENTS:
 *     DeviceObject = Pointer to functional device object of the root bus driver
 *     Irp          = Pointer to IRP that should be handled
 * RETURNS:
 *     Status
 */
NTSTATUS
STDCALL
PnpRootFdoPnpControl(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp)
{
  PPNPROOT_FDO_DEVICE_EXTENSION DeviceExtension;
  PIO_STACK_LOCATION IrpSp;
  NTSTATUS Status;

  DPRINT("Called\n");

  DeviceExtension = (PPNPROOT_FDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

  Status = Irp->IoStatus.Status;

  IrpSp = IoGetCurrentIrpStackLocation(Irp);

  switch (IrpSp->MinorFunction) {
  case IRP_MN_QUERY_DEVICE_RELATIONS:
    Status = PnpRootQueryDeviceRelations(DeviceObject, Irp, IrpSp);
    break;

  case IRP_MN_START_DEVICE:
    DeviceExtension->State = dsStarted;
    Status = STATUS_SUCCESS;
    break;

  case IRP_MN_STOP_DEVICE:
    /* Root device cannot be stopped */
    Status = STATUS_UNSUCCESSFUL;
    break;

  default:
    DPRINT("Unknown IOCTL 0x%X\n", IrpSp->MinorFunction);
    Status = STATUS_NOT_IMPLEMENTED;
    break;
  }

  if (Status != STATUS_PENDING) {
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
  }

  DPRINT("Leaving. Status 0x%X\n", Status);

  return Status;
}


/*
 * FUNCTION: Handle power management IRPs for the root bus device object
 * ARGUMENTS:
 *     DeviceObject = Pointer to functional device object of the root bus driver
 *     Irp          = Pointer to IRP that should be handled
 * RETURNS:
 *     Status
 */
NTSTATUS
STDCALL
PnpRootFdoPowerControl(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp)
{
  PIO_STACK_LOCATION IrpSp;
  NTSTATUS Status;

  DPRINT("Called\n");

  IrpSp = IoGetCurrentIrpStackLocation(Irp);

  switch (IrpSp->MinorFunction) {
  default:
    DPRINT("Unknown IOCTL 0x%X\n", IrpSp->MinorFunction);
    Status = STATUS_NOT_IMPLEMENTED;
    break;
  }

  if (Status != STATUS_PENDING) {
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
  }

  DPRINT("Leaving. Status 0x%X\n", Status);

  return Status;
}


/*
 * FUNCTION: Handle Plug and Play IRPs
 * ARGUMENTS:
 *     DeviceObject = Pointer to PDO or FDO
 *     Irp          = Pointer to IRP that should be handled
 * RETURNS:
 *     Status
 */
NTSTATUS
STDCALL
PnpRootPnpControl(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp)
{
  PPNPROOT_COMMON_DEVICE_EXTENSION DeviceExtension;
  NTSTATUS Status;

  DeviceExtension = (PPNPROOT_COMMON_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

  DPRINT("DeviceObject 0x%p  DeviceExtension 0x%p  IsFDO %d\n",
    DeviceObject,
    DeviceExtension,
    DeviceExtension->IsFDO);

  if (DeviceExtension->IsFDO) {
    Status = PnpRootFdoPnpControl(DeviceObject, Irp);
  } else {
    Status = PnpRootPdoPnpControl(DeviceObject, Irp);
  }

  return Status;
}


/*
 * FUNCTION: Handle power management IRPs
 * ARGUMENTS:
 *     DeviceObject = Pointer to PDO or FDO
 *     Irp          = Pointer to IRP that should be handled
 * RETURNS:
 *     Status
 */
NTSTATUS
STDCALL
PnpRootPowerControl(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp)
{
  PPNPROOT_COMMON_DEVICE_EXTENSION DeviceExtension;
  NTSTATUS Status;

  DeviceExtension = (PPNPROOT_COMMON_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

  if (DeviceExtension->IsFDO) {
    Status = PnpRootFdoPowerControl(DeviceObject, Irp);
  } else {
    Status = PnpRootPdoPowerControl(DeviceObject, Irp);
  }

  return Status;
}


NTSTATUS
STDCALL
PnpRootAddDevice(
  IN PDRIVER_OBJECT DriverObject,
  IN PDEVICE_OBJECT PhysicalDeviceObject)
{
  PPNPROOT_FDO_DEVICE_EXTENSION DeviceExtension;
  NTSTATUS Status;

  DPRINT("Called\n");

  Status = IoCreateDevice(
    DriverObject,
    sizeof(PNPROOT_FDO_DEVICE_EXTENSION),
    NULL,
    FILE_DEVICE_BUS_EXTENDER,
    FILE_DEVICE_SECURE_OPEN,
    TRUE,
    &PnpRootDeviceObject);
  if (!NT_SUCCESS(Status)) {
    KEBUGCHECKEX(PHASE1_INITIALIZATION_FAILED, Status, 0, 0, 0);
  }

  DeviceExtension = (PPNPROOT_FDO_DEVICE_EXTENSION)PnpRootDeviceObject->DeviceExtension;

  RtlZeroMemory(DeviceExtension, sizeof(PNPROOT_FDO_DEVICE_EXTENSION));

  DeviceExtension->Common.IsFDO = TRUE;

  DeviceExtension->State = dsStopped;

  DeviceExtension->Ldo = IoAttachDeviceToDeviceStack(
    PnpRootDeviceObject,
    PhysicalDeviceObject);

  if (!PnpRootDeviceObject) {
    CPRINT("PnpRootDeviceObject 0x%p\n", PnpRootDeviceObject);
    KEBUGCHECKEX(PHASE1_INITIALIZATION_FAILED, Status, 0, 0, 0);
  }

  if (!PhysicalDeviceObject) {
    CPRINT("PhysicalDeviceObject 0x%p\n", PhysicalDeviceObject);
    KEBUGCHECKEX(PHASE1_INITIALIZATION_FAILED, Status, 0, 0, 0);
  }

  InitializeListHead(&DeviceExtension->DeviceListHead);

  DeviceExtension->DeviceListCount = 0;

  KeInitializeSpinLock(&DeviceExtension->DeviceListLock);

  PnpRootDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

  //PnpRootDeviceObject->Flags |= DO_POWER_PAGABLE;

  DPRINT("Done AddDevice()\n");

  return STATUS_SUCCESS;
}


NTSTATUS
STDCALL
PnpRootDriverEntry(
  IN PDRIVER_OBJECT DriverObject,
  IN PUNICODE_STRING RegistryPath)
{
  DPRINT("Called\n");

  DriverObject->MajorFunction[IRP_MJ_PNP] = PnpRootPnpControl;
  DriverObject->MajorFunction[IRP_MJ_POWER] = PnpRootPowerControl;
  DriverObject->DriverExtension->AddDevice = PnpRootAddDevice;

  return STATUS_SUCCESS;
}

/* EOF */
